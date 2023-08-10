import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property int projectCount: 0

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

    footer: RowLayout {
        id: buttonRow
        spacing: 0

        ButtonGroup { id: buttonGroup }

        C.FooterButton {
            text: qsTr("Connect")
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: stackLayout.currentIndex === 0
            icon.source: checked ? "qrc:/icons/view_grid_plus.svg" : "qrc:/icons/view_grid_plus_outline.svg"
            onClicked: stackLayout.currentIndex = 0
            visible: App.config.showConnectPage
        }

        C.FooterButton {
            id: projectsButton
            ButtonGroup.group: buttonGroup
            text: App.alias_Projects
            checkable: true
            checked: stackLayout.currentIndex === 1
            icon.source: checked ? "qrc:/icons/clipboard_multiple.svg" : "qrc:/icons/clipboard_multiple_outline.svg"
            onClicked: stackLayout.currentIndex = 1
            enabled: !App.config.showConnectPage || (page.projectCount > 0)
        }

        C.FooterButton {
            text: qsTr("Map")
            icon.source: C.Style.mapIconSource
            onClicked: {
                appPageStack.push("qrc:/MapsPage.qml", StackView.Immediate)
            }
        }

        C.FooterButton {
            text: qsTr("Settings")
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: stackLayout.currentIndex === 2
            icon.source: checked ? "qrc:/icons/settings.svg" : "qrc:/icons/settings_outline.svg"
            onClicked: stackLayout.currentIndex = 2
        }
    }

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        clip: true

        Component.onCompleted: {
            page.projectCount = App.projectManager.count()
            currentIndex = App.config.showConnectPage && page.projectCount === 0 ? 0 : 1
        }

        onCurrentIndexChanged: {
            if (buttonRow.children[stackLayout.currentIndex].checkable) {
                buttonRow.children[stackLayout.currentIndex].checked = true
            }
        }

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

            if (!App.config.showConnectPage) {
                stackLayout.currentIndex = 1
            } else if (stackLayout.currentIndex === 0 && page.projectCount !== 0) {
                stackLayout.currentIndex = 1
            } else if (stackLayout.currentIndex === 1 && page.projectCount === 0) {
                stackLayout.currentIndex = 0
            }
        }
    }
}
