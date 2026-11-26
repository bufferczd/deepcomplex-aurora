import QtQuick
import QtQuick.Layouts

Item {
    id: card

    default property alias contentData: bodyContainer.data
    property bool activeGlow: false
    property color accentColor: Theme.accent
    property int cardPadding: Theme.cardPad
    property bool hoverLift: true

    readonly property string edgeMode: Theme.cardEdgeStyle
    readonly property bool useFancyCanvas: edgeMode === "chamfer" || edgeMode === "gradient"

    readonly property color resolvedFill: activeGlow
        ? Qt.rgba(
            Theme.isLight ? (accentColor.r * 0.14 + 0.86) : (accentColor.r * 0.18 + Theme.cardBg.r * 0.85),
            Theme.isLight ? (accentColor.g * 0.14 + 0.86) : (accentColor.g * 0.18 + Theme.cardBg.g * 0.85),
            Theme.isLight ? (accentColor.b * 0.14 + 0.86) : (accentColor.b * 0.18 + Theme.cardBg.b * 0.85),
            Math.min(0.95, Theme.glassOpacity + 0.08)
          )
        : (cardHover.hovered && hoverLift ? Theme.cardHoverBg : Theme.cardBg)

    readonly property color resolvedBorder: {
        if (edgeMode === "flat" && !activeGlow) return "transparent"
        if (edgeMode === "neon" || activeGlow) {
            return Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.55 + Theme.glowIntensity * 0.40)
        }
        return (cardHover.hovered && hoverLift) ? Theme.borderHover : Theme.borderSubtle
    }

    readonly property real resolvedBorderWidth: {
        if (edgeMode === "flat" && !activeGlow) return 0
        if (edgeMode === "neon" || activeGlow) return Math.max(1.2, Theme.borderWidth + 0.4)
        return Theme.borderWidth
    }

    Layout.fillWidth: true
    implicitHeight: bodyContainer.implicitHeight + cardPadding * 2

    HoverHandler { id: cardHover }

    // Standard native card frame (glass / neon / flat)
    Rectangle {
        visible: !card.useFancyCanvas
        anchors.fill: parent
        radius: Theme.cardRadius
        color: card.resolvedFill
        border.width: card.resolvedBorderWidth
        border.color: card.resolvedBorder

        Behavior on color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: 200; easing.type: Easing.OutCubic } }

        Rectangle {
            visible: card.edgeMode !== "flat"
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: parent.radius
            anchors.rightMargin: parent.radius
            height: 1
            color: (card.activeGlow || card.edgeMode === "neon")
                   ? Qt.rgba(card.accentColor.r, card.accentColor.g, card.accentColor.b, 0.48)
                   : Theme.specularTop
        }
    }

    // Antialiased Canvas frame for Chamfered Cut Corners & Dual-Tone Gradient Card Borders
    Canvas {
        id: cardCanvas
        visible: card.useFancyCanvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative

        property color fillProp: card.resolvedFill
        property color accentProp: card.accentColor
        property string modeProp: card.edgeMode
        property bool glowProp: card.activeGlow
        property bool hovProp: cardHover.hovered

        onFillPropChanged: if (visible) requestPaint()
        onAccentPropChanged: if (visible) requestPaint()
        onModePropChanged: if (visible) requestPaint()
        onGlowPropChanged: if (visible) requestPaint()
        onHovPropChanged: if (visible) requestPaint()
        onWidthChanged: if (visible) requestPaint()
        onHeightChanged: if (visible) requestPaint()
        onVisibleChanged: if (visible) requestPaint()
        Component.onCompleted: if (visible) requestPaint()

        Connections {
            target: Theme
            function onOrbSecondaryChanged() { if (cardCanvas.visible) cardCanvas.requestPaint() }
            function onCornerRadiusChanged() { if (cardCanvas.visible) cardCanvas.requestPaint() }
            function onBorderWidthChanged() { if (cardCanvas.visible) cardCanvas.requestPaint() }
            function onGlowIntensityChanged() { if (cardCanvas.visible) cardCanvas.requestPaint() }
        }

        onPaint: {
            if (!visible || width < 6 || height < 6) return
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            var bw = Math.max(1.0, card.resolvedBorderWidth)
            var pad = bw * 0.5 + 0.5
            var x = pad, y = pad, w = width - pad * 2, h = height - pad * 2

            ctx.beginPath()
            if (card.edgeMode === "chamfer") {
                var cut = Math.min(16, Math.min(w, h) * 0.22)
                ctx.moveTo(x + cut, y)
                ctx.lineTo(x + w - cut, y)
                ctx.lineTo(x + w, y + cut)
                ctx.lineTo(x + w, y + h - cut)
                ctx.lineTo(x + w - cut, y + h)
                ctx.lineTo(x + cut, y + h)
                ctx.lineTo(x, y + h - cut)
                ctx.lineTo(x, y + cut)
                ctx.closePath()
            } else {
                var r = Math.min(Theme.cardRadius, w * 0.5, h * 0.5)
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

            ctx.fillStyle = card.resolvedFill
            ctx.fill()

            var grad = ctx.createLinearGradient(0, 0, width, height)
            var c1 = card.accentColor
            var c2 = Theme.orbSecondary
            var alpha = (card.activeGlow || cardHover.hovered) ? 0.90 : (0.45 + Theme.glowIntensity * 0.30)
            grad.addColorStop(0.0, Qt.rgba(c1.r, c1.g, c1.b, alpha))
            grad.addColorStop(1.0, Qt.rgba(c2.r, c2.g, c2.b, alpha))
            ctx.lineWidth = bw
            ctx.strokeStyle = grad
            ctx.stroke()
        }
    }

    ColumnLayout {
        id: bodyContainer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: card.cardPadding
        spacing: Theme.compactMode ? 7 : 10
    }
}
