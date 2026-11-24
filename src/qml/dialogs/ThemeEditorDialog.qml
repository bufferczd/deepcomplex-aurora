import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    id: themeDialog
    title: I18n.tr(mouseController.language, "themeDialogTitle")
    headerIcon: "color-management"
    width: Math.min((parent ? parent.width : 1140) - 36, 860)
    height: Math.min((parent ? parent.height : 850) - 44, 730)
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside

    signal pickCustomAccentRequested(color currentColor)
    signal pickCustomSecondaryRequested(color currentColor)
    signal pickCustomBorderRequested(color currentColor)

    property string categoryFilter: "all" // "all", "oled", "dark", "light"

    readonly property var allPresetsModel: [
        // 8 OLED Pure Black (#000000) Themes
        { id: "oled_pure",    cat: "oled",  oled: true,  light: false, key: "themePresetOledPure",    bg: "#000000", c1: "#15803d", c2: "#166534" },
        { id: "oled_cyan",    cat: "oled",  oled: true,  light: false, key: "themePresetOledCyan",    bg: "#000000", c1: "#00e5ff", c2: "#38bdf8" },
        { id: "oled_crimson", cat: "oled",  oled: true,  light: false, key: "themePresetOledCrimson", bg: "#000000", c1: "#ff1e42", c2: "#f97316" },
        { id: "oled_violet",  cat: "oled",  oled: true,  light: false, key: "themePresetOledViolet",  bg: "#000000", c1: "#a855f7", c2: "#ec4899" },
        { id: "oled_gold",    cat: "oled",  oled: true,  light: false, key: "themePresetOledGold",    bg: "#000000", c1: "#f59e0b", c2: "#fbbf24" },
        { id: "oled_ice",     cat: "oled",  oled: true,  light: false, key: "themePresetOledIce",     bg: "#000000", c1: "#3b82f6", c2: "#6366f1" },
        { id: "oled_magenta", cat: "oled",  oled: true,  light: false, key: "themePresetOledMagenta", bg: "#000000", c1: "#f43f5e", c2: "#d946ef" },
        { id: "oled_mono",    cat: "oled",  oled: true,  light: false, key: "themePresetOledMono",    bg: "#000000", c1: "#f8fafc", c2: "#94a3b8" },
        // 10 Modern Dark Glass Themes
        { id: "aurora",       cat: "dark",  oled: false, light: false, key: "themePresetAurora",      bg: "#0B1120", c1: "#38bdf8", c2: "#7c3aed" },
        { id: "cyber",        cat: "dark",  oled: false, light: false, key: "themePresetCyber",       bg: "#11091C", c1: "#f43f5e", c2: "#06b6d4" },
        { id: "emerald",      cat: "dark",  oled: false, light: false, key: "themePresetEmerald",     bg: "#071A16", c1: "#10b981", c2: "#14b8a6" },
        { id: "crimson",      cat: "dark",  oled: false, light: false, key: "themePresetCrimson",     bg: "#1C0B0E", c1: "#fb7185", c2: "#f59e0b" },
        { id: "amethyst",     cat: "dark",  oled: false, light: false, key: "themePresetAmethyst",    bg: "#130D24", c1: "#a78bfa", c2: "#e879f9" },
        { id: "ocean",        cat: "dark",  oled: false, light: false, key: "themePresetOcean",       bg: "#061826", c1: "#0ea5e9", c2: "#2dd4bf" },
        { id: "solar",        cat: "dark",  oled: false, light: false, key: "themePresetSolar",       bg: "#1C1006", c1: "#f97316", c2: "#f59e0b" },
        { id: "sakura",       cat: "dark",  oled: false, light: false, key: "themePresetSakura",      bg: "#1E0E1B", c1: "#f472b6", c2: "#a855f7" },
        { id: "nord",         cat: "dark",  oled: false, light: false, key: "themePresetNord",        bg: "#202736", c1: "#88c0d0", c2: "#81a1c1" },
        { id: "dracula",      cat: "dark",  oled: false, light: false, key: "themePresetDracula",     bg: "#1F212E", c1: "#bd93f9", c2: "#ff79c6" },
        // 2 Modern Light Glass Themes
        { id: "frost",        cat: "light", oled: false, light: true,  key: "themePresetFrost",       bg: "#E4EDF8", c1: "#0284c7", c2: "#6366f1" },
        { id: "sand",         cat: "light", oled: false, light: true,  key: "themePresetSand",        bg: "#F3ECE1", c1: "#d97706", c2: "#0d9488" }
    ]

    readonly property var filteredPresetsModel: {
        if (categoryFilter === "all") return allPresetsModel
        let out = []
        for (let i = 0; i < allPresetsModel.length; ++i) {
            if (allPresetsModel[i].cat === categoryFilter) out.push(allPresetsModel[i])
        }
        return out
    }

    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: 12

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.textSecondary
            text: I18n.tr(mouseController.language, "themeDialogSub")
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 12

                // 1. 20 Theme Presets with Category Filters (OLED / Dark / Light)
                GlassCard {
                    cardPadding: 14

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Controls.Label {
                            Layout.fillWidth: true
                            font.bold: true
                            color: Theme.textPrimary
                            text: I18n.tr(mouseController.language, "themePresetSec20")
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Repeater {
                            model: [
                                { id: "all",   key: "themeCatAll" },
                                { id: "oled",  key: "themeCatOled" },
                                { id: "dark",  key: "themeCatDark" },
                                { id: "light", key: "themeCatLight" }
                            ]

                            ModernButton {
                                Layout.fillWidth: true
                                implicitHeight: 32
                                highlighted: themeDialog.categoryFilter === modelData.id
                                text: I18n.tr(mouseController.language, modelData.key)
                                onClicked: themeDialog.categoryFilter = modelData.id
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 4
                        columnSpacing: 8
                        rowSpacing: 8

                        Repeater {
                            model: themeDialog.filteredPresetsModel

                            Rectangle {
                                id: presetCard
                                Layout.fillWidth: true
                                implicitHeight: 68
                                radius: Theme.buttonRadius
                                property bool selected: mouseController.uiThemePreset === modelData.id
                                color: modelData.bg
                                border.width: selected ? 2.2 : 1
                                border.color: selected ? Theme.accent : (modelData.oled ? Qt.rgba(1, 1, 1, 0.22) : Theme.borderSubtle)

                                HoverHandler { cursorShape: Qt.PointingHandCursor }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    spacing: 6

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 4

                                        Rectangle { width: 13; height: 13; radius: 7; color: modelData.c1; border.width: 1; border.color: Qt.rgba(1,1,1,0.4) }
                                        Rectangle { width: 13; height: 13; radius: 7; color: modelData.c2; border.width: 1; border.color: Qt.rgba(1,1,1,0.4) }

                                        Rectangle {
                                            visible: modelData.oled
                                            radius: 4
                                            implicitWidth: oledTagLbl.implicitWidth + 8
                                            implicitHeight: 16
                                            color: Qt.rgba(1, 1, 1, 0.12)
                                            border.width: 1
                                            border.color: Qt.rgba(1, 1, 1, 0.28)
                                            Controls.Label {
                                                id: oledTagLbl
                                                anchors.centerIn: parent
                                                font.bold: true
                                                font.pixelSize: 9
                                                color: "#FFFFFF"
                                                text: "OLED"
                                            }
                                        }

                                        Item { Layout.fillWidth: true }

                                        Kirigami.Icon {
                                            visible: presetCard.selected
                                            width: 15; height: 15
                                            source: "dialog-ok-apply"
                                        }
                                    }

                                    Controls.Label {
                                        Layout.fillWidth: true
                                        font.bold: presetCard.selected
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                        color: modelData.light ? "#0F172A" : "#F8FAFC"
                                        text: I18n.tr(mouseController.language, modelData.key)
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: mouseController.uiThemePreset = modelData.id
                                }
                            }
                        }
                    }

                    // Global True-Black OLED Override
                    Kirigami.Separator { Layout.fillWidth: true }

                    SettingSwitchRow {
                        labelText: I18n.tr(mouseController.language, "themeTrueBlackOled")
                        hintText: I18n.tr(mouseController.language, "themeTrueBlackOledHint")
                        checkedValue: mouseController.uiTrueBlackOled
                        onSwitchToggled: (s) => mouseController.uiTrueBlackOled = s
                    }
                }

                // 2. Primary Accent + Auto-Read DPI Color + Custom Secondary Dual-Tone Glow
                GlassCard {
                    cardPadding: 14

                    Controls.Label {
                        font.bold: true
                        color: Theme.textPrimary
                        text: I18n.tr(mouseController.language, "themeAccentSec")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ModernButton {
                            Layout.fillWidth: true
                            highlighted: mouseController.uiAccentMode === "autoDpi"
                            text: I18n.tr(mouseController.language, "themeAccentAutoDpi")
                            icon.name: "speedometer"
                            onClicked: mouseController.uiAccentMode = "autoDpi"
                        }

                        ModernButton {
                            Layout.fillWidth: true
                            highlighted: mouseController.uiAccentMode === "preset"
                            text: I18n.tr(mouseController.language, "themeAccentPreset")
                            icon.name: "color-management"
                            onClicked: mouseController.uiAccentMode = "preset"
                        }

                        ModernButton {
                            Layout.fillWidth: true
                            highlighted: mouseController.uiAccentMode === "custom"
                            text: I18n.tr(mouseController.language, "themeAccentCustom")
                            icon.name: "color-picker"
                            onClicked: mouseController.uiAccentMode = "custom"
                        }
                    }

                    // Dedicated ThemeDpiMonitor controls (100% separate from DPI editor & Extra Mode safe)
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: themeDpiMonitor.deviceAccessAvailable
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Controls.CheckBox {
                                Layout.fillWidth: true
                                checked: themeDpiMonitor.autoSync
                                onToggled: {
                                    themeDpiMonitor.autoSync = checked
                                    if (checked) mouseController.uiAccentMode = "autoDpi"
                                }
                                text: I18n.tr(mouseController.language, "optAutoSyncDpiColor")
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
                                color: Theme.textSecondary
                                text: I18n.tr(mouseController.language, "secondsUnit")
                            }

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
                                         + (themeDpiMonitor.extraModePassive ? (" • " + I18n.tr(mouseController.language, "dpiSyncExtraSafeTag")) : "")
                                         + (themeDpiMonitor.lastReadTime.length > 0
                                            ? (" • " + I18n.tr(mouseController.language, "dpiSyncLastRead") + " " + themeDpiMonitor.lastReadTime)
                                            : ""))
                                      : I18n.tr(mouseController.language, "dpiSyncOffStatus")
                            }
                        }
                    }

                    // Primary custom accent swatches
                    RowLayout {
                        Layout.fillWidth: true
                        visible: mouseController.uiAccentMode === "custom"
                        spacing: 8

                        Repeater {
                            model: ["#00ff66", "#38bdf8", "#818cf8", "#c084fc", "#f43f5e", "#f97316", "#facc15", "#10b981", "#2dd4bf", "#f8fafc"]

                            Rectangle {
                                width: 26
                                height: 26
                                radius: 13
                                color: modelData
                                border.width: mouseController.uiCustomAccent.toLowerCase() === modelData.toLowerCase() ? 2.5 : 1
                                border.color: mouseController.uiCustomAccent.toLowerCase() === modelData.toLowerCase() ? Theme.textPrimary : Qt.rgba(1,1,1,0.35)

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: mouseController.uiCustomAccent = modelData
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        ModernButton {
                            text: I18n.tr(mouseController.language, "themePickCustomColor")
                            icon.name: "color-picker"
                            onClicked: themeDialog.pickCustomAccentRequested(mouseController.uiCustomAccent)
                        }
                    }

                    Kirigami.Separator { Layout.fillWidth: true }

                    // Secondary Glow / Dual-Tone Color Override
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Controls.CheckBox {
                            Layout.fillWidth: true
                            checked: mouseController.uiUseCustomSecondary
                            onToggled: mouseController.uiUseCustomSecondary = checked
                            text: I18n.tr(mouseController.language, "themeUseCustomSecondary")
                        }

                        Rectangle {
                            visible: mouseController.uiUseCustomSecondary
                            width: 24; height: 24; radius: 12
                            color: mouseController.uiCustomSecondary
                            border.width: 1.5; border.color: Theme.textPrimary
                        }

                        ModernButton {
                            visible: mouseController.uiUseCustomSecondary
                            implicitHeight: 32
                            text: I18n.tr(mouseController.language, "themePickSecondaryBtn")
                            icon.name: "color-picker"
                            onClicked: themeDialog.pickCustomSecondaryRequested(mouseController.uiCustomSecondary)
                        }
                    }
                }

                // 3. Background Animation Style (5 styles incl. Pure OLED #000)
                GlassCard {
                    cardPadding: 14

                    RowLayout {
                        Layout.fillWidth: true

                        Controls.Label {
                            Layout.fillWidth: true
                            font.bold: true
                            color: Theme.textPrimary
                            text: I18n.tr(mouseController.language, "themeBgStyleSec")
                        }

                        Controls.Switch {
                            text: I18n.tr(mouseController.language, mouseController.animatedUi ? "motionOnBtn" : "motionOffBtn")
                            checked: mouseController.animatedUi
                            onToggled: mouseController.animatedUi = checked
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 5
                        columnSpacing: 6
                        rowSpacing: 6

                        Repeater {
                            model: [
                                { id: "aurora",   key: "themeBgAurora",   icon: "weather-clear-night" },
                                { id: "waves",    key: "themeBgWaves",    icon: "view-media-visualization" },
                                { id: "grid",     key: "themeBgGrid",     icon: "view-grid" },
                                { id: "solid",    key: "themeBgSolid",    icon: "draw-rectangle" },
                                { id: "oledPure", key: "themeBgOledPure", icon: "color-profile" }
                            ]

                            ModernButton {
                                Layout.fillWidth: true
                                highlighted: mouseController.uiBackgroundStyle === modelData.id
                                text: I18n.tr(mouseController.language, modelData.key)
                                icon.name: modelData.icon
                                onClicked: mouseController.uiBackgroundStyle = modelData.id
                            }
                        }
                    }
                }

                // 4. Deep Geometry, Border, Glow, Speed & Density Tuning
                GlassCard {
                    cardPadding: 14

                    Controls.Label {
                        font.bold: true
                        color: Theme.textPrimary
                        text: I18n.tr(mouseController.language, "themeFineTuneSec")
                    }

                    // Glass Opacity (35% - 98%)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Controls.Label { Layout.preferredWidth: 230; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "themeGlassOpacity") }
                        Controls.Slider {
                            Layout.fillWidth: true; from: 0.35; to: 0.98; stepSize: 0.02
                            value: mouseController.uiGlassOpacity
                            onMoved: mouseController.uiGlassOpacity = value
                        }
                        Controls.Label { Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight; font.bold: true; text: Math.round(mouseController.uiGlassOpacity * 100) + "%" }
                    }

                    // Corner Radius (0px - 24px)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Controls.Label { Layout.preferredWidth: 230; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "themeCornerRadius") }
                        Controls.Slider {
                            Layout.fillWidth: true; from: 0; to: 24; stepSize: 1
                            value: mouseController.uiCornerRadius
                            onMoved: mouseController.uiCornerRadius = Math.round(value)
                        }
                        Controls.Label { Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight; font.bold: true; text: mouseController.uiCornerRadius + " px" }
                    }

                    // Border Thickness (0.0px - 3.0px)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Controls.Label { Layout.preferredWidth: 230; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "themeBorderWidth") }
                        Controls.Slider {
                            Layout.fillWidth: true; from: 0.0; to: 3.0; stepSize: 0.2
                            value: mouseController.uiBorderWidth
                            onMoved: mouseController.uiBorderWidth = value
                        }
                        Controls.Label { Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight; font.bold: true; text: mouseController.uiBorderWidth.toFixed(1) + " px" }
                    }

                    // Ambient Glow Intensity (0% - 100%)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Controls.Label { Layout.preferredWidth: 230; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "themeGlowIntensity") }
                        Controls.Slider {
                            Layout.fillWidth: true; from: 0.0; to: 1.0; stepSize: 0.05
                            value: mouseController.uiGlowIntensity
                            onMoved: mouseController.uiGlowIntensity = value
                        }
                        Controls.Label { Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight; font.bold: true; text: Math.round(mouseController.uiGlowIntensity * 100) + "%" }
                    }

                    // Animation Speed (0.4x - 2.0x)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Controls.Label { Layout.preferredWidth: 230; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "themeAnimSpeed") }
                        Controls.Slider {
                            Layout.fillWidth: true; from: 0.4; to: 2.0; stepSize: 0.1
                            value: mouseController.uiAnimationSpeed
                            onMoved: mouseController.uiAnimationSpeed = value
                        }
                        Controls.Label { Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight; font.bold: true; text: mouseController.uiAnimationSpeed.toFixed(1) + "x" }
                    }

                    Kirigami.Separator { Layout.fillWidth: true }

                    SettingSwitchRow {
                        labelText: I18n.tr(mouseController.language, "themeCompactMode")
                        checkedValue: mouseController.uiCompactMode
                        onSwitchToggled: (s) => mouseController.uiCompactMode = s
                    }
                }

                // 4.5 Button Shapes, Visual Styles & Card Edge Styles (with live preview)
                GlassCard {
                    cardPadding: 14

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Controls.Label {
                            Layout.fillWidth: true
                            font.bold: true
                            color: Theme.textPrimary
                            text: I18n.tr(mouseController.language, "themeButtonsEdgesSec")
                        }

                        Controls.Label {
                            color: Theme.textSecondary
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            text: I18n.tr(mouseController.language, "themePreviewLabel")
                        }

                        ModernButton {
                            implicitHeight: 32
                            highlighted: true
                            text: I18n.tr(mouseController.language, "themePreviewPrimary")
                            icon.name: "dialog-ok-apply"
                        }

                        ModernButton {
                            implicitHeight: 32
                            text: I18n.tr(mouseController.language, "themePreviewSecondary")
                            icon.name: "configure"
                        }
                    }

                    Controls.Label {
                        color: Theme.textSecondary
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "themeButtonShapeLabel")
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 8
                        rowSpacing: 8

                        Repeater {
                            model: [
                                { id: "rounded",  key: "btnShapeRounded" },
                                { id: "pill",     key: "btnShapePill" },
                                { id: "sharp",    key: "btnShapeSharp" },
                                { id: "squircle", key: "btnShapeSquircle" },
                                { id: "leaf",     key: "btnShapeLeaf" },
                                { id: "chamfer",  key: "btnShapeChamfer" }
                            ]
                            ModernButton {
                                Layout.fillWidth: true
                                highlighted: mouseController.uiButtonShape === modelData.id
                                text: I18n.tr(mouseController.language, modelData.key)
                                onClicked: mouseController.uiButtonShape = modelData.id
                            }
                        }
                    }

                    Controls.Label {
                        color: Theme.textSecondary
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "themeButtonStyleLabel")
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 8
                        rowSpacing: 8

                        Repeater {
                            model: [
                                { id: "glass",    key: "btnStyleGlass" },
                                { id: "solid",    key: "btnStyleSolid" },
                                { id: "outline",  key: "btnStyleOutline" },
                                { id: "neon",     key: "btnStyleNeon" },
                                { id: "gradient", key: "btnStyleGradient" },
                                { id: "soft",     key: "btnStyleSoft" }
                            ]
                            ModernButton {
                                Layout.fillWidth: true
                                highlighted: mouseController.uiButtonStyle === modelData.id
                                text: I18n.tr(mouseController.language, modelData.key)
                                onClicked: mouseController.uiButtonStyle = modelData.id
                            }
                        }
                    }

                    Controls.Label {
                        color: Theme.textSecondary
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "themeCardEdgeLabel")
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 5
                        columnSpacing: 6
                        rowSpacing: 6

                        Repeater {
                            model: [
                                { id: "glass",    key: "cardEdgeGlass" },
                                { id: "neon",     key: "cardEdgeNeon" },
                                { id: "gradient", key: "cardEdgeGradient" },
                                { id: "chamfer",  key: "cardEdgeChamfer" },
                                { id: "flat",     key: "cardEdgeFlat" }
                            ]
                            ModernButton {
                                Layout.fillWidth: true
                                highlighted: mouseController.uiCardEdgeStyle === modelData.id
                                text: I18n.tr(mouseController.language, modelData.key)
                                onClicked: mouseController.uiCardEdgeStyle = modelData.id
                            }
                        }
                    }
                }

                // 5. Window Borders, Edges & Titlebar Chrome Customization
                GlassCard {
                    cardPadding: 14

                    Controls.Label {
                        font.bold: true
                        color: Theme.textPrimary
                        text: I18n.tr(mouseController.language, "themeBordersSec")
                    }

                    // Main Window & Dialog Corner Shape (Sharp 0px / Rounded / Extra Round / 45° Chamfer)
                    Controls.Label {
                        color: Theme.textSecondary
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "themeWindowShapeLabel")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: [
                                { id: "sharp",   key: "winShapeSharp" },
                                { id: "rounded", key: "winShapeRounded" },
                                { id: "extra",   key: "winShapeExtra" },
                                { id: "chamfer", key: "winShapeChamfer" }
                            ]
                            ModernButton {
                                Layout.fillWidth: true
                                highlighted: mouseController.uiWindowShape === modelData.id
                                text: I18n.tr(mouseController.language, modelData.key)
                                onClicked: mouseController.uiWindowShape = modelData.id
                            }
                        }
                    }

                    Controls.Label {
                        color: Theme.textSecondary
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "themeBorderStyleLabel")
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 8
                        rowSpacing: 8

                        Repeater {
                            model: [
                                { id: "accent",   key: "borderStyleAccent",   icon: "color-management" },
                                { id: "gradient", key: "borderStyleGradient", icon: "format-fill-color" },
                                { id: "rainbow",  key: "borderStyleRainbow",  icon: "color-picker" },
                                { id: "subtle",   key: "borderStyleSubtle",   icon: "draw-rectangle" },
                                { id: "custom",   key: "borderStyleCustom",   icon: "fill-color" },
                                { id: "none",     key: "borderStyleNone",     icon: "dialog-cancel" }
                            ]

                            ModernButton {
                                Layout.fillWidth: true
                                highlighted: mouseController.uiBorderStyle === modelData.id
                                text: I18n.tr(mouseController.language, modelData.key)
                                icon.name: modelData.icon
                                onClicked: mouseController.uiBorderStyle = modelData.id
                            }
                        }
                    }

                    // Custom border color swatches + picker
                    RowLayout {
                        Layout.fillWidth: true
                        visible: mouseController.uiBorderStyle === "custom"
                        spacing: 8

                        Repeater {
                            model: ["#00ff66", "#38bdf8", "#a855f7", "#f43f5e", "#f59e0b", "#06b6d4", "#ffffff", "#64748b"]
                            Rectangle {
                                width: 26; height: 26; radius: 13
                                color: modelData
                                border.width: mouseController.uiCustomBorderColor.toLowerCase() === modelData.toLowerCase() ? 2.5 : 1
                                border.color: mouseController.uiCustomBorderColor.toLowerCase() === modelData.toLowerCase() ? Theme.textPrimary : Qt.rgba(1,1,1,0.35)
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: mouseController.uiCustomBorderColor = modelData
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        ModernButton {
                            implicitHeight: 32
                            text: I18n.tr(mouseController.language, "themePickBorderColor")
                            icon.name: "color-picker"
                            onClicked: themeDialog.pickCustomBorderRequested(mouseController.uiCustomBorderColor)
                        }
                    }

                    // Border Opacity Slider
                    RowLayout {
                        Layout.fillWidth: true
                        visible: mouseController.uiBorderStyle !== "none"
                        spacing: 12
                        Controls.Label { Layout.preferredWidth: 230; color: Theme.textSecondary; text: I18n.tr(mouseController.language, "themeBorderOpacity") }
                        Controls.Slider {
                            Layout.fillWidth: true; from: 0.10; to: 1.0; stepSize: 0.05
                            value: mouseController.uiBorderOpacity
                            onMoved: mouseController.uiBorderOpacity = value
                        }
                        Controls.Label { Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight; font.bold: true; text: Math.round(mouseController.uiBorderOpacity * 100) + "%" }
                    }

                    SettingSwitchRow {
                        labelText: I18n.tr(mouseController.language, "themeWindowEdgeGlow")
                        checkedValue: mouseController.uiWindowEdgeGlow
                        onSwitchToggled: (s) => mouseController.uiWindowEdgeGlow = s
                    }

                    Kirigami.Separator { Layout.fillWidth: true }

                    // Window Control Button Styles (Circle / Pill / Traffic Lights)
                    Controls.Label {
                        color: Theme.textSecondary
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "themeWinBtnStyleLabel")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: [
                                { id: "circle",  key: "winBtnCircle" },
                                { id: "pill",    key: "winBtnPill" },
                                { id: "traffic", key: "winBtnTraffic" }
                            ]
                            ModernButton {
                                Layout.fillWidth: true
                                highlighted: mouseController.uiWindowButtonStyle === modelData.id
                                text: I18n.tr(mouseController.language, modelData.key)
                                onClicked: mouseController.uiWindowButtonStyle = modelData.id
                            }
                        }
                    }

                    // Top Titlebar Style (Glass / Accent / Transparent / OLED)
                    Controls.Label {
                        color: Theme.textSecondary
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "themeTitlebarStyleLabel")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: [
                                { id: "glass",       key: "titlebarGlass" },
                                { id: "accent",      key: "titlebarAccent" },
                                { id: "transparent", key: "titlebarTransparent" },
                                { id: "oled",        key: "titlebarOled" }
                            ]
                            ModernButton {
                                Layout.fillWidth: true
                                highlighted: mouseController.uiTitlebarStyle === modelData.id
                                text: I18n.tr(mouseController.language, modelData.key)
                                onClicked: mouseController.uiTitlebarStyle = modelData.id
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ModernButton {
                text: I18n.tr(mouseController.language, "themeResetBtn")
                icon.name: "edit-undo"
                onClicked: mouseController.resetUiTheme()
            }

            Item { Layout.fillWidth: true }

            ModernButton {
                text: I18n.tr(mouseController.language, "closeBtn")
                icon.name: "dialog-ok-apply"
                highlighted: true
                onClicked: themeDialog.close()
            }
        }
    }
}
