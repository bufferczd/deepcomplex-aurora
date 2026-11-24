import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "security-high"
    id: permissionsDialog
    title: I18n.tr(mouseController.language, "permissionsTitle")
    width: Math.min((parent ? parent.width : 960) - 48, 740)
    height: Math.min((parent ? parent.height : 820) - 64, 660)
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside


    signal explainCommandsRequested()

    property string feedbackText: ""

    Timer {
        id: feedbackClearTimer
        interval: 3500
        onTriggered: permissionsDialog.feedbackText = ""
    }

    function showCopiedFeedback() {
        feedbackText = I18n.tr(mouseController.language, "copiedFeedback")
        feedbackClearTimer.restart()
    }

    onOpened: {
        feedbackText = ""
        mouseController.refreshAppPermissions()
    }

    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.85
            text: I18n.tr(mouseController.language, "permissionsSub")
        }

        // شريط ملخص التحقق الحي
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: summaryRow.implicitHeight + 16
            radius: 8
            color: Kirigami.Theme.alternateBackgroundColor
            border.width: 1
            border.color: mouseController.hostAccessAvailable ? Theme.accent : Kirigami.Theme.disabledTextColor

            RowLayout {
                id: summaryRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 8
                spacing: 10

                Controls.BusyIndicator {
                    visible: mouseController.permissionsChecking
                    running: mouseController.permissionsChecking
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                }

                Kirigami.Icon {
                    visible: !mouseController.permissionsChecking
                    width: 20
                    height: 20
                    source: "security-high"
                }

                Controls.Label {
                    Layout.fillWidth: true
                    font.bold: true
                    wrapMode: Text.WordWrap
                    text: mouseController.permissionsSummary
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: permissionsDialog.feedbackText.length > 0
            type: Kirigami.MessageType.Positive
            text: permissionsDialog.feedbackText
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Kirigami.Theme.disabledTextColor

            ListView {
                id: permissionsListView
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                spacing: 8
                model: mouseController.appPermissions

                delegate: Rectangle {
                    id: permCard
                    width: ListView.view.width
                    implicitHeight: cardCol.implicitHeight + 20
                    radius: 8
                    color: modelData.state === "ok"
                           ? Kirigami.Theme.alternateBackgroundColor
                           : (modelData.state === "unknown"
                              ? Qt.rgba(0.95, 0.65, 0.0, 0.08)
                              : Qt.rgba(0.6, 0.2, 0.2, 0.10))
                    border.width: 1
                    border.color: modelData.state === "unknown"
                                  ? Kirigami.Theme.neutralTextColor
                                  : (modelData.state === "ok" ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.negativeTextColor)

                    property bool hasCommand: modelData.enableCommand && modelData.enableCommand.length > 0
                    // كشف الأمر تلقائياً للصلاحيات غير المفعّلة أو غير المؤكدة، مع إمكانية كشفه يدوياً لأي صلاحية أخرى
                    property bool commandRevealed: modelData.state !== "ok"

                    ColumnLayout {
                        id: cardCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 10
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Kirigami.Icon {
                                width: 28
                                height: 28
                                source: modelData.icon
                                Layout.alignment: Qt.AlignTop
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Controls.Label {
                                        font.bold: true
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                        text: modelData.title
                                    }

                                    // شارة مستوى الخطورة
                                    Rectangle {
                                        radius: 4
                                        implicitWidth: riskLbl.implicitWidth + 12
                                        implicitHeight: riskLbl.implicitHeight + 4
                                        color: modelData.riskLevel === "high"
                                               ? Qt.rgba(0.92, 0.18, 0.18, 0.24)
                                               : (modelData.riskLevel === "medium"
                                                  ? Qt.rgba(0.95, 0.65, 0.0, 0.22)
                                                  : Qt.rgba(0.0, 0.80, 0.45, 0.18))

                                        Controls.Label {
                                            id: riskLbl
                                            anchors.centerIn: parent
                                            font.bold: true
                                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                                            text: modelData.riskLevel === "high"
                                                  ? I18n.tr(mouseController.language, "riskHighBadge")
                                                  : (modelData.riskLevel === "medium"
                                                     ? I18n.tr(mouseController.language, "riskMedBadge")
                                                     : I18n.tr(mouseController.language, "riskLowBadge"))
                                        }
                                    }

                                    // شارة الحالة الحالية
                                    Rectangle {
                                        radius: 4
                                        implicitWidth: statusLbl.implicitWidth + 12
                                        implicitHeight: statusLbl.implicitHeight + 4
                                        color: modelData.state === "unknown"
                                               ? Qt.rgba(0.95, 0.65, 0.0, 0.22)
                                               : (modelData.state === "ok"
                                                  ? Qt.rgba(0.0, 0.85, 0.45, 0.18)
                                                  : Qt.rgba(0.9, 0.2, 0.2, 0.20))

                                        Controls.Label {
                                            id: statusLbl
                                            anchors.centerIn: parent
                                            font.bold: true
                                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                                            text: modelData.state === "unknown"
                                                  ? I18n.tr(mouseController.language, "permUnknownBadge")
                                                  : (modelData.state === "ok"
                                                     ? I18n.tr(mouseController.language, "permEnabledBadge")
                                                     : I18n.tr(mouseController.language, "permDisabledBadge"))
                                        }
                                    }

                                    Controls.ToolButton {
                                        visible: permCard.hasCommand
                                        icon.name: permCard.commandRevealed ? "arrow-up" : "utilities-terminal"
                                        text: permCard.commandRevealed
                                              ? I18n.tr(mouseController.language, "hideCmdBtn")
                                              : I18n.tr(mouseController.language, "revealCmdBtn")
                                        onClicked: permCard.commandRevealed = !permCard.commandRevealed
                                    }
                                }

                                Controls.Label {
                                    Layout.fillWidth: true
                                    font.family: "monospace"
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    opacity: 0.72
                                    wrapMode: Text.WrapAnywhere
                                    text: modelData.flag
                                }

                                Controls.Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    opacity: 0.85
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    text: modelData.description
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: modelData.riskNote && modelData.riskNote.length > 0
                                    spacing: 6

                                    Kirigami.Icon {
                                        width: 14
                                        height: 14
                                        source: modelData.riskLevel === "high"
                                                ? "security-low"
                                                : (modelData.riskLevel === "medium" ? "security-medium" : "security-high")
                                    }

                                    Controls.Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                                        color: modelData.riskLevel === "high"
                                               ? Kirigami.Theme.negativeTextColor
                                               : (modelData.riskLevel === "medium" ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.textColor)
                                        text: "<b>" + I18n.tr(mouseController.language, "riskLabelPrefix") + "</b> " + modelData.riskNote
                                        textFormat: Text.StyledText
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    visible: modelData.checkNote && modelData.checkNote.length > 0
                                    spacing: 6

                                    Kirigami.Icon {
                                        width: 14
                                        height: 14
                                        source: modelData.state === "ok"
                                                ? "dialog-ok-apply"
                                                : (modelData.state === "unknown" ? "dialog-warning" : "dialog-information")
                                    }

                                    Controls.Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                        font.italic: true
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                                        opacity: 0.9
                                        text: modelData.checkNote
                                    }
                                }
                            }
                        }

                        // صندوق كشف ونسخ أمر التفعيل اليدوي (بدون أي تحكم برمجي)
                        Rectangle {
                            Layout.fillWidth: true
                            visible: permCard.hasCommand && permCard.commandRevealed
                            implicitHeight: cmdBoxCol.implicitHeight + 14
                            radius: 6
                            color: Kirigami.Theme.backgroundColor
                            border.width: 1
                            border.color: Kirigami.Theme.disabledTextColor

                            ColumnLayout {
                                id: cmdBoxCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: 7
                                spacing: 4

                                Controls.Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    opacity: 0.8
                                    text: I18n.tr(mouseController.language, "enableCmdLabel")
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Controls.Label {
                                        Layout.fillWidth: true
                                        LayoutMirroring.enabled: false
                                        horizontalAlignment: Text.AlignLeft
                                        wrapMode: Text.WrapAnywhere
                                        font.family: "monospace"
                                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                                        opacity: 0.92
                                        text: "\u200E" + modelData.enableCommand
                                    }

                                    ModernButton {
                                        text: I18n.tr(mouseController.language, "copyCmdBtn")
                                        icon.name: "edit-copy"
                                        highlighted: modelData.state !== "ok"
                                        onClicked: {
                                            mouseController.copyCommandToClipboard(modelData.enableCommand)
                                            permissionsDialog.showCopiedFeedback()
                                        }
                                    }
                                }
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
                text: I18n.tr(mouseController.language, "verifyPermsBtn")
                icon.name: "view-refresh"
                highlighted: true
                enabled: !mouseController.permissionsChecking
                onClicked: mouseController.refreshAppPermissions()
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "explainCmdBtn")
                icon.name: "help-about"
                onClicked: permissionsDialog.explainCommandsRequested()
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "permCopyResetBtn")
                icon.name: "edit-copy"
                onClicked: {
                    mouseController.copyPermissionsResetCommand()
                    permissionsDialog.showCopiedFeedback()
                }
            }

            Item { Layout.fillWidth: true }

            ModernButton {
                text: I18n.tr(mouseController.language, "closeBtn")
                icon.name: "dialog-close"
                onClicked: permissionsDialog.close()
            }
        }
    }
}
