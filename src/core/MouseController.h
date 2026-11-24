#pragma once

#include "ProfileStorage.h"
#include "../driver/M700ADriver.h"
#include "../system/BackgroundPortal.h"
#include "../system/SandboxManager.h"

#include <QColor>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <atomic>
#include <thread>

class MouseController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(QString devicePath READ devicePath NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool statusError READ statusError NOTIFY statusMessageChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

    Q_PROPERTY(bool autostartEnabled READ autostartEnabled WRITE setAutostartEnabled NOTIFY appSettingsChanged)
    Q_PROPERTY(bool autostartBusy READ autostartBusy NOTIFY appSettingsChanged)
    Q_PROPERTY(bool startMinimized READ startMinimized WRITE setStartMinimized NOTIFY appSettingsChanged)
    Q_PROPERTY(bool minimizeToTray READ minimizeToTray WRITE setMinimizeToTray NOTIFY appSettingsChanged)
    Q_PROPERTY(bool autoReadOnStartup READ autoReadOnStartup WRITE setAutoReadOnStartup NOTIFY appSettingsChanged)
    Q_PROPERTY(bool applyOnStartup READ applyOnStartup WRITE setApplyOnStartup NOTIFY appSettingsChanged)
    Q_PROPERTY(bool autoDetectDevice READ autoDetectDevice WRITE setAutoDetectDevice NOTIFY appSettingsChanged)
    Q_PROPERTY(QColor activeThemeAccent READ activeThemeAccent NOTIFY activeThemeAccentChanged)
    Q_PROPERTY(bool showHiddenEffects READ showHiddenEffects WRITE setShowHiddenEffects NOTIFY appSettingsChanged)
    Q_PROPERTY(bool showHexCodes READ showHexCodes WRITE setShowHexCodes NOTIFY appSettingsChanged)
    Q_PROPERTY(bool enableKeyCapture READ enableKeyCapture WRITE setEnableKeyCapture NOTIFY appSettingsChanged)
    Q_PROPERTY(bool showProgressAndLog READ showProgressAndLog WRITE setShowProgressAndLog NOTIFY appSettingsChanged)
    Q_PROPERTY(bool showStartupSplash READ showStartupSplash WRITE setShowStartupSplash NOTIFY appSettingsChanged)
    Q_PROPERTY(bool showRgbPreviewStrip READ showRgbPreviewStrip WRITE setShowRgbPreviewStrip NOTIFY appSettingsChanged)
    Q_PROPERTY(bool animatedUi READ animatedUi WRITE setAnimatedUi NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiThemePreset READ uiThemePreset WRITE setUiThemePreset NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiAccentMode READ uiAccentMode WRITE setUiAccentMode NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiCustomAccent READ uiCustomAccent WRITE setUiCustomAccent NOTIFY appSettingsChanged)
    Q_PROPERTY(double uiGlassOpacity READ uiGlassOpacity WRITE setUiGlassOpacity NOTIFY appSettingsChanged)
    Q_PROPERTY(int uiCornerRadius READ uiCornerRadius WRITE setUiCornerRadius NOTIFY appSettingsChanged)
    Q_PROPERTY(double uiGlowIntensity READ uiGlowIntensity WRITE setUiGlowIntensity NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiBackgroundStyle READ uiBackgroundStyle WRITE setUiBackgroundStyle NOTIFY appSettingsChanged)
    Q_PROPERTY(bool uiTrueBlackOled READ uiTrueBlackOled WRITE setUiTrueBlackOled NOTIFY appSettingsChanged)
    Q_PROPERTY(double uiBorderWidth READ uiBorderWidth WRITE setUiBorderWidth NOTIFY appSettingsChanged)
    Q_PROPERTY(double uiAnimationSpeed READ uiAnimationSpeed WRITE setUiAnimationSpeed NOTIFY appSettingsChanged)
    Q_PROPERTY(bool uiCompactMode READ uiCompactMode WRITE setUiCompactMode NOTIFY appSettingsChanged)
    Q_PROPERTY(bool uiUseCustomSecondary READ uiUseCustomSecondary WRITE setUiUseCustomSecondary NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiCustomSecondary READ uiCustomSecondary WRITE setUiCustomSecondary NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiBorderStyle READ uiBorderStyle WRITE setUiBorderStyle NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiCustomBorderColor READ uiCustomBorderColor WRITE setUiCustomBorderColor NOTIFY appSettingsChanged)
    Q_PROPERTY(double uiBorderOpacity READ uiBorderOpacity WRITE setUiBorderOpacity NOTIFY appSettingsChanged)
    Q_PROPERTY(bool uiWindowEdgeGlow READ uiWindowEdgeGlow WRITE setUiWindowEdgeGlow NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiWindowButtonStyle READ uiWindowButtonStyle WRITE setUiWindowButtonStyle NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiTitlebarStyle READ uiTitlebarStyle WRITE setUiTitlebarStyle NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiButtonShape READ uiButtonShape WRITE setUiButtonShape NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiButtonStyle READ uiButtonStyle WRITE setUiButtonStyle NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiCardEdgeStyle READ uiCardEdgeStyle WRITE setUiCardEdgeStyle NOTIFY appSettingsChanged)
    Q_PROPERTY(QString uiWindowShape READ uiWindowShape WRITE setUiWindowShape NOTIFY appSettingsChanged)
    Q_PROPERTY(bool autoSwitchByApp READ autoSwitchByApp WRITE setAutoSwitchByApp NOTIFY appSettingsChanged)
    Q_PROPERTY(int fallbackProfileIndex READ fallbackProfileIndex WRITE setFallbackProfileIndex NOTIFY appSettingsChanged)
    Q_PROPERTY(bool firstRunCompleted READ firstRunCompleted WRITE setFirstRunCompleted NOTIFY appSettingsChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString configFilePath READ configFilePath CONSTANT)
    Q_PROPERTY(bool launchedFromAutostart READ launchedFromAutostart CONSTANT)
    Q_PROPERTY(bool trayAvailable READ trayAvailable NOTIFY appPermissionsChanged)

    Q_PROPERTY(bool deviceAccessAvailable READ deviceAccessAvailable NOTIFY appPermissionsChanged)
    Q_PROPERTY(bool trayPermissionAvailable READ trayPermissionAvailable NOTIFY appPermissionsChanged)
    Q_PROPERTY(bool hostAccessAvailable READ hostAccessAvailable NOTIFY hostAccessChanged)
    Q_PROPERTY(QVariantList appPermissions READ appPermissions NOTIFY appPermissionsChanged)
    Q_PROPERTY(bool permissionsChecking READ permissionsChecking NOTIFY appPermissionsChanged)
    Q_PROPERTY(QString permissionsSummary READ permissionsSummary NOTIFY appPermissionsChanged)
    Q_PROPERTY(bool udevHealthy READ udevHealthy NOTIFY udevStatusChanged)
    Q_PROPERTY(QString udevErrorReason READ udevErrorReason NOTIFY udevStatusChanged)

    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY unsavedChangesChanged)
    Q_PROPERTY(QStringList unsavedChangesList READ unsavedChangesList NOTIFY unsavedChangesChanged)

    Q_PROPERTY(bool isBusy READ isBusy NOTIFY operationChanged)
    Q_PROPERTY(int operationProgress READ operationProgress NOTIFY operationChanged)
    Q_PROPERTY(QString operationLog READ operationLog NOTIFY operationChanged)
    Q_PROPERTY(QString operationTitle READ operationTitle NOTIFY operationChanged)
    Q_PROPERTY(bool operationSuccess READ operationSuccess NOTIFY operationChanged)

    Q_PROPERTY(int activeProfile READ activeProfile NOTIFY activeProfileChanged)
    Q_PROPERTY(QStringList profileNames READ profileNames NOTIFY profilesMetaChanged)
    Q_PROPERTY(QString activeProfileName READ activeProfileName NOTIFY profilesMetaChanged)
    Q_PROPERTY(QString activeProfileLinkedApps READ activeProfileLinkedApps NOTIFY profilesMetaChanged)
    Q_PROPERTY(QVariantList installedFlatpakApps READ installedFlatpakApps NOTIFY installedFlatpakAppsChanged)
    Q_PROPERTY(bool autoApplyOnSwitch READ autoApplyOnSwitch WRITE setAutoApplyOnSwitch NOTIFY profilesMetaChanged)

    Q_PROPERTY(int activeDpiLevel READ activeDpiLevel WRITE setActiveDpiLevel NOTIFY stateChanged)
    Q_PROPERTY(QVariantList buttonCodes READ buttonCodes NOTIFY stateChanged)
    Q_PROPERTY(QVariantList buttonMediaUsages READ buttonMediaUsages NOTIFY stateChanged)
    Q_PROPERTY(QVariantList macroLibrary READ macroLibrary NOTIFY macroLibraryChanged)
    Q_PROPERTY(int fireTimeMs READ fireTimeMs WRITE setFireTimeMs NOTIFY stateChanged)

    Q_PROPERTY(int ledMode READ ledMode WRITE setLedMode NOTIFY stateChanged)
    Q_PROPERTY(int ledSpeed READ ledSpeed WRITE setLedSpeed NOTIFY stateChanged)
    Q_PROPERTY(int ledBrightness READ ledBrightness WRITE setLedBrightness NOTIFY stateChanged)
    Q_PROPERTY(int ledDirection READ ledDirection WRITE setLedDirection NOTIFY stateChanged)
    Q_PROPERTY(bool ledSymmetry READ ledSymmetry WRITE setLedSymmetry NOTIFY stateChanged)
    Q_PROPERTY(bool ledSingleColor READ ledSingleColor WRITE setLedSingleColor NOTIFY stateChanged)
    Q_PROPERTY(bool userColorEnabled READ userColorEnabled WRITE setUserColorEnabled NOTIFY stateChanged)
    Q_PROPERTY(int reactiveEffect READ reactiveEffect WRITE setReactiveEffect NOTIFY stateChanged)
    Q_PROPERTY(QVariantList reactiveButtons READ reactiveButtons NOTIFY stateChanged)

    Q_PROPERTY(QVariantList dpiIndices READ dpiIndices NOTIFY stateChanged)
    Q_PROPERTY(QVariantList dpiEnabled READ dpiEnabled NOTIFY stateChanged)
    Q_PROPERTY(QVariantList dpiColors READ dpiColors NOTIFY stateChanged)
    Q_PROPERTY(QVariantList userColors READ userColors NOTIFY stateChanged)
    Q_PROPERTY(QVariantList userColorSlotsEnabled READ userColorSlotsEnabled NOTIFY stateChanged)
    Q_PROPERTY(QVariantList extraModeColors READ extraModeColors NOTIFY stateChanged)

