import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.ScrollView {
    id: settingsPage
    clip: true
    contentWidth: availableWidth
    padding: 18

    property color accentColor: "#38bdf8"

    signal closePageRequested()
    signal exportBackupRequested()
    signal importBackupRequested()
    signal openPermissionsRequested()
    signal openThemeEditorRequested()
    signal openOnboardingRequested()
    signal replaySplashRequested()

    readonly property string projectWebsiteUrl: "https://github.com/bufferczd/deepcomplex-aurora"
    property bool websiteCopied: false

    Timer {
        id: websiteCopyTimer
        interval: 2500
        onTriggered: settingsPage.websiteCopied = false
    }
    signal confirmResetAllRequested()

    LayoutMirroring.enabled: mouseController.language === "ar"
    LayoutMirroring.childrenInherit: true

    ColumnLayout {
        width: settingsPage.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: mouseController.statusMessage.length > 0
            text: mouseController.statusMessage
            type: mouseController.statusError
                  ? Kirigami.MessageType.Error
                  : (!mouseController.connected ? Kirigami.MessageType.Warning : Kirigami.MessageType.Positive)
        }

        // 1. التشغيل التلقائي وشريط المهام
        GlassCard {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                Kirigami.Heading {
                    level: 3
                    text: I18n.tr(mouseController.language, "secStartup")
                }

                Kirigami.Separator { Layout.fillWidth: true }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optAutostart")
                    hintText: I18n.tr(mouseController.language, "optAutostartHint")
                    switchEnabled: !mouseController.autostartBusy
                    checkedValue: mouseController.autostartEnabled
                    onSwitchToggled: (s) => mouseController.autostartEnabled = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optStartMin")
                    switchEnabled: mouseController.trayAvailable
                    checkedValue: mouseController.startMinimized
                    onSwitchToggled: (s) => mouseController.startMinimized = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optMinTray")
                    hintText: I18n.tr(mouseController.language, "optMinTrayHint")
                    switchEnabled: mouseController.trayAvailable
                    checkedValue: mouseController.minimizeToTray
                    onSwitchToggled: (s) => mouseController.minimizeToTray = s
                }

                // تحذير وشرح تفعيل صلاحية شريط المهام عند تعطيلها افتراضياً
                Rectangle {
                    Layout.fillWidth: true
                    visible: !mouseController.trayPermissionAvailable
                    implicitHeight: trayOptInCol.implicitHeight + 20
                    radius: Theme.buttonRadius
                    color: Qt.rgba(0.96, 0.62, 0.04, 0.10)
                    border.width: 1.2
                    border.color: "#f59e0b"

                    ColumnLayout {
                        id: trayOptInCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        spacing: 7

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Kirigami.Icon { width: 18; height: 18; source: "dialog-warning"; color: "#f59e0b"; isMask: true }
                            Controls.Label {
                                Layout.fillWidth: true
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                color: "#fbbf24"
                                text: I18n.tr(mouseController.language, "permWarnTitle")
                            }
                        }

                        Controls.Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            text: I18n.tr(mouseController.language, "trayNeedsPerm")
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Controls.Label {
                                Layout.fillWidth: true
                                LayoutMirroring.enabled: false
                                horizontalAlignment: Text.AlignLeft
                                wrapMode: Text.WrapAnywhere
                                font.family: "monospace"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.9
                                text: "‎" + mouseController.trayGrantCommand()
                            }

                            ModernButton {
                                text: I18n.tr(mouseController.language, "copyCmdBtn")
                                icon.name: "edit-copy"
                                onClicked: mouseController.copyTrayGrantCommand()
                            }
                        }
                    }
                }
            }
        }

        // 2. أتمتة قراءة واتصال الماوس
        GlassCard {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                Kirigami.Heading {
                    level: 3
                    text: I18n.tr(mouseController.language, "secMouseAuto")
                }

                Kirigami.Separator { Layout.fillWidth: true }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optAutoRead")
                    checkedValue: mouseController.autoReadOnStartup
                    onSwitchToggled: (s) => mouseController.autoReadOnStartup = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optApplyStart")
                    checkedValue: mouseController.applyOnStartup
                    onSwitchToggled: (s) => mouseController.applyOnStartup = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optAutoDetect")
                    checkedValue: mouseController.autoDetectDevice
                    onSwitchToggled: (s) => mouseController.autoDetectDevice = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optAutoApplySwitch")
                    checkedValue: mouseController.autoApplyOnSwitch
                    onSwitchToggled: (s) => mouseController.autoApplyOnSwitch = s
                }

                // مراقب DPI للثيم المستقل كلياً عن لوحة تعديل DPI ووضع Extra Mode
                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optAutoSyncDpiColor")
                    hintText: I18n.tr(mouseController.language, "optAutoSyncDpiColorHint")
                    switchEnabled: themeDpiMonitor.deviceAccessAvailable
                    checkedValue: themeDpiMonitor.autoSync
                    onSwitchToggled: (s) => themeDpiMonitor.autoSync = s
                }

                Rectangle {
                    Layout.fillWidth: true
                    visible: themeDpiMonitor.deviceAccessAvailable
                    implicitHeight: dpiSyncBox.implicitHeight + 18
                    radius: Theme.buttonRadius
                    color: Theme.sidebarBg
                    border.width: 1
                    border.color: themeDpiMonitor.autoSync
                                  ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.55)
                                  : Theme.borderSubtle

                    ColumnLayout {
                        id: dpiSyncBox
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 9
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Controls.Label {
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: I18n.tr(mouseController.language, "dpiSyncIntervalLabel")
                            }

                            Controls.SpinBox {
                                from: 1
                                to: 60
                                stepSize: 1
                                editable: true
                                value: themeDpiMonitor.intervalSec
                                onValueModified: {
                                    themeDpiMonitor.intervalSec = value
                                    if (!themeDpiMonitor.autoSync) themeDpiMonitor.autoSync = true
                                }
                            }

                            Controls.Label {
                                opacity: 0.8
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: I18n.tr(mouseController.language, "secondsUnit")
                            }

                            Item { Layout.fillWidth: true }

                            ModernButton {
                                implicitHeight: 32
                                text: I18n.tr(mouseController.language, "readNowBtn")
                                icon.name: "view-refresh"
                                enabled: !mouseController.isBusy
                                onClicked: themeDpiMonitor.readNow()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Repeater {
                                model: [1, 2, 3, 5, 10, 30]
                                ModernButton {
                                    Layout.fillWidth: true
                                    implicitHeight: 30
                                    text: modelData + " " + I18n.tr(mouseController.language, "secondsUnit")
                                    highlighted: themeDpiMonitor.intervalSec === modelData
                                    onClicked: {
                                        themeDpiMonitor.intervalSec = modelData
                                        if (!themeDpiMonitor.autoSync) {
                                            themeDpiMonitor.autoSync = true
                                        }
                                    }
                                }
                            }
                        }

                        // مؤشر حي يؤكد عمل مراقب الثيم المستقل (مع حماية Extra Mode)
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: themeDpiMonitor.autoSync ? "#22c55e" : "#64748b"
                            }

                            Controls.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                color: Theme.textSecondary
                                text: themeDpiMonitor.autoSync
                                      ? (I18n.tr(mouseController.language, "dpiSyncActiveStatus")
                                         + " (" + themeDpiMonitor.intervalSec + " " + I18n.tr(mouseController.language, "secondsUnit") + ")"
                                         + (themeDpiMonitor.extraModePassive ? ("  •  " + I18n.tr(mouseController.language, "dpiSyncExtraSafeTag")) : "")
                                         + (themeDpiMonitor.lastReadTime.length > 0
                                            ? ("  •  " + I18n.tr(mouseController.language, "dpiSyncLastRead") + " " + themeDpiMonitor.lastReadTime
                                               + (themeDpiMonitor.dpiLevel > 0 ? (" • DPI " + themeDpiMonitor.dpiLevel) : ""))
                                            : ""))
                                      : I18n.tr(mouseController.language, "dpiSyncOffStatus")
                            }
                        }
                    }
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optAutoSwitchApp")
                    hintText: I18n.tr(mouseController.language, "optAutoSwitchAppHint")
                    switchEnabled: mouseController.hostAccessAvailable
                    checkedValue: mouseController.autoSwitchByApp
                    onSwitchToggled: (s) => mouseController.autoSwitchByApp = s
                }

                // تحذير أمني صريح لتفعيل صلاحية النظام المضيف (Opt-in)
                Rectangle {
                    Layout.fillWidth: true
                    visible: !mouseController.hostAccessAvailable
                    implicitHeight: hostOptInCol.implicitHeight + 20
                    radius: Theme.buttonRadius
                    color: Qt.rgba(0.94, 0.27, 0.27, 0.11)
                    border.width: 1.4
                    border.color: "#f59e0b"

                    ColumnLayout {
                        id: hostOptInCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        spacing: 7

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Kirigami.Icon { width: 20; height: 20; source: "dialog-warning"; color: "#f59e0b"; isMask: true }
                            Controls.Label {
                                Layout.fillWidth: true
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                color: "#fbbf24"
                                text: I18n.tr(mouseController.language, "hostPermWarnTitle")
                            }
                        }

                        Controls.Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            text: I18n.tr(mouseController.language, "autoSwitchNeedsHost")
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Controls.Label {
                                Layout.fillWidth: true
                                LayoutMirroring.enabled: false
                                horizontalAlignment: Text.AlignLeft
                                wrapMode: Text.WrapAnywhere
                                font.family: "monospace"
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                opacity: 0.9
                                text: "\u200E" + mouseController.hostAccessGrantCommand()
                            }

                            ModernButton {
                                text: I18n.tr(mouseController.language, "copyCmdBtn")
                                icon.name: "edit-copy"
                                onClicked: mouseController.copyHostAccessCommand()
                            }
                        }
                    }
                }
            }
        }

        // 3. الميزات المتقدمة والمخفية والواجهة واللغة
        GlassCard {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                Kirigami.Heading {
                    level: 3
                    text: I18n.tr(mouseController.language, "secAdvanced")
                }

                Kirigami.Separator { Layout.fillWidth: true }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optStartupSplash")
                    checkedValue: mouseController.showStartupSplash
                    onSwitchToggled: (s) => mouseController.showStartupSplash = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optShowRgbStrip")
                    checkedValue: mouseController.showRgbPreviewStrip
                    onSwitchToggled: (s) => mouseController.showRgbPreviewStrip = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optAnimatedUi")
                    hintText: I18n.tr(mouseController.language, "optAnimatedUiHint")
                    checkedValue: mouseController.animatedUi
                    onSwitchToggled: (s) => mouseController.animatedUi = s
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Controls.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: I18n.tr(mouseController.language, "themeDialogTitle")
                    }
                    ModernButton {
                        text: I18n.tr(mouseController.language, "themeBtn")
                        icon.name: "color-management"
                        highlighted: true
                        onClicked: settingsPage.openThemeEditorRequested()
                    }
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optKeyCapture")
                    hintText: I18n.tr(mouseController.language, "optKeyCaptureHint")
                    checkedValue: mouseController.enableKeyCapture
                    onSwitchToggled: (s) => mouseController.enableKeyCapture = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optProgressLog")
                    hintText: I18n.tr(mouseController.language, "optProgressLogHint")
                    checkedValue: mouseController.showProgressAndLog
                    onSwitchToggled: (s) => mouseController.showProgressAndLog = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optHiddenFx")
                    hintText: I18n.tr(mouseController.language, "optHiddenFxHint")
                    checkedValue: mouseController.showHiddenEffects
                    onSwitchToggled: (s) => mouseController.showHiddenEffects = s
                }

                SettingSwitchRow {
                    labelText: I18n.tr(mouseController.language, "optShowHex")
                    checkedValue: mouseController.showHexCodes
                    onSwitchToggled: (s) => mouseController.showHexCodes = s
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing

                    Controls.Label {
                        text: I18n.tr(mouseController.language, "optLang")
                    }

                    Controls.ComboBox {
                        id: settingsLangCombo
                        Layout.fillWidth: true
                        textRole: "label"
                        model: [
                            { label: "العربية (Arabic)", code: "ar" },
                            { label: "English", code: "en" },
                            { label: "Русский (Russian)", code: "ru" },
                            { label: "中文 (Chinese)", code: "zh" }
                        ]
                        function syncLang() {
                            for (let i = 0; i < model.length; ++i) {
                                if (model[i].code === mouseController.language) {
                                    currentIndex = i
                                    return
                                }
                            }
                        }
                        Component.onCompleted: syncLang()
                        Connections {
                            target: mouseController
                            function onLanguageChanged() { settingsLangCombo.syncLang() }
                        }
                        onActivated: (idx) => mouseController.language = model[idx].code
                    }
                }
            }
        }

        // 4. النسخ الاحتياطي وإدارة البيانات (منسّق بشبكة متوازنة ومسار LTR سليم)
        GlassCard {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Heading {
                    level: 3
                    text: I18n.tr(mouseController.language, "secBackup")
                }

                Kirigami.Separator { Layout.fillWidth: true }

                // شبكة متساوية العرض من عمودين × صفين للأزرار الأساسية
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: Kirigami.Units.mediumSpacing
                    rowSpacing: Kirigami.Units.mediumSpacing

                    ModernButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 40
                        text: I18n.tr(mouseController.language, "btnExport")
                        icon.name: "document-export"
                        onClicked: settingsPage.exportBackupRequested()
                    }

                    ModernButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 40
                        text: I18n.tr(mouseController.language, "btnImport")
                        icon.name: "document-import"
                        onClicked: settingsPage.importBackupRequested()
                    }

                    ModernButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 40
                        text: I18n.tr(mouseController.language, "permissionsBtn")
                        icon.name: "security-high"
                        highlighted: true
                        onClicked: settingsPage.openPermissionsRequested()
                    }

                    ModernButton {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 40
                        text: I18n.tr(mouseController.language, "openTutorialBtn")
                        icon.name: "help-about"
                        onClicked: settingsPage.openOnboardingRequested()
                    }
                }

                // صندوق مسار ملف الإعدادات بتنسيق واضح واتجاه LTR سليم + زر نسخ
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: pathRow.implicitHeight + 14
                    radius: 6
                    color: Kirigami.Theme.alternateBackgroundColor
                    border.width: 1
                    border.color: Kirigami.Theme.disabledTextColor

                    RowLayout {
                        id: pathRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 8
                        spacing: 8

                        Controls.Label {
                            font.bold: true
                            opacity: 0.78
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            text: I18n.tr(mouseController.language, "lblConfigPath")
                        }

                        Controls.Label {
                            Layout.fillWidth: true
                            LayoutMirroring.enabled: false
                            horizontalAlignment: Text.AlignLeft
                            wrapMode: Text.WrapAnywhere
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            opacity: 0.85
                            text: "\u200E" + mouseController.configFilePath
                        }

                        Controls.ToolButton {
                            icon.name: "edit-copy"
                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.text: I18n.tr(mouseController.language, "copyPathTip")
                            onClicked: mouseController.copyConfigFilePath()
                        }
                    }
                }

                Kirigami.Separator { Layout.fillWidth: true }

                // شريط إعادة الضبط الكامل منفصل بوضوح في الأسفل
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.mediumSpacing

                    Controls.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        opacity: 0.72
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "resetAllHint")
                    }

                    ModernButton {
                        Layout.minimumHeight: 38
                        text: I18n.tr(mouseController.language, "btnResetAll")
                        icon.name: "edit-delete"
                        onClicked: settingsPage.confirmResetAllRequested()
                    }
                }
            }
        }

        // 5. حول التطبيق (الإصدار + العتاد + الرخصة)
        GlassCard {
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                ThemedAppIcon {
                    iconSize: 44
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Kirigami.Heading {
                        level: 3
                        text: I18n.tr(mouseController.language, "secAbout")
                    }

                    Controls.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        opacity: 0.85
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "aboutVersionLabel") + " DeepComplex v" + mouseController.appVersion
                              + "  •  " + I18n.tr(mouseController.language, "aboutLicenseLabel") + " GPL-3.0-or-later"
                    }

                    Controls.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        opacity: 0.78
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "aboutHardwareLabel") + " Delux M700A (Instant A725 • USB 30FA:1040)"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        spacing: 8

                        Controls.Label {
                            opacity: 0.85
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            text: I18n.tr(mouseController.language, "aboutWebsiteLabel")
                        }

                        Controls.Label {
                            id: repoLinkLabel
                            Layout.fillWidth: true
                            LayoutMirroring.enabled: false
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            font.family: "monospace"
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            font.underline: repoLinkMouse.containsMouse
                            color: Qt.lighter(Theme.accent, 1.35)
                            text: "‎" + settingsPage.projectWebsiteUrl

                            MouseArea {
                                id: repoLinkMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally(settingsPage.projectWebsiteUrl)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 6
                        spacing: 8

                        ModernButton {
                            implicitHeight: 34
                            highlighted: true
                            icon.name: "globe"
                            text: I18n.tr(mouseController.language, "openWebsiteBtn")
                            onClicked: Qt.openUrlExternally(settingsPage.projectWebsiteUrl)
                        }

                        ModernButton {
                            implicitHeight: 34
                            icon.name: settingsPage.websiteCopied ? "dialog-ok-apply" : "edit-copy"
                            text: settingsPage.websiteCopied
                                  ? I18n.tr(mouseController.language, "websiteCopiedMsg")
                                  : I18n.tr(mouseController.language, "copyWebsiteBtn")
                            onClicked: {
                                mouseController.copyCommandToClipboard(settingsPage.projectWebsiteUrl)
                                settingsPage.websiteCopied = true
                                websiteCopyTimer.restart()
                            }
                        }
                    }
                }
            }
        }
    }
}
