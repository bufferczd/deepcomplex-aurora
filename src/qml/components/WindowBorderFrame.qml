import QtQuick

Item {
    id: frameRoot

    property int frameRadius: Theme.windowRadius
    property real frameWidth: Math.max(0.0, Theme.borderWidth)
    property string styleMode: Theme.borderStyle // "accent", "gradient", "rainbow", "subtle", "custom", "none"
    property string shapeMode: Theme.windowShape // "sharp", "rounded", "extra", "chamfer"
    property real borderAlpha: Theme.borderOpacity
    property bool edgeGlow: Theme.windowEdgeGlow
    property bool isMaximized: false
    property bool animate: Theme.animatedUi

    readonly property bool activeVisible: styleMode !== "none" && frameWidth > 0.05
    readonly property int effectiveRadius: (isMaximized || shapeMode === "sharp") ? 0 : frameRadius
    readonly property bool isChamfered: !isMaximized && shapeMode === "chamfer"
    readonly property bool useCanvasRenderer: isChamfered || styleMode === "gradient" || styleMode === "rainbow"

    visible: activeVisible

    // Optional soft inner rim glow (non-chamfer fast path)
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        visible: !frameRoot.isChamfered && frameRoot.edgeGlow && frameRoot.styleMode !== "subtle"
        radius: Math.max(0, frameRoot.effectiveRadius - 1)
        color: "transparent"
        border.width: Math.min(6, frameRoot.frameWidth + 3.0)
        border.color: {
            let c = frameRoot.styleMode === "custom" ? Theme.customBorderColor : Theme.accent
            return Qt.rgba(c.r, c.g, c.b, 0.18 * Math.max(0.35, Theme.glowIntensity) * frameRoot.borderAlpha)
        }
    }

    // Fast native border for non-chamfered solid borders (accent / subtle / custom)
    Rectangle {
        anchors.fill: parent
        visible: !frameRoot.useCanvasRenderer
        radius: frameRoot.effectiveRadius
        color: "transparent"
        border.width: frameRoot.frameWidth
        border.color: {
            if (frameRoot.styleMode === "subtle") {
                return Theme.isLight
                       ? Qt.rgba(0.08, 0.14, 0.28, 0.28 * frameRoot.borderAlpha)
                       : Qt.rgba(1.0, 1.0, 1.0, 0.30 * frameRoot.borderAlpha)
            }
            let c = frameRoot.styleMode === "custom" ? Theme.customBorderColor : Theme.accent
            return Qt.rgba(c.r, c.g, c.b, frameRoot.borderAlpha)
        }
        Behavior on border.color { ColorAnimation { duration: 200 } }
    }

    // Antialiased Canvas border for Chamfered 45° corners, Dual-Tone Gradient & Animated Rainbow RGB Neon
    Canvas {
        id: fancyBorderCanvas
        anchors.fill: parent
        visible: frameRoot.useCanvasRenderer
        renderStrategy: Canvas.Cooperative

        property real hueShift: 0.0
        onHueShiftChanged: requestPaint()

        NumberAnimation on hueShift {
            running: frameRoot.visible && frameRoot.animate && frameRoot.styleMode === "rainbow"
            loops: Animation.Infinite
            from: 0.0
            to: 1.0
            duration: Math.round(4500 / Theme.speedFactor)
        }

        function hslColor(h, alpha) {
            let hh = ((h % 1.0) + 1.0) % 1.0
            return Qt.hsla(hh, 0.92, Theme.isLight ? 0.48 : 0.60, alpha)
        }

        function traceFramePath(ctx, inset, bw) {
            var x = inset, y = inset, w = width - inset * 2, h = height - inset * 2
            ctx.beginPath()
            if (frameRoot.isChamfered) {
                var cut = Math.max(8, Theme.windowChamferCut - inset)
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
                var r = Math.max(0, frameRoot.effectiveRadius - inset)
                if (r <= 0.5) {
                    ctx.rect(x, y, w, h)
                } else {
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
            }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            if (width < 4 || height < 4) return

            var bw = Math.max(1.0, frameRoot.frameWidth)
            var inset = bw * 0.5

            // Inner rim glow for chamfered mode
            if (frameRoot.edgeGlow && frameRoot.isChamfered && frameRoot.styleMode !== "subtle") {
                var glowCol = frameRoot.styleMode === "custom" ? Theme.customBorderColor : Theme.accent
                traceFramePath(ctx, inset + 1.5, bw)
                ctx.lineWidth = Math.min(6, bw + 3.0)
                ctx.strokeStyle = Qt.rgba(glowCol.r, glowCol.g, glowCol.b, 0.18 * Math.max(0.35, Theme.glowIntensity) * frameRoot.borderAlpha)
                ctx.stroke()
            }

            traceFramePath(ctx, inset, bw)
            ctx.lineWidth = bw

            if (frameRoot.styleMode === "rainbow") {
                var a = frameRoot.borderAlpha
                var rg = ctx.createLinearGradient(0, 0, width, height)
                rg.addColorStop(0.00, hslColor(hueShift + 0.00, a))
                rg.addColorStop(0.25, hslColor(hueShift + 0.25, a))
                rg.addColorStop(0.50, hslColor(hueShift + 0.50, a))
                rg.addColorStop(0.75, hslColor(hueShift + 0.75, a))
                rg.addColorStop(1.00, hslColor(hueShift + 1.00, a))
                ctx.strokeStyle = rg
            } else if (frameRoot.styleMode === "gradient") {
                var c1 = Theme.accent
                var c2 = Theme.orbSecondary
                var gg = ctx.createLinearGradient(0, 0, width, height)
                gg.addColorStop(0.0, Qt.rgba(c1.r, c1.g, c1.b, frameRoot.borderAlpha))
                gg.addColorStop(0.5, Qt.rgba(c2.r, c2.g, c2.b, frameRoot.borderAlpha))
                gg.addColorStop(1.0, Qt.rgba(c1.r, c1.g, c1.b, frameRoot.borderAlpha))
                ctx.strokeStyle = gg
            } else if (frameRoot.styleMode === "subtle") {
                ctx.strokeStyle = Theme.isLight
                                  ? Qt.rgba(0.08, 0.14, 0.28, 0.28 * frameRoot.borderAlpha)
                                  : Qt.rgba(1.0, 1.0, 1.0, 0.30 * frameRoot.borderAlpha)
            } else {
                var sc = frameRoot.styleMode === "custom" ? Theme.customBorderColor : Theme.accent
                ctx.strokeStyle = Qt.rgba(sc.r, sc.g, sc.b, frameRoot.borderAlpha)
            }
            ctx.stroke()
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onVisibleChanged: if (visible) requestPaint()
        Connections {
            target: frameRoot
            function onFrameRadiusChanged() { fancyBorderCanvas.requestPaint() }
            function onFrameWidthChanged() { fancyBorderCanvas.requestPaint() }
            function onStyleModeChanged() { fancyBorderCanvas.requestPaint() }
            function onShapeModeChanged() { fancyBorderCanvas.requestPaint() }
            function onBorderAlphaChanged() { fancyBorderCanvas.requestPaint() }
            function onEdgeGlowChanged() { fancyBorderCanvas.requestPaint() }
            function onIsMaximizedChanged() { fancyBorderCanvas.requestPaint() }
        }
        Connections {
            target: Theme
            function onAccentChanged() { fancyBorderCanvas.requestPaint() }
            function onOrbSecondaryChanged() { fancyBorderCanvas.requestPaint() }
            function onCustomBorderColorChanged() { fancyBorderCanvas.requestPaint() }
            function onCornerRadiusChanged() { fancyBorderCanvas.requestPaint() }
            function onGlowIntensityChanged() { fancyBorderCanvas.requestPaint() }
        }
        Component.onCompleted: requestPaint()
    }
}
