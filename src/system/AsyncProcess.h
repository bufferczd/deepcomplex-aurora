#pragma once

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <memory>
#include <type_traits>
#include <utility>

namespace SystemUtils {

inline bool isRunningInFlatpak() {
    static const bool cached = QFile::exists(QStringLiteral("/.flatpak-info"));
    return cached;
}

inline QString locText(const QString &lang, const QString &ar, const QString &en, const QString &ru, const QString &zh) {
    if (lang == QStringLiteral("en")) return en;
    if (lang == QStringLiteral("ru")) return ru;
    if (lang == QStringLiteral("zh")) return zh;
    return ar;
}

inline bool readRegularFileBounded(const QString &path, qint64 maxBytes, QByteArray &outBytes) {
    QFileInfo info(path);
    if (!info.exists() || !info.isFile()) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray data = file.read(maxBytes + 1);
    file.close();
    if (data.size() > maxBytes) return false;
    outBytes = std::move(data);
    return true;
}

inline void terminateChildProcesses(QObject *owner) {
    if (!owner) return;
    const auto childProcs = owner->findChildren<QProcess *>();
    for (QProcess *proc : childProcs) {
        if (proc && proc->state() != QProcess::NotRunning) {
            proc->disconnect(owner);
            proc->kill();
            proc->waitForFinished(500);
        }
    }
}

template <typename OnFinishedFn, typename OnFailureFn>
void runSafeAsyncProcess(QObject *owner,
                         const QString &program,
                         const QStringList &args,
                         int timeoutMs,
                         OnFinishedFn &&onFinished,
                         OnFailureFn &&onFailure) {
    if (!owner) return;
    auto *proc = new QProcess(owner);
    auto settled = std::make_shared<bool>(false);
    auto *timer = new QTimer(proc);
    timer->setSingleShot(true);
    timer->setInterval(timeoutMs);

    QPointer<QObject> safeOwner(owner);
    auto failureCb = std::make_shared<std::decay_t<OnFailureFn>>(std::forward<OnFailureFn>(onFailure));

    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     owner, [proc, timer, settled, safeOwner, onFinished = std::forward<OnFinishedFn>(onFinished)](int exitCode, QProcess::ExitStatus exitStatus) mutable {
        if (!*settled) {
            *settled = true;
            timer->stop();
            if (safeOwner) onFinished(proc, exitCode, exitStatus);
        }
        proc->deleteLater();
    });

    QObject::connect(proc, &QProcess::errorOccurred,
                     owner, [proc, timer, settled, safeOwner, failureCb](QProcess::ProcessError err) mutable {
        if (err == QProcess::FailedToStart && !*settled) {
            *settled = true;
            timer->stop();
            if (safeOwner) (*failureCb)();
            proc->deleteLater();
        }
    });

    QObject::connect(timer, &QTimer::timeout,
                     owner, [proc, settled, safeOwner, failureCb]() mutable {
        if (*settled) return;
        *settled = true;
        if (proc->state() != QProcess::NotRunning) {
            proc->kill();
            if (proc->waitForFinished(250)) proc->deleteLater();
        } else {
            proc->deleteLater();
        }
        if (safeOwner) (*failureCb)();
    });

    timer->start();
    proc->start(program, args);
}

} // namespace SystemUtils
