import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "dialog-warning"
    id: repairDialog
    title: I18n.tr(mouseController.language, "repairTitle")
    signal repairOpened()
    signal explainUdevCommandRequested()
    onOpened: repairOpened()
    width: Math.min((parent ? parent.width : 960) - 48, 620)
    closePolicy: Controls.Popup.CloseOnEscape


    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Kirigami.Icon {
                width: 36
                height: 36
                source: mouseController.udevHealthy ? "dialog-ok-apply" : "dialog-error"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Kirigami.Heading {
                    Layout.fillWidth: true
                    level: 3
                    text: mouseController.udevHealthy ? I18n.tr(mouseController.language, "driverInstalled") : I18n.tr(mouseController.language, "repairSub")
                }

                Controls.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    visible: mouseController.udevErrorReason.length > 0
                    color: Kirigami.Theme.negativeTextColor
                    text: mouseController.udevErrorReason
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: I18n.tr(mouseController.language, "repairSteps")
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: repairCmdLbl.implicitHeight + 14
            radius: 6
            color: Kirigami.Theme.alternateBackgroundColor
            border.width: 1
            border.color: Kirigami.Theme.disabledTextColor

            Controls.Label {
                id: repairCmdLbl
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 8
                LayoutMirroring.enabled: false
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WrapAnywhere
                font.family: "monospace"
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                opacity: 0.88
                text: "\u200E" + mouseController.udevManualCommand()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ModernButton {
                visible: mouseController.hostAccessAvailable
                text: I18n.tr(mouseController.language, "repairNowBtn")
                icon.name: "dialog-password"
                highlighted: true
                onClicked: mouseController.installUdevRules()
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "copyManualCmd")
                icon.name: "edit-copy"
                highlighted: !mouseController.hostAccessAvailable
                onClicked: mouseController.copyUdevManualCommand()
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "explainCmdBtn")
                icon.name: "help-about"
                onClicked: repairDialog.explainUdevCommandRequested()
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "recheckBtn")
                icon.name: "view-refresh"
                onClicked: mouseController.checkUdevHealth()
            }

            Item { Layout.fillWidth: true }

            ModernButton {
                text: I18n.tr(mouseController.language, "closeBtn")
                icon.name: "dialog-close"
                onClicked: repairDialog.close()
            }
        }
    }
}
