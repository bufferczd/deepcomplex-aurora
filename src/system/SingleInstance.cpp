#include "SingleInstance.h"

#include <QDBusConnection>
#include <QDBusMessage>

namespace {
constexpr auto kServiceName = "org.deepcomplex.App";
constexpr auto kObjectPath  = "/org/deepcomplex/App";
constexpr auto kInterface   = "org.deepcomplex.App.Instance";
} // namespace

SingleInstanceGuard::SingleInstanceGuard(QObject *parent)
    : QObject(parent) {}

SingleInstanceGuard::~SingleInstanceGuard() {
    if (m_registered) {
        QDBusConnection bus = QDBusConnection::sessionBus();
        if (bus.isConnected()) {
            bus.unregisterObject(QString::fromLatin1(kObjectPath));
            bus.unregisterService(QString::fromLatin1(kServiceName));
        }
    }
}

bool SingleInstanceGuard::acquireOrActivate(bool activateExisting) {
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        // Fallback: if session D-Bus is unavailable, allow startup normally without crashing
        return true;
    }

    if (bus.registerService(QString::fromLatin1(kServiceName))) {
        m_registered = true;
        bus.registerObject(
            QString::fromLatin1(kObjectPath),
            this,
            QDBusConnection::ExportScriptableSlots
        );
        return true;
    }

    // Another instance already owns org.deepcomplex.App
    if (activateExisting) {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            QString::fromLatin1(kServiceName),
            QString::fromLatin1(kObjectPath),
            QString::fromLatin1(kInterface),
            QStringLiteral("Activate")
        );
        bus.call(msg, QDBus::Block, 1500);
    }
    return false;
}

void SingleInstanceGuard::Activate() {
    emit activateRequested();
}
