#include "SandboxManager.h"
#include "AsyncProcess.h"
#include "../driver/M700ADriver.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QSystemTrayIcon>
#include <QTime>
#include <QVariantMap>
#include <algorithm>
#include <unistd.h>

using SystemUtils::isRunningInFlatpak;
using SystemUtils::locText;
using SystemUtils::runSafeAsyncProcess;
using SystemUtils::terminateChildProcesses;

SandboxManager::SandboxManager(QObject *parent)
    : QObject(parent) {
    initDefaultActivePermissions();
}

SandboxManager::~SandboxManager() {
    m_shuttingDown = true;
    terminateChildProcesses(this);
}

QString SandboxManager::loc(const QString &ar, const QString &en, const QString &ru, const QString &zh) const {
    return locText(m_language, ar, en, ru, zh);
}

bool SandboxManager::deviceAccessAvailable() const {
    if (!isRunningInFlatpak()) return true;
    return m_activePerms.value(QStringLiteral("device_all"), false);
}

bool SandboxManager::trayPermissionAvailable() const {
    if (!isRunningInFlatpak()) return true;
    return m_activePerms.value(QStringLiteral("tray_dbus"), false);
}

QString SandboxManager::udevManualCommand() {
    return QStringLiteral(
        "echo 'SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"30fa\", ATTRS{idProduct}==\"1040\", MODE=\"0660\", TAG+=\"uaccess\"' "
        "| sudo tee /etc/udev/rules.d/70-delux-m700a.rules && sudo rm -f /etc/udev/rules.d/99-delux-m700a.rules "
        "&& sudo udevadm control --reload-rules && sudo udevadm trigger"
    );
}

QString SandboxManager::hostAccessGrantCommand() {
    return QStringLiteral("flatpak override --user --talk-name=org.freedesktop.Flatpak org.deepcomplex.App");
}

QString SandboxManager::trayGrantCommand() {
    return QStringLiteral("flatpak override --user --talk-name=org.kde.StatusNotifierWatcher org.deepcomplex.App");
}

QString SandboxManager::permissionsResetCommand() {
    return QStringLiteral("flatpak override --user --reset org.deepcomplex.App");
}

QString SandboxManager::buildEnablePermissionsCommand(bool includeDevice, bool includeTray, bool includeHost, bool includeUdev) {
    QStringList flags;
    if (includeDevice) flags << QStringLiteral("--device=all");
    if (includeTray) flags << QStringLiteral("--talk-name=org.kde.StatusNotifierWatcher");
    if (includeHost) flags << QStringLiteral("--talk-name=org.freedesktop.Flatpak");

    QStringList parts;
    if (!flags.isEmpty()) {
        parts << (QStringLiteral("flatpak override --user ") + flags.join(QLatin1Char(' ')) + QStringLiteral(" org.deepcomplex.App"));
    }
    if (includeUdev) {
        parts << udevManualCommand();
    }
    return parts.join(QStringLiteral(" && "));
}

