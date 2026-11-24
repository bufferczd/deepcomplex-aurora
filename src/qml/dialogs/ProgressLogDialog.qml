import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "utilities-log-viewer"
    id: progressLogDialog
    signal openRepairRequested()
    title: I18n.tr(mouseController.language, "logTitle")
    width: Math.min((parent ? parent.width : 960) - 64, 620)
    height: Math.min((parent ? parent.height : 820) - 96, 440)
    closePolicy: mouseController.isBusy ? Controls.Popup.NoAutoClose : (Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside)


    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Controls.BusyIndicator {
                running: mouseController.isBusy
                visible: mouseController.isBusy
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
            }

            Kirigami.Icon {
                visible: !mouseController.isBusy
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                source: mouseController.operationSuccess ? "dialog-ok-apply" : "dialog-error"
            }

            Controls.Label {
                Layout.fillWidth: true
                font.bold: true
                elide: Text.ElideRight
                text: mouseController.isBusy
                      ? (mouseController.operationTitle || I18n.tr(mouseController.language, "workingText"))
                      : (mouseController.operationSuccess ? I18n.tr(mouseController.language, "doneOkText") : I18n.tr(mouseController.language, "doneErrText"))
            }

            Controls.Label {
                font.bold: true
                text: mouseController.operationProgress + "%"
            }
        }

        Controls.ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 100
            value: mouseController.operationProgress
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Controls.TextArea {
                id: logArea
                readOnly: true
                wrapMode: TextEdit.WrapAnywhere
                font.family: "monospace"
                font.pointSize: 9
                text: mouseController.operationLog
                onTextChanged: cursorPosition = length
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ModernButton {
                text: I18n.tr(mouseController.language, "copyLog")
                icon.name: "edit-copy"
                enabled: mouseController.operationLog.length > 0
                onClicked: mouseController.copyOperationLog()
            }

            Item { Layout.fillWidth: true }

            ModernButton {
                visible: !mouseController.isBusy && !mouseController.operationSuccess
                text: I18n.tr(mouseController.language, "repairGuideBtn")
                icon.name: "tools-wizard"
                highlighted: true
                onClicked: {
                    progressLogDialog.close()
                    progressLogDialog.openRepairRequested()
                }
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "closeBtn")
                icon.name: "dialog-close"
                highlighted: !mouseController.isBusy && mouseController.operationSuccess
                enabled: !mouseController.isBusy
                onClicked: progressLogDialog.close()
            }
        }
    }
}
