#include "ThemeDpiMonitor.h"
#include "AsyncProcess.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTime>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace {
constexpr int64_t kRapidSwitchCooldownMs = 3000;
}

ThemeDpiMonitor::ThemeDpiMonitor(QObject *parent)
    : QObject(parent) {
    loadSettings();

    m_pollTimer.setInterval(std::clamp(m_intervalSec, 1, 60) * 1000);
    connect(&m_pollTimer, &QTimer::timeout, this, [this]() {
        // 1. Never interrupt active/recent rapid DPI button switching (let MCU finish EEPROM commit!)
        const int64_t lastMs = m_lastInterruptMs.load();
        if (lastMs > 0 && (steadyNowMs() - lastMs) < kRapidSwitchCooldownMs) {
            return;
        }
        // 2. When hardware is in Extra Mode (0x09) and palette is cached, stay 100% passive
        if (m_ready && m_inExtraMode) {
            return;
        }
        // 3. Once palette is cached, use ultra-fast 1-byte check (~20ms) instead of 40-byte read (~850ms)
        triggerRead(false, m_ready);
    });
}

ThemeDpiMonitor::~ThemeDpiMonitor() {
    m_shuttingDown.store(true);
    m_pollTimer.stop();
    stopPassiveWatcher();
    joinReadThreadIfNeeded();
}

int64_t ThemeDpiMonitor::steadyNowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

QString ThemeDpiMonitor::configFilePath() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + QStringLiteral("/theme-dpi-monitor.json");
}

void ThemeDpiMonitor::loadSettings() {
    QByteArray raw;
    if (!SystemUtils::readRegularFileBounded(configFilePath(), 64 * 1024, raw)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) return;
    const QJsonObject obj = doc.object();
    m_autoSync = obj.value(QStringLiteral("autoSync")).toBool(m_autoSync);
    m_intervalSec = std::clamp(obj.value(QStringLiteral("intervalSec")).toInt(m_intervalSec), 1, 60);
}

void ThemeDpiMonitor::saveSettings() const {
    const QString path = configFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QJsonObject obj;
    obj[QStringLiteral("autoSync")] = m_autoSync;
    obj[QStringLiteral("intervalSec")] = m_intervalSec;

    QSaveFile sf(path);
    if (sf.open(QIODevice::WriteOnly)) {
        sf.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        sf.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        sf.commit();
    }
}

QString ThemeDpiMonitor::dpiLabelForIndex(int tableIndex) {
    const size_t idx = static_cast<size_t>(std::clamp(tableIndex, 0, 16));
    return QString::number(M700ADriver::kDpiValueTable[idx]) + QStringLiteral(" DPI");
}

void ThemeDpiMonitor::setDeviceAccessAvailable(bool available) {
    if (m_deviceAccessAvailable == available) return;
    m_deviceAccessAvailable = available;
    if (available) {
        if (m_autoSync) {
            m_pollTimer.setInterval(std::clamp(m_intervalSec, 1, 60) * 1000);
            m_pollTimer.start();
            startPassiveWatcher();
        }
        // Perform one quiet initial palette sync after startup settles
        QTimer::singleShot(400, this, [this]() {
            if (!m_shuttingDown.load() && m_deviceAccessAvailable && !m_ready) {
                triggerRead(false, false);
            }
        });
    } else {
        m_pollTimer.stop();
        stopPassiveWatcher();
    }
    emit settingsChanged();
}

void ThemeDpiMonitor::setEditorBusy(bool busy) {
    m_editorBusy.store(busy);
}

void ThemeDpiMonitor::setLanguage(const QString &lang) {
    m_language = lang;
}

void ThemeDpiMonitor::setAutoSync(bool enabled) {
    m_autoSync = enabled;
    m_pollTimer.setInterval(std::clamp(m_intervalSec, 1, 60) * 1000);
    if (enabled && m_deviceAccessAvailable) {
        m_pollTimer.start();
        startPassiveWatcher();
        if (!m_ready) {
            triggerRead(false, false);
        }
    } else {
        m_pollTimer.stop();
        stopPassiveWatcher();
    }
    saveSettings();
    emit settingsChanged();
}

