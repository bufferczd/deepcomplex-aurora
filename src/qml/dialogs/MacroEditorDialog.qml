import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "input-keyboard"
    id: macroDialog
    title: I18n.tr(mouseController.language, "macroTitle")
    onClosed: capturing = false
    width: Math.min((parent ? parent.width : 980) - 44, 740)
    height: Math.min((parent ? parent.height : 840) - 56, 680)
    closePolicy: capturing ? Controls.Popup.NoAutoClose : Controls.Popup.CloseOnEscape

    property int editingLibraryIndex: -1   // -1 = إنشاء ماكرو جديد في الاستوديو
    property string editName: ""
    property bool capturing: false
    property bool useFixedDelay: true
    property int fixedDelayMs: 24          // مخصص من 8 إلى 1016 ms (خطوة العتاد 8 ms)
    property double lastEventMs: 0
    property int editRepeatMode: 0
    property int editRepeatCount: 1
    property var editSteps: []

    function clampDelay(ms) {
        return Math.max(8, Math.min(1016, Math.round(ms)))
    }

    function syncMacroInputs() {
        macroNameInput.text = editName
        macroExecModeCombo.currentIndex = editRepeatMode
        macroRepeatSpin.value = editRepeatCount
        fixedDelaySpin.value = fixedDelayMs
    }

    function openForNewMacro() {
        editingLibraryIndex = -1
        editName = ""
        capturing = false
        lastEventMs = 0
        editRepeatMode = 0
        editRepeatCount = 1
        editSteps = []
        syncMacroInputs()
        open()
    }

    function openForLibraryIndex(idx) {
        editingLibraryIndex = idx
        capturing = false
        lastEventMs = 0
        let data = mouseController.getLibraryMacro(idx)
        editName = data.name || ""
        editRepeatMode = data.repeatMode !== undefined ? data.repeatMode : 0
        editRepeatCount = data.repeatCount !== undefined ? data.repeatCount : 1
        let cloned = []
        if (data.steps) {
            for (let i = 0; i < data.steps.length; ++i) {
                let s = data.steps[i]
                cloned.push({
                    hidCode: s.hidCode,
                    isDown: s.isDown,
                    delayMs: clampDelay(s.delayMs !== undefined ? s.delayMs : fixedDelayMs),
                    label: s.label || mouseController.hidUsageName(s.hidCode)
                })
            }
        }
        editSteps = cloned
        syncMacroInputs()
        open()
    }

    function updateStepDelay(stepIdx, newDelay) {
        if (stepIdx < 0 || stepIdx >= editSteps.length) return
        editSteps[stepIdx].delayMs = clampDelay(newDelay)
    }

    function applyFixedDelayToAll() {
        let d = clampDelay(fixedDelayMs)
        let copy = []
        for (let i = 0; i < editSteps.length; ++i) {
            let s = editSteps[i]
            copy.push({
                hidCode: s.hidCode,
                isDown: s.isDown,
                delayMs: d,
                label: s.label
            })
        }
        editSteps = copy
    }

    function recordKeyEvent(qtKey, nativeScanCode, isDown) {
        if (!capturing) return
        // استخدام nativeScanCode الفيزيائي أولاً ليتم التقاط المفاتيح حتى عندما يكون الكيبورد بالعربية أو الروسية
        let hid = mouseController.keyEventToHidUsage(qtKey, nativeScanCode)
        if (hid === 0) return

        let now = Date.now()
        let delay = clampDelay(fixedDelayMs)
        if (!useFixedDelay && lastEventMs > 0) {
            delay = clampDelay(now - lastEventMs)
        }
        lastEventMs = now

        let copy = editSteps.slice()
        if (copy.length >= 120) return
        copy.push({
            hidCode: hid,
            isDown: isDown,
            delayMs: delay,
            label: mouseController.hidUsageName(hid)
        })
        editSteps = copy
    }

    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        // اسم الماكرو
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Controls.Label {
                text: I18n.tr(mouseController.language, "macroNameLabel")
            }

            Controls.TextField {
                id: macroNameInput
                Layout.fillWidth: true
                enabled: !macroDialog.capturing
                placeholderText: I18n.tr(mouseController.language, "macroNamePh")
                onTextEdited: macroDialog.editName = text
            }
        }

        // منطقة التقاط المفاتيح التفاعلية + تخصيص التأخير الثابت
        Rectangle {
            id: captureBox
            Layout.fillWidth: true
            implicitHeight: captureCol.implicitHeight + 20
            radius: Theme.buttonRadius
            color: macroDialog.capturing
                   ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.16)
                   : Theme.sidebarBg
            border.width: macroDialog.capturing ? 2 : 1
            border.color: macroDialog.capturing ? Theme.accent : Theme.borderSubtle
            focus: macroDialog.capturing

            Keys.onPressed: (event) => {
                if (!macroDialog.capturing) return
                if (event.isAutoRepeat) { event.accepted = true; return }
                macroDialog.recordKeyEvent(event.key, event.nativeScanCode, true)
                event.accepted = true
            }
            Keys.onReleased: (event) => {
                if (!macroDialog.capturing) return
                if (event.isAutoRepeat) { event.accepted = true; return }
                macroDialog.recordKeyEvent(event.key, event.nativeScanCode, false)
                event.accepted = true
            }

            ColumnLayout {
                id: captureCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 10
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ModernButton {
                        text: macroDialog.capturing ? I18n.tr(mouseController.language, "stopCapture") : I18n.tr(mouseController.language, "startCapture")
                        icon.name: macroDialog.capturing ? "media-playback-stop" : "media-record"
                        highlighted: true
                        onClicked: {
                            macroDialog.capturing = !macroDialog.capturing
                            macroDialog.lastEventMs = 0
                            if (macroDialog.capturing) {
                                captureBox.forceActiveFocus()
                            }
                        }
                    }

                    Controls.CheckBox {
                        text: I18n.tr(mouseController.language, "fixedDelay")
                        enabled: !macroDialog.capturing
                        checked: macroDialog.useFixedDelay
                        onToggled: macroDialog.useFixedDelay = checked
                    }

                    Controls.SpinBox {
                        id: fixedDelaySpin
                        enabled: !macroDialog.capturing
                        from: 8
                        to: 1016
                        stepSize: 8
                        editable: true
                        value: macroDialog.fixedDelayMs
                        onValueModified: macroDialog.fixedDelayMs = macroDialog.clampDelay(value)
                    }

                    Controls.Label {
                        opacity: 0.85
                        text: "ms"
                    }

                    ModernButton {
                        implicitHeight: 32
                        text: I18n.tr(mouseController.language, "applyDelayAllBtn")
                        icon.name: "dialog-ok-apply"
                        enabled: !macroDialog.capturing && macroDialog.editSteps.length > 0
                        onClicked: macroDialog.applyFixedDelayToAll()
                    }

                    Item { Layout.fillWidth: true }

                    ModernButton {
                        text: I18n.tr(mouseController.language, "clearSteps")
                        icon.name: "edit-clear"
                        enabled: !macroDialog.capturing && macroDialog.editSteps.length > 0
                        onClicked: macroDialog.editSteps = []
                    }
                }

                // أزرار سريعة للتأخير المخصص (تعمل للكيبورد ونقرات الماوس)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Controls.Label {
                        opacity: 0.78
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        text: I18n.tr(mouseController.language, "macroDelayUnitHint")
                    }

                    Item { Layout.fillWidth: true }

                    Repeater {
                        model: [8, 16, 24, 48, 96]
                        ModernButton {
                            implicitHeight: 26
                            leftPadding: 8
                            rightPadding: 8
                            enabled: !macroDialog.capturing
                            highlighted: macroDialog.fixedDelayMs === modelData
                            text: modelData + " ms"
                            onClicked: {
                                macroDialog.fixedDelayMs = modelData
                                fixedDelaySpin.value = modelData
                            }
                        }
                    }
                }

                Controls.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.bold: macroDialog.capturing
                    opacity: macroDialog.capturing ? 1.0 : 0.78
                    text: macroDialog.capturing ? I18n.tr(mouseController.language, "captureListening") : I18n.tr(mouseController.language, "captureIdle")
                }
            }
        }

        // إدراج نقرات الماوس داخل تسلسل الماكرو بالتأخير المخصص
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Controls.Label {
                text: I18n.tr(mouseController.language, "insertMouseLabel")
                opacity: 0.85
            }

            Repeater {
                model: [
                    { code: 0xF1, key: "mouseLeftBtn" },
                    { code: 0xF2, key: "mouseRightBtn" },
                    { code: 0xF3, key: "mouseMiddleBtn" }
                ]
                ModernButton {
                    enabled: !macroDialog.capturing
                    text: I18n.tr(mouseController.language, modelData.key)
                    icon.name: "input-mouse"
                    onClicked: {
                        let copy = macroDialog.editSteps.slice()
                        if (copy.length + 2 > 120) return
                        let lbl = mouseController.hidUsageName(modelData.code)
                        let d = macroDialog.clampDelay(macroDialog.fixedDelayMs)
                        copy.push({ hidCode: modelData.code, isDown: true, delayMs: d, label: lbl })
                        copy.push({ hidCode: modelData.code, isDown: false, delayMs: d, label: lbl })
                        macroDialog.editSteps = copy
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Controls.Label {
                opacity: 0.72
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                text: "(" + macroDialog.fixedDelayMs + " ms)"
            }
        }

        // إعدادات طريقة التنفيذ والتكرار
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Controls.Label {
                text: I18n.tr(mouseController.language, "execModeLabel")
            }

            Controls.ComboBox {
                id: macroExecModeCombo
                Layout.fillWidth: true
                enabled: !macroDialog.capturing
                model: [I18n.tr(mouseController.language, "execOnce"), I18n.tr(mouseController.language, "execToggle"), I18n.tr(mouseController.language, "execHold")]
                Component.onCompleted: currentIndex = macroDialog.editRepeatMode
                onModelChanged: currentIndex = macroDialog.editRepeatMode
                onActivated: (idx) => macroDialog.editRepeatMode = idx
            }

            Controls.Label {
                visible: macroDialog.editRepeatMode === 0
                text: I18n.tr(mouseController.language, "repeatCountLabel")
            }

            Controls.SpinBox {
                id: macroRepeatSpin
                visible: macroDialog.editRepeatMode === 0
                enabled: !macroDialog.capturing
                from: 1
                to: 99
                Component.onCompleted: value = macroDialog.editRepeatCount
                onValueModified: macroDialog.editRepeatCount = value
            }
        }

        // عرض الخطوات المسجلة (مع إمكانية تعديل تأخير كل خطوة مباشرة)
        Controls.Label {
            text: I18n.tr(mouseController.language, "recordedSteps") + " (" + macroDialog.editSteps.length + ")"
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.buttonRadius
            color: Theme.inputBg
            border.width: 1
            border.color: Theme.borderSubtle

            Controls.Label {
                anchors.centerIn: parent
                visible: macroDialog.editSteps.length === 0
                opacity: 0.6
                text: I18n.tr(mouseController.language, "noStepsYet")
            }

            ListView {
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                spacing: 4
                model: macroDialog.editSteps
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 38
                    radius: Math.min(8, Theme.buttonRadius)
                    color: modelData.isDown
                           ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.14)
                           : Qt.rgba(0.5, 0.5, 0.5, 0.12)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        spacing: 8

                        Controls.Label {
                            text: (index + 1) + "."
                            opacity: 0.65
                            Layout.minimumWidth: 24
                        }

                        Controls.Label {
                            font.bold: true
                            text: (modelData.isDown ? "↓ " : "↑ ") + modelData.label
                        }

                        Item { Layout.fillWidth: true }

                        Controls.SpinBox {
                            implicitHeight: 30
                            enabled: !macroDialog.capturing
                            from: 8
                            to: 1016
                            stepSize: 8
                            editable: true
                            value: modelData.delayMs
                            onValueModified: macroDialog.updateStepDelay(index, value)
                        }

                        Controls.Label {
                            opacity: 0.75
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            text: "ms"
                        }

                        Controls.ToolButton {
                            icon.name: "list-remove"
                            enabled: !macroDialog.capturing
                            onClicked: {
                                let copy = macroDialog.editSteps.slice()
                                copy.splice(index, 1)
                                macroDialog.editSteps = copy
                            }
                        }
                    }
                }
            }
        }

        // أزرار الحفظ والإلغاء
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            ModernButton {
                text: I18n.tr(mouseController.language, "cancelBtn")
                icon.name: "dialog-cancel"
                onClicked: {
                    macroDialog.capturing = false
                    macroDialog.close()
                }
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "saveMacro")
                icon.name: "dialog-ok-apply"
                highlighted: true
                enabled: macroDialog.editSteps.length > 0
                onClicked: {
                    macroDialog.capturing = false
                    if (macroDialog.editingLibraryIndex < 0) {
                        mouseController.createLibraryMacro(
                            macroDialog.editName,
                            macroDialog.editRepeatMode,
                            macroDialog.editRepeatCount,
                            macroDialog.editSteps
                        )
                    } else {
                        mouseController.updateLibraryMacro(
                            macroDialog.editingLibraryIndex,
                            macroDialog.editName,
                            macroDialog.editRepeatMode,
                            macroDialog.editRepeatCount,
                            macroDialog.editSteps
                        )
                    }
                    macroDialog.close()
                }
            }
        }
    }
}
