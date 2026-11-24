import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ModernDialog {
    headerIcon: "view-list-icons"
    id: flatpakAppsDialog
    title: I18n.tr(mouseController.language, "pickFlatpakTitle") + " (" + mouseController.activeProfileName + ")"
    width: Math.min((parent ? parent.width : 960) - 48, 600)
    height: Math.min((parent ? parent.height : 820) - 80, 520)
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside

    property string searchQuery: ""

    onOpened: {
        searchQuery = ""
        flatpakSearchField.text = ""
        if (mouseController.hostAccessAvailable) {
            mouseController.refreshInstalledFlatpakApps()
        }
    }

    function filteredApps() {
        let q = searchQuery.trim().toLowerCase()
        let all = mouseController.installedFlatpakApps
        if (q.length === 0) return all
        let out = []
        for (let i = 0; i < all.length; ++i) {
            let item = all[i]
            if (item.name.toLowerCase().indexOf(q) >= 0 || item.appId.toLowerCase().indexOf(q) >= 0) {
                out.push(item)
            }
        }
        return out
    }


    contentItem: ColumnLayout {
        LayoutMirroring.enabled: mouseController.language === "ar"
        LayoutMirroring.childrenInherit: true
        spacing: Kirigami.Units.mediumSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Controls.TextField {
                id: flatpakSearchField
                Layout.fillWidth: true
                placeholderText: I18n.tr(mouseController.language, "searchAppsPh")
                onTextEdited: flatpakAppsDialog.searchQuery = text
            }

            ModernButton {
                text: I18n.tr(mouseController.language, "refreshAppsBtn")
                icon.name: "view-refresh"
                onClicked: mouseController.refreshInstalledFlatpakApps()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 6
            color: Kirigami.Theme.backgroundColor
            border.width: 1
            border.color: Kirigami.Theme.disabledTextColor

            Controls.Label {
                anchors.centerIn: parent
                width: parent.width - 32
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: flatpakAppsList.count === 0
                opacity: 0.7
                text: I18n.tr(mouseController.language, "noFlatpakApps")
            }

            ListView {
                id: flatpakAppsList
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                spacing: 4
                model: {
                    // ربط التحديث عند تغير قائمة التطبيقات أو نص البحث
                    let _ = mouseController.installedFlatpakApps
                    let _q = flatpakAppsDialog.searchQuery
                    return flatpakAppsDialog.filteredApps()
                }
                delegate: Controls.ItemDelegate {
                    id: appDelegate
                    width: ListView.view.width
                    height: 52
                    onClicked: mouseController.toggleAppForActiveProfile(modelData.appId)

                    property bool isLinked: {
                        let _ = mouseController.activeProfileLinkedApps
                        return mouseController.isAppLinkedToActiveProfile(modelData.appId)
                    }

                    background: Rectangle {
                        radius: 6
                        color: appDelegate.isLinked
                               ? Qt.rgba(0.0, 0.85, 0.45, 0.16)
                               : (appDelegate.hovered ? Kirigami.Theme.alternateBackgroundColor : "transparent")
                        border.width: appDelegate.isLinked ? 1 : 0
                        border.color: Theme.accent
                    }

                    contentItem: RowLayout {
                        spacing: 10

                        Controls.CheckBox {
                            checked: appDelegate.isLinked
                            onToggled: mouseController.toggleAppForActiveProfile(modelData.appId)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Controls.Label {
                                Layout.fillWidth: true
                                font.bold: true
                                elide: Text.ElideRight
                                text: modelData.name
                            }

                            Controls.Label {
                                Layout.fillWidth: true
                                opacity: 0.68
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                                elide: Text.ElideRight
                                text: modelData.appId
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            ModernButton {
                text: I18n.tr(mouseController.language, "closeBtn")
                icon.name: "dialog-ok-apply"
                highlighted: true
                onClicked: flatpakAppsDialog.close()
            }
        }
    }
}