void ThemeDpiMonitor::setIntervalSec(int seconds) {
    const int clamped = std::clamp(seconds, 1, 60);
    m_intervalSec = clamped;
    m_pollTimer.setInterval(clamped * 1000);
    if (m_autoSync && m_deviceAccessAvailable) {
        m_pollTimer.start();
        startPassiveWatcher();
        if (!m_ready) {
            triggerRead(false, false);
        }
    }
    saveSettings();
    emit settingsChanged();
}

void ThemeDpiMonitor::syncFromCompletedHardwareTransfer(int activeLevel,
                                                        const std::array<int, 6> &dpiIndices,
                                                        const std::array<RgbColor, 6> &dpiColors,
                                                        bool inExtraMode) {
    const int clampedLvl = std::clamp(activeLevel, 1, 6);
    const size_t idx = static_cast<size_t>(clampedLvl - 1);

    m_hwDpiIndices = dpiIndices;
    m_hwDpiColors = dpiColors;
    m_inExtraMode = inExtraMode;
    m_ready = true;

    m_dpiLevel = clampedLvl;
    m_dpiColor = QColor(dpiColors[idx].r, dpiColors[idx].g, dpiColors[idx].b);
    m_dpiLabel = dpiLabelForIndex(dpiIndices[idx]);
    m_lastReadTime = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));

    // Mark fresh hardware transfer so timer doesn't immediately re-poll
    m_lastInterruptMs.store(steadyNowMs());
    m_interruptSeq.fetch_add(1);

    emit dpiStateChanged();
}

void ThemeDpiMonitor::readNow() {
    triggerRead(true, false);
}

void ThemeDpiMonitor::resetDefaults() {
    m_autoSync = false;
    m_intervalSec = 3;
    m_pollTimer.stop();
    stopPassiveWatcher();
    saveSettings();
    emit settingsChanged();
}

void ThemeDpiMonitor::joinReadThreadIfNeeded() {
    if (m_readThread.joinable()) {
        m_readThread.join();
    }
}

void ThemeDpiMonitor::stopPassiveWatcher() {
    m_watcherRun.store(false);
    if (m_watcherThread.joinable()) {
        m_watcherThread.join();
    }
}