QVariantList SandboxManager::commandExplanations(bool includeDevice, bool includeTray, bool includeHost, bool includeUdev, bool includeReset) const {
    QVariantList list;

    auto addItem = [&](const QString &icon, const QString &cmd, const QString &title,
                       const QString &purpose, const QString &breakdown,
                       const QString &boundaries, const QString &impact,
                       const QString &verifyUndo, const QString &riskLevel, bool optionalFlag) {
        QVariantMap m;
        m[QStringLiteral("icon")] = icon;
        m[QStringLiteral("command")] = cmd;
        m[QStringLiteral("title")] = title;
        m[QStringLiteral("purpose")] = purpose;
        m[QStringLiteral("breakdown")] = breakdown;
        m[QStringLiteral("boundaries")] = boundaries;
        m[QStringLiteral("impact")] = impact;
        m[QStringLiteral("verifyUndo")] = verifyUndo;
        m[QStringLiteral("riskLevel")] = riskLevel; // "low", "medium", "high"
        m[QStringLiteral("optional")] = optionalFlag;
        list.append(m);
    };

    if (includeDevice || includeTray || includeHost) {
        addItem(
            QStringLiteral("utilities-terminal"),
            QStringLiteral("flatpak override --user ... org.deepcomplex.App"),
            loc(QStringLiteral("الأمر الأساسي لتعديل صلاحيات Flatpak"),
                QStringLiteral("Base Flatpak Permission Override Command"),
                QStringLiteral("Базовая команда настройки прав Flatpak"),
                QStringLiteral("Flatpak 用户级权限配置基础命令")),
            loc(QStringLiteral("ينشئ أو يحدّث ملف تجاوزات نصياً خاصاً بتطبيق DeepComplex فقط داخل حسابك الشخصي لتمرير الصلاحيات التي تختارها إلى محرّك العزل (bubblewrap) عند كل تشغيل لاحق."),
                QStringLiteral("Creates or updates a plain-text override file specifically for DeepComplex in your user account so bubblewrap applies your chosen permissions on subsequent launches."),
                QStringLiteral("Создаёт или обновляет текстовый файл переопределений только для DeepComplex в вашем профиле пользователя."),
                QStringLiteral("仅在您的用户目录下为 DeepComplex 创建或更新纯文本权限覆盖文件，供下次启动时由沙盒加载。")),
            loc(QStringLiteral("• flatpak: أداة سطر الأوامر الرسمية لإدارة تطبيقات Flatpak وصناديق العزل.\n"
                               "• override: أمر فرعي لتخصيص صلاحيات تطبيق واحد دون تعديل ملفات التطبيق المثبّتة نفسها؛ يحفظ الإعدادات في الملف النصي:\n  ~/.local/share/flatpak/overrides/org.deepcomplex.App\n"
                               "• --user: يحصر التغيير في حسابك الحالي فقط، لا يمس المستخدمين الآخرين على الجهاز ولا يحتاج لصلاحيات المدير (sudo).\n"
                               "• org.deepcomplex.App: المعرّف الفريد لتطبيق DeepComplex، لضمان عدم تطبيق أي صلاحية على أي تطبيق آخر."),
                QStringLiteral("• flatpak: Official CLI tool managing Flatpak apps and sandboxes.\n"
                               "• override: Subcommand customizing permissions for one app without altering installed app files; stored in plain text at:\n  ~/.local/share/flatpak/overrides/org.deepcomplex.App\n"
                               "• --user: Scopes changes strictly to your personal user account (no sudo required, no system-wide changes).\n"
                               "• org.deepcomplex.App: Unique App ID ensuring no other application receives these flags."),
                QStringLiteral("• flatpak: официальная утилита управления песочницами Flatpak.\n"
                               "• override: сохраняет настройки прав в текстовый файл ~/.local/share/flatpak/overrides/org.deepcomplex.App без изменения самого пакета.\n"
                               "• --user: действует только для текущего пользователя (sudo не нужен).\n"
                               "• org.deepcomplex.App: точный идентификатор приложения DeepComplex."),
                QStringLiteral("• flatpak：官方 Flatpak 管理命令行工具。\n"
                               "• override：在不修改应用本体的前提下覆盖权限，纯文本保存于 ~/.local/share/flatpak/overrides/org.deepcomplex.App。\n"
                               "• --user：仅作用于当前用户账户，无需 sudo。\n"
                               "• org.deepcomplex.App：应用唯一 ID，不会影响其他任何应用。")),
            loc(QStringLiteral("✓ يسمح به: تطبيق الصلاحيات التي حدّدتها أنت فقط عند التشغيل القادم لـ DeepComplex.\n"
                               "✗ يمنعه: لا يمنح الإنترنت (--share=network مغلقة)، لا يمنح الوصول لمجلدك الشخصي (~) أو مستنداتك أو صورك، ولا يؤثر على أي جلسة مفتوحة حالياً قبل إعادة التشغيل."),
                QStringLiteral("✓ Allows: Applying only the flags you explicitly chose on the next launch of DeepComplex.\n"
                               "✗ Remains blocked: No network access (--share=network is off), no access to your Home/Documents/Pictures folders, and no effect on any other app."),
                QStringLiteral("✓ Разрешает: применение выбранных флагов при следующем запуске DeepComplex.\n"
                               "✗ Заблокировано: нет доступа к сети и личным папкам пользователя."),
                QStringLiteral("✓ 允许：下次启动 DeepComplex 时生效您勾选的权限。\n"
                               "✗ 仍禁止：无网络权限，无法访问主目录或个人文件，也不影响其他应用。")),
            loc(QStringLiteral("خطورة منخفضة بحد ذاته: مجرد كتابة ملف نصي قابل للقراءة في مجلد حسابك. الخطورة الحقيقية تعتمد كلياً على الأعلام الممرّرة معه (والمفصّلة أدناه)."),
                QStringLiteral("Low risk by itself: merely writes a human-readable INI file in your home config. Actual security impact depends strictly on the flags attached below."),
                QStringLiteral("Низкий риск сам по себе: записывает читаемый INI-файл. Реальный риск зависит от переданных флагов."),
                QStringLiteral("本身风险低：仅在用户目录写入可读 INI 文件，实际风险取决于附加的权限参数。")),
            loc(QStringLiteral("• للتحقق مما تم حفظه: flatpak override --user --show org.deepcomplex.App\n"
                               "• للتراجع الكامل: flatpak override --user --reset org.deepcomplex.App"),
                QStringLiteral("• Verify saved overrides: flatpak override --user --show org.deepcomplex.App\n"
                               "• Undo completely: flatpak override --user --reset org.deepcomplex.App"),
                QStringLiteral("• Проверить: flatpak override --user --show org.deepcomplex.App\n"
                               "• Отменить всё: flatpak override --user --reset org.deepcomplex.App"),
                QStringLiteral("• 验证已保存的配置：flatpak override --user --show org.deepcomplex.App\n"
                               "• 完全撤销：flatpak override --user --reset org.deepcomplex.App")),
            QStringLiteral("low"), false
        );
    }

    if (includeDevice) {
        addItem(
            QStringLiteral("input-mouse"),
            QStringLiteral("--device=all"),
            loc(QStringLiteral("صلاحية الوصول المباشر لماوس Delux (/dev/hidraw*)"),
                QStringLiteral("Direct Mouse Hardware Access (/dev/hidraw*)"),
                QStringLiteral("Прямой доступ к мыши (/dev/hidraw*)"),
                QStringLiteral("鼠标直接硬件访问权限 (/dev/hidraw*)")),
            loc(QStringLiteral("يربط عقد الأجهزة في /dev داخل الـ Sandbox لكي يرى DeepComplex منفذ /dev/hidraw* الخاص بماوس Delux M700A ويرسل حزم HID Feature Report (بطول 8 بايت) لقراءة وكتابة ذاكرة EEPROM في شريحة Instant A725."),
                QStringLiteral("Bind-mounts /dev device nodes into the sandbox so DeepComplex can locate /dev/hidraw* for the Delux M700A and send 8-byte HID Feature Reports to read/write the Instant A725 EEPROM."),
                QStringLiteral("Монтирует узлы /dev в песочницу, чтобы DeepComplex видел /dev/hidraw* мыши Delux M700A и обменивался пакетами HID Feature Report с памятью Instant A725."),
                QStringLiteral("将 /dev 设备节点挂载进沙盒，使 DeepComplex 能定位 Delux M700A 的 /dev/hidraw* 并通过 8 字节 HID Feature Report 读写 Instant A725 板载 EEPROM。")),
            loc(QStringLiteral("• --device=...: خيار Flatpak الذي يتحكم بالأجهزة المادية المكشوفة داخل مجلد /dev في الحاوية.\n"
                               "• all: يكشف جميع عقد /dev (بما في ذلك /dev/hidraw0, /dev/hidraw1... و/dev/dri للرسوميات).\n"
                               "• لماذا لا نستخدم --device=input؟ لأن خيار input يكشف فقط /dev/input/event* (لأذرع الألعاب) ولا يكشف /dev/hidraw*.\n"
                               "• لماذا لا نستخدم --device=usb؟ لأن خيار usb يكشف /dev/bus/usb فقط، بينما نواة لينكس تحجز الماوس لنظام hidraw (وإجبار فصله عبر libusb قد يوقف المؤشر لحظياً).\n"
                               "• ماذا يفعل كود DeepComplex فعلياً؟ يمسح sysfs بحثاً حصراً عن idVendor=30fa و idProduct=1040 وواجهة HID رقم 1 (Usage Page 0xFF00)، ثم يفتح ذلك الملف الوحيد فقط."),
                QStringLiteral("• --device=...: Flatpak option controlling which hardware nodes are mounted in the sandbox /dev.\n"
                               "• all: Mounts /dev nodes including /dev/hidraw* and GPU (/dev/dri).\n"
                               "• Why not --device=input? That only exposes /dev/input/event* (gamepads) and hides /dev/hidraw*.\n"
                               "• Why not --device=usb? That exposes /dev/bus/usb; detaching the kernel HID driver via libusb interrupts cursor movement, whereas hidraw works safely alongside the kernel driver.\n"
                               "• What DeepComplex code actually opens: Scans sysfs strictly for Vendor 30FA, Product 1040, interface 1 (vendor page), and opens ONLY that single /dev/hidrawX node."),
                QStringLiteral("• --device=all: монтирует узлы /dev, включая /dev/hidraw*.\n"
                               "• Почему не input/usb? --device=input не открывает /dev/hidraw*, а hidraw безопаснее libusb, так как не отключает курсор мыши.\n"
                               "• Что реально открывает DeepComplex: ищет строго USB 30FA:1040 (интерфейс 1) и открывает только этот один узел /dev/hidrawX."),
                QStringLiteral("• --device=all：挂载包括 /dev/hidraw* 在内的 /dev 设备节点。\n"
                               "• 为何不用 input 或 usb？input 不包含 /dev/hidraw*，而 hidraw 无需卸载内核驱动，不会导致光标卡顿。\n"
                               "• DeepComplex 实际行为：严格筛选 USB 30FA:1040 的 vendor 接口，仅打开该鼠标对应的单个 /dev/hidrawX 节点。")),
            loc(QStringLiteral("✓ يسمح به: رؤية منافذ /dev/hidraw* وقراءة/كتابة ذاكرة ماوس Delux M700A (شرط وجود قاعدة udev).\n"
                               "✗ يمنعه: لا يمنح صلاحيات روت (ملفات الأجهزة الأخرى المحمية بـ root مثل الأقراص /dev/sda أو لوحات المفاتيح تظل مرفوضة بواسطة صلاحيات نواة لينكس Linux DAC/uaccess)، ولا يمنح اتصال إنترنت ولا وصولاً لملفاتك الشخصية."),
                QStringLiteral("✓ Allows: Seeing /dev/hidraw* nodes and reading/writing Delux M700A EEPROM (when udev rule allows your user).\n"
                               "✗ Remains blocked: Does NOT bypass Linux kernel permissions—root-owned devices (disks /dev/nvme*, other users' devices, unpermitted hidraw keyboards) still return Permission Denied. Still no network or home-folder access."),
                QStringLiteral("✓ Разрешает: доступ к /dev/hidraw* мыши Delux M700A (при наличии правила udev).\n"
                               "✗ Заблокировано: права ядра Linux сохраняются (диски и чужие устройства недоступны); нет доступа к сети и файлам."),
                QStringLiteral("✓ 允许：访问已由 udev 授权的 Delux M700A 的 /dev/hidraw* 节点。\n"
                               "✗ 仍禁止：无法绕过 Linux 内核权限（磁盘及其他未授权设备仍被拒绝），且无网络和个人文件权限。")),
            loc(QStringLiteral("خطورة متوسطة: تُصنّف متوسطة لأن Flatpak يكشف أجهزة إضافية يملك حسابك صلاحية عليها (مثل الكاميرا أو وحدات التحكم إن كانت موصولة). التخفيف: DeepComplex مفتوح المصدر تماماً، لا يفتح أي جهاز غير 30FA:1040، ومعزول بدون إنترنت فلا يمكنه تسريب أي بيانات."),
                QStringLiteral("Medium risk: Rated medium because /dev/all also exposes other user-accessible devices (such as webcams or gamepads) to the sandbox. Mitigation: DeepComplex is open-source, strictly targets USB 30FA:1040 only, and has zero network access."),
                QStringLiteral("Средний риск: в песочнице становятся видны другие устройства вашего пользователя (веб-камера, геймпад). Смягчение: открытый код, доступ строго к 30FA:1040 и полное отсутствие сети."),
                QStringLiteral("中等风险：沙盒内会同时可见当前用户有权访问的其他设备（如摄像头/手柄）。缓解措施：代码完全开源，仅访问 30FA:1040，且应用无网络权限。")),
            loc(QStringLiteral("• للتحقق داخل التطبيق: افتح نافذة «التحقق من الصلاحيات» لرؤية عدد منافذ /dev/hidraw* المرصودة.\n"
                               "• لإلغاء هذه الصلاحية وحدها: flatpak override --user --nodevice=all org.deepcomplex.App"),
                QStringLiteral("• Verify: Open Verify Permissions to see detected /dev/hidraw* count.\n"
                               "• Undo only this permission: flatpak override --user --nodevice=all org.deepcomplex.App"),
                QStringLiteral("• Проверить: откройте «Проверка разрешений».\n"
                               "• Отменить только это: flatpak override --user --nodevice=all org.deepcomplex.App"),
                QStringLiteral("• 验证：打开“检查应用权限”查看检测到的 /dev/hidraw* 节点数。\n"
                               "• 单独撤销：flatpak override --user --nodevice=all org.deepcomplex.App")),
            QStringLiteral("medium"), false
        );
    }

    if (includeTray) {
        addItem(
            QStringLiteral("preferences-desktop-notification"),
            QStringLiteral("--talk-name=org.kde.StatusNotifierWatcher"),
            loc(QStringLiteral("صلاحية أيقونة شريط المهام (System Tray)"),
                QStringLiteral("System Tray Icon Permission"),
                QStringLiteral("Разрешение значка в системном трее"),
                QStringLiteral("系统托盘图标权限")),
            loc(QStringLiteral("تفتح قناة اتصال محددة عبر Session D-Bus لتسجيل أيقونة DeepComplex وقائمتها المنبثقة (تبديل البروفايلات الـ 5 وإظهار/إخفاء النافذة) في شريط مهام KDE Plasma."),
                QStringLiteral("Opens a targeted Session D-Bus channel so DeepComplex can register its tray icon and quick 5-profile context menu in the KDE Plasma panel."),
                QStringLiteral("Открывает точечный канал Session D-Bus для регистрации значка DeepComplex и меню переключения 5 профилей в трее KDE Plasma."),
                QStringLiteral("开启定向的 Session D-Bus 通道，使 DeepComplex 能在 KDE Plasma 系统托盘显示图标与 5 组配置快捷菜单。")),
            loc(QStringLiteral("• --talk-name=...: يمنح التطبيق حق استدعاء دوال خدمة D-Bus واحدة بالاسم فقط (لا يمنح ملكية الخدمة ولا التجسس على بقية خدمات D-Bus).\n"
                               "• org.kde.StatusNotifierWatcher: الاسم القياسي لخدمة شريط المهام في KDE Plasma (بروتوكول StatusNotifierItem/SNI الحديث المتوافق مع Wayland).\n"
                               "• كيف تعمل مع التطبيق؟ عند إغلاق النافذة مع تفعيل «التصغير لشريط المهام» يظل التطبيق يعمل بخفة في الخلفية وتظهر أيقونته قرب الساعة."),
                QStringLiteral("• --talk-name=...: Grants permission to call methods on a single named Session D-Bus service only (cannot spy on other D-Bus traffic).\n"
                               "• org.kde.StatusNotifierWatcher: Standard Wayland-native tray watcher service in KDE Plasma (StatusNotifierItem/SNI protocol).\n"
                               "• App behavior: Enables the 'Minimize to tray' and 'Start minimized' switches in Settings."),
                QStringLiteral("• --talk-name=...: разрешает обращение строго к одной службе D-Bus по имени.\n"
                               "• org.kde.StatusNotifierWatcher: служба системного трея KDE Plasma (протокол SNI для Wayland)."),
                QStringLiteral("• --talk-name=...：仅允许向指定名称的单个 D-Bus 服务发起调用。\n"
                               "• org.kde.StatusNotifierWatcher：KDE Plasma 原生托盘服务（兼容 Wayland 的 SNI 协议）。")),
            loc(QStringLiteral("✓ يسمح به: عرض أيقونة وتلميح (Tooltip) وقائمة البروفايلات في شريط المهام.\n"
                               "✗ يمنعه: لا يمنح أي وصول لملفاتك، لا يقرأ إشعارات التطبيقات الأخرى، ولا يتحكم بنوافذ النظام."),
                QStringLiteral("✓ Allows: Showing the app icon, tooltip, and profile menu in the system tray.\n"
                               "✗ Remains blocked: Zero access to files, other apps' notifications, clipboard snooping, or system windows."),
                QStringLiteral("✓ Разрешает: отображение значка и меню профилей в трее.\n"
                               "✗ Заблокировано: нет доступа к файлам, уведомлениям и окнам других программ."),
                QStringLiteral("✓ 允许：在托盘显示图标、提示与配置切换菜单。\n"
                               "✗ 仍禁止：无法访问文件、其他应用通知或系统窗口。")),
            loc(QStringLiteral("خطورة منخفضة جداً: من أضيق وأأمن صلاحيات D-Bus في Flatpak، وتُترك معطّلة افتراضياً لزيادة الأمان."),
                QStringLiteral("Very low risk: one of the narrowest, safest D-Bus permissions in Flatpak, kept disabled by default for increased security."),
                QStringLiteral("Очень низкий риск: одно из самых узких и безопасных разрешений D-Bus."),
                QStringLiteral("风险极低：Flatpak 中最安全、权限范围最窄的 D-Bus 权限之一。")),
            loc(QStringLiteral("• للتحقق: افتح الإعدادات وستجد خيارات «التصغير إلى شريط المهام» أصبحت نشطة بعد إعادة التشغيل.\n"
                               "• لإلغائها وحدها: flatpak override --user --no-talk-name=org.kde.StatusNotifierWatcher org.deepcomplex.App"),
                QStringLiteral("• Verify: Restart app and check Settings -> System Tray options become enabled.\n"
                               "• Undo only this permission: flatpak override --user --no-talk-name=org.kde.StatusNotifierWatcher org.deepcomplex.App"),
                QStringLiteral("• Проверить: перезапустите приложение и проверьте настройки трея.\n"
                               "• Отменить: flatpak override --user --no-talk-name=org.kde.StatusNotifierWatcher org.deepcomplex.App"),
                QStringLiteral("• 验证：重启应用后在设置中查看托盘选项已解锁。\n"
                               "• 单独撤销：flatpak override --user --no-talk-name=org.kde.StatusNotifierWatcher org.deepcomplex.App")),
            QStringLiteral("low"), true
        );
    }

    if (includeHost) {
        addItem(
            QStringLiteral("dialog-warning"),
            QStringLiteral("--talk-name=org.freedesktop.Flatpak"),
            loc(QStringLiteral("صلاحية النظام المضيف الاختيارية (Opt-in)"),
                QStringLiteral("Optional Host System Bridge (Opt-in)"),
                QStringLiteral("Опциональный доступ к хосту (Opt-in)"),
                QStringLiteral("可选宿主机通信权限 (Opt-in)")),
            loc(QStringLiteral("تفتح واجهة Flatpak على النظام المضيف لكي يتمكن DeepComplex من استعلام التطبيقات المثبّتة والجارية وتبديل بروفايل الماوس تلقائياً عند فتح لعبة أو برنامج مرتبط ببروفايل معيّن."),
                QStringLiteral("Opens the host Flatpak D-Bus interface so DeepComplex can query installed/running apps and auto-switch mouse profiles when a linked app or game starts."),
                QStringLiteral("Открывает D-Bus интерфейс Flatpak на хосте для чтения списка установленных и запущенных приложений с целью автопереключения профилей."),
                QStringLiteral("开放宿主机 Flatpak D-Bus 接口，使 DeepComplex 能够查询已安装/正在运行的应用并在启动关联游戏时自动切换鼠标配置。")),
            loc(QStringLiteral("• --talk-name=org.freedesktop.Flatpak: تمنح إذن مخاطبة خدمة org.freedesktop.Flatpak.Development على Session Bus.\n"
                               "• الأداة المرتبطة (flatpak-spawn --host): تسمح للبرنامج بتشغيل أوامر على النظام المضيف خارج الـ Sandbox بصلاحيات حسابك العادي (بدون sudo).\n"
                               "• ما هي الأوامر التي يشغّلها DeepComplex فعلياً بها؟\n"
                               "  1) flatpak-spawn --host true (فقط للتحقق من تفعيل الصلاحية).\n"
                               "  2) flatpak-spawn --host flatpak list --app (لقائمة تطبيقات Flatpak لربطها بالبروفايلات).\n"
                               "  3) flatpak-spawn --host sh -c 'flatpak ps ...; ps -eo comm=,args=' (لاكتشاف تشغيل التطبيق المرتبط كل 2.5 ثانية فقط عند تفعيل ميزة التبديل التلقائي).\n"
                               "  4) فحص وجود الملف /etc/udev/rules.d/70-delux-m700a.rules."),
                QStringLiteral("• --talk-name=org.freedesktop.Flatpak: Permits talking to org.freedesktop.Flatpak.Development on Session Bus.\n"
                               "• Underlying mechanism (flatpak-spawn --host): Allows launching host commands outside the sandbox under your regular user privileges.\n"
                               "• Exact commands DeepComplex runs when enabled:\n"
                               "  1) flatpak-spawn --host true (startup verification).\n"
                               "  2) flatpak-spawn --host flatpak list --app (populate installed Flatpak app picker).\n"
                               "  3) flatpak-spawn --host sh -c 'flatpak ps ...; ps -eo comm=,args=' (only when Auto-Switch by App is enabled, to detect linked apps).\n"
                               "  4) Checking if /etc/udev/rules.d/70-delux-m700a.rules exists."),
                QStringLiteral("• --talk-name=org.freedesktop.Flatpak: открывает доступ к flatpak-spawn --host.\n"
                               "• Команды, используемые DeepComplex: проверка true, flatpak list --app, flatpak ps / ps (только при включённой автосмене профилей) и проверка наличия файла udev."),
                QStringLiteral("• --talk-name=org.freedesktop.Flatpak：允许调用 flatpak-spawn --host。\n"
                               "• DeepComplex 实际运行的命令：启动检测 true、flatpak list --app、flatpak ps / ps（仅在开启按应用自动切换时），以及检查 udev 规则文件是否存在。")),
            loc(QStringLiteral("✓ يسمح به: قراءة العمليات والتطبيقات الجارية على المضيف وتبديل البروفايلات تلقائياً.\n"
                               "✗ ملاحظة مهمة: من الناحية الأمنية التقنية، أي تطبيق يملك هذه الصلاحية يستطيع نظرياً تنفيذ أوامر كالمستخدم العادي خارج العزل (أي أنه يعادل تشغيل تطبيق لينكس تقليدي غير معزول بـ Flatpak)."),
                QStringLiteral("✓ Allows: Reading running host processes/Flatpaks for automatic profile switching.\n"
                               "✗ Critical boundary: Technically, any app holding this permission can run arbitrary commands as your regular user outside the sandbox (equivalent to a standard non-sandboxed Linux binary)."),
                QStringLiteral("✓ Разрешает: мониторинг запущенных приложений для автосмены профилей.\n"
                               "✗ Важно: технически снимает изоляцию песочницы на уровне вашего пользователя."),
                QStringLiteral("✓ 允许：检测宿主机运行的程序以自动切换配置。\n"
                               "✗ 重要边界：从技术角度看，拥有此权限的应用理论上可在沙盒外以普通用户身份执行命令（等同于传统非沙盒程序）。")),
            loc(QStringLiteral("خطورة مرتفعة: تُعدّ من أخطر صلاحيات Flatpak لأنها تكسر عزل الـ Sandbox، لذلك تركها DeepComplex معطّلة افتراضياً لزيادة الأمان؛ لا تفعّلها إلا إذا كنت تحتاج فعلاً ميزة التبديل التلقائي للبروفايل حسب اللعبة/التطبيق."),
                QStringLiteral("High risk: Considered a full sandbox-escape permission, so DeepComplex keeps it disabled by default for increased security—enable it ONLY if you actively want app/game-based automatic profile switching."),
                QStringLiteral("Высокий риск: снимает изоляцию Flatpak. Отключено по умолчанию — включайте только если действительно нужна автосмена профилей по приложениям."),
                QStringLiteral("高风险：属于突破沙盒隔离的高危权限，为提高安全性 DeepComplex 默认关闭，仅在确实需要按应用自动切换配置时才启用。")),
            loc(QStringLiteral("• للتحقق: افتح «التحقق من الصلاحيات» بعد إعادة التشغيل (ستتحول الحالة إلى مفعّلة).\n"
                               "• لإلغائها فوراً والعودة للعزل: flatpak override --user --no-talk-name=org.freedesktop.Flatpak org.deepcomplex.App"),
                QStringLiteral("• Verify: Restart and check Verify Permissions (status becomes Enabled).\n"
                               "• Undo immediately: flatpak override --user --no-talk-name=org.freedesktop.Flatpak org.deepcomplex.App"),
                QStringLiteral("• Проверить: перезапустите и откройте «Проверка разрешений».\n"
                               "• Отменить: flatpak override --user --no-talk-name=org.freedesktop.Flatpak org.deepcomplex.App"),
                QStringLiteral("• 验证：重启后在“检查应用权限”中确认已启用。\n"
                               "• 立即撤销：flatpak override --user --no-talk-name=org.freedesktop.Flatpak org.deepcomplex.App")),
            QStringLiteral("high"), true
        );
    }

    if (includeUdev) {
        addItem(
            QStringLiteral("dialog-password"),
            QStringLiteral("echo 'SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"30fa\", ATTRS{idProduct}==\"1040\", MODE=\"0660\", TAG+=\"uaccess\"' | sudo tee /etc/udev/rules.d/70-delux-m700a.rules && sudo rm -f /etc/udev/rules.d/99-delux-m700a.rules"),
            loc(QStringLiteral("إنشاء قاعدة صلاحيات النظام الآمنة للماوس (udev)"),
                QStringLiteral("Create Secure System udev Rule for Delux M700A"),
                QStringLiteral("Создание безопасного правила udev для Delux M700A"),
                QStringLiteral("创建 Delux M700A 安全系统 udev 规则")),
            loc(QStringLiteral("يكتب ملف قاعدة من سطر واحد في /etc/udev/rules.d/ لتعليم نواة لينكس أن ماوس Delux M700A (30FA:1040) يُسمح للمستخدم الجالس أمام الجهاز حالياً بالقراءة والكتابة عليه دون تشغيل التطبيق بصلاحيات روت."),
                QStringLiteral("Writes a single-line rule into /etc/udev/rules.d/ instructing Linux udev to grant the physically logged-in desktop user read/write access to USB 30FA:1040 without running the app as root."),
                QStringLiteral("Записывает однострочное правило в /etc/udev/rules.d/, разрешая текущему локальному пользователю чтение/запись USB 30FA:1040 без запуска приложения от root."),
                QStringLiteral("在 /etc/udev/rules.d/ 写入单行规则，指示 Linux udev 仅向当前本地登录用户授予 USB 30FA:1040 的免 root 读写权限。")),
            loc(QStringLiteral("• echo '...': يطبع نص القاعدة كما هو بين علامتي الاقتباس المفردة.\n"
                               "• SUBSYSTEM==\"hidraw\": شرط يطابق أجهزة الواجهة البشرية الخام hidraw فقط.\n"
                               "• ATTRS{idVendor}==\"30fa\": شرط يطابق معرّف الشركة المصنّعة للشريحة (Instant Microelectronics = 30FA).\n"
                               "• ATTRS{idProduct}==\"1040\": شرط يطابق معرّف طراز ماوس Delux M700A (1040) حصراً.\n"
                               "• MODE=\"0660\": صلاحيات القراءة والكتابة للمالك والمجموعة فقط (rw-rw----)، ونمنع تماماً 0666 غير الآمن.\n"
                               "• TAG+=\"uaccess\": وسم systemd-logind القياسي الذي يضيف ACL مؤقتاً للمستخدم المسجّل دخوله محلياً فقط.\n"
                               "• | sudo tee /etc/udev/rules.d/70-delux-m700a.rules: يمرّر السطر عبر الأنبوب (|) لكتابته في المسار المحمي بصلاحيات المدير؛ الرقم 70 مهم جداً ليعمل قبل قاعدة النظام 73-seat-late.rules التي تنفّذ وسم uaccess.\n"
                               "• && sudo rm -f /etc/udev/rules.d/99-delux-m700a.rules: يحذف أي ملف قاعدة قديم يحمل الرقم 99 إن كان موجوداً سابقاً."),
                QStringLiteral("• echo '...': Outputs the exact literal rule string.\n"
                               "• SUBSYSTEM==\"hidraw\": Matches raw HID interfaces only.\n"
                               "• ATTRS{idVendor}==\"30fa\": Matches Instant Microelectronics USB Vendor ID strictly.\n"
                               "• ATTRS{idProduct}==\"1040\": Matches Delux M700A USB Product ID strictly.\n"
                               "• MODE=\"0660\": Sets owner/group read-write (rw-rw----), avoiding world-writable 0666.\n"
                               "• TAG+=\"uaccess\": Instructs systemd-logind to grant a dynamic ACL solely to the active local console user.\n"
                               "• | sudo tee /etc/udev/rules.d/70-delux-m700a.rules: Pipes the text into the root-owned rules folder with priority 70 (crucial so it runs BEFORE 73-seat-late.rules processes uaccess).\n"
                               "• && sudo rm -f .../99-delux-m700a.rules: Cleans up any legacy priority-99 rule file if present."),
                QStringLiteral("• SUBSYSTEM==\"hidraw\" + idVendor==30fa + idProduct==1040: строгое совпадение только с мышью Delux M700A.\n"
                               "• MODE=\"0660\", TAG+=\"uaccess\": безопасные права только для активного локального пользователя (не 0666).\n"
                               "• Приоритет 70: выполняется до системного правила 73-seat-late.rules.\n"
                               "• rm -f 99-delux-m700a.rules: удаляет устаревшее правило, если оно было."),
                QStringLiteral("• SUBSYSTEM==\"hidraw\" + idVendor==30fa + idProduct==1040：严格仅匹配 Delux M700A 鼠标。\n"
                               "• MODE=\"0660\", TAG+=\"uaccess\"：通过 systemd-logind 仅向当前本地登录用户授予动态访问权限（绝不使用不安全的 0666）。\n"
                               "• 优先级 70：确保在系统 73-seat-late.rules 之前执行。\n"
                               "• rm -f 99-delux-m700a.rules：清理可能残留的旧版规则文件。")),
            loc(QStringLiteral("✓ يسمح به: فتح ملف /dev/hidrawX الخاص بماوس Delux M700A (30FA:1040) من حسابك المحلي فقط.\n"
                               "✗ يمنعه: لا يمس أي جهاز USB آخر (لوحة مفاتيح، فلاشة، سماعة، ماوس آخر)، ولا يسمح لمستخدمين عن بُعد (SSH) بالوصول للماوس، ولا يُبقي صلاحيات روت للتطبيق."),
                QStringLiteral("✓ Allows: Opening ONLY the Delux M700A (30FA:1040) /dev/hidrawX node from your local desktop session.\n"
                               "✗ Remains blocked: Zero effect on keyboards, USB drives, webcams, other mice, or remote SSH users. App still runs completely unprivileged."),
                QStringLiteral("✓ Разрешает: доступ только к /dev/hidrawX мыши 30FA:1040 из локальной сессии.\n"
                               "✗ Заблокировано: другие USB-устройства и удалённые пользователи не затрагиваются."),
                QStringLiteral("✓ 允许：仅本地桌面会话访问 30FA:1040 鼠标对应的 /dev/hidrawX。\n"
                               "✗ 仍禁止：不影响键盘、U 盘等任何其他设备，也不向远程 SSH 用户开放。")),
            loc(QStringLiteral("خطورة منخفضة: صحيح أنه يطلب كلمة مرور sudo لمرة واحدة عند التثبيت، لكن الملف الناتج نصي ثابت من سطر واحد ومحكم بأفضل ممارسات أمان لينكس (uaccess + 0660 + Vendor/Product محددين)."),
                QStringLiteral("Low risk: Requires sudo once solely to write into /etc/udev/rules.d/, but the resulting 1-line rule follows strict Linux security best practices (uaccess + 0660 + exact VID:PID)."),
                QStringLiteral("Низкий риск: sudo нужен один раз для записи в /etc/udev/rules.d/, но правило соответствует строгим стандартам безопасности Linux (uaccess + 0660)."),
                QStringLiteral("低风险：仅安装写入 /etc/udev/rules.d/ 时需一次 sudo，规则本身严格遵循 Linux 安全最佳实践（uaccess + 0660 + 精确 VID:PID）。")),
            loc(QStringLiteral("• للتحقق من محتوى الملف: cat /etc/udev/rules.d/70-delux-m700a.rules\n"
                               "• للتحقق من الصلاحية المطبّقة على المنفذ: getfacl /dev/hidraw*\n"
                               "• لإزالة القاعدة نهائياً: sudo rm -f /etc/udev/rules.d/70-delux-m700a.rules && sudo udevadm control --reload-rules && sudo udevadm trigger"),
                QStringLiteral("• Verify file contents: cat /etc/udev/rules.d/70-delux-m700a.rules\n"
                               "• Verify applied ACL on device: getfacl /dev/hidraw*\n"
                               "• Remove completely: sudo rm -f /etc/udev/rules.d/70-delux-m700a.rules && sudo udevadm control --reload-rules && sudo udevadm trigger"),
                QStringLiteral("• Проверить файл: cat /etc/udev/rules.d/70-delux-m700a.rules\n"
                               "• Удалить: sudo rm -f /etc/udev/rules.d/70-delux-m700a.rules && sudo udevadm control --reload-rules && sudo udevadm trigger"),
                QStringLiteral("• 验证文件内容：cat /etc/udev/rules.d/70-delux-m700a.rules\n"
                               "• 彻底移除：sudo rm -f /etc/udev/rules.d/70-delux-m700a.rules && sudo udevadm control --reload-rules && sudo udevadm trigger")),
            QStringLiteral("low"), false
        );

        addItem(
            QStringLiteral("view-refresh"),
            QStringLiteral("sudo udevadm control --reload-rules && sudo udevadm trigger"),
            loc(QStringLiteral("تفعيل قاعدة udev فوراً بدون إعادة تشغيل الجهاز"),
                QStringLiteral("Reload & Apply udev Rules Immediately"),
                QStringLiteral("Применение правил udev без перезагрузки"),
                QStringLiteral("立即重载并应用 udev 规则（无需重启电脑）")),
            loc(QStringLiteral("يُحدّث خدمة إدارة الأجهزة في نواة لينكس (systemd-udevd) لتقرأ الملف الجديد وتطبّق الصلاحية فوراً على الماوس المتصل حالياً دون الحاجة لإعادة تشغيل الحاسوب."),
                QStringLiteral("Notifies systemd-udevd to reload rules from disk and immediately re-evaluate connected devices so the mouse gets access without rebooting."),
                QStringLiteral("Обновляет правила systemd-udevd и сразу применяет их к подключённой мыши без перезагрузки ПК."),
                QStringLiteral("通知 systemd-udevd 重新加载规则并立即应用到已连接的鼠标，无需重启电脑。")),
            loc(QStringLiteral("• udevadm: الأداة الرسمية للتحكم في خدمة الأجهزة systemd-udevd.\n"
                               "• control --reload-rules: يأمر الخدمة بقراءة قواعد /etc/udev/rules.d/ المحدّثة في الذاكرة.\n"
                               "• &&: عامل ربط شرطي في الطرفية يعني (لا تنفّذ الأمر التالي إلا إذا نجح الأمر السابق برمز خروج 0).\n"
                               "• udevadm trigger: يرسل إشارة للنواة لإعادة تطبيق القواعد على الأجهزة المتصلة حالياً (بدلاً من انتظار إعادة التشغيل)."),
                QStringLiteral("• udevadm: Official Linux tool controlling the systemd-udevd daemon.\n"
                               "• control --reload-rules: Reloads updated rule files from /etc/udev/rules.d/ into memory.\n"
                               "• &&: Shell logical AND—only runs the second command if the first succeeds.\n"
                               "• udevadm trigger: Replays uevents for currently connected hardware so the new rule applies immediately."),
                QStringLiteral("• udevadm control --reload-rules: перечитывает правила с диска.\n"
                               "• &&: выполняет вторую команду только при успехе первой.\n"
                               "• udevadm trigger: немедленно применяет правила к подключённым устройствам."),
                QStringLiteral("• udevadm control --reload-rules：从磁盘重新加载规则。\n"
                               "• &&：仅在前一条命令成功时执行下一条。\n"
                               "• udevadm trigger：立即对当前已连接的设备重新触发规则应用。")),
            loc(QStringLiteral("✓ يسمح به: تحديث صلاحيات أجهزة USB المتصلة لتتطابق مع قواعد النظام الحالية.\n"
                               "✗ يمنعه: لا يعدّل أي ملفات ولا يثبّت أي برامج ولا يقطع اتصالك بالشبكة."),
                QStringLiteral("✓ Allows: Refreshing kernel device permissions to match current rules.\n"
                               "✗ Remains blocked: Modifies no files, installs nothing, and makes no persistent changes beyond applying rules."),
                QStringLiteral("✓ Разрешает: немедленное обновление прав устройств.\n"
                               "✗ Заблокировано: не меняет файлы и не устанавливает ПО."),
                QStringLiteral("✓ 允许：刷新已连接硬件的设备节点权限。\n"
                               "✗ 仍禁止：不修改任何文件，不安装任何软件。")),
            loc(QStringLiteral("خطورة منخفضة: أمر نظام لينكس قياسي وروتيني وآمن تماماً. إذا لم يتحدث المنفذ فوراً افصل سلك الماوس وأعد توصيله لثانية واحدة."),
                QStringLiteral("Low risk: Standard, routine, completely safe Linux administration command. If access does not update immediately, unplug and replug the mouse once."),
                QStringLiteral("Низкий риск: стандартная безопасная системная команда Linux."),
                QStringLiteral("低风险：标准且完全安全的 Linux 系统管理命令。若未立即生效，重新插拔一次鼠标即可。")),
            loc(QStringLiteral("• للتحقق: اضغط «إعادة الفحص» في نافذة الاستقبال أو «التحقق من الصلاحيات الآن».\n"
                               "• لا يحتاج لتراجع مستقل لأنه يطبّق القواعد الموجودة حالياً فقط."),
                QStringLiteral("• Verify: Click Recheck in Onboarding or Verify Permissions Now.\n"
                               "• No separate undo needed (it simply applies whatever rules currently exist)."),
                QStringLiteral("• Проверить: нажмите «Проверить» в окне приложения."),
                QStringLiteral("• 验证：在应用内点击“重新检测”或“立即检查权限”。")),
            QStringLiteral("low"), false
        );
    }

    if (includeReset) {
        addItem(
            QStringLiteral("edit-undo"),
            permissionsResetCommand(),
            loc(QStringLiteral("أمر التراجع وإلغاء جميع الصلاحيات الممنوحة (Reset)"),
                QStringLiteral("Revoke & Reset All Flatpak Overrides"),
                QStringLiteral("Сброс всех выданных разрешений Flatpak"),
                QStringLiteral("撤销并重置所有 Flatpak 权限")),
            loc(QStringLiteral("يحذف كل التجاوزات التي أضفتها عبر flatpak override ويعيد تطبيق DeepComplex إلى وضع العزل الأقصى الافتراضي (صلاحيات العرض فقط)."),
                QStringLiteral("Deletes all user flatpak overrides and restores DeepComplex to its strictest default isolated sandbox (display sockets only)."),
                QStringLiteral("Удаляет все пользовательские переопределения и возвращает строгую изоляцию по умолчанию (только отображение окна)."),
                QStringLiteral("删除所有用户级 flatpak override，将 DeepComplex 恢复至最严格的默认隔离状态（仅图形显示权限）。")),
            loc(QStringLiteral("• flatpak override --user: يستهدف ملف التجاوزات الخاص بحسابك فقط.\n"
                               "• --reset: يمسح الملف ~/.local/share/flatpak/overrides/org.deepcomplex.App بالكامل.\n"
                               "• org.deepcomplex.App: يحصر الحذف في DeepComplex وحده دون المساس بتجاوزات أي تطبيق Flatpak آخر."),
                QStringLiteral("• flatpak override --user: Targets your personal override store only.\n"
                               "• --reset: Deletes ~/.local/share/flatpak/overrides/org.deepcomplex.App completely.\n"
                               "• org.deepcomplex.App: Affects DeepComplex only; other apps' overrides remain untouched."),
                QStringLiteral("• --reset: полностью удаляет файл ~/.local/share/flatpak/overrides/org.deepcomplex.App, не затрагивая другие приложения."),
                QStringLiteral("• --reset：彻底删除 ~/.local/share/flatpak/overrides/org.deepcomplex.App，不影响其他应用。")),
            loc(QStringLiteral("✓ يسمح به: إلغاء فوري لـ --device=all و StatusNotifierWatcher و org.freedesktop.Flatpak بعد إعادة التشغيل.\n"
                               "✗ ملاحظة: لا يحذف بروفايلاتك المحفوظة في ~/.var/app/org.deepcomplex.App ولا يحذف قاعدة udev من النظام."),
                QStringLiteral("✓ Allows: Cleanly revoking --device=all, StatusNotifierWatcher, and org.freedesktop.Flatpak on next launch.\n"
                               "✗ Note: Does NOT delete your saved mouse profiles/macros in ~/.var/app/org.deepcomplex.App or remove /etc/udev/rules.d/70-delux-m700a.rules."),
                QStringLiteral("✓ Отзывает все выданные права Flatpak.\n"
                               "✗ Ваши сохранённые профили и макросы при этом не удаляются."),
                QStringLiteral("✓ 撤销所有额外 Flatpak 权限。\n"
                               "✗ 不会删除您保存的鼠标配置文件与宏，也不会删除系统 udev 规则。")),
            loc(QStringLiteral("خطورة منخفضة (أمر تعزيز أمان): يقلّص صلاحيات التطبيق ويزيد العزل إلى الحد الأقصى."),
                QStringLiteral("Low risk (security hardening): reduces app privileges back to maximum sandbox isolation."),
                QStringLiteral("Низкий риск (усиление безопасности): возвращает максимальную изоляцию."),
                QStringLiteral("低风险（安全加固命令）：将应用权限收紧回最高沙盒隔离级别。")),
            loc(QStringLiteral("• للتحقق: نفّذ flatpak override --user --show org.deepcomplex.App (سيظهر الناتج فارغاً)."),
                QStringLiteral("• Verify: Run flatpak override --user --show org.deepcomplex.App (output will be empty)."),
                QStringLiteral("• Проверить: flatpak override --user --show org.deepcomplex.App (пустой вывод)."),
                QStringLiteral("• 验证：运行 flatpak override --user --show org.deepcomplex.App（输出应为空）。")),
            QStringLiteral("low"), true
        );
    }

    return list;
}

