#pragma once

#include <QObject>
#include <QString>

class SingleInstanceGuard : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepcomplex.App.Instance")

public:
    explicit SingleInstanceGuard(QObject *parent = nullptr);
    ~SingleInstanceGuard() override;

    // Returns true if this process is the primary instance.
    // Returns false if another instance is already running (and notifies it if activateExisting == true).
    bool acquireOrActivate(bool activateExisting);

public slots:
    Q_SCRIPTABLE void Activate();

signals:
    void activateRequested();

private:
    bool m_registered = false;
};