void ThemeDpiMonitor::startPassiveWatcher() {
    if (m_shuttingDown.load() || !m_deviceAccessAvailable) return;
    if (m_watcherRun.exchange(true)) return;
    if (m_watcherThread.joinable()) m_watcherThread.join();

    m_watcherThread = std::thread([this]() {
        int fd = -1;
        while (m_watcherRun.load() && !m_shuttingDown.load()) {
            if (fd < 0) {
                auto path = M700ADriver::findDevicePath();
                if (path.has_value()) {
                    fd = ::open(path->c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
                    if (fd >= 0) {
                        uint8_t drain[64];
                        while (::read(fd, drain, sizeof(drain)) > 0) {}
                    }
                }
                if (fd < 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(400));
                    continue;
                }
            }

            struct pollfd pfd{};
            pfd.fd = fd;
            pfd.events = POLLIN;
            const int prc = ::poll(&pfd, 1, 200);
            if (prc < 0) {
                if (errno == EINTR) continue;
                ::close(fd);
                fd = -1;
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                continue;
            }
            if (prc == 0) continue;

            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                ::close(fd);
                fd = -1;
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                continue;
            }

            if (pfd.revents & POLLIN) {
                uint8_t buf[64]{};
                int batchLatestIdx = -1;
                bool sawAnyDpiReport = false;

                // Drain ALL currently queued reports and coalesce rapid bursts to the latest DPI index!
                while (m_watcherRun.load() && !m_shuttingDown.load()) {
                    const ssize_t n = ::read(fd, buf, sizeof(buf));
                    if (n < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        if (errno == EINTR) continue;
                        ::close(fd);
                        fd = -1;
                        break;
                    }
                    if (n == 0) {
                        ::close(fd);
                        fd = -1;
                        break;
                    }
                    // Ignore transient packets while editor is writing/reading EEPROM
                    if (m_editorBusy.load()) continue;

                    // Delux M700A (Instant A725) passive DPI Interrupt Reports:
                    //   DPI+:      0x06, 0xF8, (0x00..0x05) -> (0x80 | newIdx)
                    //   DPI-:      0x06, 0xF6, (0x00..0x05) -> (0x80 | newIdx)
                    //   DPI Cycle: 0x06, 0xF7, (0x00..0x05) -> (0x80 | newIdx)
                    if (n >= 3 && buf[0] == 0x06 && (buf[1] == 0xF6 || buf[1] == 0xF7 || buf[1] == 0xF8)) {
                        sawAnyDpiReport = true;
                        if ((buf[2] & 0x80) != 0) {
                            const int newIdx = static_cast<int>(buf[2] & 0x07);
                            if (newIdx >= 0 && newIdx <= 5) {
                                batchLatestIdx = newIdx;
                            }
                        }
                    }
                }

                if (sawAnyDpiReport) {
                    // Push back periodic EEPROM polling immediately so it never collides with rapid clicking
                    m_lastInterruptMs.store(steadyNowMs());
                }

                if (batchLatestIdx >= 0) {
                    m_interruptSeq.fetch_add(1);
                    m_latestInterruptIdx.store(batchLatestIdx);

                    // Coalesce main-thread UI dispatches: only queue ONE invokeMethod if none is pending!
                    bool expected = false;
                    if (m_interruptUiPending.compare_exchange_strong(expected, true)) {
                        QMetaObject::invokeMethod(this, [this]() {
                            m_interruptUiPending.store(false);
                            if (m_shuttingDown.load()) return;

                            const int latestIdx = m_latestInterruptIdx.load();
                            if (latestIdx < 0 || latestIdx > 5) return;

                            const size_t sIdx = static_cast<size_t>(latestIdx);
                            const int newLevel = latestIdx + 1;

                            m_dpiLevel = newLevel;
                            m_lastReadTime = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));

                            if (m_ready) {
                                const auto &col = m_hwDpiColors[sIdx];
                                m_dpiColor = QColor(col.r, col.g, col.b);
                                m_dpiLabel = dpiLabelForIndex(m_hwDpiIndices[sIdx]);
                                emit dpiStateChanged();
                            } else {
                                emit dpiStateChanged();
                                triggerRead(false, false);
                            }
                        }, Qt::QueuedConnection);
                    }
                }
            }
        }
        if (fd >= 0) ::close(fd);
    });
}