void SandboxManager::updateUdevHealthState(bool healthy, const QString &reason) {
    const bool changed = (m_udevHealthy != healthy) || (m_udevErrorReason != reason);
    m_udevHealthy = healthy;
    m_udevErrorReason = reason;
    if (healthy) m_udevAlertEmitted = false;
    if (changed) emit udevStatusChanged();
    if (!healthy && !m_udevAlertEmitted) {
        m_udevAlertEmitted = true;
        emit udevMissingDetected();
    }
}

void SandboxManager::markUdevUnhealthy(const QString &reason) {
    const bool changed = m_udevHealthy || (m_udevErrorReason != reason);
    m_udevHealthy = false;
    m_udevErrorReason = reason;
    if (changed) emit udevStatusChanged();
}

void SandboxManager::initDefaultActivePermissions() {
    // Maximum isolation defaults matching org.deepcomplex.App.json:
    // Only display sockets & IPC enabled by default; device/tray/host are disabled until opted-in.
    m_activePerms.clear();
    m_activePerms[QStringLiteral("device_all")]     = false;
    m_activePerms[QStringLiteral("tray_dbus")]      = false;
    m_activePerms[QStringLiteral("host_spawn")]     = false;
    m_activePerms[QStringLiteral("socket_wayland")] = true;
    m_activePerms[QStringLiteral("socket_x11")]     = true;
    m_activePerms[QStringLiteral("share_ipc")]      = true;
    m_activePerms[QStringLiteral("device_dri")]     = true;
}

