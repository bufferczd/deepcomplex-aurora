import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "document-save"
    id: unsavedDialog
    property var pendingAction: null
    title: I18n.tr(mouseController.language, "unsavedTitle")
    width: Math.min((parent ? parent.width : 960) - 64, 540)
    closePolicy: Controls.Popup.CloseOnEscape


    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: I18n.tr(mouseController.language, "unsavedDesc")
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: Math.min(200, changesCol.implicitHeight + 20)
            clip: true
            radius: 6
            color: Kirigami.Theme.alternateBackgroundColor
            border.width: 1
            border.color: Kirigami.Theme.neutralTextColor

            Flickable {
                anchors.fill: parent
                anchors.margins: 10
                clip: true
                contentWidth: width
                contentHeight: changesCol.implicitHeight
                Controls.ScrollBar.vertical: Controls.ScrollBar { policy: Controls.ScrollBar.AsNeeded }

                ColumnLayout {
                    id: changesCol
                    width: parent.width
                    spacing: 6

                    Repeater {
                        model: mouseController.unsavedChangesList
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Kirigami.Icon {
                                width: 16
                                height: 16
                                source: "dialog-warning"
                            }
                            Controls.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                font.bold: true
                                text: modelData
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
                text: I18n.tr(mouseController.language, "unsavedDiscard")
                icon.name: "edit-delete"
                onClicked: {
                    unsavedDialog.close()
                    if (unsavedDialog.pendingAction) {
                        let fn = unsavedDialog.pendingAction
                        unsavedDialog.pendingAction = null
                        fn()
                    }
                }
            }

            Item { Layout.fillWidth: true }

            ModernButton {
                text: I18n.tr(mouseController.language, "cancelBtn")
                icon.name: "dialog-cancel"
                onClicked: {
                    unsavedDialog.pendingAction = null
                    unsavedDialog.close()
                }
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "unsavedApply")
                icon.name: "document-save"
                highlighted: true
                enabled: mouseController.connected && !mouseController.isBusy
                onClicked: {
                    unsavedDialog.close()
                    mouseController.applyToMouse()
                    unsavedDialog.pendingAction = null
                }
            }
        }
    }
}
