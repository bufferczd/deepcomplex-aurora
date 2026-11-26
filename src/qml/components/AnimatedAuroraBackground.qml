import QtQuick

Item {
    id: bgRoot

    property bool running: Theme.animatedUi
    property color accentColor: Theme.accent
    property string styleMode: Theme.backgroundStyle
    property real glowStrength: Theme.glowIntensity
    property real speedScale: Theme.speedFactor
    property int cornerRadius: 0
    property bool chamfered: false

    property color smoothAccent: accentColor
    Behavior on smoothAccent { ColorAnimation { duration: 600; easing.type: Easing.InOutQuad } }
    onSmoothAccentChanged: {
        if (orbPrimary.visible) orbPrimary.requestPaint()
        if (waveCanvas.visible) waveCanvas.requestPaint()
    }

    property color smoothOrb2: Theme.orbSecondary
    Behavior on smoothOrb2 { ColorAnimation { duration: 600; easing.type: Easing.InOutQuad } }
    onSmoothOrb2Changed: {
        if (orbSecondary.visible) orbSecondary.requestPaint()
        if (waveCanvas.visible) waveCanvas.requestPaint()
    }

    property color smoothOrb3: Theme.orbTertiary
    Behavior on smoothOrb3 { ColorAnimation { duration: 600; easing.type: Easing.InOutQuad } }
    onSmoothOrb3Changed: if (orbTertiary.visible) orbTertiary.requestPaint()

    function traceClipPath(ctx, w, h) {
        ctx.beginPath()
        if (bgRoot.chamfered) {
            var cut = Theme.windowChamferCut
            ctx.moveTo(cut, 0)
            ctx.lineTo(w - cut, 0)
            ctx.lineTo(w, cut)
            ctx.lineTo(w, h - cut)
            ctx.lineTo(w - cut, h)
            ctx.lineTo(cut, h)
            ctx.lineTo(0, h - cut)
            ctx.lineTo(0, cut)
            ctx.closePath()
        } else if (bgRoot.cornerRadius > 0) {
            var r = Math.min(bgRoot.cornerRadius, Math.min(w, h) * 0.5)
            ctx.moveTo(r, 0)
            ctx.lineTo(w - r, 0)
            ctx.quadraticCurveTo(w, 0, w, r)
            ctx.lineTo(w, h - r)
            ctx.quadraticCurveTo(w, h, w - r, h)
            ctx.lineTo(r, h)
            ctx.quadraticCurveTo(0, h, 0, h - r)
            ctx.lineTo(0, r)
            ctx.quadraticCurveTo(0, 0, r, 0)
            ctx.closePath()
        } else {
            ctx.rect(0, 0, w, h)
        }
    }

    clip: true

    // Base background (pure #000000 in OLED Pure mode or True-Black OLED override)
    Rectangle {
        anchors.fill: parent
        visible: !bgRoot.chamfered && (bgRoot.styleMode === "oledPure" || Theme.effectiveOled)
        radius: bgRoot.cornerRadius
        color: "#000000"
    }

    Rectangle {
        anchors.fill: parent
        visible: !bgRoot.chamfered && bgRoot.styleMode !== "oledPure" && !Theme.effectiveOled
        radius: bgRoot.cornerRadius
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Theme.bgTop }
            GradientStop { position: 0.55; color: Theme.bgMid }
            GradientStop { position: 1.0; color: Theme.bgBottom }
        }
    }

    // Chamfered 45-degree base background (always visible, zero offscreen mask!)
    Canvas {
        id: chamferBaseCanvas
        anchors.fill: parent
        visible: bgRoot.chamfered
        renderStrategy: Canvas.Cooperative
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            if (width < 4 || height < 4) return
            bgRoot.traceClipPath(ctx, width, height)
            if (bgRoot.styleMode === "oledPure" || Theme.effectiveOled) {
                ctx.fillStyle = "#000000"
            } else {
                var g = ctx.createLinearGradient(0, 0, 0, height)
                g.addColorStop(0.0, Theme.bgTop.toString())
                g.addColorStop(0.55, Theme.bgMid.toString())
                g.addColorStop(1.0, Theme.bgBottom.toString())
                ctx.fillStyle = g
            }
            ctx.fill()
        }
        onVisibleChanged: if (visible) requestPaint()
        onWidthChanged: if (visible) requestPaint()
        onHeightChanged: if (visible) requestPaint()
        Connections {
            target: Theme
            function onBgTopChanged() { if (chamferBaseCanvas.visible) chamferBaseCanvas.requestPaint() }
            function onBgMidChanged() { if (chamferBaseCanvas.visible) chamferBaseCanvas.requestPaint() }
            function onBgBottomChanged() { if (chamferBaseCanvas.visible) chamferBaseCanvas.requestPaint() }
            function onEffectiveOledChanged() { if (chamferBaseCanvas.visible) chamferBaseCanvas.requestPaint() }
            function onBackgroundStyleChanged() { if (chamferBaseCanvas.visible) chamferBaseCanvas.requestPaint() }
            function onCornerRadiusChanged() { if (chamferBaseCanvas.visible) chamferBaseCanvas.requestPaint() }
            function onWindowShapeChanged() { if (chamferBaseCanvas.visible) chamferBaseCanvas.requestPaint() }
        }
    }

    // ============ STYLE: AURORA / GRID ORBS (bounded so alpha=0 at all 4 outer corners) ============
    Item {
        anchors.fill: parent
        visible: bgRoot.styleMode === "aurora" || bgRoot.styleMode === "grid"
        opacity: (bgRoot.styleMode === "grid" ? 0.45 : (Theme.effectiveOled ? 0.65 : 1.0)) * bgRoot.glowStrength

        Canvas {
            id: orbPrimary
            width: Math.max(Math.min(bgRoot.width, bgRoot.height) * 0.72, 340)
            height: width
            opacity: Theme.isLight ? 0.24 : 0.38
            renderStrategy: Canvas.Cooperative

            property real driftX: 0.30
            property real driftY: 0.28
            x: bgRoot.width * driftX - width * 0.5
            y: bgRoot.height * driftY - height * 0.5

            onPaint: {
                if (!visible || width < 4 || height < 4) return
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                var cx = width * 0.5, cy = height * 0.5, r = width * 0.5
                var c = bgRoot.smoothAccent
                var g = ctx.createRadialGradient(cx, cy, 0, cx, cy, r)
                g.addColorStop(0.0, Qt.rgba(c.r, c.g, c.b, 0.85))
                g.addColorStop(0.42, Qt.rgba(c.r, c.g, c.b, 0.26))
                g.addColorStop(1.0, Qt.rgba(c.r, c.g, c.b, 0.0))
                ctx.fillStyle = g
                ctx.fillRect(0, 0, width, height)
            }
            onVisibleChanged: if (visible) requestPaint()
            Component.onCompleted: if (visible) requestPaint()
            onWidthChanged: if (visible) requestPaint()

            ParallelAnimation {
                running: bgRoot.running && (bgRoot.styleMode === "aurora" || bgRoot.styleMode === "grid")
                loops: Animation.Infinite
                SequentialAnimation {
                    NumberAnimation { target: orbPrimary; property: "driftX"; from: 0.28; to: 0.54; duration: Math.round(11000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                    NumberAnimation { target: orbPrimary; property: "driftX"; from: 0.54; to: 0.28; duration: Math.round(11000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                }
                SequentialAnimation {
                    NumberAnimation { target: orbPrimary; property: "driftY"; from: 0.26; to: 0.48; duration: Math.round(8500 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                    NumberAnimation { target: orbPrimary; property: "driftY"; from: 0.48; to: 0.26; duration: Math.round(8500 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                }
                SequentialAnimation {
                    NumberAnimation { target: orbPrimary; property: "scale"; from: 0.92; to: 1.06; duration: Math.round(7000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                    NumberAnimation { target: orbPrimary; property: "scale"; from: 1.06; to: 0.92; duration: Math.round(7000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                }
            }
        }

        Canvas {
            id: orbSecondary
            width: Math.max(Math.min(bgRoot.width, bgRoot.height) * 0.66, 320)
            height: width
            opacity: Theme.isLight ? 0.20 : 0.32
            renderStrategy: Canvas.Cooperative

            property real driftX: 0.70
            property real driftY: 0.70
            x: bgRoot.width * driftX - width * 0.5
            y: bgRoot.height * driftY - height * 0.5

            onPaint: {
                if (!visible || width < 4 || height < 4) return
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                var cx = width * 0.5, cy = height * 0.5, r = width * 0.5
                var c = bgRoot.smoothOrb2
                var g = ctx.createRadialGradient(cx, cy, 0, cx, cy, r)
                g.addColorStop(0.0, Qt.rgba(c.r, c.g, c.b, 0.88))
                g.addColorStop(0.45, Qt.rgba(c.r, c.g, c.b, 0.28))
                g.addColorStop(1.0, Qt.rgba(c.r, c.g, c.b, 0.0))
                ctx.fillStyle = g
                ctx.fillRect(0, 0, width, height)
            }
            onVisibleChanged: if (visible) requestPaint()
            Component.onCompleted: if (visible) requestPaint()
            onWidthChanged: if (visible) requestPaint()

            ParallelAnimation {
                running: bgRoot.running && bgRoot.styleMode === "aurora"
                loops: Animation.Infinite
                SequentialAnimation {
                    NumberAnimation { target: orbSecondary; property: "driftX"; from: 0.72; to: 0.46; duration: Math.round(13000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                    NumberAnimation { target: orbSecondary; property: "driftX"; from: 0.46; to: 0.72; duration: Math.round(13000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                }
                SequentialAnimation {
                    NumberAnimation { target: orbSecondary; property: "driftY"; from: 0.72; to: 0.46; duration: Math.round(10000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                    NumberAnimation { target: orbSecondary; property: "driftY"; from: 0.46; to: 0.72; duration: Math.round(10000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                }
            }
        }

        Canvas {
            id: orbTertiary
            visible: bgRoot.styleMode === "aurora"
            width: Math.max(Math.min(bgRoot.width, bgRoot.height) * 0.56, 280)
            height: width
            opacity: Theme.isLight ? 0.18 : 0.25
            renderStrategy: Canvas.Cooperative

            property real driftX: 0.66
            property real driftY: 0.30
            x: bgRoot.width * driftX - width * 0.5
            y: bgRoot.height * driftY - height * 0.5

            onPaint: {
                if (!visible || width < 4 || height < 4) return
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                var cx = width * 0.5, cy = height * 0.5, r = width * 0.5
                var c = bgRoot.smoothOrb3
                var g = ctx.createRadialGradient(cx, cy, 0, cx, cy, r)
                g.addColorStop(0.0, Qt.rgba(c.r, c.g, c.b, 0.85))
                g.addColorStop(0.45, Qt.rgba(c.r, c.g, c.b, 0.24))
                g.addColorStop(1.0, Qt.rgba(c.r, c.g, c.b, 0.0))
                ctx.fillStyle = g
                ctx.fillRect(0, 0, width, height)
            }
            onVisibleChanged: if (visible) requestPaint()
            Component.onCompleted: if (visible) requestPaint()
            onWidthChanged: if (visible) requestPaint()

            ParallelAnimation {
                running: bgRoot.running && bgRoot.styleMode === "aurora"
                loops: Animation.Infinite
                SequentialAnimation {
                    NumberAnimation { target: orbTertiary; property: "driftX"; from: 0.68; to: 0.32; duration: Math.round(15000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                    NumberAnimation { target: orbTertiary; property: "driftX"; from: 0.32; to: 0.68; duration: Math.round(15000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                }
                SequentialAnimation {
                    NumberAnimation { target: orbTertiary; property: "driftY"; from: 0.30; to: 0.68; duration: Math.round(12000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                    NumberAnimation { target: orbTertiary; property: "driftY"; from: 0.68; to: 0.30; duration: Math.round(12000 / bgRoot.speedScale); easing.type: Easing.InOutSine }
                }
            }
        }
    }

    // ============ STYLE: WAVES (strictly clipped to window shape!) ============
    Canvas {
        id: waveCanvas
        anchors.fill: parent
        visible: bgRoot.styleMode === "waves"
        opacity: (Theme.isLight ? 0.32 : 0.48) * bgRoot.glowStrength
        renderStrategy: Canvas.Cooperative

        property real phase: 0.0
        onPhaseChanged: if (visible) requestPaint()

        NumberAnimation on phase {
            running: bgRoot.running && bgRoot.styleMode === "waves"
            loops: Animation.Infinite
            from: 0.0
            to: Math.PI * 2
            duration: Math.round(9000 / bgRoot.speedScale)
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            if (width < 4 || height < 4) return

            ctx.save()
            bgRoot.traceClipPath(ctx, width, height)
            ctx.clip()

            var c1 = bgRoot.smoothAccent
            var c2 = bgRoot.smoothOrb2
            var layers = [
                { yBase: height * 0.35, amp: 46, freq: 0.0045, speed: 1.0, col: c1, alpha: 0.26 },
                { yBase: height * 0.58, amp: 58, freq: 0.0036, speed: -1.0, col: c2, alpha: 0.24 },
                { yBase: height * 0.76, amp: 38, freq: 0.0055, speed: 1.0, col: c1, alpha: 0.20 }
            ]
            for (var l = 0; l < layers.length; ++l) {
                var w = layers[l]
                ctx.beginPath()
                ctx.moveTo(0, height)
                for (var x = 0; x <= width; x += 24) {
                    var y = w.yBase + Math.sin(x * w.freq + phase * w.speed + l * 1.4) * w.amp
                    ctx.lineTo(x, y)
                }
                ctx.lineTo(width, height)
                ctx.closePath()
                var g = ctx.createLinearGradient(0, w.yBase - w.amp, 0, height)
                g.addColorStop(0.0, Qt.rgba(w.col.r, w.col.g, w.col.b, w.alpha))
                g.addColorStop(1.0, Qt.rgba(w.col.r, w.col.g, w.col.b, 0.0))
                ctx.fillStyle = g
                ctx.fill()
            }
            ctx.restore()
        }
        onWidthChanged: if (visible) requestPaint()
        onHeightChanged: if (visible) requestPaint()
        onVisibleChanged: if (visible) requestPaint()
        Connections {
            target: bgRoot
            function onCornerRadiusChanged() { if (waveCanvas.visible) waveCanvas.requestPaint() }
            function onChamferedChanged() { if (waveCanvas.visible) waveCanvas.requestPaint() }
        }
    }

    // ============ GRID / DOT-MATRIX OVERLAY (strictly clipped to window shape!) ============
    Canvas {
        id: gridOverlay
        anchors.fill: parent
        visible: bgRoot.styleMode !== "solid" && bgRoot.styleMode !== "oledPure"
        opacity: bgRoot.styleMode === "grid" ? 0.32 : (Theme.effectiveOled ? 0.10 : 0.16)
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            if (width < 4 || height < 4) return

            ctx.save()
            bgRoot.traceClipPath(ctx, width, height)
            ctx.clip()

            if (bgRoot.styleMode === "grid") {
                ctx.strokeStyle = Theme.isLight ? "rgba(15, 23, 42, 0.10)" : "rgba(148, 163, 184, 0.16)"
                ctx.lineWidth = 1
                var gstep = 36
                ctx.beginPath()
                for (var gx = 0; gx < width; gx += gstep) { ctx.moveTo(gx, 0); ctx.lineTo(gx, height) }
                for (var gy = 0; gy < height; gy += gstep) { ctx.moveTo(0, gy); ctx.lineTo(width, gy) }
                ctx.stroke()
            } else {
                ctx.fillStyle = Theme.isLight ? "rgba(15, 23, 42, 0.22)" : "rgba(148, 163, 184, 0.45)"
                var step = 28
                for (var y = 14; y < height; y += step) {
                    for (var x = 14; x < width; x += step) {
                        ctx.fillRect(x, y, 1.5, 1.5)
                    }
                }
            }
            ctx.restore()
        }
        onWidthChanged: if (visible) requestPaint()
        onHeightChanged: if (visible) requestPaint()
        onVisibleChanged: if (visible) requestPaint()
        Connections {
            target: bgRoot
            function onCornerRadiusChanged() { if (gridOverlay.visible) gridOverlay.requestPaint() }
            function onChamferedChanged() { if (gridOverlay.visible) gridOverlay.requestPaint() }
        }
        Connections {
            target: Theme
            function onIsLightChanged() { if (gridOverlay.visible) gridOverlay.requestPaint() }
            function onBackgroundStyleChanged() { if (gridOverlay.visible) gridOverlay.requestPaint() }
            function onWindowShapeChanged() { if (gridOverlay.visible) gridOverlay.requestPaint() }
            function onCornerRadiusChanged() { if (gridOverlay.visible) gridOverlay.requestPaint() }
        }
        Component.onCompleted: if (visible) requestPaint()
    }

    // Top edge luminous shimmer sweep (strictly inset inside top corners!)
    Item {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: bgRoot.chamfered ? Theme.windowChamferCut : bgRoot.cornerRadius
        anchors.rightMargin: bgRoot.chamfered ? Theme.windowChamferCut : bgRoot.cornerRadius
        height: 2
        clip: true
        visible: bgRoot.styleMode !== "solid" && bgRoot.styleMode !== "oledPure"

        Rectangle {
            id: topShimmer
            y: 0
            height: 2
            width: Math.max(80, parent.width * 0.36)
            radius: 1
            opacity: (bgRoot.running ? 0.85 : 0.30) * Math.max(0.20, bgRoot.glowStrength)
            x: -width
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: bgRoot.smoothAccent }
                GradientStop { position: 1.0; color: "transparent" }
            }

            SequentialAnimation on x {
                running: bgRoot.running && bgRoot.styleMode !== "solid" && bgRoot.styleMode !== "oledPure"
                loops: Animation.Infinite
                NumberAnimation { from: -topShimmer.width; to: bgRoot.width; duration: Math.round(5500 / bgRoot.speedScale); easing.type: Easing.InOutCubic }
                PauseAnimation { duration: Math.round(1800 / bgRoot.speedScale) }
            }
        }
    }
}
