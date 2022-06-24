import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        id: headerBar
        text: qsTr("History")
        menuVisible: form.provider.hasPatrol
        menuEnabled: form.provider.patrolLegs.length > 0
        menuIcon: "qrc:/icons/finance.svg"
        onMenuClicked: {
            form.pushPage("qrc:/SMART/PatrolStatsPage.qml")
        }
    }

    ButtonGroup {
        buttons: buttonRow.children
    }

    footer: ColumnLayout {
        spacing: 0
        width: parent.width

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
            opacity: Material.theme === Material.Dark ? 0.12 : 0.12
        }

        RowLayout {
            id: buttonRow
            spacing: 0
            visible: !form.editing
            Layout.fillWidth: true
            property int buttonCount: {
                var result = 0
                result += form.provider.hasCollect ? 1 : 0
                result += form.provider.hasIncident ? 1 : 0
                result += form.provider.hasPatrol ? 1 : 0
                page.footer.visible = result > 1
                return result
            }

            property int buttonWidth: page.width / buttonCount
            property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Collect")
                checkable: true
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: checked ? "qrc:/icons/assignment_turned_in_fill.svg" : "qrc:/icons/assignment_turned_in_outline.svg"
                Material.foreground: buttonRow.buttonColor
                visible: form.provider.hasCollect
                onClicked: stackLayout.currentIndex = 0
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: form.provider.getPatrolText()
                checkable: true
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/shoe_print.svg"
                Material.foreground: buttonRow.buttonColor
                visible: form.provider.hasPatrol
                onClicked: stackLayout.currentIndex = 1
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Incidents")
                checkable: true
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: checked ? "qrc:/icons/assignment_turned_in_fill.svg" : "qrc:/icons/assignment_turned_in_outline.svg"
                Material.foreground: buttonRow.buttonColor
                visible: form.provider.hasIncident
                onClicked: stackLayout.currentIndex = 2
            }
        }
    }

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        property bool loadComplete: false
        Component.onCompleted: {
            let defaultIndex = 0
            if (form.provider.hasCollect) {
                defaultIndex = 0
            } else if (form.provider.hasPatrol) {
                defaultIndex = 1
            } else if (form.provider.hasIncident) {
                defaultIndex = 2
            }

            currentIndex = form.getControlState("HistoryPage", "", "TabIndex", defaultIndex)
            loadComplete = true
        }

        clip: true

        C.Form {
            id: formCollect
            projectUid: form.provider.hasCollect ? form.project.uid : ""
            stateSpace: Globals.formSpaceCollect
            width: parent.width
            height: parent.height

            C.ListViewV {
                id: listViewCollect
                anchors.fill: parent

                model: C.SightingListModel {}
                delegate: HistoryRowDelegate {
                    form: formCollect
                    onClicked: pushSightingPage(listViewCollect.model, model.index)
                }
            }

            Label {
                anchors.fill: parent
                text: qsTr("No data")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: formCollect.connected && listViewCollect.model.count === 0
                opacity: 0.5
                font.pixelSize: App.settings.font20
            }
        }

        C.Form {
            id: formPatrol
            projectUid: form.provider.hasPatrol ? form.project.uid : ""
            stateSpace: Globals.formSpacePatrol
            readonly: true
            width: parent.width
            height: parent.height

            C.ListViewV {
                id: listViewPatrol
                anchors.fill: parent
                model: C.SightingListModel {}
                delegate: HistoryRowDelegate {
                    form: formPatrol
                    onClicked: pushSightingPage(listViewPatrol.model, model.index)
                }
            }

            Label {
                anchors.fill: parent
                text: qsTr("No data")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: formPatrol.connected && listViewPatrol.model.count === 0
                opacity: 0.5
                font.pixelSize: App.settings.font20
            }
        }

        C.Form {
            id: formIncident
            projectUid: form.provider.hasIncident ? form.project.uid : ""
            stateSpace: Globals.formSpaceIncident
            readonly: true
            width: parent.width
            height: parent.height

            C.ListViewV {
                id: listViewIncident
                anchors.fill: parent
                model: C.SightingListModel {}
                delegate: HistoryRowDelegate {
                    form: formIncident
                    onClicked: pushSightingPage(listViewIncident.model, model.index)
                }
            }

            Label {
                anchors.fill: parent
                text: qsTr("No data")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: formIncident.connected && listViewIncident.model.count === 0
                opacity: 0.5
                font.pixelSize: App.settings.font20
            }
        }
    }

    Connections {
        target: stackLayout

        function onCurrentIndexChanged() {
            buttonRow.children[stackLayout.currentIndex].checked = true
            if (stackLayout.loadComplete) {
                form.setControlState("HistoryPage", "", "TabIndex", stackLayout.currentIndex)
            }
        }
    }

    function pushSightingPage(model, index) {
        var sightingList = []
        for (var i = 0; i < model.count; i++) {
            var sighting = model.get(i)
            sightingList.push( { createTime: sighting.createTime, stateSpace: sighting.stateSpace, sightingUid: sighting.rootRecordUid } )
        }

        form.pushPage("qrc:/SightingPage.qml", { sightingList: sightingList, currentIndex: index })
    }
}
