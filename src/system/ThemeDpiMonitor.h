#pragma once

#include "../driver/M700ADriver.h"

#include <QColor>
#include <QObject>
#include <QString>
#include <QTimer>
#include <array>
#include <atomic>
#include <cstdint>
#include <thread>

// Dedicated, strictly read-only hardware DPI monitor for the UI Theme & Titlebar Chip.
// 100% decoupled from MouseController, DPI profile editing, and Extra Mode zone editing:
//   - NEVER modifies mouse EEPROM or user profiles.
//   - Coalesces rapid HID Interrupt bursts (0x06 0xF8 0x8X) so rapid DPI clicking never lags or glitches.
//   - Uses sequence-guarded + cooldown-guarded EEPROM reads so slow EEPROM reads NEVER overwrite newer rapid clicks.
//   - Automatically suspends periodic EEPROM polling in Extra Mode (0x09) and uses fast 1-byte checks when idle.
class ThemeDpiMonitor : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool ready READ ready NOTIFY dpiStateChanged)
    Q_PROPERTY(int dpiLevel READ dpiLevel NOTIFY dpiStateChanged)
    Q_PROPERTY(QColor dpiColor READ dpiColor NOTIFY dpiStateChanged)
    Q_PROPERTY(QString dpiLabel READ dpiLabel NOTIFY dpiStateChanged)
    Q_PROPERTY(QString lastReadTime READ lastReadTime NOTIFY dpiStateChanged)
    Q_PROPERTY(bool extraModePassive READ extraModePassive NOTIFY dpiStateChanged)

    Q_PROPERTY(bool autoSync READ autoSync WRITE setAutoSync NOTIFY settingsChanged)
    Q_PROPERTY(int intervalSec READ intervalSec WRITE setIntervalSec NOTIFY settingsChanged)
    Q_PROPERTY(bool deviceAccessAvailable READ deviceAccessAvailable NOTIFY settingsChanged)

public:
    explicit ThemeDpiMonitor(QObject *parent = nullptr);
    ~ThemeDpiMonitor() override;

    bool ready() const { return m_ready; }
    int dpiLevel() const { return m_dpiLevel; }
    QColor dpiColor() const { return m_dpiColor; }
    QString dpiLabel() const { return m_dpiLabel; }
    QString lastReadTime() const { return m_lastReadTime; }
    bool extraModePassive() const { return m_inExtraMode; }

    bool autoSync() const { return m_autoSync; }
    int intervalSec() const { return m_intervalSec; }
    bool deviceAccessAvailable() const { return m_deviceAccessAvailable; }

    void setAutoSync(bool enabled);
    void setIntervalSec(int seconds);

    // Non-invasive coordination hooks called from main.cpp signals
    void setDeviceAccessAvailable(bool available);
    void setEditorBusy(bool busy);
    void setLanguage(const QString &lang);

    // One-way read-only cache refresh from completed MouseController Read/Apply (zero extra USB traffic!)
    void syncFromCompletedHardwareTransfer(int activeLevel,
                                           const std::array<int, 6> &dpiIndices,
                                           const std::array<RgbColor, 6> &dpiColors,
                                           bool inExtraMode);

    Q_INVOKABLE void readNow();
    Q_INVOKABLE void resetDefaults();

signals:
    void dpiStateChanged();
    void settingsChanged();
    void statusFeedback(const QString &message, bool isError);

private:
    QString configFilePath() const;
    void loadSettings();
    void saveSettings() const;
    static QString dpiLabelForIndex(int tableIndex);
    static int64_t steadyNowMs();

    void triggerRead(bool userTriggered, bool levelOnlyFast);
    void joinReadThreadIfNeeded();
    void startPassiveWatcher();
    void stopPassiveWatcher();

    bool m_ready = false;
    int m_dpiLevel = 0;
    QColor m_dpiColor{0, 255, 0};
    QString m_dpiLabel;
    QString m_lastReadTime;
    bool m_inExtraMode = false;

    std::array<int, 6> m_hwDpiIndices{{3, 5, 10, 13, 15, 16}};
    std::array<RgbColor, 6> m_hwDpiColors{{
        {255, 255, 0}, {255, 0, 0}, {0, 255, 0},
        {0, 255, 255}, {0, 0, 255}, {255, 0, 255}
    }};

    bool m_autoSync = false;
    int m_intervalSec = 3;
    bool m_deviceAccessAvailable = false;
    std::atomic<bool> m_editorBusy{false};
    QString m_language = QStringLiteral("ar");

    QTimer m_pollTimer;
    std::atomic<bool> m_shuttingDown{false};
    std::atomic<bool> m_readBusy{false};
    std::thread m_readThread;

    std::atomic<bool> m_watcherRun{false};
    std::thread m_watcherThread;

    // Sequence & cooldown guards for rock-solid rapid DPI switching
    std::atomic<uint64_t> m_interruptSeq{0};
    std::atomic<int64_t> m_lastInterruptMs{0};
    std::atomic<int> m_latestInterruptIdx{-1};
    std::atomic<bool> m_interruptUiPending{false};
};
