#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>

class BackgroundPortal : public QObject {
    Q_OBJECT
public:
    explicit BackgroundPortal(QObject *parent = nullptr);
    ~BackgroundPortal() override;

    bool isBusy() const { return m_busy; }
    void requestAutostart(bool enable, const QString &language);
    void cancel();

signals:
    void busyChanged();
    void requestFinished(bool ok, bool autostartGranted, const QString &statusMessage);

private slots:
    void onPortalResponse(uint response, const QVariantMap &results);

private:
    void cleanupRequest();

    bool m_busy = false;
    bool m_requestedState = false;
    bool m_shuttingDown = false;
    quint32 m_tokenCounter = 0;
    QString m_language = QStringLiteral("ar");
    QString m_pendingRequestPath;
    QTimer m_timeoutTimer;
};
