import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "color-management"
    id: extraModeZoneDialog
    signal pickColorRequested(int slot, color currentColor)
    title: I18n.tr(mouseController.language, "zoneMapTitle")
    width: Math.min((parent ? parent.width : 960) - 48, 560)
    height: Math.min((parent ? parent.height : 820) - 64, 620)
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    onOpened: mouseOutlineCanvas.requestPaint()




    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Warning
            text: I18n.tr(mouseController.language, "dpiColorWarnExtra")
        }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.85
            text: I18n.tr(mouseController.language, "zoneMapSub")
        }

        // منطقة الرسم التفاعلي للماوس وحوله النقاط الـ 7
        Item {
            id: mouseCanvasArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 380
            LayoutMirroring.enabled: false // إحداثيات الرسم الفيزيائي للماوس ثابتة دائماً

            // خلفية داكنة أنيقة تبرز إضاءة الـ RGB
            Rectangle {
                anchors.fill: parent
                radius: 14
                color: "#090b0d"
                border.width: 1
                border.color: "#222930"
            }

            // رسم جسم ماوس Delux M700A بمواقع الـ LED الدقيقة 100% (شريط U-Shape غير متماثل + العجلة + الشعار)
            Canvas {
                id: mouseOutlineCanvas
                anchors.fill: parent

                property var cols: mouseController.extraModeColors
                onColsChanged: if (visible) requestPaint()
                onWidthChanged: if (visible) requestPaint()
                onHeightChanged: if (visible) requestPaint()
                onVisibleChanged: if (visible) requestPaint()
                Component.onCompleted: if (visible) requestPaint()

                function drawRoundedRect(ctx, x, y, w, h, r) {
                    ctx.beginPath()
                    ctx.moveTo(x + r, y)
                    ctx.lineTo(x + w - r, y)
                    ctx.arcTo(x + w, y, x + w, y + r, r)
                    ctx.lineTo(x + w, y + h - r)
                    ctx.arcTo(x + w, y + h, x + w - r, y + h, r)
                    ctx.lineTo(x + r, y + h)
                    ctx.arcTo(x, y + h, x, y + h - r, r)
                    ctx.lineTo(x, y + r)
                    ctx.arcTo(x, y, x + r, y, r)
                    ctx.closePath()
                }

                function drawConnector(ctx, x1, y1, x2, y2, colorStr, dashed) {
                    ctx.save()
                    ctx.beginPath()
                    if (dashed) ctx.setLineDash([5, 4])
                    ctx.moveTo(x1, y1)
                    ctx.lineTo(x2, y2)
                    ctx.strokeStyle = colorStr
                    ctx.lineWidth = 2.0
                    ctx.globalAlpha = 0.85
                    ctx.stroke()

                    ctx.setLineDash([])
                    ctx.beginPath()
                    ctx.arc(x2, y2, 4.2, 0, Math.PI * 2)
                    ctx.fillStyle = colorStr
                    ctx.globalAlpha = 1.0
                    ctx.fill()
                    ctx.lineWidth = 1.5
                    ctx.strokeStyle = "#ffffff"
                    ctx.stroke()
                    ctx.restore()
                }

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    var cx = width / 2
                    var cy = height / 2
                    var mx = cx - 68
                    var my = cy - 130
                    var mw = 136
                    var mh = 260

                    var c0 = (cols && cols.length >= 6 ? cols[0] : "#ff00ff").toString()
                    var c1 = (cols && cols.length >= 6 ? cols[1] : "#ff0000").toString()
                    var c2 = (cols && cols.length >= 6 ? cols[2] : "#00ffff").toString()
                    var c3 = (cols && cols.length >= 6 ? cols[3] : "#ffff00").toString()
                    var c4 = (cols && cols.length >= 6 ? cols[4] : "#00ff00").toString()
                    var c5 = (cols && cols.length >= 6 ? cols[5] : "#0044ff").toString()

                    // 1. شريط الإضاءة السفلي على شكل حرف U (يبدأ من منتصف الجانبين ويلف حول المؤخرة فقط)
                    // النصف الأيسر (من الأمام للخلف: 2 -> 3 -> 4 -> 5 -> 6)
                    var leftGrad = ctx.createLinearGradient(cx - 72, my + 88, cx, my + mh + 4)
                    leftGrad.addColorStop(0.00, c1)
                    leftGrad.addColorStop(0.28, c2)
                    leftGrad.addColorStop(0.56, c3)
                    leftGrad.addColorStop(0.80, c4)
                    leftGrad.addColorStop(1.00, c5)

                    ctx.save()
                    ctx.beginPath()
                    ctx.moveTo(cx - 72, my + 88)
                    ctx.lineTo(cx - 72, my + 194)
                    ctx.arcTo(cx - 72, my + mh + 4, cx, my + mh + 4, 68)
                    ctx.lineTo(cx, my + mh + 4)
                    ctx.lineCap = "round"
                    ctx.lineWidth = 10
                    ctx.strokeStyle = leftGrad
                    ctx.globalAlpha = 0.30
                    ctx.stroke()
                    ctx.lineWidth = 4.2
                    ctx.globalAlpha = 0.98
                    ctx.stroke()
                    ctx.restore()

                    // النصف الأيمن (من الأمام للخلف: 1 -> 2 -> 3 -> 4 -> 5)
                    var rightGrad = ctx.createLinearGradient(cx + 72, my + 88, cx, my + mh + 4)
                    rightGrad.addColorStop(0.00, c0)
                    rightGrad.addColorStop(0.26, c1)
                    rightGrad.addColorStop(0.52, c2)
                    rightGrad.addColorStop(0.78, c3)
                    rightGrad.addColorStop(1.00, c4)

                    ctx.save()
                    ctx.beginPath()
                    ctx.moveTo(cx + 72, my + 88)
                    ctx.lineTo(cx + 72, my + 194)
                    ctx.arcTo(cx + 72, my + mh + 4, cx, my + mh + 4, 68)
                    ctx.lineTo(cx, my + mh + 4)
                    ctx.lineCap = "round"
                    ctx.lineWidth = 10
                    ctx.strokeStyle = rightGrad
                    ctx.globalAlpha = 0.30
                    ctx.stroke()
                    ctx.lineWidth = 4.2
                    ctx.globalAlpha = 0.98
                    ctx.stroke()
                    ctx.restore()

                    // 2. جسم الماوس الأسود
                    drawRoundedRect(ctx, mx, my, mw, mh, 62)
                    ctx.fillStyle = "#14181c"
                    ctx.fill()
                    ctx.lineWidth = 2
                    ctx.strokeStyle = "#2d353e"
                    ctx.stroke()

                    // 3. خط فاصل الأزرار الأمامية
                    ctx.beginPath()
                    ctx.moveTo(cx, my)
                    ctx.lineTo(cx, my + 88)
                    ctx.strokeStyle = "#2d353e"
                    ctx.lineWidth = 2
                    ctx.stroke()

                    ctx.beginPath()
                    ctx.moveTo(mx + 2, my + 88)
                    ctx.quadraticCurveTo(cx, my + 102, mx + mw - 2, my + 88)
                    ctx.stroke()

                    // 4. عجلة التمرير المضيئة بلون القناة 1
                    drawRoundedRect(ctx, cx - 9, my + 22, 18, 38, 8)
                    ctx.fillStyle = c0
                    ctx.fill()
                    ctx.lineWidth = 2
                    ctx.strokeStyle = "#0a0c0e"
                    ctx.stroke()

                    // 5. زر DPI
                    drawRoundedRect(ctx, cx - 5, my + 68, 10, 14, 4)
                    ctx.fillStyle = "#2b323a"
                    ctx.fill()

                    // 6. توهج الشعار الداخلي (القناة 6)
                    ctx.save()
                    var glowGrad = ctx.createRadialGradient(cx, my + 195, 4, cx, my + 195, 50)
                    glowGrad.addColorStop(0.0, c5)
                    glowGrad.addColorStop(1.0, "rgba(0,0,0,0)")
                    ctx.fillStyle = glowGrad
                    ctx.globalAlpha = 0.65
                    ctx.beginPath()
                    ctx.arc(cx, my + 195, 50, 0, Math.PI * 2)
                    ctx.fill()
                    ctx.restore()

                    // 7. فتحات خلية النحل (Honeycomb)
                    ctx.fillStyle = "#080a0c"
                    ctx.strokeStyle = "#20272f"
                    ctx.lineWidth = 1
                    for (var row = 0; row < 6; ++row) {
                        var colsInRow = (row === 0 || row === 5) ? 4 : 5
                        var hy = my + 124 + row * 21
                        var startX = cx - ((colsInRow - 1) * 20) / 2
                        for (var col = 0; col < colsInRow; ++col) {
                            var hx = startX + col * 20
                            ctx.beginPath()
                            ctx.arc(hx, hy, 6.0, 0, Math.PI * 2)
                            ctx.fill()
                            ctx.stroke()
                        }
                    }

                    // 8. خطوط التوصيل الدقيقة 100% من كل قناة إلى جميع اللمبات الفعلية المرتبطة بها
                    // القناة 1: العجلة + بداية الشريط الأيمن
                    var n1x = width * 0.50, n1y = height * 0.08
                    drawConnector(ctx, n1x, n1y, cx, my + 41, c0, false)
                    drawConnector(ctx, n1x, n1y, cx + 72, my + 96, c0, true)

                    // القناة 2: بداية الشريط الأيسر + أعلى الشريط الأيمن (مائل للخلف)
                    var n2x = width * 0.14, n2y = height * 0.28
                    drawConnector(ctx, n2x, n2y, cx - 72, my + 96, c1, false)
                    drawConnector(ctx, cx - 72, my + 96, cx + 72, my + 128, c1, true)

                    // القناة 3: وسط الشريط الأيسر + وسط الشريط الأيمن
                    var n3x = width * 0.14, n3y = height * 0.48
                    drawConnector(ctx, n3x, n3y, cx - 72, my + 138, c2, false)
                    drawConnector(ctx, cx - 72, my + 138, cx + 72, my + 166, c2, true)

                    // القناة 4: أسفل وسط الأيسر + مؤخرة الأيمن
                    var n4x = width * 0.86, n4y = height * 0.58
                    drawConnector(ctx, n4x, n4y, cx + 68, my + 206, c3, false)
                    drawConnector(ctx, cx + 68, my + 206, cx - 70, my + 184, c3, true)

                    // القناة 5: مؤخرة الأيسر + منحنى المؤخرة الأيمن
                    var n5x = width * 0.86, n5y = height * 0.78
                    drawConnector(ctx, n5x, n5y, cx + 38, my + 250, c4, false)
                    drawConnector(ctx, cx + 38, my + 250, cx - 56, my + 228, c4, true)

                    // القناة 6: منحنى المؤخرة الأيسر + الشعار الداخلي
                    var n6x = width * 0.16, n6y = height * 0.86
                    drawConnector(ctx, n6x, n6y, cx - 24, my + 258, c5, false)
                    drawConnector(ctx, cx - 24, my + 258, cx, my + 195, c5, true)
                }
            }

            // النقاط الـ 6 التفاعلية المرتبة بدقة حول الماوس
            Repeater {
                model: [
                    { slot: 0, rx: 0.50, ry: 0.08, key: "zone1" },
                    { slot: 1, rx: 0.14, ry: 0.28, key: "zone2" },
                    { slot: 2, rx: 0.14, ry: 0.48, key: "zone3" },
                    { slot: 3, rx: 0.86, ry: 0.58, key: "zone4" },
                    { slot: 4, rx: 0.86, ry: 0.78, key: "zone5" },
                    { slot: 5, rx: 0.16, ry: 0.86, key: "zone6" }
                ]

                Item {
                    width: 44
                    height: 44
                    x: mouseCanvasArea.width * modelData.rx - width / 2
                    y: mouseCanvasArea.height * modelData.ry - height / 2

                    Rectangle {
                        anchors.fill: parent
                        radius: 22
                        color: mouseController.extraModeColors[modelData.slot]
                        border.width: nodeArea.containsMouse ? 3.5 : 2.5
                        border.color: nodeArea.containsMouse ? "#ffffff" : "#d0d7de"

                        Controls.Label {
                            anchors.centerIn: parent
                            text: (modelData.slot + 1).toString()
                            font.bold: true
                            font.pixelSize: 15
                            color: "#000000"
                            style: Text.Outline
                            styleColor: "#ffffff"
                        }

                        MouseArea {
                            id: nodeArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                extraModeZoneDialog.pickColorRequested(modelData.slot, mouseController.extraModeColors[modelData.slot])
                            }
                        }

                        Controls.ToolTip.visible: nodeArea.containsMouse
                        Controls.ToolTip.text: I18n.tr(mouseController.language, modelData.key)
                    }
                }
            }
        }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            opacity: 0.72
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            text: I18n.tr(mouseController.language, "zoneMapNote")
        }

        // أزرار سريعة أسفل النافذة: تطبيق على الماوس + إغلاق
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ModernButton {
                text: I18n.tr(mouseController.language, "saveApply")
                icon.name: "document-save"
                highlighted: true
                enabled: mouseController.connected && !mouseController.isBusy
                onClicked: mouseController.applyToMouse()
            }

            Item { Layout.fillWidth: true }

            ModernButton {
                text: I18n.tr(mouseController.language, "closeBtn")
                icon.name: "dialog-close"
                onClicked: extraModeZoneDialog.close()
            }
        }
    }
}