void SandboxManager::parseIniActivePerms(const QString &iniText) {
    QString currentSection;
    const QStringList lines = iniText.split(QLatin1Char('\n'));

    auto setVal = [&](const QString &key, bool val) {
        if (m_activePerms.contains(key)) m_activePerms[key] = val;
    };

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';'))) continue;
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            currentSection = line.mid(1, line.size() - 2).trimmed();
            continue;
        }
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0) continue;
        const QString k = line.left(eq).trimmed();
        const QString v = line.mid(eq + 1).trimmed();

        if (currentSection == QStringLiteral("Context")) {
            const QStringList tokens = v.split(QLatin1Char(';'), Qt::SkipEmptyParts);
            if (k == QStringLiteral("devices")) {
                setVal(QStringLiteral("device_all"), false);
                setVal(QStringLiteral("device_dri"), false);
                for (const QString &tok : tokens) {
                    const QString t = tok.trimmed();
                    if (t == QStringLiteral("!all")) setVal(QStringLiteral("device_all"), false);
                    else if (t == QStringLiteral("all")) {
                        setVal(QStringLiteral("device_all"), true);
                        setVal(QStringLiteral("device_dri"), true);
                    } else if (t == QStringLiteral("!dri")) setVal(QStringLiteral("device_dri"), false);
                    else if (t == QStringLiteral("dri")) setVal(QStringLiteral("device_dri"), true);
                }
            } else if (k == QStringLiteral("shared")) {
                setVal(QStringLiteral("share_ipc"), false);
                for (const QString &tok : tokens) {
                    const QString t = tok.trimmed();
                    if (t == QStringLiteral("!ipc")) setVal(QStringLiteral("share_ipc"), false);
                    else if (t == QStringLiteral("ipc")) setVal(QStringLiteral("share_ipc"), true);
                }
            } else if (k == QStringLiteral("sockets")) {
                setVal(QStringLiteral("socket_wayland"), false);
                setVal(QStringLiteral("socket_x11"), false);
                for (const QString &tok : tokens) {
                    const QString t = tok.trimmed();
                    if (t == QStringLiteral("!wayland")) setVal(QStringLiteral("socket_wayland"), false);
                    else if (t == QStringLiteral("wayland")) setVal(QStringLiteral("socket_wayland"), true);
                    else if (t == QStringLiteral("!fallback-x11") || t == QStringLiteral("!x11")) setVal(QStringLiteral("socket_x11"), false);
                    else if (t == QStringLiteral("fallback-x11") || t == QStringLiteral("x11")) setVal(QStringLiteral("socket_x11"), true);
                }
            }
        } else if (currentSection == QStringLiteral("Session Bus Policy")) {
            if (k == QStringLiteral("org.freedesktop.Flatpak")) setVal(QStringLiteral("host_spawn"), v != QStringLiteral("none"));
            else if (k == QStringLiteral("org.kde.StatusNotifierWatcher")) setVal(QStringLiteral("tray_dbus"), v != QStringLiteral("none"));
        }
    }

    const bool waylandActive = m_activePerms.value(QStringLiteral("socket_wayland")) &&
                               QGuiApplication::platformName().startsWith(QStringLiteral("wayland"), Qt::CaseInsensitive);
    if (waylandActive && !m_activePerms.value(QStringLiteral("socket_x11"))) {
        m_activePerms[QStringLiteral("socket_x11")] = true;
    }
}

