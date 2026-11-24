#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

class SandboxManager : public QObject {
    Q_OBJECT
public:
    explicit SandboxManager(QObject *parent = nullptr);
    ~SandboxManager() override;

    void setLanguage(const QString &lang) { m_language = lang; }
    void probeHostAccessOnStartup();

    bool deviceAccessAvailable() const;
    bool trayPermissionAvailable() const;
    bool hostAccessAvailable() const { return m_hostAccessAvailable; }
    bool permissionsChecking() const { return m_permRefreshRunning; }
    QString permissionsSummary() const;

    bool udevHealthy() const { return m_udevHealthy; }
    QString udevErrorReason() const { return m_udevErrorReason; }
    void markUdevUnhealthy(const QString &reason);

    QVariantList appPermissions() const;
    QVariantList installedFlatpakApps() const { return m_installedFlatpakApps; }

    static QString udevManualCommand();
    static QString hostAccessGrantCommand();
    static QString trayGrantCommand();
    static QString permissionsResetCommand();
    static QString buildEnablePermissionsCommand(bool includeDevice, bool includeTray, bool includeHost, bool includeUdev);
    QVariantList commandExplanations(bool includeDevice, bool includeTray, bool includeHost, bool includeUdev, bool includeReset) const;

    void checkUdevHealth();
    bool installUdevRules();
    void refreshAppPermissions();

    void refreshInstalledFlatpakApps();
    void pollRunningApps(const QStringList &profileLinkedApps);

signals:
    void hostAccessChanged();
    void appPermissionsChanged();
    void udevStatusChanged();
    void udevMissingDetected();
    void udevInstalledSucceeded();
    void installedFlatpakAppsChanged();
    void runningAppsMatched(int matchedProfileIndex, const QString &matchedToken);
    void statusMessage(const QString &message, bool isError);

private:
    void initDefaultActivePermissions();
    void readSandboxInfo();
    void runLiveLocalChecks();
    void parseIniActivePerms(const QString &iniText);
    void updateUdevHealthState(bool healthy, const QString &reason);
    QString loc(const QString &ar, const QString &en, const QString &ru, const QString &zh) const;

    QString m_language = QStringLiteral("ar");
    bool m_shuttingDown = false;

    bool m_hostAccessAvailable = false;
    bool m_permRefreshRunning = false;
    QMap<QString, bool> m_activePerms;

    // Live runtime verification diagnostics
    int m_liveHidrawNodeCount = 0;
    bool m_liveMousePresent = false;
    bool m_liveMouseRwOk = false;
    QString m_liveMousePath;
    bool m_liveBgPortalOk = false;
    bool m_liveTrayDbusOk = false;
    QString m_lastCheckedTime;

    bool m_udevHealthy = true;
    bool m_udevAlertEmitted = false;
    bool m_udevCheckRunning = false;
    bool m_udevOpRunning = false;
    QString m_udevErrorReason;

    bool m_appCheckRunning = false;
    bool m_appsRefreshRunning = false;
    QVariantList m_installedFlatpakApps;
};
