#include "MouseController.h"
#include "../driver/HidKeyMap.h"
#include "../system/AsyncProcess.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMetaObject>
#include <QTime>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <unistd.h>

namespace {
constexpr int kMaxOperationLogChars = 64 * 1024;
constexpr size_t kMaxMacroLibraryLimit = 500;
QString resolveLocalFilePath(const QUrl &url) {
    if (url.isLocalFile()) return url.toLocalFile();
    if (url.scheme().isEmpty() && url.toString().startsWith(QLatin1Char('/'))) return url.toString();
    return {};
}
}

MouseController::MouseController(bool launchedFromAutostart, bool trayAvailable, QObject *parent)
    : QObject(parent)
    , m_bgPortal(this)
    , m_sandbox(this)
    , m_launchedFromAutostart(launchedFromAutostart)
    , m_trayAvailable(trayAvailable) {
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(300);
    connect(&m_saveTimer, &QTimer::timeout, this, &MouseController::saveProfilesToDisk);

    m_hotplugTimer.setInterval(2500);
    connect(&m_hotplugTimer, &QTimer::timeout, this, &MouseController::pollDeviceConnection);

    m_udevWatchTimer.setInterval(5000);
    connect(&m_udevWatchTimer, &QTimer::timeout, this, &MouseController::checkUdevHealth);

    m_appWatchTimer.setInterval(2500);
    connect(&m_appWatchTimer, &QTimer::timeout, this, [this]() {
        if (!m_sandbox.hostAccessAvailable() || !m_cfg.autoSwitchByApp || m_isBusy || m_shuttingDown.load()) return;
        QStringList linked;
        linked.reserve(kProfileCount);
        for (const auto &p : m_cfg.profiles) linked.append(p.linkedApps);
        m_sandbox.pollRunningApps(linked);
    });

    connect(&m_bgPortal, &BackgroundPortal::busyChanged, this, &MouseController::appSettingsChanged);
    connect(&m_bgPortal, &BackgroundPortal::requestFinished, this,
            [this](bool ok, bool granted, const QString &msg) {
        if (ok) {
            m_cfg.autostartEnabled = granted;
            saveProfilesToDisk();
        }
        setStatus(msg, !ok);
        emit appSettingsChanged();
    });

    connect(&m_sandbox, &SandboxManager::hostAccessChanged, this, [this]() {
        if (m_sandbox.hostAccessAvailable() && m_cfg.autoSwitchByApp) {
            if (!m_appWatchTimer.isActive()) m_appWatchTimer.start();
        } else {
            m_appWatchTimer.stop();
            m_autoSwitchedActive = false;
        }
        emit hostAccessChanged();
        emit appSettingsChanged();
    });
    connect(&m_sandbox, &SandboxManager::appPermissionsChanged, this, &MouseController::appPermissionsChanged);
    connect(&m_sandbox, &SandboxManager::udevStatusChanged, this, &MouseController::udevStatusChanged);
    connect(&m_sandbox, &SandboxManager::udevStatusChanged, this, &MouseController::appPermissionsChanged);
    connect(&m_sandbox, &SandboxManager::udevMissingDetected, this, &MouseController::udevMissingDetected);
    connect(&m_sandbox, &SandboxManager::udevInstalledSucceeded, this, &MouseController::pollDeviceConnection);
    connect(&m_sandbox, &SandboxManager::installedFlatpakAppsChanged, this, &MouseController::installedFlatpakAppsChanged);
    connect(&m_sandbox, &SandboxManager::runningAppsMatched, this, &MouseController::onRunningAppsMatched);
    connect(&m_sandbox, &SandboxManager::statusMessage, this, &MouseController::setStatus);

    initDefaultProfiles();
    m_cfg.macroLibrary = ProfileStorage::defaultMacroLibrary();
    const bool loadedFromDisk = ProfileStorage::loadFromDisk(m_cfg);
    m_sandbox.setLanguage(m_cfg.language);
    m_sandbox.probeHostAccessOnStartup();

    auto detectedPath = M700ADriver::findDevicePath();
    if (detectedPath.has_value()) {
        m_connected = true;
        m_devicePath = QString::fromStdString(*detectedPath);

        const bool canAccess = (::access(detectedPath->c_str(), R_OK | W_OK) == 0);
        if (!canAccess) {
            const QString errMsg = translateDriverError(DriverError::OpenFailed, *detectedPath + ": Permission denied");
            m_sandbox.markUdevUnhealthy(errMsg);
            setStatus(errMsg, true);
        } else if (!loadedFromDisk || m_cfg.autoReadOnStartup) {
            setStatus(loc(
                QStringLiteral("تم الاتصال بـ %1 — جاري قراءة إعدادات الماوس...").arg(m_devicePath),
                QStringLiteral("Connected to %1 — reading mouse settings...").arg(m_devicePath),
                QStringLiteral("Подключено к %1 — чтение настроек мыши...").arg(m_devicePath),
                QStringLiteral("已连接到 %1 — 正在读取鼠标设置...").arg(m_devicePath)
            ), false);
            QTimer::singleShot(150, this, [this]() {
                if (!m_shuttingDown.load() && m_connected && m_sandbox.udevHealthy()) refreshFromMouse();
            });
        } else if (m_cfg.applyOnStartup) {
            setStatus(loc(
                QStringLiteral("تم الاتصال بـ %1 — جاري تطبيق البروفايل عند التشغيل...").arg(m_devicePath),
                QStringLiteral("Connected to %1 — applying startup profile...").arg(m_devicePath),
                QStringLiteral("Подключено к %1 — применение стартового профиля...").arg(m_devicePath),
                QStringLiteral("已连接到 %1 — 正在应用启动配置文件...").arg(m_devicePath)
            ), false);
            QTimer::singleShot(150, this, [this]() {
                if (!m_shuttingDown.load() && m_connected && m_sandbox.udevHealthy()) applyToMouse();
            });
        } else {
            setStatus(loc(
                QStringLiteral("تم تحميل البروفايلات الخمسة بنجاح • متصل بـ %1").arg(m_devicePath),
                QStringLiteral("5 profiles loaded • Connected to %1").arg(m_devicePath),
                QStringLiteral("Загружено 5 профилей • Подключено к %1").arg(m_devicePath),
                QStringLiteral("已加载 5 组配置文件 • 已连接到 %1").arg(m_devicePath)
            ), false);
        }
    } else {
        m_connected = false;
        if (!m_sandbox.deviceAccessAvailable()) {
            setStatus(loc(
                QStringLiteral("وضع العزل الأقصى نشط: صلاحية الوصول للماوس (--device=all) معطّلة افتراضياً. فعّلها من نافذة الاستقبال أو «التحقق من الصلاحيات» ثم أعد تشغيل التطبيق."),
                QStringLiteral("Strict isolation active: mouse access (--device=all) is disabled by default. Enable it via the Welcome wizard or Verify Permissions, then restart."),
                QStringLiteral("Строгая изоляция: доступ к мыши (--device=all) отключён по умолчанию. Включите его в мастере приветствия или окне проверки прав и перезапустите приложение."),
                QStringLiteral("严格隔离模式已启用：鼠标访问权限 (--device=all) 默认关闭。请通过欢迎向导或权限检查窗口复制启用命令并重启应用。")
            ), false);
        } else {
            setStatus(translateDriverError(DriverError::NoDevice, ""), true);
        }
    }

    // Inside strict Flatpak isolation without --device=all, /dev/hidraw* can never appear until restart,
    // so skip periodic hotplug & udev polling timers to keep idle CPU & D-Bus wakeups at zero!
    const bool canDetectHardware = !SystemUtils::isRunningInFlatpak() || m_sandbox.deviceAccessAvailable();
    if (canDetectHardware) {
        if (m_cfg.autoDetectDevice && (!SystemUtils::isRunningInFlatpak() || m_sandbox.deviceAccessAvailable())) m_hotplugTimer.start();
        m_udevWatchTimer.start();
        QTimer::singleShot(350, this, &MouseController::checkUdevHealth);
    } else if (m_sandbox.hostAccessAvailable()) {
        m_udevWatchTimer.start();
        QTimer::singleShot(350, this, &MouseController::checkUdevHealth);
    }
    if (m_sandbox.hostAccessAvailable() && m_cfg.autoSwitchByApp) m_appWatchTimer.start();

    connect(this, &MouseController::stateChanged, this, &MouseController::recomputeUnsavedChanges);
    connect(this, &MouseController::languageChanged, this, &MouseController::recomputeUnsavedChanges);
    connect(this, &MouseController::languageChanged, this, &MouseController::appPermissionsChanged);

    syncBaselineFromActive();

    emit languageChanged();
    emit hostAccessChanged();
    emit appPermissionsChanged();
    emit appSettingsChanged();
    emit profilesMetaChanged();
    emit activeProfileChanged();
    emit macroLibraryChanged();
    emit stateChanged();
}

MouseController::~MouseController() {
    m_shuttingDown.store(true);
    m_bgPortal.cancel();
    m_hotplugTimer.stop();
    m_udevWatchTimer.stop();
    m_appWatchTimer.stop();
    joinWorkerIfNeeded();
    if (m_saveTimer.isActive()) {
        m_saveTimer.stop();
        saveProfilesToDisk();
    }
}

void MouseController::joinWorkerIfNeeded() {
    if (m_workerThread.joinable()) m_workerThread.join();
}

QString MouseController::loc(const QString &ar, const QString &en, const QString &ru, const QString &zh) const {
    return SystemUtils::locText(m_cfg.language, ar, en, ru, zh);
}

void MouseController::appendLogLine(const QString &line) {
    const QString stamp = QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    if (!m_operationLog.isEmpty()) m_operationLog += QLatin1Char('\n');
    m_operationLog += QStringLiteral("[%1] %2").arg(stamp, line);
    if (m_operationLog.size() > kMaxOperationLogChars) {
        m_operationLog = m_operationLog.right(kMaxOperationLogChars);
    }
}