void SandboxManager::readSandboxInfo() {
    initDefaultActivePermissions();
    if (!isRunningInFlatpak()) {
        m_activePerms[QStringLiteral("device_all")] = true;
        m_activePerms[QStringLiteral("tray_dbus")]  = true;
        m_activePerms[QStringLiteral("host_spawn")] = true;
        m_activePerms[QStringLiteral("device_dri")] = true;
        return;
    }
    QFile infoFile(QStringLiteral("/.flatpak-info"));
    if (infoFile.open(QIODevice::ReadOnly)) {
        const QString infoText = QString::fromUtf8(infoFile.read(256 * 1024));
        infoFile.close();
        for (auto it = m_activePerms.begin(); it != m_activePerms.end(); ++it) {
            it.value() = false;
        }
        parseIniActivePerms(infoText);
    }
}

void SandboxManager::runLiveLocalChecks() {
    const QDir devDir(QStringLiteral("/dev"));
    const QStringList hidNodes = devDir.entryList({QStringLiteral("hidraw*")}, QDir::System | QDir::Files);
    m_liveHidrawNodeCount = static_cast<int>(hidNodes.size());

    auto path = M700ADriver::findDevicePath();
    m_liveMousePresent = path.has_value();
    if (path.has_value()) {
        m_liveMousePath = QString::fromStdString(*path);
        m_liveMouseRwOk = (::access(path->c_str(), R_OK | W_OK) == 0);
    } else {
        m_liveMousePath.clear();
        m_liveMouseRwOk = false;
    }

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected() && bus.interface()) {
        m_liveBgPortalOk = bus.interface()->isServiceRegistered(QStringLiteral("org.freedesktop.portal.Desktop")).value();
        m_liveTrayDbusOk = bus.interface()->isServiceRegistered(QStringLiteral("org.kde.StatusNotifierWatcher")).value();
    } else {
        m_liveBgPortalOk = false;
        m_liveTrayDbusOk = false;
    }

    m_lastCheckedTime = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
}

void SandboxManager::probeHostAccessOnStartup() {
    runLiveLocalChecks();
    readSandboxInfo();

    if (!isRunningInFlatpak()) {
        m_hostAccessAvailable = true;
        return;
    }

    if (!m_activePerms.value(QStringLiteral("host_spawn"))) {
        m_hostAccessAvailable = false;
        return;
    }

    m_hostAccessAvailable = true;
    runSafeAsyncProcess(
        this,
        QStringLiteral("flatpak-spawn"),
        {QStringLiteral("--host"), QStringLiteral("true")},
        5000,
        [this](QProcess *, int exitCode, QProcess::ExitStatus exitStatus) {
            const bool verified = (exitStatus == QProcess::NormalExit && exitCode == 0);
            m_hostAccessAvailable = verified;
            m_activePerms[QStringLiteral("host_spawn")] = verified;
            if (verified) checkUdevHealth();
            emit hostAccessChanged();
            emit appPermissionsChanged();
        },
        [this]() {
            m_hostAccessAvailable = false;
            m_activePerms[QStringLiteral("host_spawn")] = false;
            emit hostAccessChanged();
            emit appPermissionsChanged();
        }
    );
}

QString SandboxManager::permissionsSummary() const {
    const QVariantList perms = appPermissions();
    int activeCount = 0;
    for (const QVariant &v : perms) {
        if (v.toMap().value(QStringLiteral("state")).toString() == QStringLiteral("ok")) {
            ++activeCount;
        }
    }
    const QString modeText = isRunningInFlatpak()
        ? (m_hostAccessAvailable
            ? loc(QStringLiteral("وضع متقدم (صلاحية المضيف مفعّلة)"),
                  QStringLiteral("Extended Mode (Host Opt-in Active)"),
                  QStringLiteral("Расширенный режим (доступ к хосту)"),
                  QStringLiteral("扩展模式（已启用宿主机权限）"))
            : (deviceAccessAvailable()
                ? loc(QStringLiteral("الوضع الآمن (صلاحية الماوس مفعّلة)"),
                      QStringLiteral("Secure Mode (Mouse Access Enabled)"),
                      QStringLiteral("Безопасный режим (доступ к мыши включён)"),
                      QStringLiteral("安全模式（鼠标访问已启用）"))
                : loc(QStringLiteral("وضع العزل الأقصى (الصلاحيات معطّلة افتراضياً)"),
                      QStringLiteral("Strict Isolation Mode (Permissions Disabled by Default)"),
                      QStringLiteral("Строгая изоляция (права отключены по умолчанию)"),
                      QStringLiteral("严格隔离模式（权限默认关闭）"))))
        : loc(QStringLiteral("تشغيل أصلي (خارج Flatpak)"),
              QStringLiteral("Native Host Session"),
              QStringLiteral("Нативный запуск"),
              QStringLiteral("原生系统运行"));

    const QString stamp = m_lastCheckedTime.isEmpty() ? QStringLiteral("--:--:--") : m_lastCheckedTime;
    return loc(
        QStringLiteral("%1 • %2/%3 مفعّلة • آخر تحقق: %4").arg(modeText).arg(activeCount).arg(perms.size()).arg(stamp),
        QStringLiteral("%1 • %2/%3 verified • Last check: %4").arg(modeText).arg(activeCount).arg(perms.size()).arg(stamp),
        QStringLiteral("%1 • %2/%3 активно • Проверка: %4").arg(modeText).arg(activeCount).arg(perms.size()).arg(stamp),
        QStringLiteral("%1 • %2/%3 项正常 • 上次检查：%4").arg(modeText).arg(activeCount).arg(perms.size()).arg(stamp)
    );
}

