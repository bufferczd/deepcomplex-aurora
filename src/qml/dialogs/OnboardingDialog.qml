import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: ""
    id: onboardingDialog
    title: I18n.tr(mouseController.language, "welcomeTitle")
    width: Math.min((parent ? parent.width : 1020) - 44, 780)
    height: Math.min((parent ? parent.height : 840) - 52, 660)
    closePolicy: Controls.Popup.CloseOnEscape

    signal explainCommandsRequested(bool incDevice, bool incTray, bool incHost, bool incUdev)

    property int currentStep: 0
    readonly property int totalSteps: 6

    property bool wantDevice: true
    property bool wantUdev: true
    property bool wantTray: true
    property bool wantHost: false
    property string copiedMsg: ""

    readonly property string generatedCommand: mouseController.buildEnablePermissionsCommand(
        wantDevice, wantTray, wantHost, wantUdev
    )

    Timer {
        id: onbCopyTimer
        interval: 3500
        onTriggered: onboardingDialog.copiedMsg = ""
    }

    function openAtStep(stepIndex) {
        currentStep = Math.max(0, Math.min(totalSteps - 1, stepIndex))
        copiedMsg = ""
        mouseController.refreshAppPermissions()
        wantDevice = true
        wantTray = !mouseController.trayPermissionAvailable
        wantHost = false
        wantUdev = true

        open()
    }

    onClosed: {
        mouseController.firstRunCompleted = true
    }

    contentItem: Item {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true

        // Clean 6-Step Technical Guide (no logo inside first window)
        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            // Interactive Step Progress Pills + Quick Skip
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: onboardingDialog.totalSteps
                        Rectangle {
                            Layout.fillWidth: true
                            height: 6
                            radius: 3
                            color: index <= onboardingDialog.currentStep ? Theme.accent : Theme.borderSubtle
                            Behavior on color { ColorAnimation { duration: 180 } }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: onboardingDialog.currentStep = index
                            }
                        }
                    }
                }

                Controls.Label {
                    font.bold: true
                    font.pixelSize: 12
                    color: Theme.textSecondary
                    text: (onboardingDialog.currentStep + 1) + " / " + onboardingDialog.totalSteps
                }

                ModernButton {
                    visible: onboardingDialog.currentStep < onboardingDialog.totalSteps - 1
                    implicitHeight: 30
                    text: I18n.tr(mouseController.language, "tourSkipBtn")
                    onClicked: onboardingDialog.currentStep = onboardingDialog.totalSteps - 1
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: onboardingDialog.currentStep

                // ======================================================
                // SLIDE 0: Hero Welcome ("ماوسك. قواعدك. بأسلوبك.")
                // ======================================================
                ColumnLayout {
                    spacing: 14

                    GlassCard {
                        activeGlow: true
                        cardPadding: 20

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Controls.Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    font.bold: true
                                    font.pixelSize: 22
                                    color: Theme.textPrimary
                                    text: I18n.tr(mouseController.language, "tourWelcomeHeading")
                                }

                                Controls.Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: 13
                                    color: Theme.textSecondary
                                    text: I18n.tr(mouseController.language, "tourWelcomeDesc")
                                }
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 10
                        rowSpacing: 10

                        Repeater {
                            model: [
                                { icon: "security-high", key: "tourPillSafe" },
                                { icon: "input-mouse", key: "tourPillHw" },
                                { icon: "color-management", key: "tourPillOled" }
                            ]

                            GlassCard {
                                cardPadding: 14
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Rectangle {
                                        width: 36; height: 36; radius: 10
                                        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.22)
                                        border.width: 1
                                        border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.65)
                                        Kirigami.Icon { anchors.centerIn: parent; width: 20; height: 20; source: modelData.icon; color: Theme.accent; isMask: true }
                                    }
                                    Controls.Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                        font.bold: true
                                        font.pixelSize: 13
                                        color: Theme.textPrimary
                                        text: I18n.tr(mouseController.language, modelData.key)
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // ======================================================
                // SLIDE 1: Profiles & 6-Stage DPI Visual Showcase
                // ======================================================
                ColumnLayout {
                    spacing: 12

                    GlassCard {
                        activeGlow: true
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14
                            Rectangle {
                                width: 46; height: 46; radius: 12
                                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.22)
                                border.width: 1; border.color: Theme.accent
                                Kirigami.Icon { anchors.centerIn: parent; width: 26; height: 26; source: "speedometer"; color: Theme.accent; isMask: true }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Controls.Label { font.bold: true; font.pixelSize: 18; color: Theme.textPrimary; text: I18n.tr(mouseController.language, "tourS1Title") }
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "tourS1Sub") }
                            }
                        }

                        // Visual 6-Stage DPI Color Strip
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Repeater {
                                model: [
                                    { dpi: "800",  col: "#facc15" },
                                    { dpi: "1200", col: "#ef4444" },
                                    { dpi: "2400", col: "#22c55e" },
                                    { dpi: "4800", col: "#06b6d4" },
                                    { dpi: "6400", col: "#3b82f6" },
                                    { dpi: "7200", col: "#d946ef" }
                                ]
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 30
                                    radius: 8
                                    color: Qt.rgba(0, 0, 0, 0.45)
                                    border.width: 1.2
                                    border.color: modelData.col
                                    RowLayout {
                                        anchors.centerIn: parent
                                        spacing: 5
                                        Rectangle { width: 8; height: 8; radius: 4; color: modelData.col }
                                        Controls.Label { font.bold: true; font.pixelSize: 11; color: "#F8FAFC"; text: modelData.dpi }
                                    }
                                }
                            }
                        }
                    }

                    Repeater {
                        model: [
                            { icon: "user-identity", key: "tourS1Card1" },
                            { icon: "speedometer",   key: "tourS1Card2" },
                            { icon: "applications-games", key: "tourS1Card3" }
                        ]
                        GlassCard {
                            cardPadding: 12
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                Kirigami.Icon { width: 20; height: 20; source: modelData.icon; color: Theme.accent; isMask: true }
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 13; color: Theme.textPrimary; text: I18n.tr(mouseController.language, modelData.key) }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // ======================================================
                // SLIDE 2: 14 RGB Effects & 6-Zone Extra Mode
                // ======================================================
                ColumnLayout {
                    spacing: 12

                    GlassCard {
                        activeGlow: true
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14
                            Rectangle {
                                width: 46; height: 46; radius: 12
                                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.22)
                                border.width: 1; border.color: Theme.accent
                                Kirigami.Icon { anchors.centerIn: parent; width: 26; height: 26; source: "color-management"; color: Theme.accent; isMask: true }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Controls.Label { font.bold: true; font.pixelSize: 18; color: Theme.textPrimary; text: I18n.tr(mouseController.language, "tourS2Title") }
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "tourS2Sub") }
                            }
                        }

                        // Visual 6-Zone Extra Mode Palette Bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: 12
                            radius: 6
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.00; color: "#ff0055" }
                                GradientStop { position: 0.20; color: "#f97316" }
                                GradientStop { position: 0.40; color: "#facc15" }
                                GradientStop { position: 0.60; color: "#22c55e" }
                                GradientStop { position: 0.80; color: "#06b6d4" }
                                GradientStop { position: 1.00; color: "#a855f7" }
                            }
                        }
                    }

                    Repeater {
                        model: [
                            { icon: "draw-brush",       key: "tourS2Card1" },
                            { icon: "color-picker",     key: "tourS2Card2" },
                            { icon: "preferences-desktop-display", key: "tourS2Card3" }
                        ]
                        GlassCard {
                            cardPadding: 12
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                Kirigami.Icon { width: 20; height: 20; source: modelData.icon; color: Theme.accent; isMask: true }
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 13; color: Theme.textPrimary; text: I18n.tr(mouseController.language, modelData.key) }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // ======================================================
                // SLIDE 3: Macro Studio & Custom Timing (8–1016 ms)
                // ======================================================
                ColumnLayout {
                    spacing: 12

                    GlassCard {
                        activeGlow: true
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14
                            Rectangle {
                                width: 46; height: 46; radius: 12
                                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.22)
                                border.width: 1; border.color: Theme.accent
                                Kirigami.Icon { anchors.centerIn: parent; width: 26; height: 26; source: "input-keyboard"; color: Theme.accent; isMask: true }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Controls.Label { font.bold: true; font.pixelSize: 18; color: Theme.textPrimary; text: I18n.tr(mouseController.language, "tourS3Title") }
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "tourS3Sub") }
                            }
                        }

                        // Visual Keycap & Timing Pipeline Preview
                        RowLayout {
                            Layout.fillWidth: true
                            LayoutMirroring.enabled: false
                            spacing: 8
                            Repeater {
                                model: ["↓ Ctrl", "16 ms", "↓ C", "8 ms", "↓ Click"]
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 28
                                    radius: 7
                                    color: index % 2 === 1 ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.24) : Theme.inputBg
                                    border.width: 1
                                    border.color: index % 2 === 1 ? Theme.accent : Theme.borderSubtle
                                    Controls.Label {
                                        anchors.centerIn: parent
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: Theme.textPrimary
                                        text: modelData
                                    }
                                }
                            }
                        }
                    }

                    Repeater {
                        model: [
                            { icon: "chronometer",    key: "tourS3Card1" },
                            { icon: "input-keyboard", key: "tourS3Card2" },
                            { icon: "media-flash",    key: "tourS3Card3" }
                        ]
                        GlassCard {
                            cardPadding: 12
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                Kirigami.Icon { width: 20; height: 20; source: modelData.icon; color: Theme.accent; isMask: true }
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 13; color: Theme.textPrimary; text: I18n.tr(mouseController.language, modelData.key) }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // ======================================================
                // SLIDE 4: 20 Themes (OLED), Edges & Isolated Live DPI
                // ======================================================
                ColumnLayout {
                    spacing: 12

                    GlassCard {
                        activeGlow: true
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14
                            Rectangle {
                                width: 46; height: 46; radius: 12
                                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.22)
                                border.width: 1; border.color: Theme.accent
                                Kirigami.Icon { anchors.centerIn: parent; width: 26; height: 26; source: "preferences-desktop-theme"; color: Theme.accent; isMask: true }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Controls.Label { font.bold: true; font.pixelSize: 18; color: Theme.textPrimary; text: I18n.tr(mouseController.language, "tourThemesTitle") }
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "tourThemesBody") }
                            }
                        }

                        // Visual Mini Theme Swatches Preview
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Repeater {
                                model: [
                                    { bg: "#000000", rim: "#15803d", lbl: "OLED" },
                                    { bg: "#000000", rim: "#00e5ff", lbl: "Cyan" },
                                    { bg: "#0B1120", rim: "#38bdf8", lbl: "Aurora" },
                                    { bg: "#1C0B0E", rim: "#fb7185", lbl: "Crimson" },
                                    { bg: "#E4EDF8", rim: "#0284c7", lbl: "Frost", light: true }
                                ]
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 28
                                    radius: 7
                                    color: modelData.bg
                                    border.width: 1.5
                                    border.color: modelData.rim
                                    Controls.Label {
                                        anchors.centerIn: parent
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: modelData.light ? "#0F172A" : "#F8FAFC"
                                        text: modelData.lbl
                                    }
                                }
                            }
                        }
                    }

                    Repeater {
                        model: [
                            { icon: "color-management", key: "tourS4Card1" },
                            { icon: "draw-rectangle",   key: "tourS4Card2" },
                            { icon: "speedometer",      key: "tourS4Card3" }
                        ]
                        GlassCard {
                            cardPadding: 12
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                Kirigami.Icon { width: 20; height: 20; source: modelData.icon; color: Theme.accent; isMask: true }
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 13; color: Theme.textPrimary; text: I18n.tr(mouseController.language, modelData.key) }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // ======================================================
                // SLIDE 5: Clean, Friendly Permissions Setup
                // ======================================================
                Controls.ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: parent.width
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Kirigami.Icon { width: 28; height: 28; source: "security-high"; color: Theme.accent; isMask: true }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; font.bold: true; font.pixelSize: 16; color: Theme.textPrimary; text: I18n.tr(mouseController.language, "tourPermHeadline") }
                                Controls.Label { Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 12; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "tourPermSub") }
                            }
                        }

                        GlassCard {
                            cardPadding: 12
                            Controls.CheckBox {
                                checked: onboardingDialog.wantDevice
                                onToggled: onboardingDialog.wantDevice = checked
                                text: I18n.tr(mouseController.language, "wizPermDevice") + (mouseController.deviceAccessAvailable ? " ✓" : "")
                            }
                            Controls.CheckBox {
                                checked: onboardingDialog.wantUdev
                                onToggled: onboardingDialog.wantUdev = checked
                                text: I18n.tr(mouseController.language, "wizPermUdev")
                            }
                            Controls.CheckBox {
                                checked: onboardingDialog.wantTray
                                onToggled: onboardingDialog.wantTray = checked
                                text: I18n.tr(mouseController.language, "wizPermTray") + (mouseController.trayPermissionAvailable ? " ✓" : "")
                            }
                            Controls.CheckBox {
                                checked: onboardingDialog.wantHost
                                onToggled: onboardingDialog.wantHost = checked
                                text: I18n.tr(mouseController.language, "wizPermHost") + (mouseController.hostAccessAvailable ? " ✓" : "")
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                visible: onboardingDialog.wantHost
                                implicitHeight: onbHostWarnRow.implicitHeight + 16
                                radius: Theme.buttonRadius
                                color: Qt.rgba(0.94, 0.27, 0.27, 0.12)
                                border.width: 1.2
                                border.color: "#f59e0b"

                                RowLayout {
                                    id: onbHostWarnRow
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 8
                                    Kirigami.Icon { width: 20; height: 20; source: "dialog-warning"; color: "#f59e0b"; isMask: true }
                                    Controls.Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                        font.pixelSize: 12
                                        color: "#fde68a"
                                        text: I18n.tr(mouseController.language, "hostPermSecurityWarn")
                                    }
                                }
                            }
                        }

                        GlassCard {
                            cardPadding: 12
                            Controls.Label { font.bold: true; text: I18n.tr(mouseController.language, "wizGeneratedCmdLabel") }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: Math.min(96, cmdTxt.implicitHeight + 16)
                                radius: Theme.buttonRadius
                                color: Theme.inputBg
                                border.width: 1
                                border.color: Theme.borderSubtle
                                Controls.Label {
                                    id: cmdTxt
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    wrapMode: Text.WrapAnywhere
                                    font.family: "monospace"
                                    font.pixelSize: 11
                                    text: onboardingDialog.generatedCommand.length > 0 ? onboardingDialog.generatedCommand : "—"
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                ModernButton {
                                    highlighted: true
                                    icon.name: "edit-copy"
                                    text: I18n.tr(mouseController.language, "copyCmdBtn")
                                    enabled: onboardingDialog.generatedCommand.length > 0
                                    onClicked: {
                                        mouseController.copyCommandToClipboard(onboardingDialog.generatedCommand)
                                        onboardingDialog.copiedMsg = I18n.tr(mouseController.language, "copiedFeedback")
                                        onbCopyTimer.restart()
                                    }
                                }
                                ModernButton {
                                    icon.name: "help-about"
                                    text: I18n.tr(mouseController.language, "explainCmdBtn")
                                    enabled: onboardingDialog.generatedCommand.length > 0
                                    onClicked: onboardingDialog.explainCommandsRequested(onboardingDialog.wantDevice, onboardingDialog.wantTray, onboardingDialog.wantHost, onboardingDialog.wantUdev)
                                }
                                ModernButton {
                                    icon.name: "view-refresh"
                                    text: I18n.tr(mouseController.language, "recheckBtn")
                                    onClicked: mouseController.refreshAppPermissions()
                                }
                            }

                            Kirigami.InlineMessage {
                                Layout.fillWidth: true
                                visible: onboardingDialog.copiedMsg.length > 0
                                type: Kirigami.MessageType.Positive
                                text: onboardingDialog.copiedMsg
                            }

                            Controls.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                opacity: 0.72
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: I18n.tr(mouseController.language, "wizRestartHint")
                            }
                        }
                    }
                }
            }

            Kirigami.Separator { Layout.fillWidth: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ModernButton {
                    text: I18n.tr(mouseController.language, "prevBtn")
                    icon.name: mouseController.language === "ar" ? "go-next" : "go-previous"
                    enabled: onboardingDialog.currentStep > 0
                    onClicked: onboardingDialog.currentStep -= 1
                }

                Item { Layout.fillWidth: true }

                ModernButton {
                    visible: onboardingDialog.currentStep < onboardingDialog.totalSteps - 1
                    text: I18n.tr(mouseController.language, "nextBtn")
                    icon.name: mouseController.language === "ar" ? "go-previous" : "go-next"
                    highlighted: true
                    onClicked: onboardingDialog.currentStep += 1
                }

                ModernButton {
                    visible: onboardingDialog.currentStep === onboardingDialog.totalSteps - 1
                    text: I18n.tr(mouseController.language, "finishBtn")
                    icon.name: "dialog-ok-apply"
                    highlighted: true
                    onClicked: {
                        mouseController.firstRunCompleted = true
                        onboardingDialog.close()
                    }
                }
            }
        }
    }
}
