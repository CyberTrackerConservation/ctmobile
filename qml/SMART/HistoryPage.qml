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

    footer: RowLayout {
        id: buttonRow
        spacing: 0
        visible: !form.editing

        ButtonGroup { id: buttonGroup }

        Component.onCompleted: {
            let count = 0
            count += form.provider.hasCollect ? 1 : 0
            count += form.provider.hasIncident ? 1 : 0
            count += form.provider.hasPatrol ? 1 : 0

            page.footer.visible = count > 1
        }

        C.FooterButton {
            ButtonGroup.group: buttonGroup
            text: qsTr("Collect")
            checkable: true
            icon.source: checked ? "qrc:/icons/assignment_turned_in_fill.svg" : "qrc:/icons/assignment_turned_in_outline.svg"
            visible: form.provider.hasCollect
            onClicked: stackLayout.currentIndex = 0
        }

        C.FooterButton {
            ButtonGroup.group: buttonGroup
            text: form.provider.getPatrolText()
            checkable: true
            icon.source: "qrc:/icons/shoe_print.svg"
            visible: form.provider.hasPatrol
            onClicked: stackLayout.currentIndex = 1
        }

        C.FooterButton {
            ButtonGroup.group: buttonGroup
            text: qsTr("Incidents")
            checkable: true
            icon.source: checked ? "qrc:/icons/assignment_turned_in_fill.svg" : "qrc:/icons/assignment_turned_in_outline.svg"
            visible: form.provider.hasIncident
            onClicked: stackLayout.currentIndex = 2
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

            C.FormSightingsListView {
                id: listViewCollect
                anchors.fill: parent
                groupExported: false
                onClicked: function (sighting, index) {
                    pushSightingPage(model, index)
                }
            }
        }

        C.Form {
            id: formPatrol
            projectUid: form.provider.hasPatrol ? form.project.uid : ""
            stateSpace: Globals.formSpacePatrol
            readonly: true
            width: parent.width
            height: parent.height

            C.FormSightingsListView {
                id: listViewPatrol
                anchors.fill: parent
                groupExported: false
                onClicked: function (sighting, index) {
                    pushSightingPage(model, index)
                }
            }
        }

        C.Form {
            id: formIncident
            projectUid: form.provider.hasIncident ? form.project.uid : ""
            stateSpace: Globals.formSpaceIncident
            readonly: true
            width: parent.width
            height: parent.height

            C.FormSightingsListView {
                id: listViewIncident
                anchors.fill: parent
                groupExported: false
                onClicked: function (sighting, index) {
                    pushSightingPage(model, index)
                }
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