QVariantList SandboxManager::appPermissions() const {
    QVariantList list;
    const QString platform = QGuiApplication::platformName();

    auto addPerm = [&](const QString &id, const QString &icon, const QString &flag,
                       const QString &title, const QString &desc, const QString &checkNote,
                       const QString &riskLevel, const QString &riskNote,
                       const QString &stateStr, const QString &enableCmd) {
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        m[QStringLiteral("icon")] = icon;
        m[QStringLiteral("flag")] = flag;
        m[QStringLiteral("title")] = title;
        m[QStringLiteral("description")] = desc;
        m[QStringLiteral("checkNote")] = checkNote;
        m[QStringLiteral("riskLevel")] = riskLevel; // "low", "medium", "high"
        m[QStringLiteral("riskNote")] = riskNote;
        m[QStringLiteral("state")] = stateStr;
        m[QStringLiteral("enableCommand")] = enableCmd;
        list.append(m);
    };

    auto flatpakEntry = [&](const QString &id, const QString &icon, const QString &flag,
                            const QString &title, const QString &desc, const QString &checkNote,
                            const QString &riskLevel, const QString &riskNote,
                            const QString &enableCmd) {
        const bool ok = m_activePerms.value(id, false);
        addPerm(id, icon, flag, title, desc, checkNote, riskLevel, riskNote, ok ? QStringLiteral("ok") : QStringLiteral("off"), enableCmd);
    };

    // 1. Direct Mouse Hardware Access (--device=all) — Medium risk
    const bool devGranted = deviceAccessAvailable();
    const QString devNote = !devGranted
        ? loc(QStringLiteral("التحقق الحي: معطّلة افتراضياً في وضع العزل الأقصى — نفّذ أمر التفعيل ثم أعد تشغيل التطبيق."),
              QStringLiteral("Live check: disabled by default in strict isolation — run the enable command then restart the app."),
              QStringLiteral("Проверка: отключено по умолчанию — выполните команду включения и перезапустите приложение."),
              QStringLiteral("实时检测：严格隔离模式下默认关闭 — 请执行启用命令后重启应用。"))
        : ((m_liveHidrawNodeCount > 0)
            ? loc(QStringLiteral("التحقق الحي: مفعّلة وتم رصد %1 منفذ /dev/hidraw* داخل الـ Sandbox.").arg(m_liveHidrawNodeCount),
                  QStringLiteral("Live check: granted and detected %1 /dev/hidraw* node(s) inside sandbox.").arg(m_liveHidrawNodeCount),
                  QStringLiteral("Проверка: активно, обнаружено %1 устройств /dev/hidraw* в песочнице.").arg(m_liveHidrawNodeCount),
                  QStringLiteral("实时检测：已启用，沙盒内检测到 %1 个 /dev/hidraw* 设备节点。").arg(m_liveHidrawNodeCount))
            : loc(QStringLiteral("التحقق الحي: مفعّلة في Flatpak، ولم يُرصد أي منفذ /dev/hidraw* حالياً."),
                  QStringLiteral("Live check: granted in Flatpak, no /dev/hidraw* nodes detected right now."),
                  QStringLiteral("Проверка: разрешено в Flatpak, но устройства /dev/hidraw* не найдены."),
                  QStringLiteral("实时检测：Flatpak 已授权，但当前未检测到 /dev/hidraw* 节点。")));
    flatpakEntry(
        QStringLiteral("device_all"), QStringLiteral("input-mouse"), QStringLiteral("--device=all  (/dev/hidraw*)"),
        loc(QStringLiteral("الوصول المباشر لماوس Delux (HID)"), QStringLiteral("Direct Mouse Hardware Access (HID)"),
            QStringLiteral("Прямой доступ к мыши (HID)"), QStringLiteral("鼠标直接硬件访问权限 (HID)")),
        loc(QStringLiteral("معطّلة افتراضياً للعزل الأقصى. مطلوبة لكشف منافذ /dev/hidraw* وقراءة وكتابة ذاكرة ماوس Delux M700A."),
            QStringLiteral("Disabled by default for strict isolation. Required to expose /dev/hidraw* and read/write Delux M700A memory."),
            QStringLiteral("Отключено по умолчанию. Необходимо для доступа к /dev/hidraw* и памяти мыши Delux M700A."),
            QStringLiteral("严格隔离下默认关闭。用于暴露 /dev/hidraw* 并读写 Delux M700A 鼠标内存。")),
        devNote,
        QStringLiteral("medium"),
        loc(QStringLiteral("متوسطة: تكشف عقد /dev لأن Flatpak لا يدعم تصفية جهاز hidraw واحد بعد. التطبيق يفتح الماوس 30FA:1040 فقط ولا يلمس لوحة المفاتيح أو الكاميرا."),
            QStringLiteral("Medium: exposes /dev nodes because Flatpak lacks single-device hidraw filtering. DeepComplex opens USB 30FA:1040 only and never accesses keyboards or webcams."),
            QStringLiteral("Средний: открывает /dev из-за ограничений Flatpak. Приложение обращается строго к мыши 30FA:1040."),
            QStringLiteral("中等：受限于 Flatpak 机制会暴露 /dev 节点，但本应用严格仅访问 30FA:1040 鼠标，不触碰键盘或摄像头。")),
        QStringLiteral("flatpak override --user --device=all org.deepcomplex.App")
    );

    // 2. System udev rule & live RW access — Low risk
    QString udevState;
    QString udevNote;
    if (!devGranted && !m_hostAccessAvailable) {
        udevState = QStringLiteral("unknown");
        udevNote = loc(
            QStringLiteral("التحقق الحي: فعّل صلاحية --device=all أولاً وأعد تشغيل التطبيق ليتمكن من اختبار منفذ الماوس."),
            QStringLiteral("Live check: enable --device=all and restart first so the app can test live mouse access."),
            QStringLiteral("Проверка: сначала включите --device=all и перезапустите приложение для проверки доступа к мыши."),
            QStringLiteral("实时检测：请先启用 --device=all 并重启应用以检测鼠标端口访问。")
        );
    } else if (m_liveMousePresent) {
        if (m_liveMouseRwOk && m_udevHealthy) {
            udevState = QStringLiteral("ok");
            udevNote = loc(
                QStringLiteral("التحقق الحي: وصول القراءة والكتابة سليم على %1 (30FA:1040).").arg(m_liveMousePath),
                QStringLiteral("Live check: read/write access verified on %1 (30FA:1040).").arg(m_liveMousePath),
                QStringLiteral("Проверка: права чтения/записи подтверждены на %1 (30FA:1040).").arg(m_liveMousePath),
                QStringLiteral("实时检测：已确认 %1 (30FA:1040) 具备读写权限。").arg(m_liveMousePath)
            );
        } else {
            udevState = QStringLiteral("off");
            udevNote = loc(
                QStringLiteral("التحقق الحي: تم رفض الوصول على %1 — نفّذ أمر التفعيل أدناه في الطرفية ثم أعد توصيل الماوس.").arg(m_liveMousePath),
                QStringLiteral("Live check: permission denied on %1 — run the enable command below in terminal, then replug mouse.").arg(m_liveMousePath),
                QStringLiteral("Проверка: доступ запрещён на %1 — выполните команду ниже в терминале и переподключите мышь.").arg(m_liveMousePath),
                QStringLiteral("实时检测：%1 权限被拒绝 — 请在终端执行下方命令后重新插拔鼠标。").arg(m_liveMousePath)
            );
        }
    } else if (m_hostAccessAvailable) {
        udevState = m_udevHealthy ? QStringLiteral("ok") : QStringLiteral("off");
        udevNote = m_udevHealthy
            ? loc(QStringLiteral("التحقق الحي: ملف القاعدة 70-delux-m700a.rules مثبّت وسليم (الماوس غير متصل حالياً)."),
                  QStringLiteral("Live check: 70-delux-m700a.rules is installed on host (mouse currently unplugged)."),
                  QStringLiteral("Проверка: правило 70-delux-m700a.rules установлено (мышь сейчас не подключена)."),
                  QStringLiteral("实时检测：70-delux-m700a.rules 规则文件已安装（鼠标当前未连接）。"))
            : loc(QStringLiteral("التحقق الحي: ملف القاعدة 70-delux-m700a.rules مفقود — نفّذ أمر التفعيل أدناه في الطرفية."),
                  QStringLiteral("Live check: 70-delux-m700a.rules is missing on host — run the enable command below in terminal."),
                  QStringLiteral("Проверка: файл 70-delux-m700a.rules отсутствует — выполните команду ниже в терминале."),
                  QStringLiteral("实时检测：宿主机缺少 70-delux-m700a.rules 规则文件，请在终端执行下方命令。"));
    } else {
        udevState = QStringLiteral("unknown");
        udevNote = loc(
            QStringLiteral("التحقق الحي: الماوس غير متصل — وصّل الماوس ثم اضغط «التحقق من الصلاحيات الآن» لاختبار الوصول الفعلي."),
            QStringLiteral("Live check: mouse unplugged — connect the mouse and click Verify Permissions Now to test live RW access."),
            QStringLiteral("Проверка: мышь не подключена — подключите мышь и нажмите «Проверить права сейчас»."),
            QStringLiteral("实时检测：鼠标未连接 — 请连接鼠标后点击“立即检查权限”以测试读写权限。")
        );
    }
    addPerm(
        QStringLiteral("udev_rule"), QStringLiteral("dialog-password"), QStringLiteral("/etc/udev/rules.d/70-delux-m700a.rules"),
        loc(QStringLiteral("قاعدة النظام للوصول للماوس (udev)"), QStringLiteral("System udev Device Rule"),
            QStringLiteral("Системное правило доступа (udev)"), QStringLiteral("系统 udev 设备权限规则")),
        loc(QStringLiteral("تمنح حسابك صلاحية القراءة والكتابة على الماوس دون صلاحيات روت عند كل تشغيل."),
            QStringLiteral("Grants your user read/write access to the mouse without needing root permissions."),
            QStringLiteral("Даёт пользователю права чтения/записи мыши без прав root."),
            QStringLiteral("授予当前用户免 root 读写鼠标的权限。")),
        udevNote,
        QStringLiteral("low"),
        loc(QStringLiteral("منخفضة: محصورة بدقة في الماوس 30FA:1040 فقط مع MODE=\"0660\" وTAG+=\"uaccess\" (ليست 0666)، ولا تفتح أي جهاز آخر."),
            QStringLiteral("Low: strictly scoped to USB ID 30FA:1040 with MODE=\"0660\" and TAG+=\"uaccess\" (never 0666); affects no other device."),
            QStringLiteral("Низкий: ограничено строго USB 30FA:1040 с MODE=\"0660\" и TAG+=\"uaccess\" (не 0666)."),
            QStringLiteral("低：严格限定于 USB 30FA:1040 且使用 MODE=\"0660\" 与 TAG+=\"uaccess\"（非 0666），不影响其他设备。")),
        udevState, udevManualCommand()
    );

    // 3. System Tray — Low risk
    const bool trayGranted = trayPermissionAvailable();
    const QString trayNote = !trayGranted
        ? loc(QStringLiteral("التحقق الحي: معطّلة افتراضياً — فعّلها اختيارياً لعرض أيقونة DeepComplex في شريط المهام."),
              QStringLiteral("Live check: disabled by default — optionally enable to show DeepComplex in the system tray."),
              QStringLiteral("Проверка: отключено по умолчанию — включите при необходимости значка в трее."),
              QStringLiteral("实时检测：默认关闭 — 如需系统托盘图标可按需手动启用。"))
        : (m_liveTrayDbusOk
            ? loc(QStringLiteral("التحقق الحي: مفعّلة وخدمة شريط المهام متاحة ونشطة في الجلسة الحالية."),
                  QStringLiteral("Live check: granted and system tray service is active in this session."),
                  QStringLiteral("Проверка: разрешено, служба трея активна в текущей сессии."),
                  QStringLiteral("实时检测：已启用，当前会话系统托盘服务运行正常。"))
            : loc(QStringLiteral("التحقق الحي: مفعّلة في Flatpak، ولم تُرصد خدمة StatusNotifierWatcher نشطة."),
                  QStringLiteral("Live check: granted in Flatpak, no active StatusNotifierWatcher detected."),
                  QStringLiteral("Проверка: разрешено в Flatpak, служба StatusNotifierWatcher не обнаружена."),
                  QStringLiteral("实时检测：Flatpak 已授权，但未检测到活动的 StatusNotifierWatcher。")));
    flatpakEntry(
        QStringLiteral("tray_dbus"), QStringLiteral("preferences-desktop-notification"), QStringLiteral("--talk-name=org.kde.StatusNotifierWatcher  (Opt-in)"),
        loc(QStringLiteral("أيقونة شريط المهام (System Tray)"), QStringLiteral("System Tray Icon"),
            QStringLiteral("Значок в системном трее"), QStringLiteral("系统托盘图标")),
        loc(QStringLiteral("معطّلة افتراضياً. تتيح عرض أيقونة DeepComplex في شريط المهام لتبديل البروفايلات الـ 5 بسرعة والتصغير في الخلفية."),
            QStringLiteral("Disabled by default. Allows showing DeepComplex in the KDE Plasma system tray for quick profile switching and background mode."),
            QStringLiteral("Отключено по умолчанию. Отображает значок DeepComplex в системном трее KDE Plasma."),
            QStringLiteral("默认关闭。启用后可在 KDE Plasma 系统托盘中显示图标以快速切换配置。")),
        trayNote,
        QStringLiteral("low"),
        loc(QStringLiteral("منخفضة: قناة D-Bus ضيقة مخصصة لأيقونة شريط المهام فقط بدون أي وصول لملفاتك أو نوافذك."),
            QStringLiteral("Low: narrow D-Bus channel limited strictly to the tray icon with zero file or window access."),
            QStringLiteral("Низкий: узкий канал D-Bus только для значка в трее, без доступа к файлам."),
            QStringLiteral("低：仅限系统托盘图标的窄 D-Bus 通道，无任何文件或窗口访问权限。")),
        trayGrantCommand()
    );

    // 4. XDG Background Portal — Low risk
    const QString bgNote = m_liveBgPortalOk
        ? loc(QStringLiteral("التحقق الحي: بوابة النظام org.freedesktop.portal.Desktop متاحة وتستجيب عبر D-Bus."),
              QStringLiteral("Live check: org.freedesktop.portal.Desktop is registered and reachable on session D-Bus."),
              QStringLiteral("Проверка: портал org.freedesktop.portal.Desktop доступен по D-Bus."),
              QStringLiteral("实时检测：会话 D-Bus 上的 org.freedesktop.portal.Desktop 门户服务正常可用。"))
        : loc(QStringLiteral("التحقق الحي: بوابة الخلفية غير مسجّلة على D-Bus حالياً (تأكد من تثبيت xdg-desktop-portal)."),
              QStringLiteral("Live check: XDG Desktop Portal is not registered on session D-Bus (ensure xdg-desktop-portal is installed)."),
              QStringLiteral("Проверка: XDG Desktop Portal не обнаружен в сессии D-Bus."),
              QStringLiteral("实时检测：当前会话 D-Bus 未检测到 XDG 桌面门户服务。"));
    addPerm(
        QStringLiteral("bg_portal"), QStringLiteral("system-run"), QStringLiteral("org.freedesktop.portal.Background  (XDG Portal)"),
        loc(QStringLiteral("بوابة التشغيل التلقائي الآمنة (XDG Portal)"), QStringLiteral("Sandboxed Autostart (XDG Background Portal)"),
            QStringLiteral("Автозапуск через XDG Background Portal"), QStringLiteral("沙盒开机自启门户 (XDG Background Portal)")),
        loc(QStringLiteral("تعمل داخل الـ Sandbox بدون أي صلاحية Flatpak إضافية لتفعيل أو إيقاف التشغيل التلقائي عبر بوابة النظام."),
            QStringLiteral("Works inside the sandbox with zero extra Flatpak permissions to manage login autostart via system portal."),
            QStringLiteral("Работает внутри песочницы без доп. прав Flatpak для управления автозапуском."),
            QStringLiteral("无需任何额外 Flatpak 权限，在沙盒内通过系统门户安全管理开机自启。")),
        bgNote,
        QStringLiteral("low"),
        loc(QStringLiteral("منخفضة: بوابة رسمية تديرها بيئة سطح المكتب نفسها دون منح التطبيق أي وصول لمجلدات النظام."),
            QStringLiteral("Low: official desktop-managed portal requiring zero filesystem permissions."),
            QStringLiteral("Низкий: официальный портал рабочего стола без доступа к файловой системе."),
            QStringLiteral("低：由桌面环境直接管理的官方门户，无需任何文件系统权限。")),
        m_liveBgPortalOk ? QStringLiteral("ok") : QStringLiteral("off"), QString()
    );

    // 5. Optional Host System Bridge (Opt-in) — HIGH RISK
    const QString hostNote = m_hostAccessAvailable
        ? loc(QStringLiteral("التحقق الحي: صلاحية النظام المضيف مفعّلة وتم التحقق منها عبر flatpak-spawn --host."),
              QStringLiteral("Live check: optional host permission is granted and verified via flatpak-spawn --host."),
              QStringLiteral("Проверка: опциональное право хоста активно и подтверждено через flatpak-spawn."),
              QStringLiteral("实时检测：可选宿主机权限已启用并通过 flatpak-spawn --host 验证。"))
        : loc(QStringLiteral("التحقق الحي: مغلقة افتراضياً — شغّل الأمر أدناه ثم أعد تشغيل التطبيق فقط إذا أردت التبديل التلقائي حسب التطبيق."),
              QStringLiteral("Live check: disabled by default — run the command below then restart only if you want app-based auto profile switching."),
              QStringLiteral("Проверка: отключено по умолчанию — включите только если нужно автопереключение профилей."),
              QStringLiteral("实时检测：默认关闭 — 仅在需要按应用自动切换配置时执行下方命令并重启。"));
    flatpakEntry(
        QStringLiteral("host_spawn"), QStringLiteral("utilities-terminal"), QStringLiteral("--talk-name=org.freedesktop.Flatpak  (Opt-in)"),
        loc(QStringLiteral("التواصل الاختياري مع النظام المضيف (Opt-in)"), QStringLiteral("Optional Host System Bridge (Opt-in)"),
            QStringLiteral("Опциональный доступ к хосту (Opt-in)"), QStringLiteral("可选宿主机通信权限 (Opt-in)")),
        loc(QStringLiteral("معطّلة افتراضياً. تفعيلها يدوياً يفتح ميزة التبديل التلقائي للبروفايل حسب التطبيق/اللعبة وفحص ملف udev."),
            QStringLiteral("Disabled by default. Enabling manually unlocks app/game auto profile switching and host udev file inspection."),
            QStringLiteral("Отключено по умолчанию. Ручное включение открывает автопереключение профилей по приложениям и проверку udev."),
            QStringLiteral("默认关闭。手动启用并重启后将解锁按应用/游戏自动切换配置及宿主机 udev 文件检测。")),
        hostNote,
        QStringLiteral("high"),
        loc(QStringLiteral("مرتفعة: تسمح بتنفيذ أوامر على النظام المضيف عبر flatpak-spawn (تكسر عزل الـ Sandbox). لا تفعّلها إلا إذا كنت تحتاج التبديل التلقائي حسب التطبيق."),
            QStringLiteral("High: allows executing commands on the host via flatpak-spawn (breaks sandbox isolation). Enable only if you need app-based auto profile switching."),
            QStringLiteral("Высокий: позволяет выполнять команды на хосте через flatpak-spawn (снимает изоляцию песочницы). Включайте только при необходимости."),
            QStringLiteral("高：允许通过 flatpak-spawn 在宿主机执行命令（会突破沙盒隔离）。仅在需要按应用自动切换配置时才启用。")),
        hostAccessGrantCommand()
    );

    // 6. Wayland — Low risk
    const QString wlNote = loc(
        QStringLiteral("التحقق الحي: خادم العرض النشط للتطبيق الآن هو (%1).").arg(platform),
        QStringLiteral("Live check: active Qt display platform is (%1).").arg(platform),
        QStringLiteral("Проверка: активная графическая платформа Qt — (%1).").arg(platform),
        QStringLiteral("实时检测：当前应用的 Qt 图形显示平台为 (%1)。").arg(platform)
    );
    flatpakEntry(
        QStringLiteral("socket_wayland"), QStringLiteral("video-display"), QStringLiteral("--socket=wayland"),
        loc(QStringLiteral("خادم العرض Wayland"), QStringLiteral("Wayland Display Socket"),
            QStringLiteral("Графический сервер Wayland"), QStringLiteral("Wayland 显示服务权限")),
        loc(QStringLiteral("صلاحية أساسية لعرض نافذة التطبيق بشكل أصلي على بيئة Wayland."),
            QStringLiteral("Core display permission for native window rendering on Wayland sessions."),
            QStringLiteral("Базовое разрешение для отображения окна в сессии Wayland."),
            QStringLiteral("在 Wayland 会话中显示应用窗口的基础权限。")),
        wlNote,
        QStringLiteral("low"),
        loc(QStringLiteral("منخفضة: معزولة بالكامل في Wayland ولا تسمح بقراءة نوافذ أو ضغطات مفاتيح التطبيقات الأخرى."),
            QStringLiteral("Low: fully isolated under Wayland—cannot inspect other windows or capture global keystrokes."),
            QStringLiteral("Низкий: полностью изолировано в Wayland, без доступа к окнам и вводу других приложений."),
            QStringLiteral("低：在 Wayland 下完全隔离，无法窥探其他窗口或全局按键输入。")),
        QStringLiteral("flatpak override --user --socket=wayland org.deepcomplex.App")
    );

    // 7. Fallback X11 — Medium risk (on X11 only; inactive on Wayland)
    const bool onWayland = platform.startsWith(QStringLiteral("wayland"), Qt::CaseInsensitive);
    flatpakEntry(
        QStringLiteral("socket_x11"), QStringLiteral("computer"), QStringLiteral("--socket=fallback-x11"),
        loc(QStringLiteral("خادم العرض الاحتياطي X11"), QStringLiteral("Fallback X11 Display Socket"),
            QStringLiteral("Резервный сервер X11"), QStringLiteral("X11 备用显示权限")),
        loc(QStringLiteral("صلاحية أساسية بديلة لتشغيل الواجهة على جلسات X11 إذا لم يكن Wayland متاحاً."),
            QStringLiteral("Core fallback display permission when running on X11 sessions."),
            QStringLiteral("Базовое запасное разрешение для отображения окна в сессиях X11."),
            QStringLiteral("当 Wayland 不可用时用于 X11 会话显示的基础权限。")),
        wlNote,
        onWayland ? QStringLiteral("low") : QStringLiteral("medium"),
        onWayland
            ? loc(QStringLiteral("منخفضة حالياً (لأن جلستك Wayland فيُعطّل X11 تلقائياً). على جلسات X11 تكون متوسطة لأن X11 لا يعزل النوافذ."),
                  QStringLiteral("Low right now (auto-disabled on your Wayland session). Medium on X11 sessions because X11 lacks inter-window isolation."),
                  QStringLiteral("Низкий сейчас (отключено в Wayland). Средний в сессиях X11 из-за отсутствия изоляции окон."),
                  QStringLiteral("当前低风险（在 Wayland 会话下自动禁用）；在 X11 会话下因缺少窗口隔离为中等风险。"))
            : loc(QStringLiteral("متوسطة على X11: بروتوكول X11 لا يعزل النوافذ عن بعضها. يُنصح باستخدام Wayland لعزل أقوى."),
                  QStringLiteral("Medium on X11: the X11 protocol does not isolate windows from each other. Wayland is recommended for stronger isolation."),
                  QStringLiteral("Средний в X11: протокол X11 не изолирует окна друг от друга. Рекомендуется Wayland."),
                  QStringLiteral("X11 下为中等风险：X11 协议不隔离应用窗口，建议使用 Wayland 以获得更强隔离。")),
        QStringLiteral("flatpak override --user --socket=fallback-x11 org.deepcomplex.App")
    );

    // 8. IPC — Low risk
    flatpakEntry(
        QStringLiteral("share_ipc"), QStringLiteral("memory"), QStringLiteral("--share=ipc"),
        loc(QStringLiteral("الذاكرة المشتركة للرسوميات (IPC)"), QStringLiteral("Shared Memory (IPC)"),
            QStringLiteral("Общая память (IPC)"), QStringLiteral("共享内存 (IPC) 权限")),
        loc(QStringLiteral("صلاحية أساسية لتحسين أداء عرض الواجهة الرسومية Qt Quick."),
            QStringLiteral("Core permission for smooth Qt Quick / X11 shared-memory GUI rendering."),
            QStringLiteral("Базовое разрешение для плавной отрисовки Qt Quick и X11."),
            QStringLiteral("提升 Qt Quick 与 X11 图形渲染性能的基础权限。")),
        loc(QStringLiteral("التحقق الحي: مقروء من سياق Sandbox الحالي."),
            QStringLiteral("Live check: verified from current sandbox context."),
            QStringLiteral("Проверка: считано из текущего контекста песочницы."),
            QStringLiteral("实时检测：已从当前沙盒上下文验证。")),
        QStringLiteral("low"),
        loc(QStringLiteral("منخفضة: تُستخدم فقط لتسريع رسوميات الواجهة ولا تمنح أي وصول لملفاتك أو النظام."),
            QStringLiteral("Low: used solely for GUI rendering performance; grants no file or system access."),
            QStringLiteral("Низкий: используется только для ускорения графики без доступа к файлам."),
            QStringLiteral("低：仅用于提升图形渲染性能，不授予任何文件或系统访问权限。")),
        QStringLiteral("flatpak override --user --share=ipc org.deepcomplex.App")
    );

    // 9. GPU Hardware Acceleration (--device=dri) — Low risk
    const bool driActive = m_activePerms.value(QStringLiteral("device_dri"), false) || QFileInfo::exists(QStringLiteral("/dev/dri"));
    addPerm(
        QStringLiteral("device_dri"), QStringLiteral("video-display"), QStringLiteral("--device=dri  (/dev/dri GPU)"),
        loc(QStringLiteral("تسريع الرسوميات العتادي (GPU / DRI)"), QStringLiteral("GPU Hardware Acceleration (DRI)"),
            QStringLiteral("Аппаратное ускорение GPU (DRI)"), QStringLiteral("GPU 硬件加速 (DRI)")),
        loc(QStringLiteral("صلاحية أساسية لتشغيل واجهة Qt Quick على كرت الشاشة ومنع استهلاك المعالج (LLVMpipe). لا تكشف الماوس أو منافذ USB."),
            QStringLiteral("Core GPU rendering permission for Qt Quick to avoid high CPU LLVMpipe software rendering. Does NOT expose USB or hidraw."),
            QStringLiteral("Базовое разрешение для аппаратного рендеринга Qt Quick на GPU (без высокой нагрузки CPU в LLVMpipe). Не открывает USB/hidraw."),
            QStringLiteral("用于 Qt Quick 硬件 GPU 渲染以避免 LLVMpipe 软件渲染高 CPU 占用，不暴露 USB 或 hidraw 设备。")),
        driActive
            ? loc(QStringLiteral("التحقق الحي: تسريع كرت الشاشة (/dev/dri) نشط — استهلاك المعالج منخفض."),
                  QStringLiteral("Live check: /dev/dri GPU acceleration active — low CPU rendering."),
                  QStringLiteral("Проверка: ускорение GPU (/dev/dri) активно — низкая нагрузка на CPU."),
                  QStringLiteral("实时检测：/dev/dri 硬件加速已启用，CPU 占用低。"))
            : loc(QStringLiteral("التحقق الحي: /dev/dri غير متاح — يعمل البرنامج بالعرض البرمجي على المعالج (LLVMpipe)."),
                  QStringLiteral("Live check: /dev/dri unavailable — falling back to CPU software rendering (LLVMpipe)."),
                  QStringLiteral("Проверка: /dev/dri недоступен — используется программный рендеринг CPU (LLVMpipe)."),
                  QStringLiteral("实时检测：/dev/dri 不可用，已回退到高 CPU 占用的软件渲染 (LLVMpipe)。")),
        QStringLiteral("low"),
        loc(QStringLiteral("منخفضة: تقتصر على معالجة الرسوميات فقط (/dev/dri) دون أي وصول للماوس أو لوحة المفاتيح أو الكاميرا أو الملفات."),
            QStringLiteral("Low: exposes only /dev/dri graphics acceleration with zero access to mice, keyboards, webcams, or files."),
            QStringLiteral("Низкий: открывает только ускорение графики /dev/dri без доступа к устройствам ввода, камере или файлам."),
            QStringLiteral("低：仅开放 /dev/dri 图形渲染接口，不授予键鼠、摄像头或文件访问权限。")),
        driActive ? QStringLiteral("ok") : QStringLiteral("off"),
        QStringLiteral("flatpak override --user --device=dri org.deepcomplex.App")
    );

    return list;
}

