#include "BackgroundPortal.h"
#include "AsyncProcess.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QStringList>

using SystemUtils::locText;

BackgroundPortal::BackgroundPortal(QObject *parent)
    : QObject(parent) {
    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(30000);
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this]() {
        if (!m_busy || m_shuttingDown) return;
        cleanupRequest();
        emit busyChanged();
        emit requestFinished(false, false, locText(
            m_language,
            QStringLiteral("انتهت مهلة الاستجابة من بوابة الخلفية (XDG Background Portal)."),
            QStringLiteral("XDG Background Portal request timed out."),
            QStringLiteral("Время ожидания XDG Background Portal истекло."),
            QStringLiteral("XDG 后台门户请求超时。")
        ));
    });
}

BackgroundPortal::~BackgroundPortal() {
    m_shuttingDown = true;
    cleanupRequest();
}

void BackgroundPortal::cancel() {
    cleanupRequest();
    emit busyChanged();
}

void BackgroundPortal::cleanupRequest() {
    m_timeoutTimer.stop();
    if (!m_pendingRequestPath.isEmpty()) {
        QDBusConnection bus = QDBusConnection::sessionBus();
        if (bus.isConnected()) {
            bus.disconnect(
                QStringLiteral("org.freedesktop.portal.Desktop"),
                m_pendingRequestPath,
                QStringLiteral("org.freedesktop.portal.Request"),
                QStringLiteral("Response"),
                this,
                SLOT(onPortalResponse(uint, QVariantMap))
            );
        }
        m_pendingRequestPath.clear();
    }
    m_busy = false;
}

void BackgroundPortal::requestAutostart(bool enable, const QString &language) {
    m_language = language;
    if (m_busy || m_shuttingDown) {
        emit busyChanged();
        return;
    }

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        emit requestFinished(false, false, locText(
            m_language,
            QStringLiteral("تعذر الاتصال بناقل D-Bus لتشغيل بوابة التشغيل التلقائي (XDG Background Portal)."),
            QStringLiteral("Cannot connect to session D-Bus for XDG Background Portal."),
            QStringLiteral("Не удалось подключиться к D-Bus для XDG Background Portal."),
            QStringLiteral("无法连接到会话 D-Bus 以调用 XDG 后台门户。")
        ));
        return;
    }

    m_busy = true;
    m_requestedState = enable;
    emit busyChanged();

    const QString token = QStringLiteral("deepcomplex_bg_%1").arg(++m_tokenCounter);
    QString sender = bus.baseService();
    if (sender.startsWith(QLatin1Char(':'))) sender.remove(0, 1);
    sender.replace(QLatin1Char('.'), QLatin1Char('_'));
    const QString expectedPath = QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2").arg(sender, token);

    m_pendingRequestPath = expectedPath;
    bus.connect(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        expectedPath,
        QStringLiteral("org.freedesktop.portal.Request"),
        QStringLiteral("Response"),
        this,
        SLOT(onPortalResponse(uint, QVariantMap))
    );

    QVariantMap options;
    options.insert(QStringLiteral("handle_token"), token);
    options.insert(QStringLiteral("reason"), locText(
        m_language,
        QStringLiteral("تطبيق إعدادات ماوس Delux M700A تلقائياً عند تسجيل الدخول"),
        QStringLiteral("Automatically apply Delux M700A mouse profiles on login"),
        QStringLiteral("Автоматическое применение профилей мыши Delux M700A при входе"),
        QStringLiteral("登录时自动应用 Delux M700A 鼠标配置文件")
    ));
    options.insert(QStringLiteral("autostart"), enable);
    if (enable) {
        options.insert(QStringLiteral("commandline"), QStringList{QStringLiteral("deepcomplex"), QStringLiteral("--autostart")});
    }
    options.insert(QStringLiteral("dbus-activatable"), false);

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Background"),
        QStringLiteral("RequestBackground")
    );
    msg << QString() << options;

    m_timeoutTimer.start();
    QDBusPendingCall pending = bus.asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(pending, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, expectedPath]() {
        watcher->deleteLater();
        if (m_shuttingDown || !m_busy) return;

        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            cleanupRequest();
            emit busyChanged();
            emit requestFinished(false, false, locText(
                m_language,
                QStringLiteral("بوابة التشغيل التلقائي (XDG Background Portal) غير متوفرة حالياً على النظام."),
                QStringLiteral("XDG Background Portal is currently unavailable on this system."),
                QStringLiteral("XDG Background Portal недоступен в текущей системе."),
                QStringLiteral("当前系统暂不支持 XDG 后台门户。")
            ));
            return;
        }

        const QString returnedPath = reply.value().path();
        if (!returnedPath.isEmpty() && returnedPath != expectedPath && returnedPath != m_pendingRequestPath) {
            QDBusConnection session = QDBusConnection::sessionBus();
            if (session.isConnected()) {
                session.disconnect(
                    QStringLiteral("org.freedesktop.portal.Desktop"),
                    m_pendingRequestPath,
                    QStringLiteral("org.freedesktop.portal.Request"),
                    QStringLiteral("Response"),
                    this,
                    SLOT(onPortalResponse(uint, QVariantMap))
                );
                m_pendingRequestPath = returnedPath;
                session.connect(
                    QStringLiteral("org.freedesktop.portal.Desktop"),
                    returnedPath,
                    QStringLiteral("org.freedesktop.portal.Request"),
                    QStringLiteral("Response"),
                    this,
                    SLOT(onPortalResponse(uint, QVariantMap))
                );
            }
        }
    });
}

void BackgroundPortal::onPortalResponse(uint response, const QVariantMap &results) {
    if (m_shuttingDown) return;
    const bool requested = m_requestedState;
    cleanupRequest();
    emit busyChanged();

    if (response == 0) {
        const bool granted = results.value(QStringLiteral("autostart"), requested).toBool();
        emit requestFinished(true, granted, granted
            ? locText(m_language,
                      QStringLiteral("تم تفعيل التشغيل التلقائي بأمان عبر بوابة النظام (XDG Background Portal)."),
                      QStringLiteral("Autostart enabled securely via XDG Background Portal."),
                      QStringLiteral("Автозапуск безопасно включён через XDG Background Portal."),
                      QStringLiteral("已通过 XDG 后台门户安全启用开机自动启动。"))
            : locText(m_language,
                      QStringLiteral("تم إيقاف التشغيل التلقائي عبر بوابة النظام."),
                      QStringLiteral("Autostart disabled via XDG Background Portal."),
                      QStringLiteral("Автозапуск отключён через XDG Background Portal."),
                      QStringLiteral("已通过 XDG 后台门户关闭开机自动启动。")));
    } else {
        emit requestFinished(false, false, locText(
            m_language,
            QStringLiteral("تم رفض أو إلغاء طلب التشغيل التلقائي من بوابة النظام."),
            QStringLiteral("Autostart request was denied or canceled by the system portal."),
            QStringLiteral("Запрос автозапуска отклонён или отменён системным порталом."),
            QStringLiteral("系统门户已拒绝或取消自动启动请求。")
        ));
    }
}