QString MouseController::translateDriverError(DriverError err, const std::string &detail) const {
    const QString d = QString::fromStdString(detail);
    switch (err) {
    case DriverError::NoDevice:
        return loc(
            QStringLiteral("لم يتم العثور على ماوس Delux M700A (30FA:1040). تأكد من توصيله."),
            QStringLiteral("Delux M700A mouse (30FA:1040) not found. Make sure it is connected."),
            QStringLiteral("Мышь Delux M700A (30FA:1040) не найдена. Проверьте подключение."),
            QStringLiteral("未找到 Delux M700A 鼠标 (30FA:1040)，请检查连接。")
        );
    case DriverError::OpenFailed:
        return loc(
            QStringLiteral("تعذر فتح المنفذ %1").arg(d),
            QStringLiteral("Failed to open device %1").arg(d),
            QStringLiteral("Не удалось открыть устройство %1").arg(d),
            QStringLiteral("无法打开设备 %1").arg(d)
        );
    case DriverError::ReadFailed:
        return loc(
            QStringLiteral("فشلت قراءة ذاكرة الماوس عبر HID Feature Report."),
            QStringLiteral("Failed to read mouse memory via HID Feature Report."),
            QStringLiteral("Ошибка чтения памяти мыши через HID Feature Report."),
            QStringLiteral("通过 HID Feature Report 读取鼠标内存失败。")
        );
    case DriverError::WriteFailed:
        return loc(
            QStringLiteral("فشل إرسال الإعدادات إلى ذاكرة الماوس."),
            QStringLiteral("Failed to write settings to mouse memory."),
            QStringLiteral("Ошибка записи настроек в память мыши."),
            QStringLiteral("写入设置到鼠标内存失败。")
        );
    case DriverError::NoLeftClick:
        return loc(
            QStringLiteral("لا يمكن الحفظ بدون تعيين زر واحد على الأقل لوظيفة (النقر الأيسر) حفاظاً على التحكم بالماوس."),
            QStringLiteral("Cannot save without at least one button assigned to Left Click."),
            QStringLiteral("Нельзя сохранить: хотя бы одна кнопка должна быть назначена на ЛКМ."),
            QStringLiteral("无法保存：必须至少保留一个按键为“左键点击”。")
        );
    case DriverError::None:
    default:
        return QString();
    }
}

void MouseController::initDefaultProfiles() {
    for (int i = 0; i < kProfileCount; ++i) {
        m_cfg.profiles[i] = ProfileStorage::defaultProfileTemplate(i, m_cfg.language);
    }
    m_cfg.activeProfile = 0;
    m_cfg.autoApplyOnSwitch = false;
}

void MouseController::saveProfilesToDisk() { ProfileStorage::saveToDisk(m_cfg); }
void MouseController::scheduleSaveToDisk() { m_saveTimer.start(); }

void MouseController::setStatus(const QString &msg, bool isError) {
    m_statusMessage = msg;
    m_statusError = isError;
    emit statusMessageChanged();
}

void MouseController::pollDeviceConnection() {
    if (m_isBusy || (SystemUtils::isRunningInFlatpak() && !m_sandbox.deviceAccessAvailable())) return;
    auto found = M700ADriver::findDevicePath();
    const bool nowConnected = found.has_value();

    if (nowConnected && !m_connected) {
        m_connected = true;
        m_devicePath = QString::fromStdString(*found);
        const bool canAccess = (::access(found->c_str(), R_OK | W_OK) == 0);
        if (!canAccess || !m_sandbox.udevHealthy()) {
            m_sandbox.checkUdevHealth();
            emit stateChanged();
            return;
        }
        if (m_cfg.autoReadOnStartup) refreshFromMouse();
        else if (m_cfg.applyOnStartup) applyToMouse();
        else {
            setStatus(loc(
                QStringLiteral("تم اكتشاف اتصال الماوس تلقائياً (%1)").arg(m_devicePath),
                QStringLiteral("Mouse connected automatically (%1)").arg(m_devicePath),
                QStringLiteral("Мышь подключена (%1)").arg(m_devicePath),
                QStringLiteral("已自动检测到鼠标连接（%1）").arg(m_devicePath)
            ), false);
            emit stateChanged();
        }
    } else if (!nowConnected && m_connected) {
        m_connected = false;
        m_devicePath.clear();
        setStatus(translateDriverError(DriverError::NoDevice, ""), true);
        emit stateChanged();
    } else if (nowConnected && m_devicePath != QString::fromStdString(*found)) {
        m_devicePath = QString::fromStdString(*found);
        emit stateChanged();
    }
}

void MouseController::onRunningAppsMatched(int matchedProfile, const QString &matchedToken) {
    if (m_shuttingDown.load() || !m_sandbox.hostAccessAvailable() || !m_cfg.autoSwitchByApp || m_isBusy) return;

    if (matchedProfile >= 0 && matchedProfile < kProfileCount) {
        m_autoSwitchedActive = true;
        if (m_cfg.activeProfile != matchedProfile) {
            m_cfg.activeProfile = matchedProfile;
            syncBaselineFromActive();
            saveProfilesToDisk();
            emit activeProfileChanged();
            emit profilesMetaChanged();
            emit stateChanged();
            setStatus(loc(
                QStringLiteral("تم التبديل تلقائياً إلى البروفايل %1 (%2) بسبب تشغيل: %3").arg(m_cfg.activeProfile + 1).arg(activeProfileName(), matchedToken),
                QStringLiteral("Auto-switched to Profile %1 (%2) for app: %3").arg(m_cfg.activeProfile + 1).arg(activeProfileName(), matchedToken),
                QStringLiteral("Автопереключение на Профиль %1 (%2) для: %3").arg(m_cfg.activeProfile + 1).arg(activeProfileName(), matchedToken),
                QStringLiteral("检测到 %3 运行，已自动切换到配置文件 %1（%2）").arg(matchedToken).arg(m_cfg.activeProfile + 1).arg(activeProfileName())
            ), false);
            if (m_connected && m_sandbox.udevHealthy()) applyToMouse();
        }
    } else if (m_autoSwitchedActive) {
        const int restore = std::clamp(m_cfg.fallbackProfileIndex, 0, kProfileCount - 1);
        m_autoSwitchedActive = false;
        if (m_cfg.activeProfile != restore) {
            m_cfg.activeProfile = restore;
            syncBaselineFromActive();
            saveProfilesToDisk();
            emit activeProfileChanged();
            emit profilesMetaChanged();
            emit stateChanged();
            setStatus(loc(
                QStringLiteral("تمت العودة تلقائياً إلى بروفايل العودة المحدد: %1 (%2)").arg(m_cfg.activeProfile + 1).arg(activeProfileName()),
                QStringLiteral("Returned automatically to fallback Profile %1 (%2)").arg(m_cfg.activeProfile + 1).arg(activeProfileName()),
                QStringLiteral("Автовозврат к выбранному профилю: %1 (%2)").arg(m_cfg.activeProfile + 1).arg(activeProfileName()),
                QStringLiteral("应用已关闭，已自动恢复到指定的默认配置 %1（%2）").arg(m_cfg.activeProfile + 1).arg(activeProfileName())
            ), false);
            if (m_connected && m_sandbox.udevHealthy()) applyToMouse();
        }
    }
}

void MouseController::setLanguage(const QString &lang) {
    if (lang != QStringLiteral("ar") && lang != QStringLiteral("en") &&
        lang != QStringLiteral("ru") && lang != QStringLiteral("zh")) return;
    if (m_cfg.language != lang) {
        m_cfg.language = lang;
        m_sandbox.setLanguage(lang);
        saveProfilesToDisk();
        emit languageChanged();
        setStatus(loc(
            QStringLiteral("تم تغيير اللغة إلى العربية."),
            QStringLiteral("Language switched to English."),
            QStringLiteral("Язык интерфейса изменён на русский."),
            QStringLiteral("界面语言已切换为中文。")
        ), false);
    }
}

void MouseController::setAutostartEnabled(bool enabled) {
    if (m_cfg.autostartEnabled == enabled) return;
    m_bgPortal.requestAutostart(enabled, m_cfg.language);
}