void SandboxManager::checkUdevHealth() {
    if (m_udevCheckRunning || m_shuttingDown) return;

    runLiveLocalChecks();
    const bool inFlatpak = isRunningInFlatpak();

    // If --device=all is disabled in Flatpak sandbox and no host access, don't trigger false udev repair popups.
    if (inFlatpak && !deviceAccessAvailable() && !m_hostAccessAvailable) {
        updateUdevHealthState(true, QString());
        return;
    }

    const bool hidPermOk = m_liveMousePresent ? m_liveMouseRwOk : true;
    const QString hidPathStr = m_liveMousePath;

    if (inFlatpak && !m_hostAccessAvailable) {
        const bool healthy = hidPermOk;
        QString reason;
        if (m_liveMousePresent && !hidPermOk) {
            reason = loc(
                QStringLiteral("تم رفض صلاحية الوصول إلى منفذ الماوس (%1: Permission denied). انسخ أمر التثبيت اليدوي لقاعدة udev وشغّله في الطرفية ثم أعد توصيل الماوس.").arg(hidPathStr),
                QStringLiteral("Permission denied on mouse port (%1). Copy the manual udev command, run it in terminal, then replug the mouse.").arg(hidPathStr),
                QStringLiteral("Нет прав доступа к порту мыши (%1). Скопируйте команду udev, выполните в терминале и переподключите мышь.").arg(hidPathStr),
                QStringLiteral("鼠标端口访问被拒绝（%1）。请复制手动 udev 命令并在终端运行，然后重新插拔鼠标。").arg(hidPathStr)
            );
        }
        updateUdevHealthState(healthy, reason);
        return;
    }

    m_udevCheckRunning = true;
    const QString checkScript = QStringLiteral(
        "test -s /etc/udev/rules.d/70-delux-m700a.rules && "
        "grep -qi '30fa' /etc/udev/rules.d/70-delux-m700a.rules && "
        "grep -qi '1040' /etc/udev/rules.d/70-delux-m700a.rules"
    );

    const QString prog = inFlatpak ? QStringLiteral("flatpak-spawn") : QStringLiteral("/bin/sh");
    const QStringList args = inFlatpak
        ? QStringList{QStringLiteral("--host"), QStringLiteral("/bin/sh"), QStringLiteral("-c"), checkScript}
        : QStringList{QStringLiteral("-c"), checkScript};

    runSafeAsyncProcess(
        this, prog, args, 8000,
        [this, hidPermOk, hidPathStr](QProcess *, int exitCode, QProcess::ExitStatus exitStatus) {
            m_udevCheckRunning = false;
            const bool fileOk = (exitStatus == QProcess::NormalExit && exitCode == 0);
            const bool healthy = fileOk && hidPermOk;

            QString reason;
            if (!fileOk) {
                reason = loc(
                    QStringLiteral("ملف تعريف الصلاحيات (/etc/udev/rules.d/70-delux-m700a.rules) غير موجود أو تالف."),
                    QStringLiteral("udev rule file (/etc/udev/rules.d/70-delux-m700a.rules) is missing or corrupted."),
                    QStringLiteral("Файл правила udev (/etc/udev/rules.d/70-delux-m700a.rules) отсутствует или повреждён."),
                    QStringLiteral("udev 权限规则文件 (/etc/udev/rules.d/70-delux-m700a.rules) 缺失或已损坏。")
                );
            } else if (!hidPermOk) {
                reason = loc(
                    QStringLiteral("تم رفض صلاحية الوصول إلى منفذ الماوس (%1: Permission denied). أعد تثبيت التعريف أو افصل الماوس وأعد توصيله.").arg(hidPathStr),
                    QStringLiteral("Permission denied on mouse port (%1). Reinstall the udev rule or replug the mouse.").arg(hidPathStr),
                    QStringLiteral("Нет прав доступа к порту мыши (%1). Переустановите правило udev или переподключите мышь.").arg(hidPathStr),
                    QStringLiteral("鼠标端口访问被拒绝（%1）。请重新安装 udev 规则或重新插拔鼠标。").arg(hidPathStr)
                );
            }
            updateUdevHealthState(healthy, reason);
        },
        [this, hidPermOk, hidPathStr]() {
            m_udevCheckRunning = false;
            QString reason;
            if (!hidPermOk) {
                reason = loc(
                    QStringLiteral("تم رفض صلاحية الوصول إلى منفذ الماوس (%1).").arg(hidPathStr),
                    QStringLiteral("Permission denied on mouse port (%1).").arg(hidPathStr),
                    QStringLiteral("Нет прав доступа к порту мыши (%1).").arg(hidPathStr),
                    QStringLiteral("鼠标端口访问被拒绝（%1）。").arg(hidPathStr)
                );
            }
            updateUdevHealthState(hidPermOk, reason);
        }
    );
}

