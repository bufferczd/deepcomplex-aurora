import QtQuick
import QtQuick.Layouts

// PS4-inspired cinematic boot animation:
// - Pure OLED black backdrop with rising luminous particles & soft volumetric light wave
// - Typographic wordmark logo ("DEEPCOMPLEX") with smooth reveal, gentle zoom, and light-sweep sheen
// - Sleek PS4-style glowing light-bar loading line directly underneath (zero extra text)
Item {
    id: splash

    property int cornerRadius: 0
    property bool chamfered: false
    property int durationMs: 1750
    property int barWidth: 270

    signal finished()

    property real animTime: 0.0        // 0.0 -> 1.0 master timeline
    property real progressVal: 0.0     // 0.0 -> 1.0 loading bar fill
    property real wordOpacity: 0.0
    property real wordScale: 0.90
    property real wordSpacing: 16.0
    property real sweepPos: -0.35      // -0.35 -> 1.35 light-sweep across wordmark
    property real barOpacity: 0.0

    visible: false
    opacity: 0.0

    function play() {
        splashSeq.stop()
        animTime = 0.0
        progressVal = 0.0
        wordOpacity = 0.0
        wordScale = 0.90
        wordSpacing = 16.0
        sweepPos = -0.35
        barOpacity = 0.0
        opacity = 1.0
        visible = true
        bgCanvas.requestPaint()
        ps4SceneCanvas.requestPaint()
        splashSeq.restart()
    }

    function skip() {
        if (!visible) return
        splashSeq.stop()
        visible = false
        opacity = 0.0
        finished()
    }

    // Pure OLED black base (rounded / sharp)
    Rectangle {
        anchors.fill: parent
        visible: !splash.chamfered
        radius: splash.cornerRadius
        color: "#000000"
    }

    // Pure OLED black base (45° chamfered)
    Canvas {
        id: bgCanvas
        anchors.fill: parent
        visible: splash.chamfered
        renderStrategy: Canvas.Cooperative
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            if (width < 4 || height < 4) return
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
            ctx.fillStyle = "#000000"
            ctx.fill()
        }
        onVisibleChanged: if (visible) requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    // PS4-style volumetric light wave + drifting luminous particles + wordmark light sweep
    Canvas {
        id: ps4SceneCanvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative

        property real t: splash.animTime
        onTChanged: if (visible) requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onVisibleChanged: if (visible) requestPaint()

        // Deterministic particle field (zero allocations per frame)
        readonly property var particles: [
            { x: 0.14, y: 0.78, r: 1.6, sp: 0.16, ph: 0.1 },
            { x: 0.21, y: 0.62, r: 2.2, sp: 0.22, ph: 1.2 },
            { x: 0.27, y: 0.85, r: 1.3, sp: 0.14, ph: 2.3 },
            { x: 0.33, y: 0.44, r: 2.0, sp: 0.19, ph: 0.7 },
            { x: 0.38, y: 0.72, r: 1.4, sp: 0.25, ph: 3.1 },
            { x: 0.43, y: 0.56, r: 2.5, sp: 0.17, ph: 1.9 },
            { x: 0.48, y: 0.82, r: 1.5, sp: 0.21, ph: 4.0 },
            { x: 0.52, y: 0.38, r: 2.1, sp: 0.15, ph: 2.7 },
            { x: 0.57, y: 0.68, r: 1.8, sp: 0.23, ph: 0.5 },
            { x: 0.62, y: 0.49, r: 2.4, sp: 0.18, ph: 3.6 },
            { x: 0.67, y: 0.79, r: 1.4, sp: 0.20, ph: 1.4 },
            { x: 0.73, y: 0.58, r: 2.0, sp: 0.16, ph: 2.9 },
            { x: 0.79, y: 0.74, r: 1.7, sp: 0.24, ph: 0.9 },
            { x: 0.85, y: 0.52, r: 1.5, sp: 0.15, ph: 4.5 },
            { x: 0.24, y: 0.36, r: 1.4, sp: 0.18, ph: 2.1 },
            { x: 0.45, y: 0.29, r: 1.9, sp: 0.14, ph: 3.8 },
            { x: 0.64, y: 0.33, r: 1.6, sp: 0.21, ph: 1.6 },
            { x: 0.76, y: 0.41, r: 2.1, sp: 0.17, ph: 0.3 }
        ]

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            if (width < 20 || height < 20) return

            ctx.save()
            // Clip to window shape so particles/wave never leak outside rounded/chamfered corners
            ctx.beginPath()
            if (splash.chamfered) {
                var cut = Theme.windowChamferCut
                ctx.moveTo(cut, 0)
                ctx.lineTo(width - cut, 0)
                ctx.lineTo(width, cut)
                ctx.lineTo(width, height - cut)
                ctx.lineTo(width - cut, height)
                ctx.lineTo(cut, height)
                ctx.lineTo(0, height - cut)
                ctx.lineTo(0, cut)
                ctx.closePath()
            } else if (splash.cornerRadius > 0) {
                var rr = Math.min(splash.cornerRadius, Math.min(width, height) * 0.5)
                ctx.moveTo(rr, 0)
                ctx.lineTo(width - rr, 0)
                ctx.quadraticCurveTo(width, 0, width, rr)
                ctx.lineTo(width, height - rr)
                ctx.quadraticCurveTo(width, height, width - rr, height)
                ctx.lineTo(rr, height)
                ctx.quadraticCurveTo(0, height, 0, height - rr)
                ctx.lineTo(0, rr)
                ctx.quadraticCurveTo(0, 0, rr, 0)
                ctx.closePath()
            } else {
                ctx.rect(0, 0, width, height)
            }
            ctx.clip()

            var c = Theme.accent
            var cBright = Qt.lighter(c, 1.45)
            var cx = width * 0.5
            var cy = height * 0.5
            var fadeEnv = Math.min(1.0, t * 3.2)

            // 1. PS4-style soft volumetric horizon glow behind wordmark
            var radGlow = ctx.createRadialGradient(cx, cy - 8, 12, cx, cy - 8, Math.max(width, height) * 0.42)
            radGlow.addColorStop(0.0, Qt.rgba(cBright.r, cBright.g, cBright.b, 0.24 * fadeEnv))
            radGlow.addColorStop(0.38, Qt.rgba(c.r, c.g, c.b, 0.11 * fadeEnv))
            radGlow.addColorStop(1.0, "rgba(0, 0, 0, 0)")
            ctx.fillStyle = radGlow
            ctx.fillRect(0, 0, width, height)

            // 2. Signature PS4 subtle flowing silk light ribbons across the dark backdrop
            for (var w = 0; w < 2; ++w) {
                ctx.beginPath()
                var baseY = cy + (w === 0 ? -16 : 18)
                var amp = (w === 0 ? 28 : 22)
                var phase = t * Math.PI * 1.8 + w * 1.7
                for (var x = 0; x <= width; x += 28) {
                    var nx = x / width
                    var wy = baseY + Math.sin(nx * Math.PI * 2.4 + phase) * amp * Math.sin(nx * Math.PI)
                    if (x === 0) ctx.moveTo(x, wy)
                    else ctx.lineTo(x, wy)
                }
                var ribGrad = ctx.createLinearGradient(0, 0, width, 0)
                ribGrad.addColorStop(0.0, "rgba(0, 0, 0, 0)")
                ribGrad.addColorStop(0.35, Qt.rgba(c.r, c.g, c.b, (w === 0 ? 0.16 : 0.10) * fadeEnv))
                ribGrad.addColorStop(0.5, Qt.rgba(cBright.r, cBright.g, cBright.b, (w === 0 ? 0.24 : 0.14) * fadeEnv))
                ribGrad.addColorStop(0.65, Qt.rgba(c.r, c.g, c.b, (w === 0 ? 0.16 : 0.10) * fadeEnv))
                ribGrad.addColorStop(1.0, "rgba(0, 0, 0, 0)")
                ctx.strokeStyle = ribGrad
                ctx.lineWidth = (w === 0 ? 2.2 : 1.4)
                ctx.stroke()
            }

            // 3. Rising luminous PS4 floating dust particles
            for (var i = 0; i < particles.length; ++i) {
                var p = particles[i]
                var px = p.x * width + Math.sin(t * 4.0 + p.ph) * 14.0
                var py = (p.y - t * p.sp) * height
                if (py < 0) py += height
                var twinkle = 0.45 + 0.55 * Math.sin(t * 7.0 + p.ph * 2.0)
                var alpha = twinkle * 0.65 * fadeEnv
                var pg = ctx.createRadialGradient(px, py, 0, px, py, p.r * 3.4)
                pg.addColorStop(0.0, Qt.rgba(0.95, 1.0, 0.96, alpha))
                pg.addColorStop(0.35, Qt.rgba(cBright.r, cBright.g, cBright.b, alpha * 0.7))
                pg.addColorStop(1.0, "rgba(0, 0, 0, 0)")
                ctx.fillStyle = pg
                ctx.beginPath()
                ctx.arc(px, py, p.r * 3.4, 0, Math.PI * 2)
                ctx.fill()
            }

            ctx.restore()
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: splash.skip()
    }

    SequentialAnimation {
        id: splashSeq
        ParallelAnimation {
            // Master atmospheric time (particles + silk waves)
            NumberAnimation { target: splash; property: "animTime"; from: 0.0; to: 1.0; duration: splash.durationMs + 250; easing.type: Easing.Linear }

            // PS4-style smooth wordmark materialization (fade in + crisp letter-spacing lock-in + slow cinematic drift)
            SequentialAnimation {
                ParallelAnimation {
                    NumberAnimation { target: splash; property: "wordOpacity"; from: 0.0; to: 1.0; duration: 680; easing.type: Easing.OutCubic }
                    NumberAnimation { target: splash; property: "wordSpacing"; from: 17.0; to: 8.5; duration: 780; easing.type: Easing.OutQuint }
                    NumberAnimation { target: splash; property: "wordScale"; from: 0.90; to: 1.00; duration: 780; easing.type: Easing.OutCubic }
                }
                NumberAnimation { target: splash; property: "wordScale"; from: 1.00; to: 1.035; duration: Math.max(400, splash.durationMs - 680); easing.type: Easing.InOutSine }
            }

            // Luminous sheen sweeping across the wordmark logo
            SequentialAnimation {
                PauseAnimation { duration: 420 }
                NumberAnimation { target: splash; property: "sweepPos"; from: -0.35; to: 1.35; duration: 920; easing.type: Easing.InOutQuad }
            }

            // PS4 light-bar loading line appearance & smooth fill underneath the name
            SequentialAnimation {
                PauseAnimation { duration: 240 }
                ParallelAnimation {
                    NumberAnimation { target: splash; property: "barOpacity"; from: 0.0; to: 1.0; duration: 360; easing.type: Easing.OutCubic }
                    NumberAnimation { target: splash; property: "progressVal"; from: 0.0; to: 1.0; duration: Math.max(800, splash.durationMs - 240); easing.type: Easing.InOutCubic }
                }
            }
        }
        PauseAnimation { duration: 180 }
        NumberAnimation { target: splash; property: "opacity"; from: 1.0; to: 0.0; duration: 360; easing.type: Easing.InCubic }
        ScriptAction {
            script: {
                splash.visible = false
                splash.finished()
            }
        }
    }

    // Centerpiece: ONLY the Name Logo ("DEEPCOMPLEX") + PS4 Light-Bar Loading Line directly underneath
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 22

        // Stylized Wordmark Logo ("DEEPCOMPLEX") with PS4 Light-Sweep Sheen
        Item {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: wordmarkText.implicitWidth + 32
            implicitHeight: wordmarkText.implicitHeight + 16
            opacity: splash.wordOpacity
            scale: splash.wordScale

            // Soft glowing back-halo behind the brand name
            Text {
                anchors.centerIn: parent
                text: "DEEPCOMPLEX"
                font.pixelSize: 40
                font.weight: Font.DemiBold
                font.letterSpacing: splash.wordSpacing
                color: Qt.lighter(Theme.accent, 1.35)
                opacity: 0.34
                scale: 1.015
            }

            // Crisp primary white-silver typographic logo
            Text {
                id: wordmarkText
                anchors.centerIn: parent
                text: "DEEPCOMPLEX"
                font.pixelSize: 40
                font.weight: Font.DemiBold
                font.letterSpacing: splash.wordSpacing
                color: "#F8FAFC"
            }

            // Traveling luminous light-sweep highlight across the logo letters
            Item {
                anchors.fill: wordmarkText
                clip: true

                Rectangle {
                    width: Math.max(48, parent.width * 0.24)
                    height: parent.height * 1.5
                    anchors.verticalCenter: parent.verticalCenter
                    x: (splash.sweepPos * parent.width) - (width * 0.5)
                    rotation: 16
                    opacity: 0.55
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.5; color: Qt.rgba(Qt.lighter(Theme.accent, 1.7).r, Qt.lighter(Theme.accent, 1.7).g, Qt.lighter(Theme.accent, 1.7).b, 0.85) }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                }
            }
        }

        // PS4-style glowing light-bar loading line directly underneath the Name Logo
        Item {
            Layout.alignment: Qt.AlignHCenter
            width: splash.barWidth
            height: 14
            opacity: splash.barOpacity

            // Subtle ambient glow halo around the active loading bar
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                x: 0
                width: Math.max(0, parent.width * splash.progressVal)
                height: 10
                radius: 5
                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.32)
            }

            // Thin dark rail
            Rectangle {
                id: rail
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 3
                radius: 1.5
                color: Qt.rgba(1, 1, 1, 0.14)

                // Smooth glowing fill bar
                Rectangle {
                    height: parent.height
                    width: Math.max(0, parent.width * splash.progressVal)
                    radius: 1.5
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: Qt.darker(Theme.accent, 1.1) }
                        GradientStop { position: 0.65; color: Qt.lighter(Theme.accent, 1.25) }
                        GradientStop { position: 1.0; color: "#FFFFFF" }
                    }
                }
            }

            // Bright PS4 light-bar leading pulse point
            Rectangle {
                visible: splash.progressVal > 0.02 && splash.progressVal < 0.99
                width: 8
                height: 8
                radius: 4
                anchors.verticalCenter: parent.verticalCenter
                x: Math.max(0, Math.min(parent.width - width, parent.width * splash.progressVal - width * 0.5))
                color: "#FFFFFF"
                opacity: 0.90
            }
        }
    }
}
