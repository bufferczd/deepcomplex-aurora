import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "edit-undo"
    id: resetConfirmDialog
    title: I18n.tr(mouseController.language, "confirmResetTitle")
    width: Math.min((parent ? parent.width : 960) - 64, 520)
    closePolicy: Controls.Popup.CloseOnEscape

    property string messageText: ""
    property var pendingResetAction: null


    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Kirigami.Icon {
                width: 32
                height: 32
                source: "dialog-warning"
            }

            Controls.Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: resetConfirmDialog.messageText
            }
        }

        // إذا كانت هناك تعديلات غير محفوظة أيضاً، نعرضها للمستخدم داخل منطقة قابلة للتمرير
        Rectangle {
            Layout.fillWidth: true
            visible: mouseController.hasUnsavedChanges
            implicitHeight: Math.min(160, resetUnsavedCol.implicitHeight + 16)
            clip: true
            radius: 6
            color: Kirigami.Theme.alternateBackgroundColor
            border.width: 1
            border.color: Kirigami.Theme.neutralTextColor

            Flickable {
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                contentWidth: width
                contentHeight: resetUnsavedCol.implicitHeight
                Controls.ScrollBar.vertical: Controls.ScrollBar { policy: Controls.ScrollBar.AsNeeded }

                ColumnLayout {
                    id: resetUnsavedCol
                    width: parent.width
                    spacing: 4

                    Repeater {
                        model: mouseController.unsavedChangesList
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Kirigami.Icon { width: 14; height: 14; source: "dialog-warning" }
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

            Item { Layout.fillWidth: true }

            ModernButton {
                text: I18n.tr(mouseController.language, "cancelBtn")
                icon.name: "dialog-cancel"
                onClicked: {
                    resetConfirmDialog.pendingResetAction = null
                    resetConfirmDialog.close()
                }
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "confirmResetBtn")
                icon.name: "edit-undo"
                highlighted: true
                onClicked: {
                    let fn = resetConfirmDialog.pendingResetAction
                    resetConfirmDialog.pendingResetAction = null
                    resetConfirmDialog.close()
                    if (fn) fn()
                }
            }
        }
    }
}
