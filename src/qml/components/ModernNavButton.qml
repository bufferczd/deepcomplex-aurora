import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Item {
    id: navBtn

    property string text: ""
    property string iconName: ""
    property bool active: false
    property string badgeText: ""
    property color accentColor: Theme.accent
    property bool rtl: false

    signal clicked()

    Layout.fillWidth: true
    implicitHeight: 44

    Rectangle {
        anchors.fill: parent
        radius: Theme.buttonShape === "sharp" ? 0 : (Theme.buttonShape === "pill" ? Math.round(parent.height * 0.5) : Theme.buttonRadius)
        color: navBtn.active
               ? Qt.rgba(navBtn.accentColor.r, navBtn.accentColor.g, navBtn.accentColor.b, Theme.isLight ? 0.18 : 0.22)
               : (mouseArea.containsMouse
                  ? (Theme.isLight ? Qt.rgba(0, 0, 0, 0.06) : Qt.rgba(1, 1, 1, 0.08))
                  : "transparent")
        border.width: navBtn.active ? 1.2 : 0
        border.color: Qt.rgba(navBtn.accentColor.r, navBtn.accentColor.g, navBtn.accentColor.b, 0.55)

        Behavior on color { ColorAnimation { duration: 180; easing.type: Easing.OutCubic } }

        Rectangle {
            width: 3
            height: navBtn.active ? parent.height * 0.58 : 0
            radius: 2
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 4
            color: navBtn.accentColor
            opacity: navBtn.active ? 1.0 : 0.0

            Behavior on height { NumberAnimation { duration: 220; easing.type: Easing.OutBack } }
            Behavior on opacity { NumberAnimation { duration: 180 } }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 12
            spacing: 10
            LayoutMirroring.enabled: navBtn.rtl
            LayoutMirroring.childrenInherit: true

            Kirigami.Icon {
                width: 20
                height: 20
                source: navBtn.iconName
                color: navBtn.active ? navBtn.accentColor : Theme.textSecondary
                isMask: true
            }

            Controls.Label {
                Layout.fillWidth: true
                text: navBtn.text
                font.bold: navBtn.active
                color: navBtn.active ? Theme.textPrimary : Theme.textSecondary
                elide: Text.ElideRight
            }

            Rectangle {
                visible: navBtn.badgeText.length > 0
                radius: 9
                implicitWidth: Math.max(20, badgeLbl.implicitWidth + 10)
                implicitHeight: 20
                color: navBtn.active
                       ? Qt.rgba(navBtn.accentColor.r, navBtn.accentColor.g, navBtn.accentColor.b, 0.35)
                       : Theme.borderSubtle

                Controls.Label {
                    id: badgeLbl
                    anchors.centerIn: parent
                    text: navBtn.badgeText
                    font.bold: true
                    font.pixelSize: 11
                    color: Theme.textPrimary
                }
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: navBtn.clicked()
        }
    }
}
