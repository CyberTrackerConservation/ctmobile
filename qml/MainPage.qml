import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var projectCount: 0

    Timer {
        interval: 500
        running: true
        repeat: false
        onTriggered: {
            if (App.settings.classicUpgradeMessage) {
                appPageStack.push("qrc:/BreakingChangePage.qml", StackView.Immediate)
            }
        }
    }

    footer: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
            opacity: Material.theme === Material.Dark ? 0.12 : 0.12
        }

        RowLayout {
            id: buttonRow
            spacing: 0
            Layout.fillWidth: true
            property int buttonCount: 4
            property int buttonWidth: page.width / buttonCount
            property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

            ButtonGroup {
                buttons: buttonRow.children
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillHeight: true
                text: qsTr("Connect")
                checkable: true
                checked: stackLayout.currentIndex === 0
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: checked ? "qrc:/icons/view_grid_plus.svg" : "qrc:/icons/view_grid_plus_outline.svg"
                Material.foreground: buttonRow.buttonColor
                onClicked: stackLayout.currentIndex = 0
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillHeight: true
                text: qsTr("Projects")
                checkable: true
                checked: stackLayout.currentIndex === 1
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: checked ? "qrc:/icons/clipboard_multiple.svg" : "qrc:/icons/clipboard_multiple_outline.svg"
                Material.foreground: buttonRow.buttonColor
                onClicked: stackLayout.currentIndex = 1
                enabled: page.projectCount > 0
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillHeight: true
                text: qsTr("Map")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/map_outline.svg"
                Material.foreground: buttonRow.buttonColor
                onClicked: {
                    if (App.requestPermissionLocation()) {
                        appPageStack.push("qrc:/MapsPage.qml", StackView.Immediate)
                    }
                }
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillHeight: true
                text: qsTr("Settings")
                checkable: true
                checked: stackLayout.currentIndex === 2
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: checked ? "qrc:/icons/settings.svg" : "qrc:/icons/settings_outline.svg"
                Material.foreground: buttonRow.buttonColor
                onClicked: stackLayout.currentIndex = 2
            }
        }
    }

    StackLayout {
        id: stackLayout
        anchors.fill: parent

        Component.onCompleted: {
            page.projectCount = App.projectManager.count()
            currentIndex = page.projectCount === 0 ? 0 : 1
        }

        clip: true

        ConnectPage {
        }

        ProjectsPage {
        }

        SettingsPage {
        }
    }

    Connections {
        target: App.projectManager

        function onProjectsChanged() {
            page.projectCount = App.projectManager.count()
            if (stackLayout.currentIndex === 0 && page.projectCount !== 0) {
                stackLayout.currentIndex = 1
            } else if (stackLayout.currentIndex === 1 && page.projectCount === 0) {
                stackLayout.currentIndex = 0
            }
        }
    }
}