public:
    static constexpr int kProfileCount = AppConfigData::kProfileCount;

    explicit MouseController(bool launchedFromAutostart = false, bool trayAvailable = false, QObject *parent = nullptr);
    ~MouseController() override;

    bool connected() const { return m_connected; }
    QString devicePath() const { return m_devicePath; }
    QString statusMessage() const { return m_statusMessage; }
    bool statusError() const { return m_statusError; }

    QString language() const { return m_cfg.language; }
    bool autostartEnabled() const { return m_cfg.autostartEnabled; }
    bool autostartBusy() const { return m_bgPortal.isBusy(); }
    bool startMinimized() const { return m_cfg.startMinimized; }
    bool minimizeToTray() const { return m_cfg.minimizeToTray; }
    bool autoReadOnStartup() const { return m_cfg.autoReadOnStartup; }
    bool applyOnStartup() const { return m_cfg.applyOnStartup; }
    bool autoDetectDevice() const { return m_cfg.autoDetectDevice; }
    QColor activeThemeAccent() const { return m_activeThemeAccent; }
    bool activeThemeIsLight() const { return !m_cfg.uiTrueBlackOled && (m_cfg.uiThemePreset == QStringLiteral("frost") || m_cfg.uiThemePreset == QStringLiteral("sand")); }
    bool showHiddenEffects() const { return m_cfg.showHiddenEffects; }
    bool showHexCodes() const { return m_cfg.showHexCodes; }
    bool enableKeyCapture() const { return m_cfg.enableKeyCapture; }
    bool showProgressAndLog() const { return m_cfg.showProgressAndLog; }
    bool showStartupSplash() const { return m_cfg.showStartupSplash; }
    bool showRgbPreviewStrip() const { return m_cfg.showRgbPreviewStrip; }
    bool animatedUi() const { return m_cfg.animatedUi; }
    QString uiThemePreset() const { return m_cfg.uiThemePreset; }
    QString uiAccentMode() const { return m_cfg.uiAccentMode; }
    QString uiCustomAccent() const { return m_cfg.uiCustomAccent; }
    double uiGlassOpacity() const { return m_cfg.uiGlassOpacity; }
    int uiCornerRadius() const { return m_cfg.uiCornerRadius; }
    double uiGlowIntensity() const { return m_cfg.uiGlowIntensity; }
    QString uiBackgroundStyle() const { return m_cfg.uiBackgroundStyle; }
    bool uiTrueBlackOled() const { return m_cfg.uiTrueBlackOled; }
    double uiBorderWidth() const { return m_cfg.uiBorderWidth; }
    double uiAnimationSpeed() const { return m_cfg.uiAnimationSpeed; }
    bool uiCompactMode() const { return m_cfg.uiCompactMode; }
    bool uiUseCustomSecondary() const { return m_cfg.uiUseCustomSecondary; }
    QString uiCustomSecondary() const { return m_cfg.uiCustomSecondary; }
    QString uiBorderStyle() const { return m_cfg.uiBorderStyle; }
    QString uiCustomBorderColor() const { return m_cfg.uiCustomBorderColor; }
    double uiBorderOpacity() const { return m_cfg.uiBorderOpacity; }
    bool uiWindowEdgeGlow() const { return m_cfg.uiWindowEdgeGlow; }
    QString uiWindowButtonStyle() const { return m_cfg.uiWindowButtonStyle; }
    QString uiTitlebarStyle() const { return m_cfg.uiTitlebarStyle; }
    QString uiButtonShape() const { return m_cfg.uiButtonShape; }
    QString uiButtonStyle() const { return m_cfg.uiButtonStyle; }
    QString uiCardEdgeStyle() const { return m_cfg.uiCardEdgeStyle; }
    QString uiWindowShape() const { return m_cfg.uiWindowShape; }
    bool autoSwitchByApp() const { return m_sandbox.hostAccessAvailable() && m_cfg.autoSwitchByApp; }
    int fallbackProfileIndex() const { return m_cfg.fallbackProfileIndex; }
    bool firstRunCompleted() const { return m_cfg.firstRunCompleted; }
    QString appVersion() const;
    QString configFilePath() const { return ProfileStorage::configFilePath(); }
    bool launchedFromAutostart() const { return m_launchedFromAutostart; }
    bool trayAvailable() const { return m_trayAvailable && m_sandbox.trayPermissionAvailable(); }

    bool deviceAccessAvailable() const { return m_sandbox.deviceAccessAvailable(); }
    bool trayPermissionAvailable() const { return m_sandbox.trayPermissionAvailable(); }
    bool hostAccessAvailable() const { return m_sandbox.hostAccessAvailable(); }
    QVariantList appPermissions() const { return m_sandbox.appPermissions(); }
    bool permissionsChecking() const { return m_sandbox.permissionsChecking(); }
    QString permissionsSummary() const { return m_sandbox.permissionsSummary(); }
    bool udevHealthy() const { return m_sandbox.udevHealthy(); }
    QString udevErrorReason() const { return m_sandbox.udevErrorReason(); }

    bool hasUnsavedChanges() const { return !m_unsavedChangesList.isEmpty(); }
    QStringList unsavedChangesList() const { return m_unsavedChangesList; }
    bool isBusy() const { return m_isBusy; }
    int operationProgress() const { return m_operationProgress; }
    QString operationLog() const { return m_operationLog; }
    QString operationTitle() const { return m_operationTitle; }
    bool operationSuccess() const { return m_operationSuccess; }

    int activeProfile() const { return m_cfg.activeProfile; }
    QStringList profileNames() const;
    QString activeProfileName() const { return m_cfg.profiles[m_cfg.activeProfile].name; }
    QString activeProfileLinkedApps() const { return m_cfg.profiles[m_cfg.activeProfile].linkedApps; }
    QVariantList installedFlatpakApps() const { return m_sandbox.installedFlatpakApps(); }
    bool autoApplyOnSwitch() const { return m_cfg.autoApplyOnSwitch; }

    int activeDpiLevel() const { return activeState().activeDpiLevel; }
    int fireTimeMs() const { return activeState().fireTimeMs; }
    int ledMode() const { return activeState().ledMode; }
    int ledSpeed() const { return activeState().ledSpeed; }
    int ledBrightness() const { return activeState().ledBrightness; }
    int ledDirection() const { return activeState().ledDirection; }
    bool ledSymmetry() const { return activeState().ledSymmetry; }
    bool ledSingleColor() const { return activeState().ledSingleColor; }
    bool userColorEnabled() const { return activeState().userColorEnabled; }
    int reactiveEffect() const { return activeState().reactiveEffect; }

    QVariantList buttonCodes() const;
    QVariantList buttonMediaUsages() const;
    QVariantList extraModeColors() const;
    QVariantList reactiveButtons() const;
    QVariantList dpiIndices() const;
    QVariantList dpiEnabled() const;
    QVariantList dpiColors() const;
    QVariantList userColors() const;
    QVariantList userColorSlotsEnabled() const;
    QVariantList macroLibrary() const;

    void setLanguage(const QString &lang);
    void setAutostartEnabled(bool enabled);
    void setStartMinimized(bool enabled);
    void setMinimizeToTray(bool enabled);
    void setAutoReadOnStartup(bool enabled);
    void setApplyOnStartup(bool enabled);
    void setAutoDetectDevice(bool enabled);
    void setShowHiddenEffects(bool enabled);
    void setShowHexCodes(bool enabled);
    void setEnableKeyCapture(bool enabled);
    void setShowProgressAndLog(bool enabled);
    void setShowStartupSplash(bool enabled);
    void setShowRgbPreviewStrip(bool enabled);
    void setAnimatedUi(bool enabled);
    void setUiThemePreset(const QString &preset);
    void setUiAccentMode(const QString &mode);
    void setUiCustomAccent(const QString &hexColor);
    void setUiGlassOpacity(double opacity);
    void setUiCornerRadius(int radius);
    void setUiGlowIntensity(double intensity);
    void setUiBackgroundStyle(const QString &style);
    void setUiTrueBlackOled(bool enabled);
    void setUiBorderWidth(double width);
    void setUiAnimationSpeed(double speed);
    void setUiCompactMode(bool compact);
    void setUiUseCustomSecondary(bool enabled);
    void setUiCustomSecondary(const QString &hexColor);
    void setUiBorderStyle(const QString &style);
    void setUiCustomBorderColor(const QString &hexColor);
    void setUiBorderOpacity(double opacity);
    void setUiWindowEdgeGlow(bool enabled);
    void setUiWindowButtonStyle(const QString &style);
    void setUiTitlebarStyle(const QString &style);
    void setUiButtonShape(const QString &shape);
    void setUiButtonStyle(const QString &style);
    void setUiCardEdgeStyle(const QString &style);
    void setUiWindowShape(const QString &shape);
    void setAutoSwitchByApp(bool enabled);
    void setFallbackProfileIndex(int index);
    void setFirstRunCompleted(bool completed);
    void setAutoApplyOnSwitch(bool enabled);

    void setActiveDpiLevel(int level);
    void setFireTimeMs(int ms);
    void setLedMode(int mode);
    void setLedSpeed(int speed);
    void setLedBrightness(int brightness);
    void setLedDirection(int direction);
    void setLedSymmetry(bool symmetry);
    void setLedSingleColor(bool singleColor);
    void setUserColorEnabled(bool enabled);
    void setReactiveEffect(int effect);

    Q_INVOKABLE void selectProfile(int index);
    Q_INVOKABLE void setActiveProfileName(const QString &name);
    Q_INVOKABLE void setActiveProfileLinkedApps(const QString &apps);
    Q_INVOKABLE void refreshInstalledFlatpakApps();
    Q_INVOKABLE bool isAppLinkedToActiveProfile(const QString &appId) const;
    Q_INVOKABLE void toggleAppForActiveProfile(const QString &appId);
    Q_INVOKABLE void resetActiveProfile();
    Q_INVOKABLE void resetAllSettingsAndProfiles();
    Q_INVOKABLE void resetUiTheme();

    Q_INVOKABLE bool exportBackup(const QUrl &fileUrl);
    Q_INVOKABLE bool importBackup(const QUrl &fileUrl);

    Q_INVOKABLE void setButtonCode(int slot, int code);
    Q_INVOKABLE void setButtonMediaUsage(int slot, int usage);
    Q_INVOKABLE void resetDefaultButtons();
    Q_INVOKABLE void setReactiveButtonEnabled(int slot, bool enabled);
    Q_INVOKABLE void setDpiIndex(int slot, int tableIndex);
    Q_INVOKABLE void setDpiEnabled(int slot, bool enabled);
    Q_INVOKABLE void setDpiColor(int slot, const QColor &color);
    Q_INVOKABLE void setUserColor(int slot, const QColor &color);
    Q_INVOKABLE void setExtraModeColor(int slot, const QColor &color);
    Q_INVOKABLE void setUserColorSlotEnabled(int slot, bool enabled);
    Q_INVOKABLE QString dpiLabelForIndex(int tableIndex) const;

    Q_INVOKABLE QVariantMap getLibraryMacro(int index) const;
    Q_INVOKABLE int createLibraryMacro(const QString &name, int repeatMode, int repeatCount, const QVariantList &steps);
    Q_INVOKABLE void updateLibraryMacro(int index, const QString &name, int repeatMode, int repeatCount, const QVariantList &steps);
    Q_INVOKABLE void deleteLibraryMacro(int index);
    Q_INVOKABLE bool exportSingleMacro(int index, const QUrl &fileUrl);
    Q_INVOKABLE bool importSingleMacro(const QUrl &fileUrl);
    Q_INVOKABLE void createQuickPresetInLibrary(const QString &presetId);
    Q_INVOKABLE void assignLibraryMacroToButton(int slot, int macroIndex);
    Q_INVOKABLE QString assignedMacroNameForButton(int slot) const;
    Q_INVOKABLE int keyEventToHidUsage(int qtKey, int nativeScanCode) const;
    Q_INVOKABLE QString hidUsageName(int hidCode) const;

    Q_INVOKABLE bool installUdevRules();
    Q_INVOKABLE void refreshAppPermissions();
    Q_INVOKABLE void copyCommandToClipboard(const QString &command);
    Q_INVOKABLE QString buildEnablePermissionsCommand(bool includeDevice, bool includeTray, bool includeHost, bool includeUdev) const;
    Q_INVOKABLE QVariantList commandExplanations(bool includeDevice, bool includeTray, bool includeHost, bool includeUdev, bool includeReset = false) const;
    Q_INVOKABLE void copyPermissionsResetCommand();
    Q_INVOKABLE QString hostAccessGrantCommand() const;
    Q_INVOKABLE QString trayGrantCommand() const;
    Q_INVOKABLE void copyTrayGrantCommand();
    Q_INVOKABLE void copyHostAccessCommand();
    Q_INVOKABLE void copyConfigFilePath();
    Q_INVOKABLE QString udevManualCommand() const;
    Q_INVOKABLE void copyUdevManualCommand();
    Q_INVOKABLE void checkUdevHealth();
    Q_INVOKABLE void copyOperationLog();

    Q_INVOKABLE void setActiveThemeAccent(const QColor &color);
    void reportExternalStatus(const QString &msg, bool isError);
    Q_INVOKABLE void refreshFromMouse();
    Q_INVOKABLE void applyToMouse();