bool SandboxManager::installUdevRules() {
    const bool inFlatpak = isRunningInFlatpak();
    if (inFlatpak && !m_hostAccessAvailable) {
        emit statusMessage(loc(
            QStringLiteral("التثبيت التلقائي لقاعدة udev غير متاح في وضع العزل — انسخ أمر التفعيل اليدوي وشغّله في الطرفية."),
            QStringLiteral("Automatic udev installation is disabled in sandbox mode — copy the manual command and run it in terminal."),
            QStringLiteral("Автоустановка udev недоступна в песочнице — скопируйте команду и выполните её в терминале."),
            QStringLiteral("沙盒模式下无法自动安装 udev 规则，请复制手动命令并在终端运行。")
        ), true);
        return false;
    }
    if (m_udevOpRunning || m_shuttingDown) return false;
    m_udevOpRunning = true;

    const QString script = QStringLiteral(
        "PATH=/usr/sbin:/usr/bin:/sbin:/bin; "
        "tmp=$(mktemp /etc/udev/rules.d/.70-delux-m700a.XXXXXX) || exit 1; "
        "trap 'rm -f \"$tmp\"' EXIT; "
        "printf 'SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"30fa\", ATTRS{idProduct}==\"1040\", MODE=\"0660\", TAG+=\"uaccess\"\\n' > \"$tmp\" && "
        "chmod 0644 \"$tmp\" && "
        "mv -f \"$tmp\" /etc/udev/rules.d/70-delux-m700a.rules && "
        "rm -f /etc/udev/rules.d/99-delux-m700a.rules && "
        "udevadm control --reload-rules && udevadm trigger"
    );

    const QString prog = inFlatpak ? QStringLiteral("flatpak-spawn") : QStringLiteral("pkexec");
    const QStringList args = inFlatpak
        ? QStringList{QStringLiteral("--host"), QStringLiteral("pkexec"), QStringLiteral("/bin/sh"), QStringLiteral("-c"), script}
        : QStringList{QStringLiteral("/bin/sh"), QStringLiteral("-c"), script};

    runSafeAsyncProcess(
        this, prog, args, 60000,
        [this](QProcess *, int exitCode, QProcess::ExitStatus exitStatus) {
            m_udevOpRunning = false;
            const bool ok = (exitStatus == QProcess::NormalExit && exitCode == 0);
            checkUdevHealth();
            if (ok) {
                emit statusMessage(loc(
                    QStringLiteral("تم تثبيت تعريف الصلاحيات الآمن (70-delux-m700a.rules) بنجاح! يمكنك الآن التحكم بالماوس مباشرة."),
                    QStringLiteral("Secure udev rule (70-delux-m700a.rules) installed successfully!"),
                    QStringLiteral("Безопасное правило udev (70-delux-m700a.rules) успешно установлено!"),
                    QStringLiteral("安全 udev 权限规则 (70-delux-m700a.rules) 安装成功！")
                ), false);
                emit udevInstalledSucceeded();
            } else {
                emit statusMessage(loc(
                    QStringLiteral("لم يكتمل تثبيت التعريف (تم إلغاء طلب sudo أو تعذر تشغيل pkexec). يمكنك نسخ الأمر اليدوي."),
                    QStringLiteral("udev rule installation was canceled or failed. You can copy the manual command."),
                    QStringLiteral("Установка правила udev отменена или не удалась. Скопируйте команду вручную."),
                    QStringLiteral("udev 规则安装已取消或失败，您可以复制手动命令执行。")
                ), true);
            }
            emit appPermissionsChanged();
        },
        [this]() {
            m_udevOpRunning = false;
            emit statusMessage(loc(
                QStringLiteral("انتهت مهلة أو تعذر تشغيل طلب صلاحيات المدير (sudo/pkexec)."),
                QStringLiteral("Administrator authorization timed out or failed to launch."),
                QStringLiteral("Время ожидания прав администратора истекло или ошибка запуска."),
                QStringLiteral("管理员授权超时或启动失败。")
            ), true);
            emit appPermissionsChanged();
        }
    );
    return true;
}

void SandboxManager::refreshAppPermissions() {
    if (m_permRefreshRunning || m_shuttingDown) return;

    runLiveLocalChecks();
    readSandboxInfo();
    checkUdevHealth();

    if (isRunningInFlatpak() && m_activePerms.value(QStringLiteral("host_spawn"))) {
        m_permRefreshRunning = true;
        emit appPermissionsChanged();

        runSafeAsyncProcess(
            this,
            QStringLiteral("flatpak-spawn"),
            {QStringLiteral("--host"), QStringLiteral("true")},
            5000,
            [this](QProcess *, int exitCode, QProcess::ExitStatus exitStatus) {
                m_permRefreshRunning = false;
                const bool verified = (exitStatus == QProcess::NormalExit && exitCode == 0);
                m_hostAccessAvailable = verified;
                m_activePerms[QStringLiteral("host_spawn")] = verified;
                m_lastCheckedTime = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
                emit hostAccessChanged();
                emit appPermissionsChanged();
            },
            [this]() {
                m_permRefreshRunning = false;
                m_hostAccessAvailable = false;
                m_activePerms[QStringLiteral("host_spawn")] = false;
                m_lastCheckedTime = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
                emit hostAccessChanged();
                emit appPermissionsChanged();
            }
        );
        return;
    }

    if (isRunningInFlatpak()) {
        m_hostAccessAvailable = false;
    }
    emit appPermissionsChanged();
}

void SandboxManager::refreshInstalledFlatpakApps() {
    if (!m_hostAccessAvailable || m_appsRefreshRunning || m_shuttingDown) return;
    m_appsRefreshRunning = true;
    const bool inFlatpak = isRunningInFlatpak();
    const QString prog = inFlatpak ? QStringLiteral("flatpak-spawn") : QStringLiteral("flatpak");
    const QStringList args = inFlatpak
        ? QStringList{QStringLiteral("--host"), QStringLiteral("flatpak"), QStringLiteral("list"), QStringLiteral("--app"), QStringLiteral("--columns=name,application")}
        : QStringList{QStringLiteral("list"), QStringLiteral("--app"), QStringLiteral("--columns=name,application")};

    runSafeAsyncProcess(
        this, prog, args, 10000,
        [this](QProcess *proc, int exitCode, QProcess::ExitStatus exitStatus) {
            m_appsRefreshRunning = false;
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                const QString out = QString::fromUtf8(proc->readAllStandardOutput());
                QVariantList apps;
                for (const QString &line : out.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
                    const QStringList parts = line.split(QLatin1Char('\t'));
                    if (parts.size() >= 2) {
                        const QString name = parts[0].trimmed();
                        const QString appId = parts[1].trimmed();
                        if (!appId.isEmpty() && appId != QStringLiteral("org.deepcomplex.App")) {
                            QVariantMap m;
                            m[QStringLiteral("name")] = name.isEmpty() ? appId : name;
                            m[QStringLiteral("appId")] = appId;
                            apps.append(m);
                        }
                    }
                }
                std::sort(apps.begin(), apps.end(), [](const QVariant &a, const QVariant &b) {
                    return a.toMap().value(QStringLiteral("name")).toString().toLower() <
                           b.toMap().value(QStringLiteral("name")).toString().toLower();
                });
                m_installedFlatpakApps = apps;
                emit installedFlatpakAppsChanged();
            }
        },
        [this]() { m_appsRefreshRunning = false; }
    );
}

void SandboxManager::pollRunningApps(const QStringList &profileLinkedApps) {
    if (!m_hostAccessAvailable || m_appCheckRunning || m_shuttingDown) return;

    bool anyLinked = false;
    for (const QString &raw : profileLinkedApps) {
        if (!raw.trimmed().isEmpty()) { anyLinked = true; break; }
    }
    if (!anyLinked) return;

    m_appCheckRunning = true;
    const QString cmd = QStringLiteral("flatpak ps --columns=application 2>/dev/null; echo '---PS---'; ps -eo comm=,args= 2>/dev/null");
    const bool inFlatpak = isRunningInFlatpak();
    const QString prog = inFlatpak ? QStringLiteral("flatpak-spawn") : QStringLiteral("/bin/sh");
    const QStringList args = inFlatpak
        ? QStringList{QStringLiteral("--host"), QStringLiteral("/bin/sh"), QStringLiteral("-c"), cmd}
        : QStringList{QStringLiteral("-c"), cmd};

    runSafeAsyncProcess(
        this, prog, args, 8000,
        [this, profileLinkedApps](QProcess *proc, int exitCode, QProcess::ExitStatus exitStatus) {
            m_appCheckRunning = false;
            if (m_shuttingDown || exitStatus != QProcess::NormalExit || exitCode != 0) return;

            const QString out = QString::fromUtf8(proc->readAllStandardOutput());
            const QStringList sections = out.split(QStringLiteral("---PS---"));
            QStringList runningFlatpaks;
            if (!sections.isEmpty()) {
                for (const QString &line : sections[0].split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
                    const QString id = line.trimmed().toLower();
                    if (!id.isEmpty() && id != QStringLiteral("org.deepcomplex.app")) runningFlatpaks.append(id);
                }
            }

            QStringList runningExecutables;
            if (sections.size() >= 2) {
                for (const QString &rawLine : sections[1].split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
                    const QString line = rawLine.trimmed();
                    if (line.isEmpty()) continue;
                    const int sp = line.indexOf(QLatin1Char(' '));
                    const QString comm = ((sp > 0) ? line.left(sp) : line).trimmed().toLower();
                    if (!comm.isEmpty() && comm != QStringLiteral("deepcomplex")) runningExecutables.append(comm);
                    if (sp > 0) {
                        const QString rest = line.mid(sp + 1).trimmed();
                        const int sp2 = rest.indexOf(QLatin1Char(' '));
                        const QString argv0 = (sp2 > 0) ? rest.left(sp2) : rest;
                        const QString baseName = QFileInfo(argv0).fileName().toLower();
                        if (!baseName.isEmpty() && baseName != comm && baseName != QStringLiteral("deepcomplex")) {
                            runningExecutables.append(baseName);
                        }
                    }
                }
            }

            int matchedProfile = -1;
            QString matchedToken;
            for (int i = 0; i < profileLinkedApps.size(); ++i) {
                const QString raw = profileLinkedApps[i].trimmed();
                if (raw.isEmpty()) continue;
                const QStringList tokens = raw.split(QLatin1Char(','), Qt::SkipEmptyParts);
                for (const QString &tok : tokens) {
                    const QString wanted = tok.trimmed().toLower();
                    if (wanted.isEmpty()) continue;
                    for (const QString &fpId : runningFlatpaks) {
                        if (fpId == wanted || fpId.endsWith(QLatin1Char('.') + wanted)) {
                            matchedProfile = i; matchedToken = tok.trimmed(); break;
                        }
                    }
                    if (matchedProfile >= 0) break;
                    for (const QString &exe : runningExecutables) {
                        if (exe == wanted) {
                            matchedProfile = i; matchedToken = tok.trimmed(); break;
                        }
                    }
                    if (matchedProfile >= 0) break;
                }
                if (matchedProfile >= 0) break;
            }
            emit runningAppsMatched(matchedProfile, matchedToken);
        },
        [this]() { m_appCheckRunning = false; }
    );
}
