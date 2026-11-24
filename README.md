DeepComplex Aurora

العربية


تطبيق لينكس للتحكم المباشر بعتاد ماوس Delux M700A (30FA:1040).


DeepComplex Aurora هو تطبيق مفتوح المصدر لأنظمة لينكس للتحكم المباشر بعتاد ماوس Delux M700A (مستشعر Instant A725 • المعرّف 30FA:1040) عبر HIDRAW.

المزايا

البروفايلات وDPI: إدارة حتى 5 بروفايلات و6 مستويات DPI (200–7200 DPI) مع لون مؤشر لكل مستوى وإمكانية التبديل التلقائي حسب التطبيق.

إضاءة RGB: التحكم بـ 14 وضع إضاءة عتادي وتخصيص ألوان 6 مناطق بشكل مستقل.

استوديو الماكرو: تسجيل اختصارات لوحة المفاتيح ونقرات الماوس مع تأخير مخصص لكل خطوة (8–1016 ms).

مراقب DPI اللحظي: عرض فوري لمستوى السرعة واللون النشط.

دعم اللغات: متوفر باللغات العربية، الإنجليزية، الروسية، والصينية.

التثبيت
الخيار 1: من صفحة الإصدارات (الأسهل والموصى به)

    حمّل أحدث ملف .flatpak من صفحة الإصدارات (Releases).

    ثبّت التطبيق وشغّله عبر الطرفية:

Bash

flatpak install --user deepcomplex-v2.8.4.flatpak
flatpak run org.deepcomplex.App

الخيار 2: البناء والتثبيت من الكود المصدري

المتطلبات: git وflatpak وflatpak-builder وبيئة تشغيل KDE 6.11 (org.kde.Platform//6.11 وorg.kde.Sdk//6.11).
Bash

git clone [https://github.com/bufferczd/deepcomplex-aurora.git](https://github.com/bufferczd/deepcomplex-aurora.git)
cd deepcomplex-aurora
chmod +x run.sh
./run.sh

    ملاحظة للأجهزة ذات الرام المحدودة: في حال توقف البناء بسبب نفاد الذاكرة، شغّل JOBS=1 ./run.sh.

[!ملاحظة]
تم بناء التطبيق بمساعدة الذكاء الاصطناعي، بينما تمت التجربة العملية على العتاد واستخراج شفرات الهكس (Hex) يدوياً بواسطة إنسان.

الرخصة

مرخّص بموجب رخصة GNU General Public License v3.0 (GPLv3) — راجع ملف LICENSE.


English

Direct Linux hardware control application for the Delux M700A (30FA:1040) gaming mouse.

DeepComplex Aurora is an open-source Linux application for direct hardware control of the Delux M700A gaming mouse (Instant A725 sensor, USB ID 30FA:1040) over HIDRAW.

Features

Profiles & DPI: Manage up to 5 profiles and 6 DPI stages (200–7200 DPI) with per-stage indicator colors and optional per-app profile switching.

RGB Lighting: Control 14 onboard lighting modes and independent 6-zone color mapping.

Macro Studio: Record keyboard shortcuts and mouse clicks with customizable per-step delays (8–1016 ms).

Live DPI Monitor: Real-time display of the active DPI stage and color.

Multi-Language Support: Available in English, Arabic, Russian, and Chinese.

## Installation

### Option 1: From Releases (Recommended)
1. Download the latest `.flatpak` file from the [Releases](https://github.com/bufferczd/deepcomplex-aurora/releases) page.
2. Install and run:
```bash
flatpak install --user deepcomplex-v2.8.4.flatpak
flatpak run org.deepcomplex.App
```
Option 2: Build from Source

Requirements: git, flatpak, flatpak-builder, and the KDE 6.11 Flatpak runtime/SDK (org.kde.Platform//6.11, org.kde.Sdk//6.11).
Bash

git clone [https://github.com/bufferczd/deepcomplex-aurora.git](https://github.com/bufferczd/deepcomplex-aurora.git)
cd deepcomplex-aurora
chmod +x run.sh
./run.sh

    Low-RAM tip: If the build fails due to low memory, run JOBS=1 ./run.sh.

[!NOTE]
Built with AI assistance, while hardware testing and hex extraction were performed manually by a human.

License

Licensed under the GNU General Public License v3.0 (GPLv3) — see LICENSE.
