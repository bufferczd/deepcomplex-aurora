import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.Dialog {
    id: dlg

    property bool showHeaderCloseButton: true
    property bool hideHeader: false
    property string headerIcon: ""

    modal: true
    padding: 16
    topPadding: 14
    bottomPadding: Theme.windowShape === "chamfer" ? Math.max(18, Theme.windowChamferCut) : 16
    anchors.centerIn: Controls.Overlay.overlay

    LayoutMirroring.enabled: mouseController.language === "ar"
    LayoutMirroring.childrenInherit: true

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: Theme.animatedUi ? 220 : 0; easing.type: Easing.OutCubic }
            NumberAnimation { property: "scale"; from: 0.93; to: 1.0; duration: Theme.animatedUi ? 230 : 0; easing.type: Easing.OutBack }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: Theme.animatedUi ? 150 : 0; easing.type: Easing.InCubic }
            NumberAnimation { property: "scale"; from: 1.0; to: 0.96; duration: Theme.animatedUi ? 150 : 0; easing.type: Easing.InCubic }
        }
    }

    Controls.Overlay.modal: Rectangle {
        color: Theme.isLight ? Qt.rgba(0.05, 0.10, 0.20, 0.34) : Qt.rgba(0.01, 0.02, 0.05, 0.66)
    }

    background: Item {
        // Optional outer rim shadow glow
        Rectangle {
            visible: Theme.windowEdgeGlow && Theme.borderStyle !== "none" && Theme.windowShape !== "chamfer"
            anchors.fill: parent
            anchors.margins: -5
            radius: Theme.dialogRadius === 0 ? 0 : (Theme.dialogRadius + 5)
            color: Qt.rgba((Theme.borderStyle === "custom" ? Theme.customBorderColor : Theme.accent).r,
                           (Theme.borderStyle === "custom" ? Theme.customBorderColor : Theme.accent).g,
                           (Theme.borderStyle === "custom" ? Theme.customBorderColor : Theme.accent).b,
                           0.11 * Theme.glowIntensity * Theme.borderOpacity)
            border.width: 1
            border.color: Qt.rgba((Theme.borderStyle === "custom" ? Theme.customBorderColor : Theme.accent).r,
                                  (Theme.borderStyle === "custom" ? Theme.customBorderColor : Theme.accent).g,
                                  (Theme.borderStyle === "custom" ? Theme.customBorderColor : Theme.accent).b,
                                  0.16 * Theme.glowIntensity * Theme.borderOpacity)
        }

        // Standard frosted glass surface (sharp 0px / rounded / extra)
        Rectangle {
            visible: Theme.windowShape !== "chamfer"
            anchors.fill: parent
            radius: Theme.dialogRadius
            color: Theme.dialogBg

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Theme.dialogRadius
                anchors.rightMargin: Theme.dialogRadius
                height: 1
                color: Theme.specularTop
            }
        }

        // Chamfered 45° frosted glass surface when windowShape === "chamfer"
        Canvas {
            id: chamferDialogBg
            visible: Theme.windowShape === "chamfer"
            anchors.fill: parent
            renderStrategy: Canvas.Cooperative
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.clearRect(0, 0, width, height)
                if (width < 6 || height < 6) return
                var cut = Theme.windowChamferCut
                ctx.beginPath()
                ctx.moveTo(cut, 0)
                ctx.lineTo(width - cut, 0)
                ctx.lineTo(width, cut)
                ctx.lineTo(width, height - cut)
                ctx.lineTo(width - cut, height)
                ctx.lineTo(cut, height)
                ctx.lineTo(0, height - cut)
                ctx.lineTo(0, cut)
                ctx.closePath()
                ctx.fillStyle = Theme.dialogBg
                ctx.fill()
            }
            onVisibleChanged: if (visible) requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            Connections {
                target: Theme
                function onDialogBgChanged() { chamferDialogBg.requestPaint() }
                function onWindowShapeChanged() { chamferDialogBg.requestPaint() }
                function onCornerRadiusChanged() { chamferDialogBg.requestPaint() }
            }
        }

        // Synchronized Window Border Frame (supports all 6 border styles + all 4 window shapes)
        WindowBorderFrame {
            anchors.fill: parent
            frameRadius: Theme.dialogRadius
            frameWidth: Math.max(0.0, Theme.borderWidth + 0.2)
            styleMode: Theme.borderStyle
            shapeMode: Theme.windowShape
            borderAlpha: Theme.borderOpacity
            edgeGlow: Theme.windowEdgeGlow
        }
    }

    header: Rectangle {
        visible: !dlg.hideHeader && dlg.title.length > 0
        implicitHeight: visible ? 50 : 0
        radius: Theme.windowShape === "chamfer" ? 0 : Theme.dialogRadius
        color: Theme.windowShape === "chamfer"
               ? "transparent"
               : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.isLight ? 0.10 : 0.13)

        Rectangle {
            visible: Theme.windowShape !== "chamfer" && parent.radius > 0
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.radius
            color: parent.color
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Theme.windowShape === "chamfer" ? Theme.windowChamferCut : 0
            anchors.rightMargin: Theme.windowShape === "chamfer" ? Theme.windowChamferCut : 0
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.borderSubtle
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.windowShape === "chamfer" ? 22 : 16
            anchors.rightMargin: Theme.windowShape === "chamfer" ? 20 : 12
            spacing: 10
            LayoutMirroring.enabled: mouseController.language === "ar"
            LayoutMirroring.childrenInherit: true

            Kirigami.Icon {
                visible: dlg.headerIcon.length > 0
                width: 20
                height: 20
                source: dlg.headerIcon
                color: Theme.accent
                isMask: true
            }

            Controls.Label {
                Layout.fillWidth: true
                text: dlg.title
                font.bold: true
                font.pixelSize: 15
                elide: Text.ElideRight
                color: Theme.textPrimary
            }

            Rectangle {
                visible: dlg.showHeaderCloseButton && dlg.closePolicy !== Controls.Popup.NoAutoClose
                width: 28
                height: 28
                radius: Theme.windowButtonStyle === "pill" ? 7 : 14
                color: closeHover.hovered ? Qt.rgba(0.94, 0.27, 0.27, 0.85) : Qt.rgba(1, 1, 1, Theme.isLight ? 0.35 : 0.08)
                border.width: 1
                border.color: closeHover.hovered ? "#ef4444" : Theme.borderSubtle
                Behavior on color { ColorAnimation { duration: 140 } }

                HoverHandler {
                    id: closeHover
                    cursorShape: Qt.PointingHandCursor
                }

                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: 14
                    height: 14
                    source: "window-close"
                    isMask: true
                    color: closeHover.hovered ? "#FFFFFF" : Theme.textPrimary
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: dlg.close()
                }
            }
        }
    }
}
