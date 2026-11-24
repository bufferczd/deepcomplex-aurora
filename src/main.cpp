#include "core/MouseController.h"
#include "system/SingleInstance.h"
#include "system/ThemeDpiMonitor.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QLinearGradient>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QRadialGradient>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QWindow>
#include <array>
#include <iostream>

namespace {

// Renders the DeepComplex Delux M700A icon dynamically tinted to the live Theme accent color
// so both the Plasma taskbar icon and System Tray icon adapt in real time.
QIcon renderThemedAppIcon(const QColor &accentInput, bool isLightTheme) {
    const QColor accent = accentInput.isValid() ? accentInput : QColor(21, 128, 61);
    const QColor accentLight = accent.lighter(120);
    const QColor accentDark = accent.darker(125);

    QPixmap pix(128, 128);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient strokeGrad(0, 0, 128, 128);
    strokeGrad.setColorAt(0.0, accentLight);
    strokeGrad.setColorAt(0.5, accent);
    strokeGrad.setColorAt(1.0, accentDark);

    // Outer rounded badge
    const QColor bgBase = isLightTheme ? QColor(238, 244, 251) : QColor(0, 0, 0);
    p.setBrush(bgBase);
    p.setPen(QPen(QBrush(strokeGrad), 5.0));
    p.drawRoundedRect(QRectF(5, 5, 118, 118), 28, 28);

    // Inner ambient radial glow
    QRadialGradient glow(64, 88, 68);
    QColor glowCol = accent;
    glowCol.setAlphaF(isLightTheme ? 0.22 : 0.34);
    glow.setColorAt(0.0, glowCol);
    glow.setColorAt(1.0, Qt::transparent);
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawRoundedRect(QRectF(9, 9, 110, 110), 24, 24);

    // Mouse silhouette body
    const QColor mouseFill = isLightTheme ? QColor(255, 255, 255) : QColor(3, 8, 5);
    p.setBrush(mouseFill);
    p.setPen(QPen(QBrush(strokeGrad), 4.0));
    p.drawRoundedRect(QRectF(38, 20, 52, 88), 26, 26);

    // Center button split line
    QColor splitCol = accent;
    splitCol.setAlphaF(0.60);
    p.setPen(QPen(splitCol, 2.5));
    p.drawLine(QPointF(64, 21), QPointF(64, 54));

    // Curved button seam
    QPainterPath seam;
    seam.moveTo(39.5, 54);
    seam.quadTo(64, 59.5, 88.5, 54);
    QColor seamCol = accent;
    seamCol.setAlphaF(0.48);
    p.setPen(QPen(seamCol, 2.2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(seam);

    // Scroll wheel & DPI LED dot
    p.setPen(Qt::NoPen);
    p.setBrush(accent);
    p.drawRoundedRect(QRectF(59, 30, 10, 19), 5, 5);
    p.drawEllipse(QPointF(64, 62), 4.0, 4.0);

    // Bottom RGB underglow arc
    QPainterPath arc;
    arc.moveTo(46, 88);
    arc.quadTo(64, 100, 82, 88);
    QPen arcPen(accent, 5.2, Qt::SolidLine, Qt::RoundCap);
    p.setPen(arcPen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(arc);

    p.end();
    return QIcon(pix);
}

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("DeepComplex"));
    QApplication::setOrganizationDomain(QStringLiteral("github.com/bufferczd/deepcomplex-aurora"));
    QApplication::setApplicationName(QStringLiteral("DeepComplex"));
    QApplication::setDesktopFileName(QStringLiteral("org.deepcomplex.App"));
#ifdef DEEPCOMPLEX_VERSION
    QApplication::setApplicationVersion(QStringLiteral(DEEPCOMPLEX_VERSION));
#else
    QApplication::setApplicationVersion(QStringLiteral("2.8.4"));
#endif
    if (QApplication::arguments().contains(QStringLiteral("--version")) ||
        QApplication::arguments().contains(QStringLiteral("-v"))) {
        std::cout << "DeepComplex " << QApplication::applicationVersion().toStdString() << std::endl;
        return 0;
    }
    app.setQuitOnLastWindowClosed(false);

    const bool launchedFromAutostart = QApplication::arguments().contains(QStringLiteral("--autostart"));

    SingleInstanceGuard singleInstance;
    if (!singleInstance.acquireOrActivate(!launchedFromAutostart)) {
        return 0;
    }

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    MouseController controller(launchedFromAutostart, trayAvailable);

    // Dedicated, completely independent DPI reader for the Theme & Titlebar Chip
    ThemeDpiMonitor themeDpiMonitor;
    themeDpiMonitor.setLanguage(controller.language());
    themeDpiMonitor.setDeviceAccessAvailable(controller.deviceAccessAvailable());
    QObject::connect(&controller, &MouseController::languageChanged, &themeDpiMonitor, [&]() {
        themeDpiMonitor.setLanguage(controller.language());
    });
    QObject::connect(&controller, &MouseController::appPermissionsChanged, &themeDpiMonitor, [&]() {
        themeDpiMonitor.setDeviceAccessAvailable(controller.deviceAccessAvailable());
    });
    QObject::connect(&controller, &MouseController::operationChanged, &themeDpiMonitor, [&]() {
        themeDpiMonitor.setEditorBusy(controller.isBusy());
    });
    QObject::connect(&controller, &MouseController::hardwareDpiSnapshotSynced, &themeDpiMonitor,
                     &ThemeDpiMonitor::syncFromCompletedHardwareTransfer);
    QObject::connect(&controller, &MouseController::fullResetPerformed, &themeDpiMonitor,
                     &ThemeDpiMonitor::resetDefaults);
    QObject::connect(&themeDpiMonitor, &ThemeDpiMonitor::statusFeedback, &controller,
                     &MouseController::reportExternalStatus);

    const QIcon initialIcon = renderThemedAppIcon(controller.activeThemeAccent(), controller.activeThemeIsLight());
    QApplication::setWindowIcon(initialIcon);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("mouseController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("themeDpiMonitor"), &themeDpiMonitor);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("org.deepcomplex.App", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    QPointer<QWindow> mainWindow = qobject_cast<QWindow *>(engine.rootObjects().constFirst());
    if (mainWindow) {
        mainWindow->setIcon(initialIcon);
    }

    auto showAndActivateWindow = [mainWindow]() {
        if (!mainWindow) return;
        mainWindow->show();
        Qt::WindowStates states = mainWindow->windowStates();
        if (states.testFlag(Qt::WindowMinimized)) {
            states.setFlag(Qt::WindowMinimized, false);
            mainWindow->setWindowStates(states);
        }
        mainWindow->raise();
        mainWindow->requestActivate();
    };

    QObject::connect(&singleInstance, &SingleInstanceGuard::activateRequested, &app, showAndActivateWindow);

    QMenu trayMenu;
    QSystemTrayIcon trayIcon(initialIcon);

    QAction *showHideAction = trayMenu.addAction(QString());
    trayMenu.addSeparator();

    std::array<QAction *, MouseController::kProfileCount> profileActions{};
    for (int i = 0; i < MouseController::kProfileCount; ++i) {
        profileActions[i] = trayMenu.addAction(QString());
        profileActions[i]->setCheckable(true);
        QObject::connect(profileActions[i], &QAction::triggered, &controller, [&controller, i]() {
            controller.selectProfile(i);
            if (!controller.autoApplyOnSwitch() && controller.connected() && controller.udevHealthy()) {
                controller.applyToMouse();
            }
        });
    }

    trayMenu.addSeparator();
    QAction *quitAction = trayMenu.addAction(QString());

    auto updateThemedIcon = [&]() {
        const QIcon themed = renderThemedAppIcon(controller.activeThemeAccent(), controller.activeThemeIsLight());
        QApplication::setWindowIcon(themed);
        if (mainWindow) mainWindow->setIcon(themed);
        trayIcon.setIcon(themed);
    };

    auto updateTrayUi = [&]() {
        const QString lang = controller.language();
        if (lang == QStringLiteral("en")) {
            showHideAction->setText(QStringLiteral("Show / Hide DeepComplex"));
            quitAction->setText(QStringLiteral("Quit"));
        } else if (lang == QStringLiteral("ru")) {
            showHideAction->setText(QStringLiteral("Показать / Скрыть DeepComplex"));
            quitAction->setText(QStringLiteral("Выход"));
        } else if (lang == QStringLiteral("zh")) {
            showHideAction->setText(QStringLiteral("显示 / 隐藏 DeepComplex"));
            quitAction->setText(QStringLiteral("退出"));
        } else {
            showHideAction->setText(QStringLiteral("إظهار / إخفاء DeepComplex"));
            quitAction->setText(QStringLiteral("إنهاء التطبيق"));
        }

        const QStringList names = controller.profileNames();
        for (int i = 0; i < MouseController::kProfileCount; ++i) {
            const QString label = QString::number(i + 1) + QStringLiteral(" • ")
                                  + (i < names.size() ? names[i] : QString::number(i + 1));
            profileActions[i]->setText(label);
            profileActions[i]->setChecked(controller.activeProfile() == i);
        }

        trayIcon.setToolTip(QStringLiteral("DeepComplex — Delux M700A (")
                            + controller.activeProfileName() + QStringLiteral(")"));
        trayIcon.setVisible(controller.trayAvailable() && (controller.minimizeToTray() || controller.startMinimized()));
    };

    auto toggleWindow = [mainWindow, showAndActivateWindow]() {
        if (!mainWindow) return;
        if (mainWindow->isVisible()) {
            mainWindow->hide();
        } else {
            showAndActivateWindow();
        }
    };

    QObject::connect(showHideAction, &QAction::triggered, &trayIcon, toggleWindow);
    QObject::connect(quitAction, &QAction::triggered, &app, &QCoreApplication::quit);
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, &trayIcon, [toggleWindow](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            toggleWindow();
        }
    });

    QObject::connect(&controller, &MouseController::languageChanged, &trayIcon, updateTrayUi);
    QObject::connect(&controller, &MouseController::profilesMetaChanged, &trayIcon, updateTrayUi);
    QObject::connect(&controller, &MouseController::activeProfileChanged, &trayIcon, updateTrayUi);
    QObject::connect(&controller, &MouseController::appSettingsChanged, &trayIcon, updateTrayUi);
    QObject::connect(&controller, &MouseController::appPermissionsChanged, &trayIcon, updateTrayUi);
    // Debounce 128x128 QPainter + D-Bus tray icon updates safely (parented to &trayIcon to avoid shutdown UAF)
    auto *iconUpdateTimer = new QTimer(&trayIcon);
    iconUpdateTimer->setSingleShot(true);
    iconUpdateTimer->setInterval(120);
    QObject::connect(iconUpdateTimer, &QTimer::timeout, &trayIcon, updateThemedIcon);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, iconUpdateTimer, &QTimer::stop);
    QObject::connect(&controller, &MouseController::activeThemeAccentChanged, iconUpdateTimer, [iconUpdateTimer]() { iconUpdateTimer->start(); });
    QObject::connect(&controller, &MouseController::appSettingsChanged, iconUpdateTimer, [iconUpdateTimer]() { iconUpdateTimer->start(); });

    trayIcon.setContextMenu(&trayMenu);
    updateTrayUi();
    updateThemedIcon();

    return app.exec();
}
