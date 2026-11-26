import QtQuick

Item {
    id: iconRoot

    property color accentColor: Theme.accent
    property bool lightSurface: Theme.isLight
    property int iconSize: 32

    implicitWidth: iconSize
    implicitHeight: iconSize
    width: iconSize
    height: iconSize

    property color smoothAccent: accentColor
    Behavior on smoothAccent {
        ColorAnimation { duration: Theme.animatedUi ? 360 : 0; easing.type: Easing.InOutQuad }
    }
    onSmoothAccentChanged: if (iconCanvas.visible) iconCanvas.requestPaint()
    onLightSurfaceChanged: if (iconCanvas.visible) iconCanvas.requestPaint()

    Canvas {
        id: iconCanvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative

        onPaint: {
            if (!visible || width < 4 || height < 4) return
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)

            var s = Math.min(width, height) / 128.0
            ctx.save()
            ctx.scale(s, s)

            var c = iconRoot.smoothAccent
            var strokeGrad = ctx.createLinearGradient(0, 0, 128, 128)
            strokeGrad.addColorStop(0.0, Qt.lighter(c, 1.20))
            strokeGrad.addColorStop(0.5, c)
            strokeGrad.addColorStop(1.0, Qt.darker(c, 1.25))

            function roundRectPath(x, y, w, h, r) {
                ctx.beginPath()
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

            // Outer rounded badge
            roundRectPath(5, 5, 118, 118, 28)
            ctx.fillStyle = iconRoot.lightSurface ? "#EEF4FB" : "#080C16"
            ctx.fill()
            ctx.lineWidth = 4.5
            ctx.strokeStyle = strokeGrad
            ctx.stroke()

            // Inner radial ambient glow
            roundRectPath(9, 9, 110, 110, 24)
            var glow = ctx.createRadialGradient(64, 90, 4, 64, 90, 66)
            glow.addColorStop(0.0, Qt.rgba(c.r, c.g, c.b, iconRoot.lightSurface ? 0.24 : 0.36))
            glow.addColorStop(1.0, Qt.rgba(c.r, c.g, c.b, 0.0))
            ctx.fillStyle = glow
            ctx.fill()

            // Mouse body silhouette
            roundRectPath(38, 20, 52, 88, 26)
            ctx.fillStyle = iconRoot.lightSurface ? "#FFFFFF" : "#0A101C"
            ctx.fill()
            ctx.lineWidth = 3.8
            ctx.strokeStyle = strokeGrad
            ctx.stroke()

            // Center button split line
            ctx.beginPath()
            ctx.moveTo(64, 21)
            ctx.lineTo(64, 54)
            ctx.strokeStyle = Qt.rgba(c.r, c.g, c.b, 0.62)
            ctx.lineWidth = 2.5
            ctx.stroke()

            // Curved button seam
            ctx.beginPath()
            ctx.moveTo(39.5, 54)
            ctx.quadraticCurveTo(64, 59.5, 88.5, 54)
            ctx.strokeStyle = Qt.rgba(c.r, c.g, c.b, 0.48)
            ctx.lineWidth = 2.2
            ctx.stroke()

            // Scroll wheel
            roundRectPath(59, 30, 10, 19, 5)
            ctx.fillStyle = c
            ctx.fill()

            // DPI indicator LED dot
            ctx.beginPath()
            ctx.arc(64, 62, 4.0, 0, Math.PI * 2)
            ctx.fillStyle = c
            ctx.fill()

            // Bottom RGB underglow arc
            ctx.beginPath()
            ctx.moveTo(46, 88)
            ctx.quadraticCurveTo(64, 100, 82, 88)
            ctx.strokeStyle = c
            ctx.lineWidth = 5.2
            ctx.lineCap = "round"
            ctx.stroke()

            ctx.restore()
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Component.onCompleted: requestPaint()
    }
}
