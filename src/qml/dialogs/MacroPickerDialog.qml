import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "input-keyboard"
    id: macroPickerDialog
    signal openStudioRequested()
    title: I18n.tr(mouseController.language, "pickMacroTitle") + (buttonTitle.length > 0 ? (" — " + buttonTitle) : "")
    width: Math.min((parent ? parent.width : 960) - 64, 560)
    height: Math.min((parent ? parent.height : 820) - 96, 480)
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside


    property int targetSlot: 3
    property string buttonTitle: ""

    function openForSlot(slot, titleStr) {
        targetSlot = slot
        buttonTitle = titleStr
        open()
    }

    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 6
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Kirigami.Theme.disabledTextColor

            Controls.Label {
                anchors.centerIn: parent
                width: parent.width - 32
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: mouseController.macroLibrary.length === 0
                opacity: 0.7
                text: I18n.tr(mouseController.language, "noMacrosInLib")
            }

            ListView {
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                spacing: 6
                model: mouseController.macroLibrary
                delegate: Controls.ItemDelegate {
                    width: ListView.view.width
                    height: 54
                    onClicked: {
                        mouseController.assignLibraryMacroToButton(macroPickerDialog.targetSlot, modelData.index)
                        macroPickerDialog.close()
                    }

                    background: Rectangle {
                        radius: 6
                        color: parent.hovered
                               ? Qt.rgba(0.0, 0.9, 0.45, 0.18)
                               : Kirigami.Theme.alternateBackgroundColor
                        border.width: 1
                        border.color: parent.hovered ? Theme.accent : Kirigami.Theme.disabledTextColor
                    }

                    contentItem: RowLayout {
                        spacing: 10

                        Kirigami.Icon {
                            width: 24
                            height: 24
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
                                opacity: 0.72
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                elide: Text.ElideRight
                                text: modelData.summary
                            }
                        }

                        Kirigami.Icon {
                            width: 18
                            height: 18
                            source: "dialog-ok-apply"
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ModernButton {
                text: I18n.tr(mouseController.language, "openStudioBtn")
                icon.name: "input-keyboard"
                onClicked: {
                    macroPickerDialog.close()
                    macroPickerDialog.openStudioRequested()
                }
            }

            Item { Layout.fillWidth: true }

            ModernButton {
                text: I18n.tr(mouseController.language, "cancelBtn")
                icon.name: "dialog-cancel"
                onClicked: macroPickerDialog.close()
            }
        }
    }
}
