import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.ScrollView {
    id: studioPage
    clip: true
    contentWidth: availableWidth
    padding: 18

    property color accentColor: "#38bdf8"

    signal closePageRequested()
    signal newMacroRequested()
    signal editMacroRequested(int macroIndex)
    signal exportMacroRequested(int macroIndex)
    signal importMacroRequested()

    LayoutMirroring.enabled: mouseController.language === "ar"
    LayoutMirroring.childrenInherit: true

    ColumnLayout {
        width: studioPage.availableWidth
        spacing: Kirigami.Units.largeSpacing

        // شريط إجراءات الاستوديو الزجاجي
        GlassCard {
            activeGlow: true
            accentColor: studioPage.accentColor
            cardPadding: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ModernButton {
                    text: I18n.tr(mouseController.language, "newMacro")
                    icon.name: "list-add"
                    highlighted: true
                    onClicked: studioPage.newMacroRequested()
                }

                ModernButton {
                    text: I18n.tr(mouseController.language, "importMacroBtn")
                    icon.name: "document-import"
                    onClicked: studioPage.importMacroRequested()
                }

                Item { Layout.fillWidth: true }

                Controls.Label {
                    opacity: 0.78
                    font.bold: true
                    text: mouseController.macroLibrary.length + " " + I18n.tr(mouseController.language, "macroStudioTitle")
                }
            }
        }

        // اختصارات سريعة لإضافة ماكرو جاهز بنقرة واحدة
        GlassCard {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Controls.Label {
                    font.bold: true
                    text: I18n.tr(mouseController.language, "quickPresets")
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: [
                            { id: "copy", label: "+ Ctrl+C" },
                            { id: "paste", label: "+ Ctrl+V" },
                            { id: "undo", label: "+ Ctrl+Z" },
                            { id: "alttab", label: "+ Alt+Tab" },
                            { id: "desktop", label: "+ Super+D" },
                            { id: "prtsc", label: "+ PrtSc" }
                        ]
                        ModernButton {
                            Layout.fillWidth: true
                            text: modelData.label
                            onClicked: mouseController.createQuickPresetInLibrary(modelData.id)
                        }
                    }
                }
            }
        }

        // رسالة في حال كانت المكتبة فارغة
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: mouseController.macroLibrary.length === 0
            text: I18n.tr(mouseController.language, "noMacrosInLib")
            type: Kirigami.MessageType.Information
        }

        // قائمة الماكروهات الجاهزة (بعدد غير محدود)
        Repeater {
            model: mouseController.macroLibrary

            GlassCard {
                Layout.fillWidth: true
                RowLayout {
                Layout.fillWidth: true
                    spacing: Kirigami.Units.mediumSpacing

                    Kirigami.Icon {
                        width: 32
                        height: 32
                        source: "input-keyboard"
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Controls.Label {
                            Layout.fillWidth: true
                            font.bold: true
                            elide: Text.ElideRight
                            text: modelData.name
                        }

                        Controls.Label {
                            Layout.fillWidth: true
                            opacity: 0.75
                            elide: Text.ElideRight
                            text: modelData.summary
                        }
                    }

                    ModernButton {
                        text: I18n.tr(mouseController.language, "exportMacroBtn")
                        icon.name: "document-export"
                        onClicked: studioPage.exportMacroRequested(modelData.index)
                    }

                    ModernButton {
                        text: I18n.tr(mouseController.language, "editMacro")
                        icon.name: "document-edit"
                        onClicked: studioPage.editMacroRequested(modelData.index)
                    }

                    ModernButton {
                        text: I18n.tr(mouseController.language, "deleteMacro")
                        icon.name: "edit-delete"
                        onClicked: mouseController.deleteLibraryMacro(modelData.index)
                    }
                }
            }
        }
    }
}
