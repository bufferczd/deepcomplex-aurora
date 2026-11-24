import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "help-about"
    id: commandExplainDialog
    title: I18n.tr(mouseController.language, "explainCmdTitle")
    width: Math.min((parent ? parent.width : 960) - 36, 820)
    height: Math.min((parent ? parent.height : 820) - 48, 690)
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside


    property bool showDevice: true
    property bool showTray: true
    property bool showHost: true
    property bool showUdev: true
    property bool showReset: true
    property string copiedFeedback: ""

    Timer {
        id: ceCopyTimer
        interval: 3000
        onTriggered: commandExplainDialog.copiedFeedback = ""
    }

    function openForSelection(incDevice, incTray, incHost, incUdev, incReset) {
        showDevice = incDevice
        showTray = incTray
        showHost = incHost
        showUdev = incUdev
        showReset = incReset
        copiedFeedback = ""
        open()
    }

    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.85
            text: I18n.tr(mouseController.language, "explainCmdSub")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: commandExplainDialog.copiedFeedback.length > 0
            type: Kirigami.MessageType.Positive
            text: commandExplainDialog.copiedFeedback
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Kirigami.Theme.disabledTextColor

            ListView {
                id: explainListView
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                spacing: 12
                model: mouseController.commandExplanations(
                    commandExplainDialog.showDevice,
                    commandExplainDialog.showTray,
                    commandExplainDialog.showHost,
                    commandExplainDialog.showUdev,
                    commandExplainDialog.showReset
                )

                delegate: Rectangle {
                    width: ListView.view.width
                    implicitHeight: itemCol.implicitHeight + 24
                    radius: 8
                    color: Kirigami.Theme.alternateBackgroundColor
                    border.width: 1
                    border.color: modelData.riskLevel === "high"
                                  ? Kirigami.Theme.negativeTextColor
                                  : (modelData.riskLevel === "medium" ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.disabledTextColor)

                    ColumnLayout {
                        id: itemCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 12
                        spacing: 8

                        // العنوان + شارة الخطورة + شارة أساسي/اختياري
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Kirigami.Icon {
                                width: 24
                                height: 24
                                source: modelData.icon
                            }

                            Controls.Label {
                                Layout.fillWidth: true
                                font.bold: true
                                wrapMode: Text.WordWrap
                                text: modelData.title
                            }

                            Rectangle {
                                radius: 4
                                implicitWidth: ceRiskLbl.implicitWidth + 12
                                implicitHeight: ceRiskLbl.implicitHeight + 4
                                color: modelData.riskLevel === "high"
                                       ? Qt.rgba(0.92, 0.18, 0.18, 0.24)
                                       : (modelData.riskLevel === "medium"
                                          ? Qt.rgba(0.95, 0.65, 0.0, 0.22)
                                          : Qt.rgba(0.0, 0.80, 0.45, 0.18))

                                Controls.Label {
                                    id: ceRiskLbl
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

                            Rectangle {
                                radius: 4
                                implicitWidth: badgeLbl.implicitWidth + 12
                                implicitHeight: badgeLbl.implicitHeight + 4
                                color: modelData.optional
                                       ? Qt.rgba(0.2, 0.55, 0.95, 0.18)
                                       : Qt.rgba(0.0, 0.85, 0.45, 0.18)

                                Controls.Label {
                                    id: badgeLbl
                                    anchors.centerIn: parent
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    text: modelData.optional
                                          ? I18n.tr(mouseController.language, "badgeOptional")
                                          : I18n.tr(mouseController.language, "badgeRequired")
                                }
                            }
                        }

                        // صندوق الأمر بخط monospace واتجاه LTR مع زر نسخ فردي
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: cmdLineRow.implicitHeight + 12
                            radius: 6
                            color: Kirigami.Theme.backgroundColor
                            border.width: 1
                            border.color: Kirigami.Theme.disabledTextColor

                            RowLayout {
                                id: cmdLineRow
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: 6
                                spacing: 8

                                Controls.Label {
                                    Layout.fillWidth: true
                                    LayoutMirroring.enabled: false
                                    horizontalAlignment: Text.AlignLeft
                                    wrapMode: Text.WrapAnywhere
                                    font.family: "monospace"
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    text: "\u200E" + modelData.command
                                }

                                Controls.ToolButton {
                                    icon.name: "edit-copy"
                                    Controls.ToolTip.visible: hovered
                                    Controls.ToolTip.text: I18n.tr(mouseController.language, "copyCmdBtn")
                                    onClicked: {
                                        mouseController.copyCommandToClipboard(modelData.command)
                                        commandExplainDialog.copiedFeedback = I18n.tr(mouseController.language, "copiedFeedback")
                                        ceCopyTimer.restart()
                                    }
                                }
                            }
                        }

                        // 1. ماذا يفعل الأمر
                        Controls.Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: "<b>" + I18n.tr(mouseController.language, "explainWhatItDoes") + "</b> " + modelData.purpose
                            textFormat: Text.StyledText
                        }

                        // 2. تفكيك الأمر كلمةً بكلمة (التفاصيل التقنية الكاملة)
                        Rectangle {
                            Layout.fillWidth: true
                            visible: modelData.breakdown && modelData.breakdown.length > 0
                            implicitHeight: bdCol.implicitHeight + 14
                            radius: 6
                            color: Kirigami.Theme.backgroundColor
                            border.width: 1
                            border.color: Qt.rgba(0.5, 0.5, 0.5, 0.25)

                            ColumnLayout {
                                id: bdCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.margins: 7
                                spacing: 4

                                Controls.Label {
                                    Layout.fillWidth: true
                                    font.bold: true
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    text: I18n.tr(mouseController.language, "explainBreakdownTitle")
                                }

                                Controls.Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    opacity: 0.90
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    text: modelData.breakdown
                                }
                            }
                        }

                        // 3. ما الذي يسمح به وما الذي يبقى ممنوعاً
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: modelData.boundaries && modelData.boundaries.length > 0
                            spacing: 2

                            Controls.Label {
                                Layout.fillWidth: true
                                font.bold: true
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: I18n.tr(mouseController.language, "explainBoundariesTitle")
                            }

                            Controls.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                opacity: 0.88
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: modelData.boundaries
                            }
                        }

                        // 4. مستوى الخطورة والملاحظة الأمنية
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Kirigami.Icon {
                                width: 16
                                height: 16
                                Layout.alignment: Qt.AlignTop
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
                                text: "<b>" + I18n.tr(mouseController.language, "explainSecurityNote") + "</b> " + modelData.impact
                                textFormat: Text.StyledText
                            }
                        }

                        // 5. كيفية التحقق والتراجع
                        ColumnLayout {
                            Layout.fillWidth: true
                            visible: modelData.verifyUndo && modelData.verifyUndo.length > 0
                            spacing: 2

                            Controls.Label {
                                Layout.fillWidth: true
                                font.bold: true
                                opacity: 0.8
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: I18n.tr(mouseController.language, "explainVerifyUndoTitle")
                            }

                            Controls.Label {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                opacity: 0.82
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                text: modelData.verifyUndo
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Item { Layout.fillWidth: true }

            ModernButton {
                text: I18n.tr(mouseController.language, "closeBtn")
                icon.name: "dialog-close"
                highlighted: true
                onClicked: commandExplainDialog.close()
            }
        }
    }
}
