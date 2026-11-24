import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Rectangle {
    id: segTabs

    property int currentIndex: 0
    property var tabs: []
    property color accentColor: Theme.accent
    property bool rtl: false

    Layout.fillWidth: true
    implicitHeight: 46
    radius: Theme.buttonShape === "sharp" ? 0 : (Theme.buttonShape === "pill" ? Math.round(height * 0.5) : Theme.buttonRadius)
    color: Theme.sidebarBg
    border.width: 1
    border.color: Theme.borderSubtle

    readonly property int tabCount: Math.max(1, tabs.length)
    readonly property real pad: 4
    readonly property real itemWidth: (width - pad * 2) / tabCount
    readonly property int visualIndex: rtl ? (tabCount - 1 - currentIndex) : currentIndex

    Rectangle {
        id: activePill
        y: segTabs.pad
        height: segTabs.height - segTabs.pad * 2
        width: Math.max(0, segTabs.itemWidth)
        x: segTabs.pad + segTabs.visualIndex * segTabs.itemWidth
        radius: Theme.buttonShape === "sharp" ? 0 : (Theme.buttonShape === "pill" ? Math.round(height * 0.5) : Theme.buttonRadius)
        color: Qt.rgba(segTabs.accentColor.r, segTabs.accentColor.g, segTabs.accentColor.b, Theme.isLight ? 0.22 : 0.28)
        border.width: 1.2
        border.color: Qt.rgba(segTabs.accentColor.r, segTabs.accentColor.g, segTabs.accentColor.b, 0.72)

        Behavior on x { NumberAnimation { duration: Theme.animatedUi ? 280 : 0; easing.type: Easing.OutCubic } }
        Behavior on color { ColorAnimation { duration: 260 } }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: segTabs.pad
        spacing: 0
        LayoutMirroring.enabled: segTabs.rtl
        LayoutMirroring.childrenInherit: true

        Repeater {
            model: segTabs.tabs

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Kirigami.Icon {
                        width: 18
                        height: 18
                        source: modelData.icon
                        color: segTabs.currentIndex === index ? Theme.accent : Theme.textSecondary
                        isMask: true
                    }

                    Controls.Label {
                        text: modelData.label
                        font.bold: segTabs.currentIndex === index
                        color: segTabs.currentIndex === index ? Theme.textPrimary : Theme.textSecondary
                        Behavior on color { ColorAnimation { duration: 180 } }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: segTabs.currentIndex = index
                }
            }
        }
    }
}
