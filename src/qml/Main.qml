import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import org.kde.kirigami as Kirigami

Controls.ApplicationWindow {
    id: root
    title: "DeepComplex Aurora — Delux M700A"
    width: 1140
    height: 850
    minimumWidth: 880
    minimumHeight: 620
    visible: false

    // True transparent frameless shell (background: Item {} removes opaque window backing!)
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    background: Item {}

    readonly property bool isMaximized: root.visibility === Window.Maximized || root.visibility === Window.FullScreen
    readonly property int effectiveWindowRadius: (isMaximized || Theme.windowShape === "sharp") ? 0 : Theme.windowRadius
    readonly property bool isRtl: mouseController.language === "ar"

    // Reactive Kirigami & Qt Palette synced with Theme singleton (supports Dark & Light Frost presets)
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.backgroundColor: Theme.bgMid
    Kirigami.Theme.alternateBackgroundColor: Theme.isLight ? "#EDF3FA" : "#141D33"
    Kirigami.Theme.textColor: Theme.textPrimary
    Kirigami.Theme.disabledTextColor: Theme.textMuted
    Kirigami.Theme.highlightColor: Theme.accent

    palette.window: Theme.bgMid
    palette.windowText: Theme.textPrimary
    palette.base: Theme.inputBg
    palette.alternateBase: Theme.isLight ? "#E2ECF8" : "#141D33"
    palette.text: Theme.textPrimary
    palette.button: Theme.buttonBg
    palette.buttonText: Theme.textPrimary
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentContrastText

    property int currentNavIndex: 0 // 0 = Dashboard, 1 = Macro Studio, 2 = Settings
    property string pickerTarget: "dpi"
    property int pickerIndex: 0
    property int exportMacroIndex: -1
    property bool bootSplashActive: mouseController.showStartupSplash &&
                                    !(mouseController.launchedFromAutostart &&
                                      mouseController.startMinimized &&
                                      mouseController.trayAvailable)
    property bool pendingUdevRepair: false

    function syncTheme() {
        Theme.presetId = mouseController.uiThemePreset
        Theme.accentMode = mouseController.uiAccentMode
        Theme.customAccent = mouseController.uiCustomAccent
        Theme.glassOpacity = mouseController.uiGlassOpacity
        Theme.cornerRadius = mouseController.uiCornerRadius
        Theme.glowIntensity = mouseController.uiGlowIntensity
        Theme.backgroundStyle = mouseController.uiBackgroundStyle
        Theme.animatedUi = mouseController.animatedUi
        Theme.trueBlackOled = mouseController.uiTrueBlackOled
        Theme.borderWidth = mouseController.uiBorderWidth
        Theme.animationSpeed = mouseController.uiAnimationSpeed
        Theme.compactMode = mouseController.uiCompactMode
        Theme.useCustomSecondary = mouseController.uiUseCustomSecondary
        Theme.customSecondary = mouseController.uiCustomSecondary
        Theme.borderStyle = mouseController.uiBorderStyle
        Theme.customBorderColor = mouseController.uiCustomBorderColor
        Theme.borderOpacity = mouseController.uiBorderOpacity
        Theme.windowEdgeGlow = mouseController.uiWindowEdgeGlow
        Theme.windowButtonStyle = mouseController.uiWindowButtonStyle
        Theme.titlebarStyle = mouseController.uiTitlebarStyle
        Theme.buttonShape = mouseController.uiButtonShape
        Theme.buttonStyle = mouseController.uiButtonStyle
        Theme.cardEdgeStyle = mouseController.uiCardEdgeStyle
        Theme.windowShape = mouseController.uiWindowShape

        syncDpiAccentOnly()
    }

    // Lightweight DPI-only theme sync (avoids reassigning 25 theme properties during rapid DPI switching!)
    function syncDpiAccentOnly() {
        let nextColor = Theme.currentPreset.accent
        if (themeDpiMonitor.ready) {
            let hc = themeDpiMonitor.dpiColor
            if (hc && (hc.r + hc.g + hc.b) > 0.12) {
                nextColor = hc
            }
        }
        if (Theme.dpiAccent !== nextColor) {
            Theme.dpiAccent = nextColor
        }
        mouseController.setActiveThemeAccent(Theme.accent)
    }

    function toggleMaximize() {
        if (root.isMaximized) {
            root.showNormal()
        } else {
            root.showMaximized()
        }
    }

    function guardUnsaved(actionFn) {
        if (mouseController.hasUnsavedChanges) {
            unsavedDialog.pendingAction = actionFn
            unsavedDialog.open()
        } else {
            actionFn()
        }
    }

    function confirmReset(messageText, actionFn) {
        resetConfirmDialog.messageText = messageText
        resetConfirmDialog.pendingResetAction = actionFn
        resetConfirmDialog.open()
    }

    function openColorPicker(target, idx, initialColor) {
        pickerTarget = target
        pickerIndex = idx
        colorDialog.selectedColor = initialColor
        colorDialog.open()
    }

    function navigateTo(index) {
        if (index === 1 && !mouseController.enableKeyCapture) return
        if (currentNavIndex === index && mainStackView.depth > 0) return
        currentNavIndex = index
        let comp = homePageComponent
        if (index === 1) comp = macroStudioPageComponent
        else if (index === 2) comp = settingsPageComponent
        mainStackView.replace(comp)
    }

    Component.onCompleted: {
        syncTheme()
        const shouldStartHidden = mouseController.launchedFromAutostart &&
                                  mouseController.startMinimized &&
                                  mouseController.trayAvailable
        if (!shouldStartHidden) {
            if (mouseController.showStartupSplash) {
                root.bootSplashActive = true
                if (progressLogDialog.visible) progressLogDialog.close()
                startupSplash.play()
            } else {
                root.bootSplashActive = false
            }
            root.show()
        } else {
            root.bootSplashActive = false
        }
        if (!mouseController.showStartupSplash && !mouseController.firstRunCompleted && !mouseController.launchedFromAutostart) {
            onboardingDialog.openAtStep(0)
        }
    }

    onClosing: (close) => {
        if (mouseController.minimizeToTray && mouseController.trayAvailable) {
            close.accepted = false
            root.hide()
        } else if (mouseController.hasUnsavedChanges) {
            close.accepted = false
            guardUnsaved(function() { Qt.quit() })
        } else {
            Qt.quit()
        }
    }

    Connections {
        target: Theme
        function onAccentChanged() { mouseController.setActiveThemeAccent(Theme.accent) }
    }

    Connections {
        target: themeDpiMonitor
        function onDpiStateChanged() { root.syncDpiAccentOnly() }
    }

    Connections {
        target: mouseController
        function onStateChanged() { root.syncTheme() }
        function onAppSettingsChanged() {
            root.syncTheme()
            if (!mouseController.enableKeyCapture && root.currentNavIndex === 1) {
                root.navigateTo(0)
            }
        }
        function onOperationStarted() {
            // NEVER let the "Reading from Mouse" progress dialog pop up before or over the Startup Logo!
            if (!root.visible || root.bootSplashActive || startupSplash.visible ||
                repairDialog.visible || onboardingDialog.visible || !mouseController.udevHealthy) {
                if (progressLogDialog.visible) progressLogDialog.close()
                return
            }
            if (mouseController.showProgressAndLog) progressLogDialog.open()
        }
        function onUdevMissingDetected() {
            if (root.bootSplashActive || startupSplash.visible) {
                root.pendingUdevRepair = true
                return
            }
            if (!mouseController.firstRunCompleted || onboardingDialog.visible || permissionsDialog.visible) return
            if (progressLogDialog.visible) progressLogDialog.close()
            if (!repairDialog.visible) repairDialog.open()
        }
    }

    // ==========================================
    // System File & Color Dialogs
    // ==========================================
    ColorDialog {
        id: colorDialog
        title: root.pickerTarget === "dpi"
               ? I18n.tr(mouseController.language, "pickDpiColor") + (root.pickerIndex + 1)
               : (root.pickerTarget === "extra"
                  ? I18n.tr(mouseController.language, "zone" + (root.pickerIndex + 1))
                  : (root.pickerTarget === "themeAccent"
                     ? I18n.tr(mouseController.language, "themePickCustomColor")
                     : (root.pickerTarget === "themeSecondary"
                        ? I18n.tr(mouseController.language, "themePickSecondaryBtn")
                        : (root.pickerTarget === "themeBorder"
                           ? I18n.tr(mouseController.language, "themePickBorderColor")
                           : I18n.tr(mouseController.language, "pickUserColor") + (root.pickerIndex + 1)))))
        onAccepted: {
            if (root.pickerTarget === "dpi") mouseController.setDpiColor(root.pickerIndex, selectedColor)
            else if (root.pickerTarget === "extra") mouseController.setExtraModeColor(root.pickerIndex, selectedColor)
            else if (root.pickerTarget === "themeAccent") {
                mouseController.uiAccentMode = "custom"
                mouseController.uiCustomAccent = selectedColor.toString()
            } else if (root.pickerTarget === "themeSecondary") {
                mouseController.uiUseCustomSecondary = true
                mouseController.uiCustomSecondary = selectedColor.toString()
            } else if (root.pickerTarget === "themeBorder") {
                mouseController.uiBorderStyle = "custom"
                mouseController.uiCustomBorderColor = selectedColor.toString()
            } else {
                mouseController.setUserColor(root.pickerIndex, selectedColor)
            }
        }
    }

    FileDialog {
        id: exportDialog
        title: I18n.tr(mouseController.language, "btnExport")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["JSON (*.json)"]
        onAccepted: mouseController.exportBackup(selectedFile)
    }

    FileDialog {
        id: importDialog
        title: I18n.tr(mouseController.language, "btnImport")
        fileMode: FileDialog.OpenFile
        nameFilters: ["JSON (*.json)"]
        onAccepted: mouseController.importBackup(selectedFile)
    }

    FileDialog {
        id: exportSingleMacroDialog
        title: I18n.tr(mouseController.language, "exportMacroBtn")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["DeepComplex Macro (*.json)"]
        onAccepted: {
            if (root.exportMacroIndex >= 0) mouseController.exportSingleMacro(root.exportMacroIndex, selectedFile)
        }
    }

    FileDialog {
        id: importSingleMacroDialog
        title: I18n.tr(mouseController.language, "importMacroBtn")
        fileMode: FileDialog.OpenFile
        nameFilters: ["DeepComplex Macro (*.json)"]
        onAccepted: mouseController.importSingleMacro(selectedFile)
    }

    // ==========================================
    // Modern Frosted-Glass Application Dialogs (All 12!)
    // ==========================================
    ThemeEditorDialog {
        id: themeDialog
        onPickCustomAccentRequested: (col) => root.openColorPicker("themeAccent", 0, col)
        onPickCustomSecondaryRequested: (col) => root.openColorPicker("themeSecondary", 0, col)
        onPickCustomBorderRequested: (col) => root.openColorPicker("themeBorder", 0, col)
    }

    ExtraModeZoneDialog {
        id: extraModeZoneDialog
        onPickColorRequested: (slot, col) => root.openColorPicker("extra", slot, col)
    }

    ResetConfirmDialog { id: resetConfirmDialog }
    UnsavedChangesDialog { id: unsavedDialog }
    CommandExplainDialog { id: commandExplainDialog }

    OnboardingDialog {
        id: onboardingDialog
        onExplainCommandsRequested: (d, t, h, u) => commandExplainDialog.openForSelection(d, t, h, u, true)
    }

    RepairDialog {
        id: repairDialog
        onRepairOpened: {
            if (progressLogDialog.visible) progressLogDialog.close()
        }
        onExplainUdevCommandRequested: commandExplainDialog.openForSelection(false, false, false, true, false)
    }

    ProgressLogDialog {
        id: progressLogDialog
        onOpenRepairRequested: repairDialog.open()
    }

    PermissionsDialog {
        id: permissionsDialog
        onExplainCommandsRequested: commandExplainDialog.openForSelection(true, true, true, true, true)
    }
    FlatpakAppsDialog { id: flatpakAppsDialog }

    MacroPickerDialog {
        id: macroPickerDialog
        onOpenStudioRequested: root.navigateTo(1)
    }

    MacroEditorDialog { id: macroDialog }

    // ==========================================
    // Modern Page Components
    // ==========================================
    Component {
        id: homePageComponent
        HomePage {
            accentColor: Theme.accent
            windowVisible: root.visible && !root.bootSplashActive && !startupSplash.visible
            onOpenMacroStudioRequested: root.navigateTo(1)
            onOpenSettingsRequested: root.navigateTo(2)
            onOpenPermissionsSetupRequested: onboardingDialog.openAtStep(onboardingDialog.totalSteps - 1)
            onOpenFlatpakAppsRequested: flatpakAppsDialog.open()
            onOpenExtraModeZonesRequested: extraModeZoneDialog.open()
            onOpenMacroPickerRequested: (slot, btnTitle) => macroPickerDialog.openForSlot(slot, btnTitle)
            onPickColorRequested: (target, idx, col) => root.openColorPicker(target, idx, col)
            onGuardActionRequested: (fn) => root.guardUnsaved(fn)
            onConfirmResetRequested: (msg, fn) => root.confirmReset(msg, fn)
        }
    }

    Component {
        id: macroStudioPageComponent
        MacroStudioPage {
            accentColor: Theme.accent
            onClosePageRequested: root.navigateTo(0)
            onNewMacroRequested: macroDialog.openForNewMacro()
            onEditMacroRequested: (idx) => macroDialog.openForLibraryIndex(idx)
            onExportMacroRequested: (idx) => {
                root.exportMacroIndex = idx
                exportSingleMacroDialog.open()
            }
            onImportMacroRequested: importSingleMacroDialog.open()
        }
    }

    Component {
        id: settingsPageComponent
        SettingsPage {
            accentColor: Theme.accent
            onClosePageRequested: root.navigateTo(0)
            onExportBackupRequested: exportDialog.open()
            onImportBackupRequested: importDialog.open()
            onOpenPermissionsRequested: permissionsDialog.open()
            onOpenThemeEditorRequested: themeDialog.open()
            onOpenOnboardingRequested: onboardingDialog.openAtStep(0)
            onReplaySplashRequested: {
                if (progressLogDialog.visible) progressLogDialog.close()
                root.bootSplashActive = true
                startupSplash.play()
            }
            onConfirmResetAllRequested: root.confirmReset(
                I18n.tr(mouseController.language, "confirmResetAllMsg"),
                function() { mouseController.resetAllSettingsAndProfiles() }
            )
        }
    }

    // ==========================================
    // Custom Rounded / Chamfered / Sharp Frameless Aurora Window Shell
    // ==========================================
    Item {
        id: outerWindowContainer
        anchors.fill: parent

        // Subtle outer ambient glow aura around window edges when unmaximized
        Rectangle {
            visible: !root.isMaximized && Theme.windowEdgeGlow && Theme.borderStyle !== "none" && Theme.windowShape !== "chamfer"
            anchors.fill: windowShell
            anchors.margins: -3
            radius: root.effectiveWindowRadius === 0 ? 0 : (root.effectiveWindowRadius + 3)
            color: Qt.rgba((Theme.borderStyle === "custom" ? Theme.customBorderColor : Theme.accent).r,
                           (Theme.borderStyle === "custom" ? Theme.customBorderColor : Theme.accent).g,
                           (Theme.borderStyle === "custom" ? Theme.customBorderColor : Theme.accent).b,
                           0.16 * Math.max(0.35, Theme.glowIntensity) * Theme.borderOpacity)
        }

        // Main Window Shell Container (Wayland-safe: zero offscreen mask mask, ALWAYS visible!)
        Item {
            id: windowShell
            anchors.fill: parent
            anchors.margins: root.isMaximized ? 0 : 4

            Item {
                id: windowContentLayer
                anchors.fill: parent
                clip: true
                opacity: (root.bootSplashActive || startupSplash.visible) ? 0.0 : 1.0
                Behavior on opacity { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }

                // 1. Live GPU-accelerated Animated Background (natively respects sharp/rounded/extra/chamfer!)
                AnimatedAuroraBackground {
                    anchors.fill: parent
                    running: root.visible && !root.bootSplashActive && !startupSplash.visible && Theme.animatedUi
                    accentColor: Theme.accent
                    styleMode: Theme.backgroundStyle
                    glowStrength: Theme.glowIntensity
                    cornerRadius: root.effectiveWindowRadius
                    chamfered: !root.isMaximized && Theme.windowShape === "chamfer"
                }

                // 2. Integrated Frameless Window Chrome + Split Workspace
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                // ---------- Custom Draggable Window Titlebar (shape-matched top corners) ----------
                Item {
                    id: customTitleBar
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.compactMode ? 42 : 48

                    // Standard rounded/sharp titlebar fill
                    Rectangle {
                        anchors.fill: parent
                        visible: root.isMaximized || Theme.windowShape !== "chamfer"
                        radius: root.effectiveWindowRadius
                        color: Theme.effectiveTitlebarBg

                        Rectangle {
                            visible: parent.radius > 0
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: parent.radius
                            color: parent.color
                        }
                    }

                    // Chamfered 45° top corners titlebar fill
                    Canvas {
                        id: chamferTitlebarCanvas
                        anchors.fill: parent
                        visible: !root.isMaximized && Theme.windowShape === "chamfer"
                        renderStrategy: Canvas.Cooperative
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()
                            ctx.clearRect(0, 0, width, height)
                            if (width < 6 || height < 6) return
                            var cut = Theme.windowChamferCut
                            ctx.beginPath()
                            ctx.moveTo(cut, 0)
                            ctx.lineTo(width - cut, 0)
                            ctx.lineTo(width, cut)
                            ctx.lineTo(width, height)
                            ctx.lineTo(0, height)
                            ctx.lineTo(0, cut)
                            ctx.closePath()
                            ctx.fillStyle = Theme.effectiveTitlebarBg.toString()
                            ctx.fill()
                        }
                        onVisibleChanged: if (visible) requestPaint()
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        Connections {
                            target: Theme
                            function onEffectiveTitlebarBgChanged() { chamferTitlebarCanvas.requestPaint() }
                            function onWindowShapeChanged() { chamferTitlebarCanvas.requestPaint() }
                            function onCornerRadiusChanged() { chamferTitlebarCanvas.requestPaint() }
                        }
                    }

                    // Drag to move window + double-click to maximize/restore
                    DragHandler {
                        target: null
                        onActiveChanged: {
                            if (active) root.startSystemMove()
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onDoubleClicked: root.toggleMaximize()
                        onPressed: (mouse) => {
                            if (mouse.button === Qt.LeftButton) root.startSystemMove()
                        }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: Theme.borderSubtle
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: (!root.isMaximized && Theme.windowShape === "chamfer") ? Math.max(18, Theme.windowChamferCut + 2) : 14
                        anchors.rightMargin: (!root.isMaximized && Theme.windowShape === "chamfer") ? Math.max(18, Theme.windowChamferCut + 2) : 14
                        spacing: 10
                        LayoutMirroring.enabled: root.isRtl
                        LayoutMirroring.childrenInherit: true

                        // Theme-Colored App Icon + Active Section Title
                        ThemedAppIcon {
                            iconSize: 24
                        }

                        Controls.Label {
                            font.bold: true
                            font.pixelSize: 14
                            color: Theme.textPrimary
                            text: "DeepComplex  •  " + (
                                root.currentNavIndex === 0
                                ? I18n.tr(mouseController.language, "navDashboard")
                                : (root.currentNavIndex === 1
                                   ? I18n.tr(mouseController.language, "macroStudioTitle")
                                   : I18n.tr(mouseController.language, "settingsTitle"))
                            )
                        }

                        Item { Layout.fillWidth: true }

                        // Active Profile Chip
                        Rectangle {
                            radius: Theme.buttonRadius
                            implicitWidth: profChipRow.implicitWidth + 16
                            implicitHeight: 28
                            color: Theme.buttonBg
                            border.width: 1
                            border.color: Theme.borderSubtle

                            RowLayout {
                                id: profChipRow
                                anchors.centerIn: parent
                                spacing: 6
                                Controls.Label {
                                    font.bold: true
                                    font.pixelSize: 12
                                    color: Theme.textPrimary
                                    text: (mouseController.activeProfile + 1) + " • " + mouseController.activeProfileName
                                }
                            }
                        }

                        // Dedicated ThemeDpiMonitor Chip (100% separate from MouseController DPI editor & Extra Mode safe!)
                        Rectangle {
                            id: liveDpiChip
                            readonly property color chipCol: themeDpiMonitor.ready
                                                             ? themeDpiMonitor.dpiColor
                                                             : Theme.accent
                            readonly property bool hasColor: (chipCol.r + chipCol.g + chipCol.b) > 0.08

                            radius: Theme.buttonRadius
                            implicitWidth: dpiChipRow.implicitWidth + 16
                            implicitHeight: 28
                            color: hasColor
                                   ? Qt.rgba(chipCol.r, chipCol.g, chipCol.b, Theme.isLight ? 0.16 : 0.22)
                                   : Theme.buttonBg
                            border.width: 1.2
                            border.color: hasColor
                                          ? Qt.rgba(chipCol.r, chipCol.g, chipCol.b, 0.75)
                                          : Theme.borderSubtle

                            Behavior on color { ColorAnimation { duration: 90 } }
                            Behavior on border.color { ColorAnimation { duration: 90 } }

                            HoverHandler {
                                id: dpiChipHover
                                cursorShape: themeDpiMonitor.deviceAccessAvailable ? Qt.PointingHandCursor : Qt.ArrowCursor
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: themeDpiMonitor.deviceAccessAvailable && !mouseController.isBusy
                                onClicked: themeDpiMonitor.readNow()
                            }

                            Controls.ToolTip.visible: dpiChipHover.hovered
                            Controls.ToolTip.text: themeDpiMonitor.ready
                                                   ? ("DPI " + themeDpiMonitor.dpiLevel
                                                      + " • " + themeDpiMonitor.dpiLabel
                                                      + " • " + themeDpiMonitor.dpiColor.toString().toUpperCase()
                                                      + (themeDpiMonitor.extraModePassive ? (" • " + I18n.tr(mouseController.language, "dpiSyncExtraSafeTag")) : "")
                                                      + (themeDpiMonitor.lastReadTime.length > 0 ? (" (" + themeDpiMonitor.lastReadTime + ")") : ""))
                                                   : I18n.tr(mouseController.language, "readNowBtn")

                            RowLayout {
                                id: dpiChipRow
                                anchors.centerIn: parent
                                spacing: 7
                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: liveDpiChip.chipCol
                                    border.width: 1
                                    border.color: Theme.isLight ? Qt.rgba(0, 0, 0, 0.25) : Qt.rgba(1, 1, 1, 0.35)
                                    Behavior on color { ColorAnimation { duration: 90 } }
                                }
                                Controls.Label {
                                    font.bold: true
                                    font.pixelSize: 12
                                    color: Theme.textPrimary
                                    text: themeDpiMonitor.ready ? themeDpiMonitor.dpiLabel : "DPI —"
                                }
                            }
                        }

                        // Subtle divider before Window Controls
                        Rectangle {
                            width: 1
                            height: 20
                            color: Theme.borderSubtle
                        }

                        // Window Control: Minimize (supports circle / pill / traffic)
                        Rectangle {
                            width: Theme.windowButtonStyle === "traffic" ? 16 : 28
                            height: Theme.windowButtonStyle === "traffic" ? 16 : 28
                            radius: Theme.windowButtonStyle === "pill" ? 7 : width / 2
                            color: Theme.windowButtonStyle === "traffic"
                                   ? (minHover.hovered ? "#eab308" : "#facc15")
                                   : (minHover.hovered ? Theme.buttonHoverBg : "transparent")
                            border.width: Theme.windowButtonStyle === "traffic" ? 1 : (minHover.hovered ? 1 : 0)
                            border.color: Theme.windowButtonStyle === "traffic" ? "#ca8a04" : Theme.borderHover
                            Behavior on color { ColorAnimation { duration: 120 } }
                            HoverHandler { id: minHover; cursorShape: Qt.PointingHandCursor }
                            Kirigami.Icon {
                                anchors.centerIn: parent
                                width: Theme.windowButtonStyle === "traffic" ? 10 : 14
                                height: width
                                visible: Theme.windowButtonStyle !== "traffic" || minHover.hovered
                                source: "window-minimize"
                                isMask: true
                                color: Theme.windowButtonStyle === "traffic" ? "#422006" : Theme.textPrimary
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.showMinimized()
                            }
                            Controls.ToolTip.visible: minHover.hovered
                            Controls.ToolTip.text: I18n.tr(mouseController.language, "winMinimizeTip")
                        }

                        // Window Control: Maximize / Restore
                        Rectangle {
                            width: Theme.windowButtonStyle === "traffic" ? 16 : 28
                            height: Theme.windowButtonStyle === "traffic" ? 16 : 28
                            radius: Theme.windowButtonStyle === "pill" ? 7 : width / 2
                            color: Theme.windowButtonStyle === "traffic"
                                   ? (maxHover.hovered ? "#16a34a" : "#22c55e")
                                   : (maxHover.hovered ? Theme.buttonHoverBg : "transparent")
                            border.width: Theme.windowButtonStyle === "traffic" ? 1 : (maxHover.hovered ? 1 : 0)
                            border.color: Theme.windowButtonStyle === "traffic" ? "#15803d" : Theme.borderHover
                            Behavior on color { ColorAnimation { duration: 120 } }
                            HoverHandler { id: maxHover; cursorShape: Qt.PointingHandCursor }
                            Kirigami.Icon {
                                anchors.centerIn: parent
                                width: Theme.windowButtonStyle === "traffic" ? 10 : 14
                                height: width
                                visible: Theme.windowButtonStyle !== "traffic" || maxHover.hovered
                                source: root.isMaximized ? "window-restore" : "window-maximize"
                                isMask: true
                                color: Theme.windowButtonStyle === "traffic" ? "#052e16" : Theme.textPrimary
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.toggleMaximize()
                            }
                            Controls.ToolTip.visible: maxHover.hovered
                            Controls.ToolTip.text: I18n.tr(mouseController.language, "winMaximizeTip")
                        }

                        // Window Control: Close
                        Rectangle {
                            width: Theme.windowButtonStyle === "traffic" ? 16 : 28
                            height: Theme.windowButtonStyle === "traffic" ? 16 : 28
                            radius: Theme.windowButtonStyle === "pill" ? 7 : width / 2
                            color: Theme.windowButtonStyle === "traffic"
                                   ? (clsHover.hovered ? "#dc2626" : "#ef4444")
                                   : (clsHover.hovered ? "#ef4444" : "transparent")
                            border.width: Theme.windowButtonStyle === "traffic" ? 1 : (clsHover.hovered ? 1 : 0)
                            border.color: Theme.windowButtonStyle === "traffic" ? "#b91c1c" : "#f87171"
                            Behavior on color { ColorAnimation { duration: 120 } }
                            HoverHandler { id: clsHover; cursorShape: Qt.PointingHandCursor }
                            Kirigami.Icon {
                                anchors.centerIn: parent
                                width: Theme.windowButtonStyle === "traffic" ? 10 : 14
                                height: width
                                visible: Theme.windowButtonStyle !== "traffic" || clsHover.hovered
                                source: "window-close"
                                isMask: true
                                color: (clsHover.hovered || Theme.windowButtonStyle === "traffic") ? "#FFFFFF" : Theme.textPrimary
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.close()
                            }
                            Controls.ToolTip.visible: clsHover.hovered
                            Controls.ToolTip.text: I18n.tr(mouseController.language, "winCloseTip")
                        }
                    }
                }

                // ---------- Split Workspace: Frosted Sidebar + Animated StackView ----------
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0
                    LayoutMirroring.enabled: root.isRtl
                    LayoutMirroring.childrenInherit: true

                    // Frosted Glass Sidebar (strictly shaped to match outer-bottom window corner in RTL & LTR!)
                    Item {
                        id: sidebarContainer
                        Layout.preferredWidth: 248
                        Layout.fillHeight: true

                        Canvas {
                            id: sidebarBgCanvas
                            anchors.fill: parent
                            renderStrategy: Canvas.Cooperative
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.reset()
                                ctx.clearRect(0, 0, width, height)
                                if (width < 4 || height < 4) return

                                var isChamfer = !root.isMaximized && Theme.windowShape === "chamfer"
                                var r = isChamfer ? 0 : root.effectiveWindowRadius
                                var cut = isChamfer ? Theme.windowChamferCut : 0

                                ctx.beginPath()
                                ctx.moveTo(0, 0)
                                ctx.lineTo(width, 0)

                                // Bottom-right corner (outer corner when RTL!)
                                if (root.isRtl && cut > 0) {
                                    ctx.lineTo(width, height - cut)
                                    ctx.lineTo(width - cut, height)
                                } else if (root.isRtl && r > 0) {
                                    ctx.lineTo(width, height - r)
                                    ctx.quadraticCurveTo(width, height, width - r, height)
                                } else {
                                    ctx.lineTo(width, height)
                                }

                                // Bottom-left corner (outer corner when LTR!)
                                if (!root.isRtl && cut > 0) {
                                    ctx.lineTo(cut, height)
                                    ctx.lineTo(0, height - cut)
                                } else if (!root.isRtl && r > 0) {
                                    ctx.lineTo(r, height)
                                    ctx.quadraticCurveTo(0, height, 0, height - r)
                                } else {
                                    ctx.lineTo(0, height)
                                }

                                ctx.closePath()
                                ctx.fillStyle = Theme.sidebarBg.toString()
                                ctx.fill()
                            }
                            onWidthChanged: requestPaint()
                            onHeightChanged: requestPaint()
                            Component.onCompleted: requestPaint()
                            Connections {
                                target: Theme
                                function onSidebarBgChanged() { sidebarBgCanvas.requestPaint() }
                                function onWindowShapeChanged() { sidebarBgCanvas.requestPaint() }
                                function onCornerRadiusChanged() { sidebarBgCanvas.requestPaint() }
                            }
                            Connections {
                                target: root
                                function onIsRtlChanged() { sidebarBgCanvas.requestPaint() }
                                function onIsMaximizedChanged() { sidebarBgCanvas.requestPaint() }
                            }
                        }

                        Rectangle {
                            width: 1
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.right: parent.right
                            color: Theme.borderSubtle
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            anchors.topMargin: 14
                            anchors.bottomMargin: (!root.isMaximized && Theme.windowShape === "chamfer")
                                                  ? Math.max(18, Theme.windowChamferCut + 2)
                                                  : ((!root.isMaximized && root.effectiveWindowRadius > 14) ? 16 : 14)
                            spacing: 10

                            // Status & Edition Badge Header
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                ThemedAppIcon {
                                    iconSize: 40
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Controls.Label {
                                        text: "Delux M700A"
                                        font.bold: true
                                        font.pixelSize: 14
                                        color: Theme.textPrimary
                                    }

                                    RowLayout {
                                        spacing: 6
                                        Rectangle {
                                            width: 8; height: 8; radius: 4
                                            color: mouseController.connected
                                                   ? "#22c55e"
                                                   : (!mouseController.deviceAccessAvailable ? "#f59e0b" : "#ef4444")

                                            SequentialAnimation on opacity {
                                                running: root.visible && Theme.animatedUi && mouseController.connected
                                                loops: Animation.Infinite
                                                NumberAnimation { from: 1.0; to: 0.35; duration: 1200; easing.type: Easing.InOutSine }
                                                NumberAnimation { from: 0.35; to: 1.0; duration: 1200; easing.type: Easing.InOutSine }
                                            }
                                        }
                                        Controls.Label {
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                            font.pixelSize: 11
                                            color: Theme.textSecondary
                                            text: I18n.tr(mouseController.language, "modernEditionTag")
                                        }
                                    }
                                }
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                            // Primary Navigation Rail
                            ModernNavButton {
                                text: I18n.tr(mouseController.language, "navDashboard")
                                iconName: "input-mouse"
                                active: root.currentNavIndex === 0
                                accentColor: Theme.accent
                                rtl: root.isRtl
                                onClicked: root.navigateTo(0)
                            }

                            ModernNavButton {
                                visible: mouseController.enableKeyCapture
                                text: I18n.tr(mouseController.language, "macroStudioBtn")
                                iconName: "input-keyboard"
                                active: root.currentNavIndex === 1
                                badgeText: mouseController.macroLibrary.length > 0 ? String(mouseController.macroLibrary.length) : ""
                                accentColor: Theme.accent
                                rtl: root.isRtl
                                onClicked: root.navigateTo(1)
                            }

                            ModernNavButton {
                                text: I18n.tr(mouseController.language, "settingsBtn")
                                iconName: "configure"
                                active: root.currentNavIndex === 2
                                accentColor: Theme.accent
                                rtl: root.isRtl
                                onClicked: root.navigateTo(2)
                            }

                            Item { Layout.fillHeight: true }

                            // Unsaved changes indicator pill
                            Rectangle {
                                Layout.fillWidth: true
                                visible: mouseController.hasUnsavedChanges
                                implicitHeight: unsavedRow.implicitHeight + 12
                                radius: Theme.buttonRadius
                                color: Qt.rgba(0.96, 0.62, 0.04, 0.16)
                                border.width: 1
                                border.color: Qt.rgba(0.96, 0.62, 0.04, 0.60)

                                RowLayout {
                                    id: unsavedRow
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.margins: 8
                                    spacing: 6

                                    Kirigami.Icon { width: 16; height: 16; source: "dialog-warning" }

                                    Controls.Label {
                                        Layout.fillWidth: true
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: Theme.isLight ? "#92400E" : "#FDE68A"
                                        elide: Text.ElideRight
                                        text: I18n.tr(mouseController.language, "unsavedBadge")
                                    }
                                }
                            }

                            // Primary Action Buttons (all using ModernButton!)
                            ModernButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 42
                                text: I18n.tr(mouseController.language, "saveApply")
                                icon.name: "document-save"
                                highlighted: true
                                enabled: mouseController.connected && !mouseController.isBusy
                                onClicked: mouseController.applyToMouse()
                            }

                            ModernButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                text: I18n.tr(mouseController.language, "readMouse")
                                icon.name: "view-refresh"
                                enabled: mouseController.deviceAccessAvailable && !mouseController.isBusy
                                onClicked: root.guardUnsaved(function() { mouseController.refreshFromMouse() })
                            }

                            ModernButton {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 36
                                text: I18n.tr(mouseController.language, "permissionsBtn")
                                icon.name: "security-high"
                                onClicked: permissionsDialog.open()
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                            // Motion quick toggle + Version
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                ModernButton {
                                    Layout.fillWidth: true
                                    implicitHeight: 32
                                    icon.name: mouseController.animatedUi ? "media-playback-pause" : "media-playback-start"
                                    text: mouseController.animatedUi
                                          ? I18n.tr(mouseController.language, "motionOnBtn")
                                          : I18n.tr(mouseController.language, "motionOffBtn")
                                    onClicked: mouseController.animatedUi = !mouseController.animatedUi
                                }

                                ModernButton {
                                    Layout.preferredWidth: 32
                                    Layout.preferredHeight: 32
                                    implicitWidth: 32
                                    implicitHeight: 32
                                    leftPadding: 0
                                    rightPadding: 0
                                    topPadding: 0
                                    bottomPadding: 0
                                    icon.name: "globe"
                                    text: ""
                                    Controls.ToolTip.visible: hovered
                                    Controls.ToolTip.text: I18n.tr(mouseController.language, "sidebarWebsiteTip") + "\nhttps://github.com/bufferczd/deepcomplex-aurora"
                                    onClicked: Qt.openUrlExternally("https://github.com/bufferczd/deepcomplex-aurora")
                                }

                                Controls.Label {
                                    color: Theme.textSecondary
                                    font.pixelSize: 11
                                    text: "v" + mouseController.appVersion
                                }
                            }
                        }
                    }

                    // Animated Content StackView (strictly inset from outer-bottom corner so ScrollBar & cards never overlap the border!)
                    Controls.StackView {
                        id: mainStackView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.bottomMargin: root.isMaximized
                                             ? 0
                                             : (Theme.windowShape === "chamfer"
                                                ? Theme.windowChamferCut
                                                : Math.ceil(root.effectiveWindowRadius * 0.55))
                        Layout.rightMargin: root.isMaximized
                                            ? 0
                                            : (Theme.windowShape === "chamfer"
                                               ? 6
                                               : Math.ceil(root.effectiveWindowRadius * 0.25))
                        clip: true
                        initialItem: homePageComponent

                        replaceEnter: Transition {
                            ParallelAnimation {
                                NumberAnimation {
                                    property: "opacity"
                                    from: 0.0
                                    to: 1.0
                                    duration: Theme.animatedUi ? 290 : 0
                                    easing.type: Easing.OutCubic
                                }
                                NumberAnimation {
                                    property: "y"
                                    from: Theme.animatedUi ? 20 : 0
                                    to: 0
                                    duration: Theme.animatedUi ? 310 : 0
                                    easing.type: Easing.OutQuint
                                }
                                NumberAnimation {
                                    property: "scale"
                                    from: Theme.animatedUi ? 0.985 : 1.0
                                    to: 1.0
                                    duration: Theme.animatedUi ? 310 : 0
                                    easing.type: Easing.OutQuint
                                }
                            }
                        }

                        replaceExit: Transition {
                            ParallelAnimation {
                                NumberAnimation {
                                    property: "opacity"
                                    from: 1.0
                                    to: 0.0
                                    duration: Theme.animatedUi ? 170 : 0
                                    easing.type: Easing.InCubic
                                }
                                NumberAnimation {
                                    property: "y"
                                    from: 0
                                    to: Theme.animatedUi ? -12 : 0
                                    duration: Theme.animatedUi ? 170 : 0
                                    easing.type: Easing.InCubic
                                }
                            }
                        }
                    }
                }
            }

            } // end windowContentLayer

            // Minimalist Startup Logo + Loading Line (very first frame on launch)
            StartupSplashOverlay {
                id: startupSplash
                anchors.fill: parent
                z: 85
                cornerRadius: root.effectiveWindowRadius
                chamfered: !root.isMaximized && Theme.windowShape === "chamfer"
                durationMs: 1750
                barWidth: 270
                onFinished: {
                    root.bootSplashActive = false
                    if (!mouseController.firstRunCompleted && !mouseController.launchedFromAutostart) {
                        onboardingDialog.openAtStep(0)
                    } else if (root.pendingUdevRepair && !permissionsDialog.visible && !mouseController.udevHealthy) {
                        root.pendingUdevRepair = false
                        if (!repairDialog.visible) repairDialog.open()
                    }
                }
            }

            // 3. Customizable Outer Window Border Frame (accent / gradient / rainbow / subtle / custom / none + all 4 shapes)
            WindowBorderFrame {
                anchors.fill: parent
                frameRadius: root.effectiveWindowRadius
                frameWidth: Math.max(0.0, Theme.borderWidth + 0.5)
                styleMode: Theme.borderStyle
                shapeMode: Theme.windowShape
                borderAlpha: Theme.borderOpacity
                edgeGlow: Theme.windowEdgeGlow
                isMaximized: root.isMaximized
                animate: root.visible && Theme.animatedUi
                z: 90
            }

            // 4. Native Wayland/X11 Window Resize Edge & Corner Handles (active when not maximized)
            Item {
                anchors.fill: parent
                visible: !root.isMaximized
                z: 100

                // Top edge
                MouseArea {
                    anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                    anchors.leftMargin: 12; anchors.rightMargin: 12; height: 6
                    cursorShape: Qt.SizeVerCursor
                    onPressed: root.startSystemResize(Qt.TopEdge)
                }
                // Bottom edge
                MouseArea {
                    anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
                    anchors.leftMargin: 12; anchors.rightMargin: 12; height: 6
                    cursorShape: Qt.SizeVerCursor
                    onPressed: root.startSystemResize(Qt.BottomEdge)
                }
                // Left edge
                MouseArea {
                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                    anchors.topMargin: 12; anchors.bottomMargin: 12; width: 6
                    cursorShape: Qt.SizeHorCursor
                    onPressed: root.startSystemResize(Qt.LeftEdge)
                }
                // Right edge
                MouseArea {
                    anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                    anchors.topMargin: 12; anchors.bottomMargin: 12; width: 6
                    cursorShape: Qt.SizeHorCursor
                    onPressed: root.startSystemResize(Qt.RightEdge)
                }
                // Corners
                MouseArea {
                    anchors.top: parent.top; anchors.left: parent.left; width: 12; height: 12
                    cursorShape: Qt.SizeFDiagCursor
                    onPressed: root.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
                }
                MouseArea {
                    anchors.top: parent.top; anchors.right: parent.right; width: 12; height: 12
                    cursorShape: Qt.SizeBDiagCursor
                    onPressed: root.startSystemResize(Qt.TopEdge | Qt.RightEdge)
                }
                MouseArea {
                    anchors.bottom: parent.bottom; anchors.left: parent.left; width: 12; height: 12
                    cursorShape: Qt.SizeBDiagCursor
                    onPressed: root.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
                }
                MouseArea {
                    anchors.bottom: parent.bottom; anchors.right: parent.right; width: 12; height: 12
                    cursorShape: Qt.SizeFDiagCursor
                    onPressed: root.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
                }
            }
        }
    }
}
