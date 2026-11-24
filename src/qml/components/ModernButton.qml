import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.Button {
    id: control

    property color accentColor: Theme.accent
    property bool danger: false

    readonly property color effectiveAccent: danger ? "#ef4444" : accentColor
    readonly property color secondaryAccent: danger ? "#f97316" : Theme.orbSecondary
    readonly property bool isActiveFill: control.highlighted || control.checked
    readonly property string shapeMode: Theme.buttonShape
    readonly property string styleMode: Theme.buttonStyle
    readonly property bool useCustomCanvas: shapeMode === "leaf" || shapeMode === "chamfer" || styleMode === "gradient"
    readonly property real effectiveRadius: {
        if (Theme.buttonShape === "sharp") return 0
        if (Theme.buttonShape === "pill") return Math.round(height * 0.5)
        if (Theme.buttonShape === "squircle") return Math.min(12, Math.round(height * 0.34))
        return Theme.buttonRadius
    }

    readonly property color computedFill: {
        let a = control.effectiveAccent
        if (styleMode === "outline") {
            if (control.isActiveFill) return Qt.rgba(a.r, a.g, a.b, control.hovered ? 0.28 : 0.16)
            if (control.down) return Qt.rgba(a.r, a.g, a.b, 0.22)
            if (control.hovered) return Qt.rgba(a.r, a.g, a.b, 0.12)
            return "transparent"
        }
        if (styleMode === "neon") {
            if (control.isActiveFill) return Qt.rgba(a.r, a.g, a.b, control.hovered ? 0.34 : 0.22)
            if (control.down) return Qt.rgba(a.r, a.g, a.b, 0.26)
            if (control.hovered) return Qt.rgba(a.r, a.g, a.b, 0.14)
            return Theme.isLight ? Qt.rgba(1, 1, 1, 0.90) : Qt.rgba(0.02, 0.03, 0.07, 0.92)
        }
        if (styleMode === "solid") {
            if (control.isActiveFill) return control.down ? Qt.darker(a, 1.15) : (control.hovered ? Qt.lighter(a, 1.08) : a)
            if (control.down) return Theme.buttonPressedBg
            if (control.hovered) return Theme.buttonHoverBg
            return Theme.isLight ? Qt.rgba(0.89, 0.93, 0.98, 0.98) : Qt.rgba(0.16, 0.21, 0.32, 0.96)
        }
        if (styleMode === "soft") {
            if (control.isActiveFill) return Qt.rgba(a.r, a.g, a.b, control.hovered ? 0.36 : 0.24)
            if (control.down) return Qt.rgba(a.r, a.g, a.b, 0.20)
            if (control.hovered) return Qt.rgba(a.r, a.g, a.b, 0.12)
            return Theme.isLight ? Qt.rgba(a.r, a.g, a.b, 0.07) : Qt.rgba(1, 1, 1, 0.06)
        }
        // Default: glass (also fallback for gradient non-active)
        if (control.isActiveFill) {
            if (control.down) return Qt.rgba(a.r, a.g, a.b, 0.85)
            if (control.hovered) return Qt.rgba(a.r, a.g, a.b, 0.95)
            return Qt.rgba(a.r, a.g, a.b, Theme.isLight ? 0.88 : 0.34)
        }
        if (control.down) return Theme.buttonPressedBg
        if (control.hovered) return Theme.buttonHoverBg
        return Theme.buttonBg
    }

    readonly property color computedBorder: {
        let a = control.effectiveAccent
        if (styleMode === "soft" && !control.isActiveFill && !control.hovered) return "transparent"
        if (styleMode === "neon") {
            return Qt.rgba(a.r, a.g, a.b, (control.isActiveFill || control.hovered) ? 1.0 : 0.65)
        }
        if (styleMode === "outline") {
            return Qt.rgba(a.r, a.g, a.b, (control.isActiveFill || control.hovered) ? 0.95 : 0.55)
        }
        if (control.isActiveFill) return Qt.rgba(a.r, a.g, a.b, 0.88)
        if (control.hovered && control.enabled) return Qt.rgba(a.r, a.g, a.b, 0.58)
        return Theme.borderSubtle
    }

    readonly property real computedBorderWidth: {
        if (styleMode === "soft" && !control.isActiveFill && !control.hovered) return 0
        if (styleMode === "neon" || styleMode === "outline") return Math.max(1.4, Theme.borderWidth + 0.3)
        return control.isActiveFill ? Math.max(1.0, Theme.borderWidth + 0.3) : Theme.borderWidth
    }

    readonly property color computedTextColor: {
        if (control.isActiveFill) {
            if (styleMode === "solid" || styleMode === "gradient") return Theme.accentContrastText
            if (styleMode === "glass" && (Theme.isLight || control.hovered || control.down)) return Theme.accentContrastText
        }
        return Theme.textPrimary
    }

    implicitHeight: Math.max(36, contentItem.implicitHeight + 14)
    implicitWidth: Math.max(44, contentItem.implicitWidth + 24)
    padding: 8
    leftPadding: 12
    rightPadding: 12
    hoverEnabled: true

    scale: (control.down && Theme.animatedUi) ? 0.97 : 1.0
    Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }

    HoverHandler {
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    background: Item {
        opacity: control.enabled ? 1.0 : 0.42

        // Optional outer neon halo when styleMode === "neon"
        Rectangle {
            visible: !control.useCustomCanvas && control.styleMode === "neon" && (control.isActiveFill || control.hovered)
            anchors.fill: parent
            anchors.margins: -3
            radius: control.effectiveRadius + 3
            color: Qt.rgba(control.effectiveAccent.r, control.effectiveAccent.g, control.effectiveAccent.b, 0.16 * Math.max(0.35, Theme.glowIntensity))
            border.width: 1
            border.color: Qt.rgba(control.effectiveAccent.r, control.effectiveAccent.g, control.effectiveAccent.b, 0.32 * Math.max(0.35, Theme.glowIntensity))
        }

        // Fast native Rectangle for rounded / pill / sharp / squircle shapes
        Rectangle {
            visible: !control.useCustomCanvas
            anchors.fill: parent
            radius: control.effectiveRadius
            color: control.computedFill
            border.width: control.computedBorderWidth
            border.color: control.computedBorder

            Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
            Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

            Rectangle {
                visible: control.styleMode === "glass" || control.styleMode === "solid"
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: parent.radius === 0 ? 0 : Math.min(parent.width * 0.25, parent.radius * 0.7)
                anchors.rightMargin: parent.radius === 0 ? 0 : Math.min(parent.width * 0.25, parent.radius * 0.7)
                height: 1
                color: control.isActiveFill ? Qt.rgba(1, 1, 1, 0.35) : Theme.specularTop
            }
        }

        // Antialiased Canvas for geometric shapes (leaf / chamfer) and dual-tone gradient fills
        Canvas {
            id: shapeCanvas
            visible: control.useCustomCanvas
            anchors.fill: parent
            renderStrategy: Canvas.Cooperative

            property color fillProp: control.computedFill
            property color borderProp: control.computedBorder
            property string sMode: control.shapeMode
            property string vMode: control.styleMode
            property bool activeProp: control.isActiveFill
            property bool hovProp: control.hovered
            property bool downProp: control.down

            onFillPropChanged: requestPaint()
            onBorderPropChanged: requestPaint()
            onSModeChanged: requestPaint()
            onVModeChanged: requestPaint()
            onActivePropChanged: requestPaint()
            onHovPropChanged: requestPaint()
            onDownPropChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onVisibleChanged: if (visible) requestPaint()
            Component.onCompleted: requestPaint()

            Connections {
                target: Theme
                function onAccentChanged() { shapeCanvas.requestPaint() }
                function onOrbSecondaryChanged() { shapeCanvas.requestPaint() }
                function onCornerRadiusChanged() { shapeCanvas.requestPaint() }
                function onBorderWidthChanged() { shapeCanvas.requestPaint() }
                function onButtonShapeChanged() { shapeCanvas.requestPaint() }
                function onButtonStyleChanged() { shapeCanvas.requestPaint() }
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.clearRect(0, 0, width, height)
                if (width < 4 || height < 4) return

                var bw = control.computedBorderWidth
                var pad = bw * 0.5 + 0.5
                var x = pad, y = pad, w = width - pad * 2, h = height - pad * 2

                ctx.beginPath()
                if (control.shapeMode === "chamfer") {
                    var cut = Math.min(10, Math.min(w, h) * 0.28)
                    ctx.moveTo(x + cut, y)
                    ctx.lineTo(x + w - cut, y)
                    ctx.lineTo(x + w, y + cut)
                    ctx.lineTo(x + w, y + h - cut)
                    ctx.lineTo(x + w - cut, y + h)
                    ctx.lineTo(x + cut, y + h)
                    ctx.lineTo(x, y + h - cut)
                    ctx.lineTo(x, y + cut)
                    ctx.closePath()
                } else if (control.shapeMode === "leaf") {
                    var rBig = Math.min(18, h * 0.48)
                    var rSmall = 3
                    ctx.moveTo(x + rSmall, y)
                    ctx.lineTo(x + w - rBig, y)
                    ctx.quadraticCurveTo(x + w, y, x + w, y + rBig)
                    ctx.lineTo(x + w, y + h - rSmall)
                    ctx.quadraticCurveTo(x + w, y + h, x + w - rSmall, y + h)
                    ctx.lineTo(x + rBig, y + h)
                    ctx.quadraticCurveTo(x, y + h, x, y + h - rBig)
                    ctx.lineTo(x, y + rSmall)
                    ctx.quadraticCurveTo(x, y, x + rSmall, y)
                    ctx.closePath()
                } else {
                    var r = Math.min(control.effectiveRadius, h * 0.5, w * 0.5)
                    ctx.moveTo(x + r, y)
                    ctx.lineTo(x + w - r, y)
                    ctx.quadraticCurveTo(x + w, y, x + w, y + r)
                    ctx.lineTo(x + w, y + h - r)
                    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
                    ctx.lineTo(x + r, y + h)
                    ctx.quadraticCurveTo(x, y + h, x, y + h - r)
                    ctx.lineTo(x, y + r)
                    ctx.quadraticCurveTo(x, y, x + r, y)
                    ctx.closePath()
                }

                if (control.styleMode === "gradient") {
                    var g = ctx.createLinearGradient(0, 0, width, height)
                    var c1 = control.effectiveAccent
                    var c2 = control.secondaryAccent
                    if (control.isActiveFill) {
                        var alpha = control.down ? 0.85 : (control.hovered ? 1.0 : 0.90)
                        g.addColorStop(0.0, Qt.rgba(c1.r, c1.g, c1.b, alpha))
                        g.addColorStop(1.0, Qt.rgba(c2.r, c2.g, c2.b, alpha))
                    } else {
                        var subA = control.hovered ? 0.30 : 0.16
                        g.addColorStop(0.0, Qt.rgba(c1.r, c1.g, c1.b, subA))
                        g.addColorStop(1.0, Qt.rgba(c2.r, c2.g, c2.b, subA))
                    }
                    ctx.fillStyle = g
                    ctx.fill()
                } else if (control.computedFill !== "transparent") {
                    ctx.fillStyle = control.computedFill
                    ctx.fill()
                }

                if (bw > 0.05 && control.computedBorder !== "transparent") {
                    ctx.lineWidth = bw
                    if (control.styleMode === "gradient") {
                        var bg = ctx.createLinearGradient(0, 0, width, height)
                        bg.addColorStop(0.0, control.effectiveAccent)
                        bg.addColorStop(1.0, control.secondaryAccent)
                        ctx.strokeStyle = bg
                    } else {
                        ctx.strokeStyle = control.computedBorder
                    }
                    ctx.stroke()
                }
            }
        }
    }

    contentItem: RowLayout {
        spacing: (control.text.length > 0 && control.icon.name.length > 0) ? 7 : 0
        opacity: control.enabled ? 1.0 : 0.55

        Item { Layout.fillWidth: true }

        Kirigami.Icon {
            visible: control.icon.name.length > 0
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            source: control.icon.name
            isMask: true
            color: control.computedTextColor
        }

        Controls.Label {
            visible: control.text.length > 0
            text: control.text
            font.bold: control.isActiveFill
            elide: Text.ElideRight
            Layout.maximumWidth: Math.max(40, control.width - (control.icon.name.length > 0 ? 44 : 20))
            color: control.computedTextColor
        }

        Item { Layout.fillWidth: true }
    }
}
