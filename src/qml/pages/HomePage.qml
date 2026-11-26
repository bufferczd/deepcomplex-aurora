import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.ScrollView {
    id: mainPage
    clip: true
    contentWidth: availableWidth
    padding: 18

    property color accentColor: "#15803d"
    property bool windowVisible: true

    signal openMacroStudioRequested()
    signal openSettingsRequested()
    signal openPermissionsSetupRequested()
    signal openFlatpakAppsRequested()
    signal openExtraModeZonesRequested()
    signal openMacroPickerRequested(int slot, string buttonTitle)
    signal pickColorRequested(string target, int index, color currentColor)
    signal guardActionRequested(var actionFn)
    signal confirmResetRequested(string messageText, var actionFn)

    readonly property var ledModeModel: I18n.buildLedModeModel(
        mouseController.language,
        mouseController.showHiddenEffects,
        mouseController.showHexCodes
    )
    readonly property var buttonActionModel: I18n.buildButtonActionModel(
        mouseController.language,
        mouseController.showHexCodes,
        mouseController.enableKeyCapture
    )
    readonly property var directionModel: [
        I18n.tr(mouseController.language, "dir1"),
        I18n.tr(mouseController.language, "dir2"),
        I18n.tr(mouseController.language, "dir3"),
        I18n.tr(mouseController.language, "dir4")
    ]
    readonly property var reactiveEffectModel: [
        I18n.tr(mouseController.language, "rEff0"),
        I18n.tr(mouseController.language, "rEff1"),
        I18n.tr(mouseController.language, "rEff2"),
        I18n.tr(mouseController.language, "rEff3")
    ]

    LayoutMirroring.enabled: mouseController.language === "ar"
    LayoutMirroring.childrenInherit: true

    ColumnLayout {
        width: mainPage.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: !mouseController.deviceAccessAvailable
            type: Kirigami.MessageType.Warning
            text: I18n.tr(mouseController.language, "deviceDisabledBanner")
            actions: [
                Kirigami.Action {
                    text: I18n.tr(mouseController.language, "openSetupBtn")
                    icon.name: "security-high"
                    onTriggered: mainPage.openPermissionsSetupRequested()
                }
            ]
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: mouseController.deviceAccessAvailable && mouseController.statusMessage.length > 0
            text: mouseController.statusMessage
            type: (!mouseController.connected || mouseController.statusError)
                  ? Kirigami.MessageType.Error
                  : Kirigami.MessageType.Positive
        }

        // بطاقة معلومات الجهاز
        GlassCard {
            activeGlow: mouseController.connected
            accentColor: mainPage.accentColor
            Layout.fillWidth: true
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                ThemedAppIcon {
                    iconSize: 48
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 2
                        text: "Delux M700A (Instant A725)"
                        elide: Text.ElideRight
                    }

                    Controls.Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        opacity: 0.78
                        text: mouseController.connected
                              ? I18n.tr(mouseController.language, "connected") + " • 30FA:1040 • " + mouseController.devicePath
                                + " • " + I18n.tr(mouseController.language, "profile") + " " + (mouseController.activeProfile + 1)
                                + " (" + mouseController.activeProfileName + ")"
                              : (!mouseController.deviceAccessAvailable
                                 ? I18n.tr(mouseController.language, "deviceIsolatedShort")
                                 : I18n.tr(mouseController.language, "disconnected"))
                    }
                }
            }
        }

        // بطاقة البروفايلات الـ 5 المستقلة
        GlassCard {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 3
                        text: I18n.tr(mouseController.language, "profilesTitle")
                        elide: Text.ElideRight
                    }

                    Controls.Switch {
                        text: I18n.tr(mouseController.language, "autoApply")
                        checked: mouseController.autoApplyOnSwitch
                        onToggled: mouseController.autoApplyOnSwitch = checked
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: 5

                        ModernButton {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            highlighted: mouseController.activeProfile === index
                            text: (index + 1) + " • " + (mouseController.profileNames[index] || (I18n.tr(mouseController.language, "profile") + " " + (index + 1)))
                            onClicked: { if (mouseController.activeProfile !== index) { if (mouseController.autoApplyOnSwitch) { mouseController.selectProfile(index) } else { mainPage.guardActionRequested(function() { mouseController.selectProfile(index) }) } } }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Controls.Label {
                        text: I18n.tr(mouseController.language, "profileNameLabel")
                    }

                    Controls.TextField {
                        id: profileNameField
                        Layout.fillWidth: true
                        placeholderText: I18n.tr(mouseController.language, "profileNamePh")
                        maximumLength: 24
                        Component.onCompleted: text = mouseController.activeProfileName
                        Connections {
                            target: mouseController
                            function onActiveProfileChanged() {
                                profileNameField.text = mouseController.activeProfileName
                            }
                            function onProfilesMetaChanged() {
                                if (!profileNameField.activeFocus) {
                                    profileNameField.text = mouseController.activeProfileName
                                }
                            }
                        }
                        onTextEdited: mouseController.setActiveProfileName(text)
                    }

                    ModernButton {
                        text: I18n.tr(mouseController.language, "resetProfile")
                        icon.name: "edit-undo"
                        onClicked: mainPage.confirmResetRequested(I18n.tr(mouseController.language, "confirmResetProfileMsg"), function() { mouseController.resetActiveProfile() })
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: mouseController.hostAccessAvailable && mouseController.autoSwitchByApp
                    spacing: 8

                    Controls.Label {
                        text: I18n.tr(mouseController.language, "linkedAppsLabel")
                    }

                    Controls.TextField {
                        id: linkedAppsField
                        Layout.fillWidth: true
                        maximumLength: 512
                        placeholderText: I18n.tr(mouseController.language, "linkedAppsPh")
                        Component.onCompleted: text = mouseController.activeProfileLinkedApps
                        Connections {
                            target: mouseController
                            function onActiveProfileChanged() {
                                linkedAppsField.text = mouseController.activeProfileLinkedApps
                            }
                            function onProfilesMetaChanged() {
                                if (!linkedAppsField.activeFocus) {
                                    linkedAppsField.text = mouseController.activeProfileLinkedApps
                                }
                            }
                        }
                        onTextEdited: mouseController.setActiveProfileLinkedApps(text)
                    }

                    ModernButton {
                        text: I18n.tr(mouseController.language, "pickFlatpakBtn")
                        icon.name: "view-list-icons"
                        highlighted: true
                        onClicked: mainPage.openFlatpakAppsRequested()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: mouseController.hostAccessAvailable && mouseController.autoSwitchByApp
                    spacing: 8

                    Controls.Label {
                        text: I18n.tr(mouseController.language, "fallbackProfileLabel")
                    }

                    Controls.ComboBox {
                        id: fallbackProfileCombo
                        Layout.fillWidth: true
                        model: [
                            "1 • " + (mouseController.profileNames[0] || (I18n.tr(mouseController.language, "profile") + " 1")),
                            "2 • " + (mouseController.profileNames[1] || (I18n.tr(mouseController.language, "profile") + " 2")),
                            "3 • " + (mouseController.profileNames[2] || (I18n.tr(mouseController.language, "profile") + " 3")),
                            "4 • " + (mouseController.profileNames[3] || (I18n.tr(mouseController.language, "profile") + " 4")),
                            "5 • " + (mouseController.profileNames[4] || (I18n.tr(mouseController.language, "profile") + " 5"))
                        ]
                        Component.onCompleted: currentIndex = mouseController.fallbackProfileIndex
                        onModelChanged: currentIndex = mouseController.fallbackProfileIndex
                        Connections {
                            target: mouseController
                            function onAppSettingsChanged() {
                                fallbackProfileCombo.currentIndex = mouseController.fallbackProfileIndex
                            }
                        }
                        onActivated: (idx) => mouseController.fallbackProfileIndex = idx
                    }
                }
            }
        }

        ModernSegmentedTabs {
            id: mainTabs
            accentColor: mainPage.accentColor
            rtl: mouseController.language === "ar"
            tabs: [
                { label: I18n.tr(mouseController.language, "tabDpi"), icon: "speedometer" },
                { label: I18n.tr(mouseController.language, "tabLed") + " (" + mainPage.ledModeModel.length + ")", icon: "color-management" },
                { label: I18n.tr(mouseController.language, "tabButtons"), icon: "input-mouse" }
            ]
            onCurrentIndexChanged: {
                if (mouseController.animatedUi) {
                    tabRevealAnim.restart()
                }
            }
        }

        StackLayout {
            id: tabStack
            Layout.fillWidth: true
            currentIndex: mainTabs.currentIndex
            transform: Translate { id: tabSlideTransform; y: 0 }

            ParallelAnimation {
                id: tabRevealAnim
                NumberAnimation { target: tabStack; property: "opacity"; from: 0.0; to: 1.0; duration: 240; easing.type: Easing.OutCubic }
                NumberAnimation { target: tabSlideTransform; property: "y"; from: 14; to: 0; duration: 260; easing.type: Easing.OutCubic }
            }

            // ==========================================
            // التبويب 1: مستويات DPI الستة وألوانها
            // ==========================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: I18n.isDpiColorUnsupportedMode(mouseController.ledMode)
                    type: Kirigami.MessageType.Warning
                    text: I18n.isExtraMode(mouseController.ledMode)
                          ? I18n.tr(mouseController.language, "dpiColorWarnExtra")
                          : I18n.tr(mouseController.language, "dpiColorWarnOff")
                    actions: [
                        Kirigami.Action {
                            text: I18n.tr(mouseController.language, "switchToStaticBtn")
                            icon.name: "color-management"
                            onTriggered: mouseController.ledMode = 0x02
                        }
                    ]
                }

                Repeater {
                    model: 6

                    GlassCard {
                        Layout.fillWidth: true
                        property int lvl: index + 1
                        property bool isActive: mouseController.activeDpiLevel === lvl
                        property int currentIdx: mouseController.dpiIndices[index]
                        property color currentColor: mouseController.dpiColors[index]
                        property bool isEnabled: mouseController.dpiEnabled[index]
                        activeGlow: isActive
                        accentColor: currentColor

                        RowLayout {
                Layout.fillWidth: true
                            spacing: Kirigami.Units.largeSpacing

                            Controls.RadioButton {
                                checked: isActive
                                text: "DPI " + lvl
                                font.bold: isActive
                                onClicked: mouseController.activeDpiLevel = lvl
                            }

                            Controls.Switch {
                                checked: isEnabled
                                onToggled: mouseController.setDpiEnabled(index, checked)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Controls.Label {
                                    text: mouseController.dpiLabelForIndex(currentIdx)
                                    font.bold: true
                                }

                                Controls.Slider {
                                    Layout.fillWidth: true
                                    from: 0
                                    to: 16
                                    stepSize: 1
                                    snapMode: Controls.Slider.SnapAlways
                                    value: currentIdx
                                    enabled: isEnabled
                                    onMoved: mouseController.setDpiIndex(index, Math.round(value))
                                }
                            }

                            Rectangle {
                                width: 34
                                height: 34
                                radius: 17
                                color: currentColor
                                border.width: 2
                                border.color: Kirigami.Theme.textColor

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        mainPage.pickColorRequested("dpi", index, currentColor)
                                    }
                                }
                            }

                            ModernButton {
                                text: I18n.tr(mouseController.language, "editColor")
                                icon.name: "color-picker"
                                onClicked: {
                                    mainPage.pickColorRequested("dpi", index, currentColor)
                                }
                            }
                        }
                    }
                }
            }

            // ==========================================
            // التبويب 2: الإضاءة والتأثيرات
            // ==========================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                // شريط معاينة حي وسلس لإضاءة RGB (اختياري من الإعدادات)
                Rectangle {
                    id: rgbPreviewContainer
                    Layout.fillWidth: true
                    implicitHeight: 10
                    radius: 5
                    clip: true
                    visible: mouseController.showRgbPreviewStrip
                    color: Qt.rgba(1, 1, 1, 0.07)
                    opacity: I18n.isOffMode(mouseController.ledMode) ? 0.18 : (0.38 + (mouseController.ledBrightness / 6.0) * 0.62)
                    border.width: 1
                    border.color: Qt.rgba(1, 1, 1, 0.16)
                    Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

                    readonly property bool isOff: I18n.isOffMode(mouseController.ledMode)
                    readonly property bool isStatic: I18n.isStaticMode(mouseController.ledMode)
                    readonly property color c0: mouseController.userColors[0] || "#16a34a"
                    readonly property color c1: mouseController.userColors[1] || "#06b6d4"
                    readonly property color c2: mouseController.userColors[3] || "#3b82f6"
                    readonly property color c3: mouseController.userColors[4] || "#a855f7"
                    readonly property color c4: mouseController.userColors[6] || "#ec4899"

                    // Seamless 2x tiled gradient (identical stops at 0.00 / 0.50 / 1.00 => zero jump on loop!)
                    Rectangle {
                        id: rgbPreviewStrip
                        width: Math.max(2, parent.width * 2)
                        height: parent.height
                        y: 0
                        property real phase: 0.0
                        x: -phase * parent.width

                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.00; color: rgbPreviewContainer.c0 }
                            GradientStop { position: 0.10; color: rgbPreviewContainer.c1 }
                            GradientStop { position: 0.20; color: rgbPreviewContainer.c2 }
                            GradientStop { position: 0.30; color: rgbPreviewContainer.c3 }
                            GradientStop { position: 0.40; color: rgbPreviewContainer.c4 }
                            GradientStop { position: 0.50; color: rgbPreviewContainer.c0 }
                            GradientStop { position: 0.60; color: rgbPreviewContainer.c1 }
                            GradientStop { position: 0.70; color: rgbPreviewContainer.c2 }
                            GradientStop { position: 0.80; color: rgbPreviewContainer.c3 }
                            GradientStop { position: 0.90; color: rgbPreviewContainer.c4 }
                            GradientStop { position: 1.00; color: rgbPreviewContainer.c0 }
                        }

                        NumberAnimation on phase {
                            running: mainPage.windowVisible && mainPage.visible && mainTabs.currentIndex === 1 &&
                                     rgbPreviewContainer.visible && mouseController.animatedUi &&
                                     !rgbPreviewContainer.isStatic && !rgbPreviewContainer.isOff
                            loops: Animation.Infinite
                            from: 0.0
                            to: 1.0
                            easing.type: Easing.Linear
                            duration: Math.max(1800, 6200 - mouseController.ledSpeed * 820)
                        }
                    }
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: I18n.isDpiColorUnsupportedMode(mouseController.ledMode)
                    type: Kirigami.MessageType.Warning
                    text: I18n.isExtraMode(mouseController.ledMode)
                          ? I18n.tr(mouseController.language, "dpiColorWarnExtra")
                          : I18n.tr(mouseController.language, "dpiColorWarnOff")
                    actions: [
                        Kirigami.Action {
                            text: I18n.tr(mouseController.language, "switchToStaticBtn")
                            icon.name: "color-management"
                            onTriggered: mouseController.ledMode = 0x02
                        }
                    ]
                }

                GlassCard {
                    Layout.fillWidth: true
                    Kirigami.FormLayout {
                        Layout.fillWidth: true

                        Kirigami.Heading {
                            Kirigami.FormData.isSection: true
                            level: 3
                            text: I18n.tr(mouseController.language, "ledSection")
                        }

                        Controls.ComboBox {
                            id: ledModeCombo
                            Kirigami.FormData.label: I18n.tr(mouseController.language, "ledModeLabel")
                            Layout.fillWidth: true
                            textRole: "label"
                            model: mainPage.ledModeModel
                            Component.onCompleted: currentIndex = I18n.indexForLedMode(mainPage.ledModeModel, mouseController.ledMode)
                            onModelChanged: currentIndex = I18n.indexForLedMode(mainPage.ledModeModel, mouseController.ledMode)
                            Connections {
                                target: mouseController
                                function onStateChanged() {
                                    ledModeCombo.currentIndex = I18n.indexForLedMode(mainPage.ledModeModel, mouseController.ledMode)
                                }
                            }
                            onActivated: (idx) => {
                                let chosenMode = mainPage.ledModeModel[idx].mode
                                mouseController.ledMode = chosenMode
                                if (I18n.isExtraMode(chosenMode)) {
                                    mainPage.openExtraModeZonesRequested()
                                }
                            }
                        }

                        ModernButton {
                            visible: I18n.isExtraMode(mouseController.ledMode)
                            Layout.fillWidth: true
                            text: I18n.tr(mouseController.language, "zoneMapBtn")
                            icon.name: "color-picker"
                            highlighted: true
                            onClicked: mainPage.openExtraModeZonesRequested()
                        }

                        RowLayout {
                            Kirigami.FormData.label: I18n.tr(mouseController.language, "brightnessLabel")
                            Layout.fillWidth: true

                            Controls.Slider {
                                Layout.fillWidth: true
                                from: 0
                                to: 6
                                stepSize: 1
                                snapMode: Controls.Slider.SnapAlways
                                enabled: !I18n.isOffMode(mouseController.ledMode)
                                value: mouseController.ledBrightness
                                onMoved: mouseController.ledBrightness = Math.round(value)
                            }

                            Controls.Label {
                                text: I18n.isOffMode(mouseController.ledMode)
                                      ? I18n.tr(mouseController.language, "offText")
                                      : (Math.round((mouseController.ledBrightness / 6.0) * 100) + "%")
                                Layout.minimumWidth: 52
                            }
                        }

                        RowLayout {
                            Kirigami.FormData.label: I18n.tr(mouseController.language, "speedLabel")
                            Layout.fillWidth: true

                            Controls.Slider {
                                Layout.fillWidth: true
                                from: 0
                                to: 5
                                stepSize: 1
                                snapMode: Controls.Slider.SnapAlways
                                enabled: !I18n.isStaticMode(mouseController.ledMode)
                                value: mouseController.ledSpeed
                                onMoved: mouseController.ledSpeed = Math.round(value)
                            }

                            Controls.Label {
                                text: I18n.isStaticMode(mouseController.ledMode)
                                      ? "—"
                                      : (I18n.tr(mouseController.language, "levelPrefix") + (mouseController.ledSpeed + 1) + "/6")
                                Layout.minimumWidth: 68
                            }
                        }

                        Controls.ComboBox {
                            id: ledDirCombo
                            Kirigami.FormData.label: I18n.tr(mouseController.language, "directionLabel")
                            Layout.fillWidth: true
                            enabled: !I18n.isStaticMode(mouseController.ledMode)
                            model: mainPage.directionModel
                            Component.onCompleted: currentIndex = mouseController.ledDirection
                            onModelChanged: currentIndex = mouseController.ledDirection
                            Connections {
                                target: mouseController
                                function onStateChanged() {
                                    ledDirCombo.currentIndex = mouseController.ledDirection
                                }
                            }
                            onActivated: (idx) => mouseController.ledDirection = idx
                        }

                        Controls.Switch {
                            Kirigami.FormData.label: I18n.tr(mouseController.language, "symmetryLabel")
                            enabled: !I18n.isStaticMode(mouseController.ledMode)
                            checked: mouseController.ledSymmetry
                            onToggled: mouseController.ledSymmetry = checked
                        }

                        Controls.Switch {
                            Kirigami.FormData.label: I18n.tr(mouseController.language, "singleColorLabel")
                            enabled: !I18n.isOffMode(mouseController.ledMode)
                            checked: mouseController.ledSingleColor
                            onToggled: mouseController.ledSingleColor = checked
                        }
                    }
                }

                GlassCard {
                    Layout.fillWidth: true
                    visible: I18n.isReactiveMode(mouseController.ledMode)
                    ColumnLayout {
                Layout.fillWidth: true
                        spacing: Kirigami.Units.mediumSpacing

                        Kirigami.Heading {
                            level: 3
                            text: I18n.tr(mouseController.language, "reactiveTitle")
                        }

                        Kirigami.FormLayout {
                            Layout.fillWidth: true

                            Controls.ComboBox {
                                id: reactiveEffectCombo
                                Kirigami.FormData.label: I18n.tr(mouseController.language, "reactiveEffectLabel")
                                Layout.fillWidth: true
                                model: mainPage.reactiveEffectModel
                                Component.onCompleted: currentIndex = mouseController.reactiveEffect
                                onModelChanged: currentIndex = mouseController.reactiveEffect
                                Connections {
                                    target: mouseController
                                    function onStateChanged() {
                                        reactiveEffectCombo.currentIndex = mouseController.reactiveEffect
                                    }
                                }
                                onActivated: (idx) => mouseController.reactiveEffect = idx
                            }
                        }

                        Controls.Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: I18n.tr(mouseController.language, "reactiveButtonsLabel")
                            opacity: 0.8
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 4
                            columnSpacing: 12
                            rowSpacing: 6

                            Repeater {
                                model: [
                                    { key: "bLeft", slot: 0 },
                                    { key: "bRight", slot: 2 },
                                    { key: "bMiddle", slot: 1 },
                                    { key: "bBack", slot: 3 },
                                    { key: "bForward", slot: 4 },
                                    { key: "bDpiMinus", slot: 5 },
                                    { key: "bDpiPlus", slot: 7 }
                                ]
                                Controls.CheckBox {
                                    text: I18n.tr(mouseController.language, modelData.key)
                                    checked: mouseController.reactiveButtons[modelData.slot]
                                    onToggled: mouseController.setReactiveButtonEnabled(modelData.slot, checked)
                                }
                            }
                        }
                    }
                }

                GlassCard {
                    Layout.fillWidth: true
                    enabled: !I18n.isExtraMode(mouseController.ledMode)
                    opacity: I18n.isExtraMode(mouseController.ledMode) ? 0.42 : 1.0
                    ColumnLayout {
                Layout.fillWidth: true
                        spacing: Kirigami.Units.mediumSpacing

                        RowLayout {
                            Layout.fillWidth: true
                            Kirigami.Heading {
                                Layout.fillWidth: true
                                level: 3
                                text: I18n.tr(mouseController.language, "userColorsTitle")
                                elide: Text.ElideRight
                            }
                            Controls.Switch {
                                text: I18n.tr(mouseController.language, "userColorsEnable")
                                checked: mouseController.userColorEnabled
                                onToggled: mouseController.userColorEnabled = checked
                            }
                        }

                        Controls.Label {
                            Layout.fillWidth: true
                            visible: I18n.isExtraMode(mouseController.ledMode)
                            wrapMode: Text.WordWrap
                            font.italic: true
                            text: I18n.tr(mouseController.language, "extraModeDisabledHint")
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            enabled: mouseController.userColorEnabled

                            Repeater {
                                model: 7

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Rectangle {
                                        Layout.alignment: Qt.AlignHCenter
                                        width: 44
                                        height: 44
                                        radius: 22
                                        color: mouseController.userColors[index]
                                        opacity: mouseController.userColorSlotsEnabled[index] ? 1.0 : 0.3
                                        border.width: 2
                                        border.color: Kirigami.Theme.textColor

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                mainPage.pickColorRequested("user", index, mouseController.userColors[index])
                                            }
                                        }
                                    }

                                    Controls.CheckBox {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: (index + 1).toString()
                                        checked: mouseController.userColorSlotsEnabled[index]
                                        onToggled: mouseController.setUserColorSlotEnabled(index, checked)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ==========================================
            // التبويب 3: تخصيص الأزرار السبعة + التقاط الأزرار والماكرو
            // ==========================================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                GlassCard {
                    Layout.fillWidth: true
                    ColumnLayout {
                Layout.fillWidth: true
                        spacing: Kirigami.Units.mediumSpacing

                        RowLayout {
                            Layout.fillWidth: true
                            Kirigami.Heading {
                                Layout.fillWidth: true
                                level: 3
                                text: I18n.tr(mouseController.language, "buttonsTitle")
                                elide: Text.ElideRight
                            }
                            ModernButton {
                                text: I18n.tr(mouseController.language, "resetButtons")
                                icon.name: "edit-undo"
                                onClicked: mainPage.confirmResetRequested(I18n.tr(mouseController.language, "confirmResetButtonsMsg"), function() { mouseController.resetDefaultButtons() })
                            }
                        }

                        Kirigami.FormLayout {
                            Layout.fillWidth: true

                            Repeater {
                                model: [
                                    { key: "k1", slot: 0 },
                                    { key: "k2", slot: 2 },
                                    { key: "k3", slot: 1 },
                                    { key: "k4", slot: 4 },
                                    { key: "k5", slot: 3 },
                                    { key: "k6", slot: 7 },
                                    { key: "k7", slot: 5 }
                                ]

                                RowLayout {
                                    id: btnRow
                                    Kirigami.FormData.label: I18n.tr(mouseController.language, modelData.key)
                                    Layout.fillWidth: true
                                    spacing: 8

                                    property int slotIndex: modelData.slot
                                    property int currentCode: mouseController.buttonCodes[slotIndex]
                                    property bool isMacroSelected: I18n.isMacroCode(currentCode) && mouseController.enableKeyCapture

                                    Controls.ComboBox {
                                        id: btnCombo
                                        Layout.fillWidth: true
                                        textRole: "label"
                                        model: mainPage.buttonActionModel

                                        Component.onCompleted: {
                                            currentIndex = I18n.indexForButtonAction(mainPage.buttonActionModel, 
                                                mouseController.buttonCodes[btnRow.slotIndex],
                                                mouseController.buttonMediaUsages[btnRow.slotIndex]
                                            )
                                        }
                                        onModelChanged: {
                                            currentIndex = I18n.indexForButtonAction(mainPage.buttonActionModel, 
                                                mouseController.buttonCodes[btnRow.slotIndex],
                                                mouseController.buttonMediaUsages[btnRow.slotIndex]
                                            )
                                        }
                                        Connections {
                                            target: mouseController
                                            function onStateChanged() {
                                                btnCombo.currentIndex = I18n.indexForButtonAction(mainPage.buttonActionModel, 
                                                    mouseController.buttonCodes[btnRow.slotIndex],
                                                    mouseController.buttonMediaUsages[btnRow.slotIndex]
                                                )
                                            }
                                        }
                                        onActivated: (idx) => {
                                            let item = mainPage.buttonActionModel[idx]
                                            if (item.code === 0x0F && item.mediaUsage > 0) {
                                                mouseController.setButtonMediaUsage(btnRow.slotIndex, item.mediaUsage)
                                            } else {
                                                mouseController.setButtonCode(btnRow.slotIndex, item.code)
                                                if (I18n.isMacroCode(item.code) && mouseController.enableKeyCapture) {
                                                    mainPage.openMacroPickerRequested(btnRow.slotIndex, I18n.tr(mouseController.language, modelData.key))
                                                }
                                            }
                                        }
                                    }

                                    ModernButton {
                                        visible: btnRow.isMacroSelected
                                        icon.name: "input-keyboard"
                                        text: I18n.tr(mouseController.language, "chooseMacroBtn") + " • " + (mouseController.buttonCodes ? mouseController.assignedMacroNameForButton(btnRow.slotIndex) : "")
                                        onClicked: mainPage.openMacroPickerRequested(btnRow.slotIndex, I18n.tr(mouseController.language, modelData.key))
                                    }
                                }
                            }
                        }
                    }
                }

                GlassCard {
                    Layout.fillWidth: true
                    Kirigami.FormLayout {
                        Layout.fillWidth: true

                        Kirigami.Heading {
                            Kirigami.FormData.isSection: true
                            level: 3
                            text: I18n.tr(mouseController.language, "fireTitle")
                        }

                        RowLayout {
                            Kirigami.FormData.label: I18n.tr(mouseController.language, "fireLabel")
                            Layout.fillWidth: true

                            Controls.Slider {
                                Layout.fillWidth: true
                                from: 8
                                to: 200
                                stepSize: 8
                                snapMode: Controls.Slider.SnapAlways
                                value: mouseController.fireTimeMs
                                onMoved: mouseController.fireTimeMs = Math.round(value)
                            }

                            Controls.Label {
                                text: mouseController.fireTimeMs + " ms"
                                Layout.minimumWidth: 56
                            }
                        }
                    }
                }
            }
        }
    }
}
