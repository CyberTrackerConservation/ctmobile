import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    QtObject {
        id: internal

        property var groupList
        property bool blockRebuild: false
    }

    Component.onCompleted: {
        if (form.provider.collectForm) {
            if (!form.provider.collectStarted) {
                form.provider.startCollect()
            }
        }

        if (form.provider.incidentForm) {
            if (!form.provider.incidentStarted) {
                form.provider.startIncident()
            }
        }

        rebuildListModel()
    }

    C.PopupLoader {
        id: confirmIncidentStop
        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Discard incident?")
                onConfirmed: {
                    internal.blockRebuild = true
                    form.provider.stopIncident(true)
                    form.popPage()
                }
            }
        }
    }

    C.PopupLoader {
        id: confirmPatrolStop
        popupComponent: Component {
            C.ConfirmPopup {
                text: form.provider.surveyMode ? qsTr("End survey?") : qsTr("End patrol?")
                onConfirmed: form.pushPage("qrc:/SMART/LocationPage.qml", { tag: "stopPatrol", clean: true })
            }
        }
    }

    C.PopupLoader {
        id: confirmCollectStop
        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Stop collecting?")
                onConfirmed: {
                    internal.blockRebuild = true
                    form.newSighting(false)
                    form.provider.stopCollect()
                    Globals.patrolChangeCount++;
                    form.popPagesToParent()
                }
            }
        }
    }

    enum BackButtonType { None, Back, TrackRate }
    property int backButtonType: {
        if (form.editing) {
            return SightingHomePage.BackButtonType.None
        }

        if (form.provider.incidentForm) {
            return SightingHomePage.BackButtonType.Back
        }

        return SightingHomePage.BackButtonType.TrackRate
    }

    header: C.PageHeader {
        topText: {
            let result = ""

            if (form.provider.surveyMode && form.provider.samplingUnits) {
                let samplingUnitUid = form.getFieldValue(form.rootRecordUid, Globals.modelSamplingUnitFieldUid)
                let samplingUnitText = form.getElementName(samplingUnitUid)
                result = "[" + samplingUnitText + "]";
            }

            if (form.editing) {
                result = result + " (" + qsTr("Edit") + ") "
            }

            return result
        }

        text: form.project.title

        menuVisible: Qt.platform.os !== "ios"
        menuIcon: App.batteryIcon
        onMenuClicked: App.showToast(App.batteryText)

        formBack: false
        backEnabled: !form.editing || backIcon.toString() === C.Style.backIconSource
        backIcon: {
            switch (backButtonType) {
            case SightingHomePage.BackButtonType.None:
                return ""

            case SightingHomePage.BackButtonType.Back:
                return C.Style.backIconSource

            case SightingHomePage.BackButtonType.TrackRate:
                return form.trackStreamer.rateIcon

            default:
                console.log("Error: unknown button type - " + backButtonType)
                return ""
            }
        }

        onBackClicked: {
            if (backButtonType === SightingHomePage.BackButtonType.None) {
                return
            }

            if (backButtonType === SightingHomePage.BackButtonType.TrackRate) {
                App.showToast(form.trackStreamer.rateFullText)
                return
            }

            if (popupPatrol.opened) {
                popupPatrol.close()
                return
            }

            if (confirmPatrolStop.opened) {
                confirmPatrolStop.close()
                return
            }

            if (form.provider.patrolForm) {
                confirmPatrolStop.open()
                return
            }

            if (form.provider.incidentForm) {
                if (recordModel.hasRecords) {
                    confirmIncidentStop.open()
                    return
                }

                internal.blockRebuild = true
                form.provider.stopIncident(true)
            }

            form.popPage()
        }
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("History")
            icon.source: "qrc:/icons/history.svg"
            onClicked: form.pushPage("qrc:/SMART/HistoryPage.qml")
            visible: !form.editing && !form.provider.incidentForm
        }

        C.FooterButton {
            text: qsTr("Map")
            icon.source: "qrc:/icons/map_outline.svg"
            onClicked: form.pushPage("qrc:/MapsPage.qml")
            visible: !form.editing && !form.provider.incidentForm
        }

        C.FooterButton {
            text: form.provider.getPatrolText()
            icon.source: "qrc:/icons/shoe_print.svg"
            visible: !form.editing && form.provider.patrolForm
            onClicked: popupPatrol.open()
        }

        C.FooterButton {
            text: qsTr("Stop")
            icon.source: "qrc:/icons/stop.svg"
            visible: !form.editing && form.provider.collectForm
            onClicked: confirmCollectStop.open()
        }

        C.FooterButton {
            text: qsTr("Incident")
            icon.source: "qrc:/icons/assignment_turned_in_outline.svg"
            visible: !form.editing && form.provider.hasIncident && form.provider.patrolForm
            onClicked: form.pushFormPage({ projectUid: form.project.uid, stateSpace: Globals.formSpaceIncident })
        }

        C.FooterButton {
            id: saveButton
            text: qsTr("Save")
            icon.source: "qrc:/icons/save.svg"
            highlight: enabled
            highlightBlink: form.getSetting("blinkSaveButton", true)
            enabled: recordModel.hasRecords
            visible: !form.editing
            onClicked: {
                // Highlight fields that are required but not specified.
                if ((form.provider.instantGPS && (form.getFieldValue(Globals.modelLocationFieldUid) === undefined)) ||
                    (form.provider.useObserver && (form.getFieldValue(Globals.modelObserverFieldUid) === undefined))) {
                    instantGPSEditor.highlightInvalid = true
                    observerIdEditor.highlightInvalid = true
                    return
                }

                // Disable highlighting for the next sighting.
                instantGPSEditor.highlightInvalid = false
                observerIdEditor.highlightInvalid = false

                deleteIncompleteRecords()

                let tag = "patrolSighting"
                if (form.provider.collectForm) {
                    tag = "collectSighting"
                } else if (form.provider.incidentForm) {
                    tag = "independentIncident"
                }

                form.pushPage("qrc:/SMART/LocationPage.qml", { tag: tag })
            }

            Connections {
                target: form

                function onFieldValueChanged(recordUid, fieldUid, oldValue, newValue) {
                    if (fieldUid === Globals.modelLocationFieldUid || fieldUid === Globals.modelObserverFieldUid) {
                        saveButton.enabled = recordModel.hasRecords
                    }
                }
            }

            Connections {
                target: recordModel

                function onHasRecordsChanged() {
                    saveButton.enabled = recordModel.hasRecords
                }
            }
        }

        C.FooterButton {
            visible: form.editing
            icon.source: "qrc:/icons/cancel.svg"
            text: qsTr("Cancel")
            onClicked: {
                internal.blockRebuild = true
                if (form.provider.incidentForm) {
                    form.provider.stopIncident(true)
                }
                form.popPagesToParent()
            }
        }

        C.FooterButton {
            visible: form.editing
            icon.source: "qrc:/icons/ok.svg"
            text: qsTr("Confirm")
            enabled: saveButton.enabled
            onClicked: {
                internal.blockRebuild = true
                deleteIncompleteRecords()
                form.saveSighting()
                if (form.provider.incidentForm) {
                    form.provider.stopIncident(false)
                }
                form.popPagesToParent()
            }
        }
    }

    ColumnLayout {
        id: extraFields
        anchors.top: parent.top
        width: parent.width
        spacing: 0

        C.FieldEditorDelegate {
            id: observerIdEditor
            Layout.fillWidth: true
            recordUid: form.rootRecordUid
            fieldUid: Globals.modelObserverFieldUid
            visible: form.provider.useObserver
            enabled: !form.editing
            C.HorizontalDivider { height: 2 }
        }

        C.FieldEditorDelegate {
            id: instantGPSEditor
            Layout.fillWidth: true
            autoLocation: false
            recordUid: form.rootRecordUid
            fieldUid: Globals.modelLocationFieldUid
            visible: form.provider.instantGPS
            enabled: !form.editing
            C.HorizontalDivider { height: 2 }
        }

        C.FieldEditorDelegate {
            Layout.fillWidth: true
            recordUid: form.rootRecordUid
            fieldUid: Globals.modelDistanceFieldUid
            visible: form.provider.useDistanceAndBearing
            enabled: !form.editing
            C.HorizontalDivider { }
        }

        C.FieldEditorDelegate {
            Layout.fillWidth: true
            recordUid: form.rootRecordUid
            fieldUid: Globals.modelBearingFieldUid
            visible: form.provider.useDistanceAndBearing
            enabled: !form.editing
            C.HorizontalDivider { height: 2 }
        }
    }

    C.ListViewV {
        id: listView
        anchors.top: extraFields.bottom
        anchors.bottom: parent.bottom
        width: parent.width

        model: ListModel {
            id: recordModel
            property bool hasRecords: false
        }

        delegate: SwipeDelegate {
            id: swipeDelegate
            width: ListView.view.width

            Component {
                id: groupDelegateComponent
                RowLayout {
                    Label {
                        id: _title
                        Layout.fillWidth: true
                        text: qsTr("Group") + " " + (model.groupIndex + 1).toString()
                        font.bold: model.fontBold
                        font.pixelSize: App.settings.font16
                        color: Material.foreground
                        opacity: 0.85
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/add.svg"
                        opacity: 0.75
                    }

                    Connections {
                        target: swipeDelegate

                        function onClicked() {
                            newRecord(model.groupUid)
                        }
                    }
                }
           }

           Component {
               id: recordDelegateComponent
               RowLayout {
                   Item {
                       width: 16
                       visible: form.provider.useGroupUI
                   }

                   Label {
                       id: controlLabel
                       Layout.fillWidth: true
                       wrapMode: Label.WordWrap
                       text: form.getElementName(model.elementUid)
                       font.bold: model.fontBold
                       font.pixelSize: App.settings.font16
                       verticalAlignment: Text.AlignVCenter
                   }

                   C.SquareIcon {
                       source: form.getElementIcon(model.elementUid, true)
                       size: C.Style.minRowHeight
                   }

                   Item {
                       height: C.Style.minRowHeight
                   }

                   Connections {
                       target: swipeDelegate

                       function onClicked() {
                           if (swipeDelegate.swipe.position === 0) {
                               let recordUid = model.recordUid
                               let elementUid = form.getFieldValue(recordUid, Globals.modelTreeFieldUid)

                               if (form.project.wizardMode) {
                                   form.pushWizardIndexPage(recordUid, { fieldUids: form.getElement(elementUid).fieldUids, header: form.getElementName(elementUid) })
                               } else {
                                   form.pushPage("qrc:/SMART/AttributesPage.qml", { title: form.getElementName(elementUid), recordUid: recordUid, elementUid: elementUid } )
                               }
                           }
                       }
                   }
               }
            }

            Component {
                 id: newGroupDelegateComponent
                RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Create a new group")
                        font.bold: true
                        font.pixelSize: App.settings.font14
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        opacity: 0.5
                    }

                    Connections {
                        target: swipeDelegate

                        function onClicked() {
                            let newGroupUid = Utils.generateUuid()
                            internal.groupList = form.getFieldValue(Globals.modelIncidentGroupListFieldUid, [Utils.generateUuid()])
                            internal.groupList.push(newGroupUid)
                            form.setFieldValue(Globals.modelIncidentGroupListFieldUid, internal.groupList)
                            recordModel.insert(recordModel.count - 1, { delegate: "group", groupIndex: internal.groupList.length - 1, groupUid: newGroupUid, fontBold: true })
                        }
                    }
                }
            }

            Component {
                 id: newSightingDelegateComponent
                RowLayout {
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.minimumHeight: C.Style.iconSize24
                        verticalAlignment: Label.AlignVCenter
                        text: recordModel.hasRecords ? qsTr("Add observation") : qsTr("Make observation")
                        font.pixelSize: App.settings.font14
                        font.bold: true
                        opacity: 0.7
                    }

                    Connections {
                        target: swipeDelegate

                        function onClicked() {
                            newRecord(model.groupUid)
                        }
                    }
                }
            }

            contentItem: Loader {
                Binding { target: background; property: "color"; value: colorContent || Style.colorContent }
                width: swipeDelegate.width - swipeDelegate.leftPadding - swipeDelegate.rightPadding
                height: item.height
                sourceComponent: {
                    switch (model.delegate) {
                    case "group":
                        return groupDelegateComponent

                    case "record":
                        return recordDelegateComponent

                    case "newgroup":
                        return newGroupDelegateComponent

                    case "newsighting":
                        return newSightingDelegateComponent

                    default:
                        return undefined
                    }
                }
            }

            swipe.enabled: (model.delegate !== "newgroup") && (model.delegate !== "newsighting")

            swipe.right: Rectangle {
                width: parent.width
                height: parent.height
                color: C.Style.colorGroove

                RowLayout {
                    anchors.fill: parent
                    Label {
                        text: {
                            switch (model.delegate) {
                            case "group": return qsTr("Delete group?")
                            case "record": return qsTr("Delete item?")
                            default: return ""
                            }
                        }
                        leftPadding: App.scaleByFontSize(20)
                        Layout.fillWidth: true
                        font.bold: model.fontBold
                        font.pixelSize: App.settings.font16
                    }

                    ToolButton {
                        flat: true
                        text: qsTr("Yes")
                        onClicked: {
                            switch (model.delegate) {
                            case "group":
                                deleteGroup(model.groupIndex, model.groupUid)
                                break

                            case "record":
                                deleteRecord(model.recordUid)
                                break
                            }

                            rebuildListModel()
                        }
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: parent.height
                        Layout.maximumWidth: parent.width / 5
                        font.bold: model.fontBold
                        font.pixelSize: App.settings.font14
                    }

                    ToolButton {
                        id: deleteNo
                        flat: true
                        text: qsTr("No")
                        onClicked: swipeDelegate.swipe.close()
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: parent.height
                        Layout.maximumWidth: parent.width / 5
                        font.bold: model.fontBold
                        font.pixelSize: App.settings.font14
                    }
                }
            }

            C.HorizontalDivider {}
        }
    }

    C.PopupLoader {
        id: popupPatrol
        popupComponent: Component {
            C.PopupBase {
                anchors.centerIn: null
                insets: 0

                x: (page.width - width) / 2
                y: page.footer.y - height
                font.pixelSize: App.settings.font13

                contentItem: ColumnLayout {
                    Component.onCompleted: formPageStack.setProjectColors(this)
                    spacing: 0

                    ItemDelegate {
                        Layout.fillWidth: true
                        icon.source: "qrc:/icons/stop.svg"
                        icon.width: C.Style.iconSize24
                        icon.height: C.Style.iconSize24
                        text: form.provider.getSightingTypeText("StopPatrol")
                        onClicked: {
                            popupPatrol.close()
                            confirmPatrolStop.open()
                        }
                    }

                    ItemDelegate {
                        Layout.fillWidth: true
                        icon.source: "qrc:/icons/pause.svg"
                        icon.width: C.Style.iconSize24
                        icon.height: C.Style.iconSize24
                        text: form.provider.getSightingTypeText("PausePatrol")
                        visible: form.provider.getProfileValue("CAN_PAUSE", true)
                        onClicked: {
                            popupPatrol.close()
                            form.pushPage("qrc:/SMART/LocationPage.qml", { tag: "pausePatrol", clean: true })
                        }
                    }

                    ItemDelegate {
                        Layout.fillWidth: true
                        icon.source: "qrc:/icons/account_edit.svg"
                        icon.width: C.Style.iconSize24
                        icon.height: C.Style.iconSize24
                        text: form.provider.getSightingTypeText("ChangePatrol")
                        onClicked: {
                            popupPatrol.close()

                            // Reset the location.
                            form.resetFieldValue(Globals.modelLocationFieldUid)

                            form.pushPage("qrc:/SMART/MetadataPage.qml")
                        }

                        C.HorizontalDivider {}
                    }

                    ItemDelegate {
                        Layout.fillWidth: true
                        icon.source: "qrc:/icons/finance.svg"
                        icon.width: C.Style.iconSize24
                        icon.height: C.Style.iconSize24
                        text: qsTr("Statistics")
                        onClicked: {
                            popupPatrol.close()
                            form.pushPage("qrc:/SMART/PatrolStatsPage.qml")
                        }

                        C.HorizontalDivider {}
                    }

                    ItemDelegate {
                        Layout.fillWidth: true
                        icon.width: C.Style.iconSize24
                        icon.height: C.Style.iconSize24
                        icon.source: "qrc:/icons/theme_light_dark.svg"
                        text: qsTr("Toggle dark theme")

                        onClicked: {
                            popupPatrol.close()
                            App.settings.darkTheme = !App.settings.darkTheme
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: form

        function onSightingModified(sightingUid) {
            rebuildListModel()
        }

        function onEditingChanged() {
            if (!form.editing) {
                internal.blockRebuild = true
                form.popPage(StackView.Immediate)
            }
        }

        function onRecordComplete(recordUid) {
            let parentRecordUid = form.getParentRecordUid(recordUid)
            if (parentRecordUid === form.rootRecordUid) {
                rebuildListModel()
            }
        }

        function onFieldValueChanged(recordUid, fieldUid, oldValue, newValue) {
            if (fieldUid === Globals.modelTreeFieldUid) {
                rebuildListModel()
            }
        }
    }

    function getRecordModelElementUid(recordUid) {
        return form.getFieldValue(recordUid, Globals.modelTreeFieldUid)
    }

    function getRecordsInGroup(groupUid) {
        let result = []
        let recordUids = form.getFieldValue(Globals.modelIncidentRecordFieldUid, [])
        for (let i = 0; i < recordUids.length; i++) {
            let recordUid = recordUids[i]
            let recordGroupUid = form.getFieldValue(recordUid, Globals.modelIncidentGroupFieldUid)
            if (recordGroupUid !== groupUid) continue
            result.push(recordUid)
        }
        return result
    }

    function newRecord(groupUid) {
        deleteIncompleteRecords()
        let newRecordUid = form.newRecord(form.rootRecordUid, Globals.modelIncidentRecordFieldUid)
        form.setFieldValue(newRecordUid, Globals.modelIncidentGroupFieldUid, groupUid)
        form.pushPage("qrc:/SMART/ModelPage.qml", { recordUid: newRecordUid })
    }

    function deleteGroup(groupIndex, groupUid) {
        let recordUids = getRecordsInGroup(groupUid)
        for (let i = 0; i < recordUids.length; i++) {
            deleteRecord(recordUids[i])
        }
        internal.groupList.splice(groupIndex, 1)
        form.setFieldValue(Globals.modelIncidentGroupListFieldUid, internal.groupList)
        form.saveState()
    }

    function deleteRecord(recordUid) {
        form.deleteRecord(recordUid)
        form.saveState()
    }

    function deleteIncompleteRecords() {
        if (formPageStack.loading) {
            return
        }

        let recordUids = form.getFieldValue(Globals.modelIncidentRecordFieldUid, [])
        let changed = false
        for (let i = 0; i < recordUids.length; i++) {
            let recordUid = recordUids[i]
            let elementUid = getRecordModelElementUid(recordUid)
            if (elementUid === undefined) {
                deleteRecord(recordUid)
            }
        }
    }

    function rebuildListModel() {

        if (internal.blockRebuild) {
            return
        }

        let useGroupUI = form.provider.useGroupUI

        listView.model = undefined
        recordModel.clear()
        recordModel.hasRecords = false

        internal.groupList = form.getFieldValue(Globals.modelIncidentGroupListFieldUid, [Utils.generateUuid()])
        form.setFieldValue(Globals.modelIncidentGroupListFieldUid, internal.groupList)

        for (let j = 0; j < internal.groupList.length; j++) {
            if (useGroupUI) {
                recordModel.append({ delegate: "group", groupIndex: j, groupUid: internal.groupList[j], fontBold: true })
            }

            let recordUids = getRecordsInGroup(internal.groupList[j])

            for (let i = 0; i < recordUids.length; i++) {
                let recordUid = recordUids[i]
                let elementUid = getRecordModelElementUid(recordUid)
                if (elementUid === undefined) {
                    continue
                }

                recordModel.append({ delegate: "record", elementUid: elementUid, groupUid: internal.groupList[j], recordUid: recordUid, fontBold: false })
                recordModel.hasRecords = true
            }
        }

        if (useGroupUI) {
            recordModel.append({ delegate: "newgroup" })
        } else {
            recordModel.append({ delegate: "newsighting", groupUid: internal.groupList[0] })
        }

        listView.model = recordModel
    }
}