void MouseController::setStartMinimized(bool enabled) {
    if (m_cfg.startMinimized != enabled) { m_cfg.startMinimized = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setMinimizeToTray(bool enabled) {
    if (m_cfg.minimizeToTray != enabled) { m_cfg.minimizeToTray = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setAutoReadOnStartup(bool enabled) {
    if (m_cfg.autoReadOnStartup != enabled) {
        m_cfg.autoReadOnStartup = enabled;
        if (enabled && m_cfg.applyOnStartup) m_cfg.applyOnStartup = false;
        saveProfilesToDisk();
        emit appSettingsChanged();
    }
}

void MouseController::setApplyOnStartup(bool enabled) {
    if (m_cfg.applyOnStartup != enabled) {
        m_cfg.applyOnStartup = enabled;
        if (enabled && m_cfg.autoReadOnStartup) m_cfg.autoReadOnStartup = false;
        saveProfilesToDisk();
        emit appSettingsChanged();
    }
}

void MouseController::setAutoDetectDevice(bool enabled) {
    if (m_cfg.autoDetectDevice != enabled) {
        m_cfg.autoDetectDevice = enabled;
        if (enabled) m_hotplugTimer.start();
        else m_hotplugTimer.stop();
        saveProfilesToDisk();
        emit appSettingsChanged();
    }
}

void MouseController::setActiveThemeAccent(const QColor &color) {
    if (!color.isValid()) return;
    if (m_activeThemeAccent != color) {
        m_activeThemeAccent = color;
        emit activeThemeAccentChanged();
    }
}

void MouseController::reportExternalStatus(const QString &msg, bool isError) {
    setStatus(msg, isError);
}

void MouseController::setShowHiddenEffects(bool enabled) {
    if (m_cfg.showHiddenEffects == enabled) return;
    m_cfg.showHiddenEffects = enabled;
    bool anyChanged = false, activeChanged = false;
    if (!enabled) {
        auto isHiddenMode = [](int mode) { return mode == 0x01 || mode == 0x04 || mode == 0x09; };
        for (int i = 0; i < kProfileCount; ++i) {
            if (isHiddenMode(m_cfg.profiles[i].state.ledMode)) {
                m_cfg.profiles[i].state.ledMode = 0x02;
                anyChanged = true;
                if (i == m_cfg.activeProfile) activeChanged = true;
            }
        }
    }
    saveProfilesToDisk();
    emit appSettingsChanged();
    if (anyChanged) emit stateChanged();
    if (activeChanged && m_connected && m_sandbox.udevHealthy()) applyToMouse();
}

void MouseController::setShowHexCodes(bool enabled) {
    if (m_cfg.showHexCodes != enabled) { m_cfg.showHexCodes = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setEnableKeyCapture(bool enabled) {
    if (m_cfg.enableKeyCapture == enabled) return;
    m_cfg.enableKeyCapture = enabled;
    bool anyChanged = false, activeChanged = false;
    if (!enabled) {
        for (int p = 0; p < kProfileCount; ++p) {
            for (int s = 0; s < 8; ++s) {
                int code = m_cfg.profiles[p].state.buttonCodes[s];
                if (code == 0x0D || code == 0x0E) {
                    m_cfg.profiles[p].state.buttonCodes[s] = ProfileStorage::kDefaultButtonCodes[s];
                    m_cfg.profiles[p].buttonMacroIds[s].clear();
                    anyChanged = true;
                    if (p == m_cfg.activeProfile) activeChanged = true;
                }
            }
        }
    }
    saveProfilesToDisk();
    emit appSettingsChanged();
    if (anyChanged) emit stateChanged();
    if (activeChanged && m_connected && m_sandbox.udevHealthy()) applyToMouse();
}

void MouseController::setShowProgressAndLog(bool enabled) {
    if (m_cfg.showProgressAndLog != enabled) { m_cfg.showProgressAndLog = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setShowStartupSplash(bool enabled) {
    if (m_cfg.showStartupSplash != enabled) { m_cfg.showStartupSplash = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setShowRgbPreviewStrip(bool enabled) {
    if (m_cfg.showRgbPreviewStrip != enabled) { m_cfg.showRgbPreviewStrip = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setAnimatedUi(bool enabled) {
    if (m_cfg.animatedUi != enabled) { m_cfg.animatedUi = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiThemePreset(const QString &preset) {
    static const QStringList kValidPresets = {
        QStringLiteral("oled_pure"), QStringLiteral("oled_cyan"), QStringLiteral("oled_crimson"),
        QStringLiteral("oled_violet"), QStringLiteral("oled_gold"), QStringLiteral("oled_ice"),
        QStringLiteral("oled_magenta"), QStringLiteral("oled_mono"),
        QStringLiteral("aurora"), QStringLiteral("cyber"), QStringLiteral("emerald"),
        QStringLiteral("crimson"), QStringLiteral("amethyst"), QStringLiteral("ocean"),
        QStringLiteral("solar"), QStringLiteral("sakura"), QStringLiteral("nord"),
        QStringLiteral("dracula"), QStringLiteral("frost"), QStringLiteral("sand")
    };
    if (!kValidPresets.contains(preset)) return;
    if (m_cfg.uiThemePreset != preset) {
        m_cfg.uiThemePreset = preset;
        if (preset == QStringLiteral("frost") || preset == QStringLiteral("sand")) {
            m_cfg.uiTrueBlackOled = false;
        }
        saveProfilesToDisk();
        emit appSettingsChanged();
    }
}

void MouseController::setUiAccentMode(const QString &mode) {
    if (mode != QStringLiteral("autoDpi") && mode != QStringLiteral("preset") && mode != QStringLiteral("custom")) return;
    if (m_cfg.uiAccentMode != mode) { m_cfg.uiAccentMode = mode; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiCustomAccent(const QString &hexColor) {
    const QColor c(hexColor);
    if (!c.isValid()) return;
    const QString norm = c.name(QColor::HexRgb);
    if (m_cfg.uiCustomAccent != norm) { m_cfg.uiCustomAccent = norm; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiGlassOpacity(double opacity) {
    const double clamped = std::clamp(opacity, 0.35, 0.98);
    if (std::abs(m_cfg.uiGlassOpacity - clamped) > 0.005) { m_cfg.uiGlassOpacity = clamped; scheduleSaveToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiCornerRadius(int radius) {
    const int clamped = std::clamp(radius, 0, 24);
    if (m_cfg.uiCornerRadius != clamped) { m_cfg.uiCornerRadius = clamped; scheduleSaveToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiGlowIntensity(double intensity) {
    const double clamped = std::clamp(intensity, 0.0, 1.0);
    if (std::abs(m_cfg.uiGlowIntensity - clamped) > 0.005) { m_cfg.uiGlowIntensity = clamped; scheduleSaveToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiBackgroundStyle(const QString &style) {
    if (style != QStringLiteral("aurora") && style != QStringLiteral("waves") &&
        style != QStringLiteral("grid") && style != QStringLiteral("solid") &&
        style != QStringLiteral("oledPure")) return;
    if (m_cfg.uiBackgroundStyle != style) { m_cfg.uiBackgroundStyle = style; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiTrueBlackOled(bool enabled) {
    if (m_cfg.uiTrueBlackOled != enabled) { m_cfg.uiTrueBlackOled = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiBorderWidth(double width) {
    const double clamped = std::clamp(width, 0.0, 3.0);
    if (std::abs(m_cfg.uiBorderWidth - clamped) > 0.02) { m_cfg.uiBorderWidth = clamped; scheduleSaveToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiAnimationSpeed(double speed) {
    const double clamped = std::clamp(speed, 0.4, 2.0);
    if (std::abs(m_cfg.uiAnimationSpeed - clamped) > 0.02) { m_cfg.uiAnimationSpeed = clamped; scheduleSaveToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiCompactMode(bool compact) {
    if (m_cfg.uiCompactMode != compact) { m_cfg.uiCompactMode = compact; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiUseCustomSecondary(bool enabled) {
    if (m_cfg.uiUseCustomSecondary != enabled) { m_cfg.uiUseCustomSecondary = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiCustomSecondary(const QString &hexColor) {
    const QColor c(hexColor);
    if (!c.isValid()) return;
    const QString norm = c.name(QColor::HexRgb);
    if (m_cfg.uiCustomSecondary != norm) { m_cfg.uiCustomSecondary = norm; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiBorderStyle(const QString &style) {
    if (style != QStringLiteral("accent") && style != QStringLiteral("gradient") &&
        style != QStringLiteral("rainbow") && style != QStringLiteral("subtle") &&
        style != QStringLiteral("custom") && style != QStringLiteral("none")) return;
    if (m_cfg.uiBorderStyle != style) { m_cfg.uiBorderStyle = style; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiCustomBorderColor(const QString &hexColor) {
    const QColor c(hexColor);
    if (!c.isValid()) return;
    const QString norm = c.name(QColor::HexRgb);
    if (m_cfg.uiCustomBorderColor != norm) { m_cfg.uiCustomBorderColor = norm; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiBorderOpacity(double opacity) {
    const double clamped = std::clamp(opacity, 0.10, 1.0);
    if (std::abs(m_cfg.uiBorderOpacity - clamped) > 0.01) { m_cfg.uiBorderOpacity = clamped; scheduleSaveToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiWindowEdgeGlow(bool enabled) {
    if (m_cfg.uiWindowEdgeGlow != enabled) { m_cfg.uiWindowEdgeGlow = enabled; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiWindowButtonStyle(const QString &style) {
    if (style != QStringLiteral("circle") && style != QStringLiteral("pill") && style != QStringLiteral("traffic")) return;
    if (m_cfg.uiWindowButtonStyle != style) { m_cfg.uiWindowButtonStyle = style; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiTitlebarStyle(const QString &style) {
    if (style != QStringLiteral("glass") && style != QStringLiteral("accent") &&
        style != QStringLiteral("transparent") && style != QStringLiteral("oled")) return;
    if (m_cfg.uiTitlebarStyle != style) { m_cfg.uiTitlebarStyle = style; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiButtonShape(const QString &shape) {
    if (shape != QStringLiteral("rounded") && shape != QStringLiteral("pill") &&
        shape != QStringLiteral("sharp") && shape != QStringLiteral("squircle") &&
        shape != QStringLiteral("leaf") && shape != QStringLiteral("chamfer")) return;
    if (m_cfg.uiButtonShape != shape) { m_cfg.uiButtonShape = shape; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiButtonStyle(const QString &style) {
    if (style != QStringLiteral("glass") && style != QStringLiteral("solid") &&
        style != QStringLiteral("outline") && style != QStringLiteral("neon") &&
        style != QStringLiteral("gradient") && style != QStringLiteral("soft")) return;
    if (m_cfg.uiButtonStyle != style) { m_cfg.uiButtonStyle = style; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiCardEdgeStyle(const QString &style) {
    if (style != QStringLiteral("glass") && style != QStringLiteral("neon") &&
        style != QStringLiteral("gradient") && style != QStringLiteral("chamfer") &&
        style != QStringLiteral("flat")) return;
    if (m_cfg.uiCardEdgeStyle != style) { m_cfg.uiCardEdgeStyle = style; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setUiWindowShape(const QString &shape) {
    if (shape != QStringLiteral("sharp") && shape != QStringLiteral("rounded") &&
        shape != QStringLiteral("extra") && shape != QStringLiteral("chamfer")) return;
    if (m_cfg.uiWindowShape != shape) { m_cfg.uiWindowShape = shape; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::resetUiTheme() {
    m_cfg.animatedUi = true;
    m_cfg.uiThemePreset = QStringLiteral("oled_pure");
    m_cfg.uiAccentMode = QStringLiteral("preset");
    m_cfg.uiCustomAccent = QStringLiteral("#15803d");
    m_cfg.uiGlassOpacity = 0.84;
    m_cfg.uiCornerRadius = 14;
    m_cfg.uiGlowIntensity = 0.65;
    m_cfg.uiBackgroundStyle = QStringLiteral("oledPure");
    m_cfg.uiTrueBlackOled = true;
    m_cfg.uiBorderWidth = 1.4;
    m_cfg.uiAnimationSpeed = 1.0;
    m_cfg.uiCompactMode = false;
    m_cfg.uiUseCustomSecondary = false;
    m_cfg.uiCustomSecondary = QStringLiteral("#166534");
    m_cfg.uiBorderStyle = QStringLiteral("accent");
    m_cfg.uiCustomBorderColor = QStringLiteral("#15803d");
    m_cfg.uiBorderOpacity = 0.85;
    m_cfg.uiWindowEdgeGlow = true;
    m_cfg.uiWindowButtonStyle = QStringLiteral("circle");
    m_cfg.uiTitlebarStyle = QStringLiteral("oled");
    m_cfg.uiButtonShape = QStringLiteral("rounded");
    m_cfg.uiButtonStyle = QStringLiteral("glass");
    m_cfg.uiCardEdgeStyle = QStringLiteral("glass");
    m_cfg.uiWindowShape = QStringLiteral("rounded");
    saveProfilesToDisk();
    emit appSettingsChanged();
}

void MouseController::setAutoSwitchByApp(bool enabled) {
    if (enabled && !m_sandbox.hostAccessAvailable()) {
        m_appWatchTimer.stop();
        m_autoSwitchedActive = false;
        setStatus(loc(
            QStringLiteral("تتطلب ميزة التبديل التلقائي حسب التطبيق تفعيل صلاحية النظام المضيف يدوياً (org.freedesktop.Flatpak) وإعادة تشغيل التطبيق."),
            QStringLiteral("Auto-switching by app requires manually enabling the optional host permission (org.freedesktop.Flatpak) and restarting."),
            QStringLiteral("Автопереключение по приложениям требует ручного включения права хоста (org.freedesktop.Flatpak) и перезапуска."),
            QStringLiteral("按应用自动切换配置需要手动启用宿主机通信权限 (org.freedesktop.Flatpak) 并重启应用。")
        ), true);
        emit appSettingsChanged();
        return;
    }
    if (m_cfg.autoSwitchByApp == enabled) return;
    m_cfg.autoSwitchByApp = enabled;
    if (enabled && m_sandbox.hostAccessAvailable()) m_appWatchTimer.start();
    else { m_appWatchTimer.stop(); m_autoSwitchedActive = false; }
    saveProfilesToDisk();
    emit appSettingsChanged();
}

void MouseController::setFallbackProfileIndex(int index) {
    const int clamped = std::clamp(index, 0, kProfileCount - 1);
    if (m_cfg.fallbackProfileIndex != clamped) { m_cfg.fallbackProfileIndex = clamped; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setFirstRunCompleted(bool completed) {
    if (m_cfg.firstRunCompleted != completed) { m_cfg.firstRunCompleted = completed; saveProfilesToDisk(); emit appSettingsChanged(); }
}

void MouseController::setAutoApplyOnSwitch(bool enabled) {
    if (m_cfg.autoApplyOnSwitch != enabled) { m_cfg.autoApplyOnSwitch = enabled; saveProfilesToDisk(); emit profilesMetaChanged(); }
}

QStringList MouseController::profileNames() const {
    QStringList list;
    for (const auto &p : m_cfg.profiles) list.append(p.name);
    return list;
}

void MouseController::selectProfile(int index) {
    const int clamped = std::clamp(index, 0, kProfileCount - 1);
    if (m_cfg.activeProfile == clamped) return;

    m_cfg.activeProfile = clamped;
    saveProfilesToDisk();
    syncBaselineFromActive();
    emit activeProfileChanged();
    emit profilesMetaChanged();
    emit stateChanged();

    if (m_cfg.autoApplyOnSwitch && m_connected && m_sandbox.udevHealthy()) {
        applyToMouse();
    } else {
        setStatus(loc(
            QStringLiteral("تم التبديل إلى البروفايل %1 (%2) — اضغط (حفظ وتطبيق) لكتابته على الماوس.").arg(m_cfg.activeProfile + 1).arg(activeProfileName()),
            QStringLiteral("Switched to Profile %1 (%2) — Click Save & Apply to write to mouse.").arg(m_cfg.activeProfile + 1).arg(activeProfileName()),
            QStringLiteral("Выбран Профиль %1 (%2) — нажмите «Сохранить и применить» для записи.").arg(m_cfg.activeProfile + 1).arg(activeProfileName()),
            QStringLiteral("已切换到配置文件 %1（%2）— 请点击“保存并应用”写入鼠标。").arg(m_cfg.activeProfile + 1).arg(activeProfileName())
        ), false);
    }
}

void MouseController::setActiveProfileName(const QString &name) {
    QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        trimmed = loc(
            QStringLiteral("بروفايل %1").arg(m_cfg.activeProfile + 1),
            QStringLiteral("Profile %1").arg(m_cfg.activeProfile + 1),
            QStringLiteral("Профиль %1").arg(m_cfg.activeProfile + 1),
            QStringLiteral("配置 %1").arg(m_cfg.activeProfile + 1)
        );
    }
    if (m_cfg.profiles[m_cfg.activeProfile].name != trimmed) {
        m_cfg.profiles[m_cfg.activeProfile].name = trimmed;
        scheduleSaveToDisk();
        emit profilesMetaChanged();
    }
}

void MouseController::setActiveProfileLinkedApps(const QString &apps) {
    const QString trimmed = apps.trimmed();
    if (m_cfg.profiles[m_cfg.activeProfile].linkedApps != trimmed) {
        m_cfg.profiles[m_cfg.activeProfile].linkedApps = trimmed;
        scheduleSaveToDisk();
        emit profilesMetaChanged();
    }
}

void MouseController::refreshInstalledFlatpakApps() { m_sandbox.refreshInstalledFlatpakApps(); }

bool MouseController::isAppLinkedToActiveProfile(const QString &appId) const {
    const QString wanted = appId.trimmed().toLower();
    if (wanted.isEmpty()) return false;
    const QStringList tokens = m_cfg.profiles[m_cfg.activeProfile].linkedApps.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &t : tokens) {
        if (t.trimmed().toLower() == wanted) return true;
    }
    return false;
}

void MouseController::toggleAppForActiveProfile(const QString &appId) {
    const QString clean = appId.trimmed();
    if (clean.isEmpty()) return;
    const QStringList tokens = m_cfg.profiles[m_cfg.activeProfile].linkedApps.split(QLatin1Char(','), Qt::SkipEmptyParts);
    QStringList result;
    bool removed = false;
    for (const QString &t : tokens) {
        const QString item = t.trimmed();
        if (item.isEmpty()) continue;
        if (item.compare(clean, Qt::CaseInsensitive) == 0) removed = true;
        else result.append(item);
    }
    if (!removed) result.append(clean);
    setActiveProfileLinkedApps(result.join(QStringLiteral(", ")));
}

void MouseController::resetActiveProfile() {
    m_cfg.profiles[m_cfg.activeProfile] = ProfileStorage::defaultProfileTemplate(m_cfg.activeProfile, m_cfg.language);
    saveProfilesToDisk();
    emit profilesMetaChanged();
    emit activeProfileChanged();
    emit stateChanged();
    setStatus(loc(
        QStringLiteral("تمت إعادة ضبط البروفايل %1 إلى الإعدادات الافتراضية.").arg(m_cfg.activeProfile + 1),
        QStringLiteral("Profile %1 reset to defaults.").arg(m_cfg.activeProfile + 1),
        QStringLiteral("Профиль %1 сброшен к значениям по умолчанию.").arg(m_cfg.activeProfile + 1),
        QStringLiteral("配置文件 %1 已恢复默认设置。").arg(m_cfg.activeProfile + 1)
    ), false);
}

void MouseController::resetAllSettingsAndProfiles() {
    initDefaultProfiles();
    m_cfg.startMinimized = false;
    m_cfg.minimizeToTray = true;
    m_cfg.autoReadOnStartup = true;
    m_cfg.applyOnStartup = false;
    m_cfg.autoDetectDevice = true;
    emit fullResetPerformed();
    m_cfg.showHiddenEffects = false;
    m_cfg.showHexCodes = false;
    m_cfg.enableKeyCapture = true;
    m_cfg.showProgressAndLog = true;
    m_cfg.showStartupSplash = true;
    m_cfg.showRgbPreviewStrip = true;
    m_cfg.animatedUi = true;
    m_cfg.uiThemePreset = QStringLiteral("oled_pure");
    m_cfg.uiAccentMode = QStringLiteral("preset");
    m_cfg.uiCustomAccent = QStringLiteral("#15803d");
    m_cfg.uiGlassOpacity = 0.84;
    m_cfg.uiCornerRadius = 14;
    m_cfg.uiGlowIntensity = 0.65;
    m_cfg.uiBackgroundStyle = QStringLiteral("oledPure");
    m_cfg.uiTrueBlackOled = true;
    m_cfg.uiBorderWidth = 1.4;
    m_cfg.uiAnimationSpeed = 1.0;
    m_cfg.uiCompactMode = false;
    m_cfg.uiUseCustomSecondary = false;
    m_cfg.uiCustomSecondary = QStringLiteral("#166534");
    m_cfg.uiBorderStyle = QStringLiteral("accent");
    m_cfg.uiCustomBorderColor = QStringLiteral("#15803d");
    m_cfg.uiBorderOpacity = 0.85;
    m_cfg.uiWindowEdgeGlow = true;
    m_cfg.uiWindowButtonStyle = QStringLiteral("circle");
    m_cfg.uiTitlebarStyle = QStringLiteral("oled");
    m_cfg.uiButtonShape = QStringLiteral("rounded");
    m_cfg.uiButtonStyle = QStringLiteral("glass");
    m_cfg.uiCardEdgeStyle = QStringLiteral("glass");
    m_cfg.uiWindowShape = QStringLiteral("rounded");
    m_cfg.autoSwitchByApp = false;
    m_cfg.fallbackProfileIndex = 0;
    m_autoSwitchedActive = false;
    m_appWatchTimer.stop();
    if (!SystemUtils::isRunningInFlatpak() || m_sandbox.deviceAccessAvailable()) m_hotplugTimer.start();
    saveProfilesToDisk();
    emit appSettingsChanged();
    emit profilesMetaChanged();
    emit activeProfileChanged();
    emit stateChanged();
    setStatus(loc(
        QStringLiteral("تمت إعادة ضبط جميع البروفايلات الـ 5 وإعدادات التطبيق."),
        QStringLiteral("All 5 profiles and app settings have been reset."),
        QStringLiteral("Все 5 профилей и настройки приложения сброшены."),
        QStringLiteral("所有 5 组配置文件及应用设置已重置。")
    ), false);
}

bool MouseController::exportBackup(const QUrl &fileUrl) {
    const QString localPath = resolveLocalFilePath(fileUrl);
    if (localPath.isEmpty()) return false;
    if (!ProfileStorage::exportBackup(localPath, m_cfg)) {
        setStatus(loc(
            QStringLiteral("تعذر تصدير النسخة الاحتياطية إلى %1").arg(localPath),
            QStringLiteral("Failed to export backup to %1").arg(localPath),
            QStringLiteral("Не удалось экспортировать резервную копию в %1").arg(localPath),
            QStringLiteral("无法导出备份到 %1").arg(localPath)
        ), true);
        return false;
    }
    setStatus(loc(
        QStringLiteral("تم تصدير النسخة الاحتياطية بنجاح إلى %1").arg(localPath),
        QStringLiteral("Backup exported successfully to %1").arg(localPath),
        QStringLiteral("Резервная копия успешно сохранена в %1").arg(localPath),
        QStringLiteral("已成功导出备份到 %1").arg(localPath)
    ), false);
    return true;
}

bool MouseController::importBackup(const QUrl &fileUrl) {
    const QString localPath = resolveLocalFilePath(fileUrl);
    if (localPath.isEmpty()) return false;
    if (!ProfileStorage::importBackup(localPath, m_cfg)) {
        setStatus(loc(
            QStringLiteral("ملف النسخة الاحتياطية غير صالح أو تعذر فتحه: %1").arg(localPath),
            QStringLiteral("Invalid or unreadable backup JSON file: %1").arg(localPath),
            QStringLiteral("Некорректный или недоступный файл резервной копии: %1").arg(localPath),
            QStringLiteral("无效或无法读取的 JSON 备份文件：%1").arg(localPath)
        ), true);
        return false;
    }

    m_sandbox.setLanguage(m_cfg.language);
    if (m_cfg.autoDetectDevice) m_hotplugTimer.start();
    else m_hotplugTimer.stop();

    m_autoSwitchedActive = false;
    if (m_sandbox.hostAccessAvailable() && m_cfg.autoSwitchByApp) m_appWatchTimer.start();
    else m_appWatchTimer.stop();

    saveProfilesToDisk();
    emit languageChanged();
    emit appSettingsChanged();
    emit profilesMetaChanged();
    emit activeProfileChanged();
    emit macroLibraryChanged();
    emit stateChanged();

    setStatus(loc(
        QStringLiteral("تم استيراد النسخة الاحتياطية بنجاح — اضغط (حفظ وتطبيق) لكتابة البروفايل على الماوس."),
        QStringLiteral("Backup imported successfully — Click Save & Apply to write to mouse."),
        QStringLiteral("Резервная копия успешно загружена — нажмите «Сохранить и применить»."),
        QStringLiteral("备份导入成功 — 请点击“保存并应用”写入鼠标。")
    ), false);
    return true;
}

QVariantList MouseController::buttonCodes() const { QVariantList l; for (int c : activeState().buttonCodes) l.append(c); return l; }
QVariantList MouseController::buttonMediaUsages() const { QVariantList l; for (uint16_t u : activeState().buttonMediaUsages) l.append(static_cast<int>(u)); return l; }
QVariantList MouseController::extraModeColors() const { QVariantList l; for (const auto &c : activeState().extraModeColors) l.append(QColor(c.r, c.g, c.b)); return l; }
QVariantList MouseController::reactiveButtons() const { QVariantList l; for (bool en : activeState().reactiveButtons) l.append(en); return l; }
QVariantList MouseController::dpiIndices() const { QVariantList l; for (int idx : activeState().dpiIndices) l.append(idx); return l; }
QVariantList MouseController::dpiEnabled() const { QVariantList l; for (bool en : activeState().dpiEnabled) l.append(en); return l; }
QVariantList MouseController::dpiColors() const { QVariantList l; for (const auto &c : activeState().dpiColors) l.append(QColor(c.r, c.g, c.b)); return l; }
QVariantList MouseController::userColors() const { QVariantList l; for (const auto &c : activeState().userColors) l.append(QColor(c.r, c.g, c.b)); return l; }
QVariantList MouseController::userColorSlotsEnabled() const { QVariantList l; for (bool en : activeState().userColorSlotsEnabled) l.append(en); return l; }

void MouseController::setActiveDpiLevel(int level) {
    const int clamped = std::clamp(level, 1, 6);
    if (activeState().activeDpiLevel != clamped) { activeState().activeDpiLevel = clamped; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setFireTimeMs(int ms) {
    const int clamped = std::clamp(ms, 8, 400);
    if (activeState().fireTimeMs != clamped) { activeState().fireTimeMs = clamped; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setLedMode(int mode) {
    const int clamped = std::clamp(mode, 0, 13);
    if (activeState().ledMode != clamped) { activeState().ledMode = clamped; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setLedSpeed(int speed) {
    const int clamped = std::clamp(speed, 0, 5);
    if (activeState().ledSpeed != clamped) { activeState().ledSpeed = clamped; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setLedBrightness(int brightness) {
    const int clamped = std::clamp(brightness, 0, 6);
    if (activeState().ledBrightness != clamped) { activeState().ledBrightness = clamped; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setLedDirection(int direction) {
    const int clamped = std::clamp(direction, 0, 3);
    if (activeState().ledDirection != clamped) { activeState().ledDirection = clamped; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setLedSymmetry(bool symmetry) {
    if (activeState().ledSymmetry != symmetry) { activeState().ledSymmetry = symmetry; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setLedSingleColor(bool singleColor) {
    if (activeState().ledSingleColor != singleColor) { activeState().ledSingleColor = singleColor; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setUserColorEnabled(bool enabled) {
    if (activeState().userColorEnabled != enabled) { activeState().userColorEnabled = enabled; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setReactiveEffect(int effect) {
    const int clamped = std::clamp(effect, 0, 3);
    if (activeState().reactiveEffect != clamped) { activeState().reactiveEffect = clamped; scheduleSaveToDisk(); emit stateChanged(); }
}

void MouseController::setButtonCode(int slot, int code) {
    if (slot < 0 || slot >= 8) return;
    const int clamped = std::clamp(code, 0, 31);
    bool changed = (activeState().buttonCodes[slot] != clamped);
    activeState().buttonCodes[slot] = clamped;
    if (clamped != 0x0F && activeState().buttonMediaUsages[slot] != 0) { activeState().buttonMediaUsages[slot] = 0; changed = true; }
    if (clamped != 0x0D && clamped != 0x0E && !m_cfg.profiles[m_cfg.activeProfile].buttonMacroIds[slot].isEmpty()) {
        m_cfg.profiles[m_cfg.activeProfile].buttonMacroIds[slot].clear();
        changed = true;
    }
    if (changed) { scheduleSaveToDisk(); emit stateChanged(); }
}

void MouseController::setButtonMediaUsage(int slot, int usage) {
    if (slot < 0 || slot >= 8) return;
    const uint16_t u = static_cast<uint16_t>(std::clamp(usage, 0, 65535));
    activeState().buttonMediaUsages[slot] = u;
    activeState().buttonCodes[slot] = (u != 0) ? 0x0F : 0x00;
    m_cfg.profiles[m_cfg.activeProfile].buttonMacroIds[slot].clear();
    scheduleSaveToDisk();
    emit stateChanged();
}

void MouseController::resetDefaultButtons() {
    activeState().buttonCodes = ProfileStorage::kDefaultButtonCodes;
    activeState().buttonMediaUsages.fill(0);
    for (auto &id : m_cfg.profiles[m_cfg.activeProfile].buttonMacroIds) id.clear();
    for (auto &m : activeState().buttonMacros) { m.steps.clear(); m.repeatMode = 0; m.repeatCount = 1; }
    activeState().fireTimeMs = 48;
    scheduleSaveToDisk();
    setStatus(loc(
        QStringLiteral("تمت استعادة تعيين الأزرار الافتراضي في البروفايل الحالي — اضغط (حفظ وتطبيق) لكتابته على الماوس."),
        QStringLiteral("Default button layout restored in active profile — Click Save & Apply."),
        QStringLiteral("Стандартная раскладка кнопок восстановлена — нажмите «Сохранить и применить»."),
        QStringLiteral("已在当前配置恢复默认按键布局 — 请点击“保存并应用”。")
    ), false);
    emit stateChanged();
}

void MouseController::setReactiveButtonEnabled(int slot, bool enabled) {
    if (slot < 0 || slot >= 8) return;
    if (activeState().reactiveButtons[slot] != enabled) { activeState().reactiveButtons[slot] = enabled; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setDpiIndex(int slot, int tableIndex) {
    if (slot < 0 || slot >= 6) return;
    const int clamped = std::clamp(tableIndex, 0, 16);
    if (activeState().dpiIndices[slot] != clamped) { activeState().dpiIndices[slot] = clamped; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setDpiEnabled(int slot, bool enabled) {
    if (slot < 0 || slot >= 6) return;
    if (activeState().dpiEnabled[slot] != enabled) { activeState().dpiEnabled[slot] = enabled; scheduleSaveToDisk(); emit stateChanged(); }
}
void MouseController::setDpiColor(int slot, const QColor &color) {
    if (slot < 0 || slot >= 6 || !color.isValid()) return;
    activeState().dpiColors[slot] = {color.red(), color.green(), color.blue()};
    scheduleSaveToDisk();
    emit stateChanged();
}
void MouseController::setUserColor(int slot, const QColor &color) {
    if (slot < 0 || slot >= 7 || !color.isValid()) return;
    activeState().userColors[slot] = {color.red(), color.green(), color.blue()};
    scheduleSaveToDisk();
    emit stateChanged();
}
void MouseController::setExtraModeColor(int slot, const QColor &color) {
    if (slot < 0 || slot >= 6 || !color.isValid()) return;
    activeState().extraModeColors[slot] = {color.red(), color.green(), color.blue()};
    scheduleSaveToDisk();
    emit stateChanged();
}
void MouseController::setUserColorSlotEnabled(int slot, bool enabled) {
    if (slot < 0 || slot >= 7) return;
    if (activeState().userColorSlotsEnabled[slot] != enabled) { activeState().userColorSlotsEnabled[slot] = enabled; scheduleSaveToDisk(); emit stateChanged(); }
}
QString MouseController::dpiLabelForIndex(int tableIndex) const {
    const int idx = std::clamp(tableIndex, 0, 16);
    return QString::number(M700ADriver::kDpiValueTable[static_cast<size_t>(idx)]) + QStringLiteral(" DPI");
}

QString MouseController::formatMacroSummary(const ButtonMacro &mac) const {
    if (mac.steps.empty()) {
        return loc(QStringLiteral("غير معيّن"), QStringLiteral("Not set"), QStringLiteral("Не задано"), QStringLiteral("未设置"));
    }
    QStringList downKeys;
    for (const auto &s : mac.steps) {
        if (s.isDown) {
            downKeys.append(s.label.empty() ? HidKeyMap::hidUsageName(s.hidCode) : QString::fromStdString(s.label));
            if (downKeys.size() >= 4) break;
        }
    }
    QString combo = downKeys.join(QStringLiteral(" + "));
    if (static_cast<int>(mac.steps.size()) > 8) combo += QStringLiteral(" …");
    return QStringLiteral("%1 (%2)").arg(combo).arg(mac.steps.size());
}

void MouseController::reconcileHardwareMacrosForProfile(int profileIdx) {
    if (profileIdx < 0 || profileIdx >= kProfileCount) return;
    auto &prof = m_cfg.profiles[profileIdx];
    auto &st = prof.state;

    auto macrosEqual = [](const ButtonMacro &a, const ButtonMacro &b) {
        if (a.repeatMode != b.repeatMode || a.repeatCount != b.repeatCount || a.steps.size() != b.steps.size()) return false;
        for (size_t s = 0; s < a.steps.size(); ++s) {
            if (a.steps[s].hidCode != b.steps[s].hidCode || a.steps[s].isDown != b.steps[s].isDown || a.steps[s].delayMs != b.steps[s].delayMs) return false;
        }
        return true;
    };

    for (int i = 0; i < 8; ++i) {
        const int code = st.buttonCodes[i];
        if (code != 0x0F) st.buttonMediaUsages[i] = 0;
        for (auto &step : st.buttonMacros[i].steps) {
            if (step.label.empty()) step.label = HidKeyMap::hidUsageName(step.hidCode).toStdString();
        }
        if (code == 0x0D || code == 0x0E) {
            QString matchedId;
            for (const auto &nm : m_cfg.macroLibrary) {
                if (macrosEqual(st.buttonMacros[i], nm.macro)) { matchedId = nm.id; break; }
            }
            prof.buttonMacroIds[i] = matchedId;
        } else {
            prof.buttonMacroIds[i].clear();
        }
    }
}

QVariantList MouseController::macroLibrary() const {
    QVariantList list;
    for (size_t i = 0; i < m_cfg.macroLibrary.size(); ++i) {
        const auto &nm = m_cfg.macroLibrary[i];
        QVariantMap m;
        m[QStringLiteral("index")] = static_cast<int>(i);
        m[QStringLiteral("id")] = nm.id;
        m[QStringLiteral("name")] = nm.name;
        m[QStringLiteral("repeatMode")] = nm.macro.repeatMode;
        m[QStringLiteral("repeatCount")] = nm.macro.repeatCount;
        m[QStringLiteral("stepCount")] = static_cast<int>(nm.macro.steps.size());
        m[QStringLiteral("summary")] = formatMacroSummary(nm.macro);
        list.append(m);
    }
    return list;
}

QVariantMap MouseController::getLibraryMacro(int index) const {
    QVariantMap out;
    if (index < 0 || index >= static_cast<int>(m_cfg.macroLibrary.size())) return out;
    const auto &nm = m_cfg.macroLibrary[static_cast<size_t>(index)];
    out[QStringLiteral("id")] = nm.id;
    out[QStringLiteral("name")] = nm.name;
    out[QStringLiteral("repeatMode")] = nm.macro.repeatMode;
    out[QStringLiteral("repeatCount")] = nm.macro.repeatCount;
    QVariantList list;
    for (const auto &s : nm.macro.steps) {
        QVariantMap m;
        m[QStringLiteral("hidCode")] = static_cast<int>(s.hidCode);
        m[QStringLiteral("isDown")] = s.isDown;
        m[QStringLiteral("delayMs")] = s.delayMs;
        m[QStringLiteral("label")] = s.label.empty() ? HidKeyMap::hidUsageName(s.hidCode) : QString::fromStdString(s.label);
        list.append(m);
    }
    out[QStringLiteral("steps")] = list;
    return out;
}

int MouseController::createLibraryMacro(const QString &name, int repeatMode, int repeatCount, const QVariantList &steps) {
    if (m_cfg.macroLibrary.size() >= kMaxMacroLibraryLimit) return -1;
    NamedMacro nm;
    nm.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString cleanName = name.trimmed().left(80);
    nm.name = cleanName.isEmpty() ? QStringLiteral("Macro %1").arg(m_cfg.macroLibrary.size() + 1) : cleanName;
    nm.macro.repeatMode = std::clamp(repeatMode, 0, 2);
    nm.macro.repeatCount = std::clamp(repeatCount, 1, 255);
    nm.macro.steps = ProfileStorage::parseMacroStepsFromVariant(steps);
    m_cfg.macroLibrary.push_back(std::move(nm));
    saveProfilesToDisk();
    emit macroLibraryChanged();
    return static_cast<int>(m_cfg.macroLibrary.size()) - 1;
}

void MouseController::updateLibraryMacro(int index, const QString &name, int repeatMode, int repeatCount, const QVariantList &steps) {
    if (index < 0 || index >= static_cast<int>(m_cfg.macroLibrary.size())) return;
    auto &nm = m_cfg.macroLibrary[static_cast<size_t>(index)];
    const QString cleanName = name.trimmed().left(80);
    if (!cleanName.isEmpty()) nm.name = cleanName;
    nm.macro.repeatMode = std::clamp(repeatMode, 0, 2);
    nm.macro.repeatCount = std::clamp(repeatCount, 1, 255);
    nm.macro.steps = ProfileStorage::parseMacroStepsFromVariant(steps);

    for (int p = 0; p < kProfileCount; ++p) {
        for (int s = 0; s < 8; ++s) {
            if (m_cfg.profiles[p].buttonMacroIds[s] == nm.id) {
                m_cfg.profiles[p].state.buttonMacros[s] = nm.macro;
                if (m_cfg.profiles[p].state.buttonCodes[s] == 0x0D || m_cfg.profiles[p].state.buttonCodes[s] == 0x0E) {
                    m_cfg.profiles[p].state.buttonCodes[s] = (nm.macro.repeatMode == 2) ? 0x0E : 0x0D;
                }
            }
        }
    }
    saveProfilesToDisk();
    emit macroLibraryChanged();
    emit stateChanged();
}

void MouseController::deleteLibraryMacro(int index) {
    if (index < 0 || index >= static_cast<int>(m_cfg.macroLibrary.size())) return;
    const QString deletedId = m_cfg.macroLibrary[static_cast<size_t>(index)].id;
    m_cfg.macroLibrary.erase(m_cfg.macroLibrary.begin() + index);
    for (int p = 0; p < kProfileCount; ++p) {
        for (int s = 0; s < 8; ++s) {
            if (m_cfg.profiles[p].buttonMacroIds[s] == deletedId) m_cfg.profiles[p].buttonMacroIds[s].clear();
        }
    }
    saveProfilesToDisk();
    emit macroLibraryChanged();
    emit stateChanged();
}

bool MouseController::exportSingleMacro(int index, const QUrl &fileUrl) {
    if (index < 0 || index >= static_cast<int>(m_cfg.macroLibrary.size())) return false;
    const QString localPath = resolveLocalFilePath(fileUrl);
    if (localPath.isEmpty()) return false;
    const auto &nm = m_cfg.macroLibrary[static_cast<size_t>(index)];
    if (!ProfileStorage::exportSingleMacro(localPath, nm)) {
        setStatus(loc(
            QStringLiteral("تعذر تصدير الماكرو إلى %1").arg(localPath),
            QStringLiteral("Failed to export macro to %1").arg(localPath),
            QStringLiteral("Не удалось экспортировать макрос в %1").arg(localPath),
            QStringLiteral("无法导出宏到 %1").arg(localPath)
        ), true);
        return false;
    }
    setStatus(loc(
        QStringLiteral("تم تصدير الماكرو (%1) بنجاح.").arg(nm.name),
        QStringLiteral("Macro (%1) exported successfully.").arg(nm.name),
        QStringLiteral("Макрос (%1) успешно экспортирован.").arg(nm.name),
        QStringLiteral("宏（%1）导出成功。").arg(nm.name)
    ), false);
    return true;
}

bool MouseController::importSingleMacro(const QUrl &fileUrl) {
    if (m_cfg.macroLibrary.size() >= kMaxMacroLibraryLimit) return false;
    const QString localPath = resolveLocalFilePath(fileUrl);
    if (localPath.isEmpty()) return false;
    NamedMacro nm;
    if (!ProfileStorage::importSingleMacro(localPath, nm)) {
        setStatus(loc(
            QStringLiteral("ملف الماكرو غير صالح أو فارغ."),
            QStringLiteral("Invalid or empty macro file."),
            QStringLiteral("Некорректный или пустой файл макроса."),
            QStringLiteral("无效或空的宏文件。")
        ), true);
        return false;
    }
    const QString importedName = nm.name;
    m_cfg.macroLibrary.push_back(std::move(nm));
    saveProfilesToDisk();
    emit macroLibraryChanged();
    setStatus(loc(
        QStringLiteral("تم استيراد الماكرو (%1) إلى استوديو الماكرو.").arg(importedName),
        QStringLiteral("Macro (%1) imported into Macro Studio.").arg(importedName),
        QStringLiteral("Макрос (%1) импортирован в Студию макросов.").arg(importedName),
        QStringLiteral("宏（%1）已导入到宏工作室。").arg(importedName)
    ), false);
    return true;
}

void MouseController::createQuickPresetInLibrary(const QString &presetId) {
    QString name;
    std::vector<MacroStep> steps;
    auto addCombo = [&](uint8_t mod, uint8_t key) {
        if (mod != 0) steps.push_back({mod, true, 24, HidKeyMap::hidUsageName(mod).toStdString()});
        steps.push_back({key, true, 24, HidKeyMap::hidUsageName(key).toStdString()});
        steps.push_back({key, false, 24, HidKeyMap::hidUsageName(key).toStdString()});
        if (mod != 0) steps.push_back({mod, false, 24, HidKeyMap::hidUsageName(mod).toStdString()});
    };
    if (presetId == QStringLiteral("copy")) { name = QStringLiteral("Copy (Ctrl+C)"); addCombo(0xE0, 0x06); }
    else if (presetId == QStringLiteral("paste")) { name = QStringLiteral("Paste (Ctrl+V)"); addCombo(0xE0, 0x19); }
    else if (presetId == QStringLiteral("undo")) { name = QStringLiteral("Undo (Ctrl+Z)"); addCombo(0xE0, 0x1D); }
    else if (presetId == QStringLiteral("alttab")) { name = QStringLiteral("Alt+Tab"); addCombo(0xE2, 0x2B); }
    else if (presetId == QStringLiteral("desktop")) { name = QStringLiteral("Desktop (Super+D)"); addCombo(0xE3, 0x07); }
    else if (presetId == QStringLiteral("prtsc")) { name = QStringLiteral("Screenshot (PrtSc)"); addCombo(0x00, 0x46); }
    else return;

    NamedMacro nm;
    nm.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    nm.name = name;
    nm.macro.repeatMode = 0;
    nm.macro.repeatCount = 1;
    nm.macro.steps = std::move(steps);
    m_cfg.macroLibrary.push_back(std::move(nm));
    saveProfilesToDisk();
    emit macroLibraryChanged();
}

void MouseController::assignLibraryMacroToButton(int slot, int macroIndex) {
    if (slot < 0 || slot >= 8) return;
    if (macroIndex < 0 || macroIndex >= static_cast<int>(m_cfg.macroLibrary.size())) return;
    const auto &nm = m_cfg.macroLibrary[static_cast<size_t>(macroIndex)];
    m_cfg.profiles[m_cfg.activeProfile].buttonMacroIds[slot] = nm.id;
    activeState().buttonMediaUsages[slot] = 0;
    activeState().buttonMacros[slot] = nm.macro;
    activeState().buttonCodes[slot] = (nm.macro.repeatMode == 2) ? 0x0E : 0x0D;
    saveProfilesToDisk();
    emit stateChanged();
}

QString MouseController::assignedMacroNameForButton(int slot) const {
    if (slot < 0 || slot >= 8) return QString();
    const QString &id = m_cfg.profiles[m_cfg.activeProfile].buttonMacroIds[slot];
    if (!id.isEmpty()) {
        for (const auto &nm : m_cfg.macroLibrary) {
            if (nm.id == id) return nm.name;
        }
    }
    return formatMacroSummary(activeState().buttonMacros[slot]);
}

int MouseController::keyEventToHidUsage(int qtKey, int nativeScanCode) const { return HidKeyMap::keyEventToHidUsage(qtKey, nativeScanCode); }
QString MouseController::hidUsageName(int hidCode) const { return HidKeyMap::hidUsageName(hidCode); }

void MouseController::syncBaselineFromActive() {
    m_lastAppliedState = activeState();
    m_hasBaselineState = true;
    if (!m_unsavedChangesList.isEmpty()) {
        m_unsavedChangesList.clear();
        emit unsavedChangesChanged();
    }
}

void MouseController::recomputeUnsavedChanges() {
    if (!m_hasBaselineState) return;
    const MouseState &cur = activeState();
    const MouseState &base = m_lastAppliedState;
    QStringList list;

    if (cur.activeDpiLevel != base.activeDpiLevel ||
        cur.dpiIndices != base.dpiIndices || cur.dpiEnabled != base.dpiEnabled) {
        list.append(loc(QStringLiteral("سرعات ومستويات DPI"), QStringLiteral("DPI Levels & Speeds"),
                        QStringLiteral("Уровни и скорости DPI"), QStringLiteral("DPI 档位与速度")));
    }
    for (int i = 0; i < 6; ++i) {
        if (cur.dpiColors[i].r != base.dpiColors[i].r || cur.dpiColors[i].g != base.dpiColors[i].g || cur.dpiColors[i].b != base.dpiColors[i].b) {
            list.append(loc(QStringLiteral("ألوان مستويات DPI"), QStringLiteral("DPI Colors"),
                            QStringLiteral("Цвета уровней DPI"), QStringLiteral("DPI 指示灯颜色")));
            break;
        }
    }
    if (cur.ledMode != base.ledMode || cur.ledBrightness != base.ledBrightness ||
        cur.ledSpeed != base.ledSpeed || cur.ledDirection != base.ledDirection ||
        cur.ledSymmetry != base.ledSymmetry || cur.ledSingleColor != base.ledSingleColor) {
        list.append(loc(QStringLiteral("وضع الإضاءة والسطوع والسرعة"), QStringLiteral("Lighting Mode, Brightness & Speed"),
                        QStringLiteral("Режим подсветки, яркость и скорость"), QStringLiteral("灯光模式、亮度与速度")));
    }
    if (cur.reactiveEffect != base.reactiveEffect || cur.reactiveButtons != base.reactiveButtons) {
        list.append(loc(QStringLiteral("إعدادات الوضع التفاعلي (Reactive)"), QStringLiteral("Reactive Mode Settings"),
                        QStringLiteral("Настройки реактивного режима"), QStringLiteral("按键触发灯效设置")));
    }
    bool userColChanged = (cur.userColorEnabled != base.userColorEnabled) || (cur.userColorSlotsEnabled != base.userColorSlotsEnabled);
    if (!userColChanged) {
        for (int i = 0; i < 7; ++i) {
            if (cur.userColors[i].r != base.userColors[i].r || cur.userColors[i].g != base.userColors[i].g || cur.userColors[i].b != base.userColors[i].b) {
                userColChanged = true; break;
            }
        }
    }
    if (!userColChanged) {
        for (int i = 0; i < 6; ++i) {
            if (cur.extraModeColors[i].r != base.extraModeColors[i].r || cur.extraModeColors[i].g != base.extraModeColors[i].g || cur.extraModeColors[i].b != base.extraModeColors[i].b) {
                userColChanged = true; break;
            }
        }
    }
    if (userColChanged) {
        list.append(loc(QStringLiteral("ألوان التأثيرات المخصصة (7 ألوان)"), QStringLiteral("Custom Effect Palette (7 Colors)"),
                        QStringLiteral("Пользовательская палитра (7 цветов)"), QStringLiteral("自定义特效色板（7 色）")));
    }
    bool btnChanged = (cur.buttonCodes != base.buttonCodes);
    if (!btnChanged) {
        for (int i = 0; i < 8; ++i) {
            if ((cur.buttonCodes[i] == 0x0F || base.buttonCodes[i] == 0x0F) && cur.buttonMediaUsages[i] != base.buttonMediaUsages[i]) {
                btnChanged = true; break;
            }
        }
    }
    if (!btnChanged) {
        for (int i = 0; i < 8; ++i) {
            const auto &a = cur.buttonMacros[i];
            const auto &b = base.buttonMacros[i];
            if (a.repeatMode != b.repeatMode || a.repeatCount != b.repeatCount || a.steps.size() != b.steps.size()) { btnChanged = true; break; }
            for (size_t s = 0; s < a.steps.size(); ++s) {
                if (a.steps[s].hidCode != b.steps[s].hidCode || a.steps[s].isDown != b.steps[s].isDown || a.steps[s].delayMs != b.steps[s].delayMs) {
                    btnChanged = true; break;
                }
            }
            if (btnChanged) break;
        }
    }
    if (btnChanged) {
        list.append(loc(QStringLiteral("تخصيص الأزرار والماكرو"), QStringLiteral("Button Remapping & Macros"),
                        QStringLiteral("Назначение кнопок и макросы"), QStringLiteral("按键自定义与宏")));
    }
    if (cur.fireTimeMs != base.fireTimeMs) {
        list.append(loc(QStringLiteral("توقيت زر الإطلاق المتكرر (Fire Key)"), QStringLiteral("Fire Key Repeat Interval"),
                        QStringLiteral("Интервал кнопки Fire Key"), QStringLiteral("火力键连点间隔")));
    }
    if (m_unsavedChangesList != list) {
        m_unsavedChangesList = list;
        emit unsavedChangesChanged();
    }
}

bool MouseController::installUdevRules() { return m_sandbox.installUdevRules(); }
void MouseController::refreshAppPermissions() { m_sandbox.refreshAppPermissions(); }
QString MouseController::buildEnablePermissionsCommand(bool includeDevice, bool includeTray, bool includeHost, bool includeUdev) const {
    return SandboxManager::buildEnablePermissionsCommand(includeDevice, includeTray, includeHost, includeUdev);
}

QVariantList MouseController::commandExplanations(bool includeDevice, bool includeTray, bool includeHost, bool includeUdev, bool includeReset) const {
    return m_sandbox.commandExplanations(includeDevice, includeTray, includeHost, includeUdev, includeReset);
}

void MouseController::copyCommandToClipboard(const QString &command) {
    if (command.trimmed().isEmpty()) return;
    if (auto *cb = QGuiApplication::clipboard()) cb->setText(command);
    setStatus(loc(
        QStringLiteral("تم نسخ أمر التفعيل إلى الحافظة — الصقه وشغّله في الطرفية."),
        QStringLiteral("Enable command copied to clipboard — paste and run it in terminal."),
        QStringLiteral("Команда включения скопирована — выполните её в терминале."),
        QStringLiteral("启用命令已复制到剪贴板 — 请在终端中粘贴运行。")
    ), false);
}
void MouseController::checkUdevHealth() { m_sandbox.checkUdevHealth(); }
QString MouseController::udevManualCommand() const { return SandboxManager::udevManualCommand(); }
QString MouseController::appVersion() const { return QGuiApplication::applicationVersion(); }
QString MouseController::hostAccessGrantCommand() const { return SandboxManager::hostAccessGrantCommand(); }
QString MouseController::trayGrantCommand() const { return SandboxManager::trayGrantCommand(); }

void MouseController::copyTrayGrantCommand() {
    if (auto *cb = QGuiApplication::clipboard()) cb->setText(SandboxManager::trayGrantCommand());
    setStatus(loc(
        QStringLiteral("تم نسخ أمر تفعيل أيقونة شريط المهام — شغّله في الطرفية ثم أعد تشغيل DeepComplex."),
        QStringLiteral("System tray permission command copied — run in terminal then restart DeepComplex."),
        QStringLiteral("Команда включения системного трея скопирована — выполните в терминале и перезапустите приложение."),
        QStringLiteral("已复制系统托盘权限命令 — 请在终端执行后重启 DeepComplex。")
    ), false);
}
void MouseController::copyUdevManualCommand() {
    if (auto *cb = QGuiApplication::clipboard()) cb->setText(SandboxManager::udevManualCommand());
    setStatus(loc(
        QStringLiteral("تم نسخ أمر تثبيت قاعدة udev اليدوي — الصقه وشغّله في الطرفية."),
        QStringLiteral("Manual udev install command copied — paste and run it in terminal."),
        QStringLiteral("Команда установки udev скопирована — выполните её в терминале."),
        QStringLiteral("已复制 udev 手动安装命令 — 请在终端粘贴执行。")
    ), false);
}

void MouseController::copyPermissionsResetCommand() {
    if (auto *cb = QGuiApplication::clipboard()) cb->setText(SandboxManager::permissionsResetCommand());
    setStatus(loc(
        QStringLiteral("تم نسخ أمر استعادة صلاحيات Flatpak الافتراضية."),
        QStringLiteral("Flatpak permissions reset command copied to clipboard."),
        QStringLiteral("Команда сброса прав Flatpak скопирована в буфер."),
        QStringLiteral("已复制 Flatpak 权限重置命令。")
    ), false);
}

void MouseController::copyConfigFilePath() {
    if (auto *cb = QGuiApplication::clipboard()) cb->setText(ProfileStorage::configFilePath());
    setStatus(loc(
        QStringLiteral("تم نسخ مسار ملف الإعدادات إلى الحافظة."),
        QStringLiteral("Config file path copied to clipboard."),
        QStringLiteral("Путь к файлу конфигурации скопирован."),
        QStringLiteral("配置文件路径已复制到剪贴板。")
    ), false);
}

void MouseController::copyOperationLog() {
    if (auto *cb = QGuiApplication::clipboard()) cb->setText(m_operationLog);
    setStatus(loc(
        QStringLiteral("تم نسخ سجل العمليات إلى الحافظة."),
        QStringLiteral("Operation log copied to clipboard."),
        QStringLiteral("Журнал операций скопирован."),
        QStringLiteral("操作日志已复制到剪贴板。")
    ), false);
}

void MouseController::copyHostAccessCommand() {
    if (auto *cb = QGuiApplication::clipboard()) cb->setText(SandboxManager::hostAccessGrantCommand());
    setStatus(loc(
        QStringLiteral("تم نسخ أمر تفعيل صلاحية النظام المضيف — شغّله في الطرفية ثم أعد تشغيل DeepComplex."),
        QStringLiteral("Host permission command copied — run it in terminal then restart DeepComplex."),
        QStringLiteral("Команда включения права хоста скопирована — выполните её в терминале и перезапустите DeepComplex."),
        QStringLiteral("已复制宿主机权限启用命令 — 请在终端执行后重启 DeepComplex。")
    ), false);
}

void MouseController::refreshFromMouse() {
    if (m_isBusy) return;
    joinWorkerIfNeeded();

    m_isBusy = true;
    m_operationProgress = 0;
    m_operationSuccess = true;
    m_operationLog.clear();
    m_operationTitle = loc(QStringLiteral("قراءة إعدادات الماوس..."), QStringLiteral("Reading from Mouse..."),
                           QStringLiteral("Чтение настроек с мыши..."), QStringLiteral("正在从鼠标读取设置..."));
    appendLogLine(m_operationTitle);
    emit operationChanged();
    emit operationStarted();

    const int targetProfile = m_cfg.activeProfile;
    const MouseState prevState = activeState();

    m_workerThread = std::thread([this, targetProfile, prevState]() {
        DriverError err = DriverError::None;
        std::string detail;
        MouseState readOut = prevState;

        auto cb = [this](int pct, const std::string &line) {
            if (m_shuttingDown.load()) return;
            const QString qline = QString::fromStdString(line);
            QMetaObject::invokeMethod(this, [this, pct, qline]() {
                if (m_shuttingDown.load()) return;
                m_operationProgress = pct;
                appendLogLine(qline);
                emit operationChanged();
            }, Qt::QueuedConnection);
        };

        const bool ok = m_driver.readState(readOut, err, detail, cb);
        if (m_shuttingDown.load()) return;

        QMetaObject::invokeMethod(this, [this, ok, err, detail, readOut, targetProfile]() {
            if (m_shuttingDown.load()) return;
            m_isBusy = false;
            m_operationProgress = 100;
            m_operationSuccess = ok;

            if (ok) {
                m_connected = true;
                m_devicePath = QString::fromStdString(readOut.devicePath);
                if (targetProfile >= 0 && targetProfile < kProfileCount) {
                    m_cfg.profiles[targetProfile].state = readOut;
                    reconcileHardwareMacrosForProfile(targetProfile);
                }
                emit hardwareDpiSnapshotSynced(readOut.activeDpiLevel, readOut.dpiIndices, readOut.dpiColors, readOut.ledMode == 0x09);
                if (m_cfg.activeProfile == targetProfile) syncBaselineFromActive();
                saveProfilesToDisk();
                const int displayIdx = (targetProfile >= 0 && targetProfile < kProfileCount) ? targetProfile : m_cfg.activeProfile;
                const QString displayName = m_cfg.profiles[displayIdx].name;
                setStatus(loc(
                    QStringLiteral("تمت قراءة إعدادات الماوس الحالية وحفظها في البروفايل %1 (%2)").arg(displayIdx + 1).arg(displayName),
                    QStringLiteral("Read current mouse settings into Profile %1 (%2)").arg(displayIdx + 1).arg(displayName),
                    QStringLiteral("Текущие настройки мыши загружены в Профиль %1 (%2)").arg(displayIdx + 1).arg(displayName),
                    QStringLiteral("已读取当前鼠标设置并保存到配置文件 %1（%2）").arg(displayIdx + 1).arg(displayName)
                ), false);
            } else {
                m_connected = false;
                setStatus(translateDriverError(err, detail), true);
            }
            emit stateChanged();
            emit operationChanged();
        }, Qt::QueuedConnection);
    });
}

void MouseController::applyToMouse() {
    if (m_isBusy) return;
    joinWorkerIfNeeded();
    saveProfilesToDisk();

    m_isBusy = true;
    m_operationProgress = 0;
    m_operationSuccess = true;
    m_operationLog.clear();
    m_operationTitle = loc(
        QStringLiteral("حفظ وتطبيق البروفايل %1 (%2)...").arg(m_cfg.activeProfile + 1).arg(activeProfileName()),
        QStringLiteral("Saving & Applying Profile %1 (%2)...").arg(m_cfg.activeProfile + 1).arg(activeProfileName()),
        QStringLiteral("Сохранение профиля %1 (%2)...").arg(m_cfg.activeProfile + 1).arg(activeProfileName()),
        QStringLiteral("正在保存并应用配置文件 %1（%2）...").arg(m_cfg.activeProfile + 1).arg(activeProfileName())
    );
    appendLogLine(m_operationTitle);
    emit operationChanged();
    emit operationStarted();

    const int appliedProfile = m_cfg.activeProfile;
    const QString appliedProfileName = activeProfileName();
    const MouseState snapshot = activeState();

    m_workerThread = std::thread([this, snapshot, appliedProfile, appliedProfileName]() {
        DriverError err = DriverError::None;
        std::string detail;

        auto cb = [this](int pct, const std::string &line) {
            if (m_shuttingDown.load()) return;
            const QString qline = QString::fromStdString(line);
            QMetaObject::invokeMethod(this, [this, pct, qline]() {
                if (m_shuttingDown.load()) return;
                m_operationProgress = pct;
                appendLogLine(qline);
                emit operationChanged();
            }, Qt::QueuedConnection);
        };

        const bool ok = m_driver.applySettings(snapshot, err, detail, cb);
        auto path = ok ? M700ADriver::findDevicePath() : std::nullopt;
        if (m_shuttingDown.load()) return;

        QMetaObject::invokeMethod(this, [this, ok, err, detail, path, appliedProfile, appliedProfileName, snapshot]() {
            if (m_shuttingDown.load()) return;
            m_isBusy = false;
            m_operationProgress = 100;
            m_operationSuccess = ok;

            if (ok) {
                m_connected = true;
                if (path.has_value()) m_devicePath = QString::fromStdString(*path);
                emit hardwareDpiSnapshotSynced(snapshot.activeDpiLevel, snapshot.dpiIndices, snapshot.dpiColors, snapshot.ledMode == 0x09);
                if (m_cfg.activeProfile == appliedProfile) syncBaselineFromActive();
                setStatus(loc(
                    QStringLiteral("تم حفظ وتطبيق البروفايل %1 (%2) على الماوس بنجاح.").arg(appliedProfile + 1).arg(appliedProfileName),
                    QStringLiteral("Profile %1 (%2) saved and applied to mouse.").arg(appliedProfile + 1).arg(appliedProfileName),
                    QStringLiteral("Профиль %1 (%2) сохранён и применён к мыши.").arg(appliedProfile + 1).arg(appliedProfileName),
                    QStringLiteral("配置文件 %1（%2）已保存并成功应用到鼠标。").arg(appliedProfile + 1).arg(appliedProfileName)
                ), false);
            } else {
                setStatus(translateDriverError(err, detail), true);
            }
            emit stateChanged();
            emit operationChanged();
        }, Qt::QueuedConnection);
    });
}