signals:
    void stateChanged();
    void statusMessageChanged();
    void activeProfileChanged();
    void profilesMetaChanged();
    void languageChanged();
    void appSettingsChanged();
    void operationChanged();
    void operationStarted();
    void macroLibraryChanged();
    void unsavedChangesChanged();
    void udevStatusChanged();
    void udevMissingDetected();
    void installedFlatpakAppsChanged();
    void hostAccessChanged();
    void appPermissionsChanged();
    void hardwareDpiSnapshotSynced(int activeLevel,
                                   const std::array<int, 6> &dpiIndices,
                                   const std::array<RgbColor, 6> &dpiColors,
                                   bool inExtraMode);
    void fullResetPerformed();
    void activeThemeAccentChanged();

private:
    MouseState &activeState() { return m_cfg.profiles[m_cfg.activeProfile].state; }
    const MouseState &activeState() const { return m_cfg.profiles[m_cfg.activeProfile].state; }

    QString loc(const QString &ar, const QString &en, const QString &ru, const QString &zh) const;
    QString translateDriverError(DriverError err, const std::string &detail) const;
    QString formatMacroSummary(const ButtonMacro &mac) const;
    void appendLogLine(const QString &line);
    void joinWorkerIfNeeded();

    void initDefaultProfiles();
    void reconcileHardwareMacrosForProfile(int profileIdx);
    void pollDeviceConnection();
    void onRunningAppsMatched(int matchedProfile, const QString &matchedToken);

    void saveProfilesToDisk();
    void scheduleSaveToDisk();
    void setStatus(const QString &msg, bool isError = false);
    void syncBaselineFromActive();
    void recomputeUnsavedChanges();

    M700ADriver m_driver;
    AppConfigData m_cfg;
    BackgroundPortal m_bgPortal;
    SandboxManager m_sandbox;

    bool m_launchedFromAutostart = false;
    bool m_trayAvailable = false;
    bool m_autoSwitchedActive = false;

    MouseState m_lastAppliedState;
    bool m_hasBaselineState = false;
    QStringList m_unsavedChangesList;

    bool m_isBusy = false;
    int m_operationProgress = 0;
    QString m_operationLog;
    QString m_operationTitle;
    bool m_operationSuccess = true;
    std::atomic<bool> m_shuttingDown{false};
    std::thread m_workerThread;
    QColor m_activeThemeAccent{21, 128, 61};

    bool m_connected = false;
    QString m_devicePath;
    bool m_statusError = false;
    QString m_statusMessage;

    QTimer m_saveTimer;
    QTimer m_hotplugTimer;
    QTimer m_udevWatchTimer;
    QTimer m_appWatchTimer;
};
