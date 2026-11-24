import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

RowLayout {
    property string labelText: ""
    property string hintText: ""
    property bool checkedValue: false
    property bool switchEnabled: true
    signal switchToggled(bool state)

    Layout.fillWidth: true
    spacing: Kirigami.Units.largeSpacing

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: labelText
        }

        Controls.Label {
            Layout.fillWidth: true
            visible: hintText.length > 0
            wrapMode: Text.WordWrap
            opacity: 0.7
            font.pointSize: Kirigami.Theme.smallFont.pointSize
            text: hintText
        }
    }

    Controls.Switch {
        enabled: switchEnabled
        checked: checkedValue
        onToggled: switchToggled(checked)
    }
}
