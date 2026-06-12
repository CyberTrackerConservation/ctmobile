import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

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

    RowLayout {
        id: buttonRow
        spacing: 0
        visible: !form.editing
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ButtonGroup { id: buttonGroup }

        Component.onCompleted: {
            let count = 0
            count += form.provider.hasCollect ? 1 : 0
            count += form.provider.hasIncident ? 1 : 0
            count += form.provider.hasPatrol ? 1 : 0

            buttonRow.visible = count > 1
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
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: buttonRow.visible ? buttonRow.top : parent.bottom
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

        C.SightingsListView {
            id: listViewCollect
            width: parent.width
            height: parent.height
            filter.projectUid: form.project.uid
            filter.stateSpace: Globals.formSpaceCollect
            active: form.provider.hasCollect

            onClicked: function (sighting, index) {
                pushSightingPage(model, index)
            }
        }

        C.SightingsListView {
            id: listViewPatrol
            width: parent.width
            height: parent.height
            filter.projectUid: form.project.uid
            filter.stateSpace: Globals.formSpacePatrol
            active: form.provider.hasPatrol

            onClicked: function (sighting, index) {
                pushSightingPage(model, index)
            }
        }

        C.SightingsListView {
            id: listViewIncident
            width: parent.width
            height: parent.height
            filter.projectUid: form.project.uid
            filter.stateSpace: Globals.formSpaceIncident
            active: form.provider.hasIncident

            onClicked: function (sighting, index) {
                pushSightingPage(model, index)
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
            sightingList.push( { createTime: sighting.createTime, projectUid: form.project.uid, stateSpace: sighting.stateSpace, sightingUid: sighting.sightingUid, canEdit: sighting.canEdit, canDelete: sighting.canDelete } )
        }

        form.pushPage("qrc:/SMART/SightingPage.qml", { sightingList: sightingList, currentIndex: index })
    }
}