void ThemeDpiMonitor::triggerRead(bool userTriggered, bool levelOnlyFast) {
    if (m_shuttingDown.load() || m_editorBusy.load()) return;

    auto loc = [this](const QString &ar, const QString &en, const QString &ru, const QString &zh) -> QString {
        if (m_language == QStringLiteral("en")) return en;
        if (m_language == QStringLiteral("ru")) return ru;
        if (m_language == QStringLiteral("zh")) return zh;
        return ar;
    };

    if (!m_deviceAccessAvailable) {
        if (userTriggered) {
            emit statusFeedback(loc(
                QStringLiteral("تعذرت قراءة لون DPI للثيم: صلاحية الوصول للماوس (--device=all) معطّلة."),
                QStringLiteral("Cannot read theme DPI color: mouse device access (--device=all) is disabled."),
                QStringLiteral("Не удалось считать цвет DPI для темы: доступ к мыши (--device=all) отключён."),
                QStringLiteral("无法读取主题 DPI 颜色：鼠标设备权限 (--device=all) 已关闭。")
            ), true);
        }
        return;
    }

    bool expected = false;
    if (!m_readBusy.compare_exchange_strong(expected, true)) return;
    joinReadThreadIfNeeded();

    const uint64_t seqBefore = m_interruptSeq.load();

    m_readThread = std::thread([this, userTriggered, levelOnlyFast, seqBefore, loc]() {
        if (levelOnlyFast) {
            int fastLevel = 0;
            const bool ok = M700ADriver::readActiveDpiLevelOnly(fastLevel);
            if (m_shuttingDown.load()) {
                m_readBusy.store(false);
                return;
            }
            QMetaObject::invokeMethod(this, [this, ok, fastLevel, seqBefore]() {
                m_readBusy.store(false);
                if (m_shuttingDown.load() || !ok || !m_ready) return;

                // CRITICAL: If user clicked DPI while this read was in flight, DISCARD stale EEPROM level!
                if (m_interruptSeq.load() != seqBefore) return;

                const int clampedLvl = std::clamp(fastLevel, 1, 6);
                const size_t idx = static_cast<size_t>(clampedLvl - 1);
                const QColor newCol(m_hwDpiColors[idx].r, m_hwDpiColors[idx].g, m_hwDpiColors[idx].b);
                const QString newLbl = dpiLabelForIndex(m_hwDpiIndices[idx]);

                const bool changed = (m_dpiLevel != clampedLvl) || (m_dpiColor != newCol) || (m_dpiLabel != newLbl);
                m_dpiLevel = clampedLvl;
                m_dpiColor = newCol;
                m_dpiLabel = newLbl;
                m_lastReadTime = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));

                if (changed) {
                    emit dpiStateChanged();
                }
            }, Qt::QueuedConnection);
            return;
        }

        ThemeDpiSnapshot snap;
        const bool ok = M700ADriver::readThemeDpiSnapshot(snap);
        if (m_shuttingDown.load()) {
            m_readBusy.store(false);
            return;
        }
        QMetaObject::invokeMethod(this, [this, ok, snap, userTriggered, seqBefore, loc]() {
            m_readBusy.store(false);
            if (m_shuttingDown.load()) return;

            if (!ok) {
                if (userTriggered) {
                    emit statusFeedback(loc(
                        QStringLiteral("تعذرت قراءة لون DPI الحي من الماوس حالياً — تأكد من توصيل الماوس وصلاحية udev."),
                        QStringLiteral("Failed to read live DPI color — check mouse connection and udev rule."),
                        QStringLiteral("Не удалось считать цвет DPI — проверьте подключение мыши и правило udev."),
                        QStringLiteral("读取实时 DPI 颜色失败 — 请检查鼠标连接与 udev 规则。")
                    ), true);
                }
                return;
            }

            // Always refresh cached hardware palette & DPI table
            m_hwDpiIndices = snap.dpiIndices;
            m_hwDpiColors = snap.dpiColors;
            m_inExtraMode = snap.inExtraMode;
            m_ready = true;

            // If user clicked DPI rapidly WHILE this full read was running, keep the NEWER live interrupt level!
            const bool interruptedDuringRead = (m_interruptSeq.load() != seqBefore);
            const int effectiveLevel = (interruptedDuringRead && m_dpiLevel >= 1 && m_dpiLevel <= 6)
                                           ? m_dpiLevel
                                           : std::clamp(snap.activeDpiLevel, 1, 6);
            const size_t idx = static_cast<size_t>(effectiveLevel - 1);

            m_dpiLevel = effectiveLevel;
            m_dpiColor = QColor(m_hwDpiColors[idx].r, m_hwDpiColors[idx].g, m_hwDpiColors[idx].b);
            m_dpiLabel = dpiLabelForIndex(m_hwDpiIndices[idx]);
            m_lastReadTime = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));

            emit dpiStateChanged();

            if (userTriggered) {
                emit statusFeedback(loc(
                    QStringLiteral("تم تحديث لون DPI للثيم: المستوى %1 (%2 • %3).")
                        .arg(m_dpiLevel).arg(m_dpiLabel, m_dpiColor.name().toUpper()),
                    QStringLiteral("Theme DPI color updated: Level %1 (%2 • %3).")
                        .arg(m_dpiLevel).arg(m_dpiLabel, m_dpiColor.name().toUpper()),
                    QStringLiteral("Цвет DPI темы обновлён: уровень %1 (%2 • %3).")
                        .arg(m_dpiLevel).arg(m_dpiLabel, m_dpiColor.name().toUpper()),
                    QStringLiteral("主题 DPI 颜色已更新：档位 %1（%2 • %3）。")
                        .arg(m_dpiLevel).arg(m_dpiLabel, m_dpiColor.name().toUpper())
                ), false);
            }
        }, Qt::QueuedConnection);
    });
}
