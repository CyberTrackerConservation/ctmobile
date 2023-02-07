import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtLocation 5.12
import QtPositioning 5.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var tag

    property double positionMax: 1
    property var lastFix
    property bool instantGPS: false
    property bool clean: false
    property bool skipVisible: false

    Component.onCompleted: {
        // Hack: need to get Provider::getStartPage to return params.
        if (tag === undefined && form.provider.patrolPaused) {
            tag = "resumePatrol"
        }

        if (clean) {
            form.resetFieldValue(Globals.modelLocationFieldUid)
        }

        lastFix = form.getFieldValue(Globals.modelLocationFieldUid)
        instantGPS = (lastFix !== undefined) && (form.provider.instantGPS || form.provider.allowManualGPS)
        if (instantGPS) {
            progressCircle.progressPercent = 100
            closeTimer.interval = 500
            closeTimer.start()
            return
        }

        if (!form.provider.patrolForm || (form.provider.getProfileValue("WAYPOINT_TIMER") > 45)) {
            positionMax = form.provider.getProfileValue("SIGHTING_FIX_COUNT", 1)
        }

        lastFix = { x: 0.0, y: 0.0, spatialReference: { wkid: 4326 } }

        progressCircle.progressPercent = 0
        progressBar.visible = false
        positionSource.active = true

        skipTimer.interval = form.provider.getProfileValue("SKIP_BUTTON_TIMEOUT", 0) * 1000
        skipTimer.running = skipTimer.interval > 0
    }

    function success(locationValue) {
        header.enabled = false
        form.setFieldValue(Globals.modelLocationFieldUid, locationValue)
        form.snapCreateTime()

        switch (page.tag) {
        case "collectSighting":
            form.setFieldValue(Globals.modelIncidentTypeFieldUid, "Observation")
            form.setFieldValue(Globals.modelCollectUserFieldUid, form.provider.collectUser)
            form.saveSighting()
            form.newSighting(false)
            showToast(qsTr("Observation saved"))
            break

        case "patrolSighting":
            form.setFieldValue(Globals.modelIncidentTypeFieldUid, "Observation")
            form.saveSighting()
            form.resetFieldValue(Globals.modelIncidentGroupListFieldUid)
            form.resetFieldValue(Globals.modelIncidentRecordFieldUid)
            form.resetFieldValue(Globals.modelLocationFieldUid)
            form.resetFieldValue(Globals.modelDistanceFieldUid)
            form.resetFieldValue(Globals.modelBearingFieldUid)
            form.newSighting(true)
            showToast(qsTr("Observation saved"))
            break

        case "independentIncident":
            if (form.provider.incidentStarted) {
                form.setFieldValue(Globals.modelIncidentTypeFieldUid, "Observation")
                form.saveSighting()
                form.provider.stopIncident()
                showToast(qsTr("Incident saved"))
            }
            break

        case "pausePatrol":
            if (form.provider.patrolStarted && !form.provider.patrolPaused) {
                form.provider.pausePatrol()
                showToast(form.provider.surveyMode ? qsTr("Survey paused") : qsTr("Patrol paused"))
            }
            break

        case "stopPatrol":
            labelStopMessage.visible = true
            if (form.provider.patrolStarted && !form.provider.patrolPaused) {
                form.provider.stopPatrol()
                showToast(form.provider.surveyMode ? qsTr("Survey completed") : qsTr("Patrol completed"))
            }
            break

        case "resumePatrol":
            if (form.provider.patrolStarted && form.provider.patrolPaused) {
                form.provider.resumePatrol()
                showToast(form.provider.surveyMode ? qsTr("Survey resumed") : qsTr("Patrol resumed"))
            }
            break
        }

        Globals.patrolChangeCount++
    }

    function close() {
        switch (page.tag) {
        case "collectSighting":
            form.popPage(StackView.Immediate)
            break

        case "patrolSighting":
            form.popPage(StackView.Immediate)
            break

        case "independentIncident":
            form.popPagesToParent()
            break

        case "pausePatrol":
            form.popPagesToParent()
            break

        case "stopPatrol":
            form.popPagesToParent()
            break

        case "resumePatrol":
            form.replaceLastPage("qrc:/SMART/SightingHomePage.qml")
            break
        }
    }

    header: C.PageHeader {
        id: title
        text: qsTr("Acquiring location")
    }

    C.Skyplot {
        id: progressCircle
        active: !page.instantGPS
        anchors.centerIn: parent
        compassVisible: false
        width: parent.width * 2 / 3
        height: parent.height * 2 / 3
        progressVisible: true
        legendVisible: true
        legendSpace: parent.height / 16
        legendDepth: parent.height / 16
        satNumbersVisible: false
    }

    Label {
        id: labelWaitForTimeCorrection
        text: qsTr("Waiting for time correction")
        visible: form.requireGPSTime && !App.timeManager.corrected
        horizontalAlignment: Qt.AlignHCenter
        x: 0
        y: progressCircle.y + progressCircle.height + 8
        width: parent.width
        font.pixelSize: App.settings.font16
        wrapMode: Label.WordWrap
    }

    Label {
        id: labelStopMessage
        visible: false
        text: form.provider.surveyMode ? qsTr("Completing survey") : qsTr("Completing patrol")
        horizontalAlignment: Qt.AlignHCenter
        x: 0
        y: progressCircle.y + progressCircle.height + 8
        width: parent.width
        font.pixelSize: App.settings.font16
        wrapMode: Label.WordWrap
    }

    RoundButton {
        id: buttonSkip
        radius: 0
        visible: skipVisible && !closeTimer.running && title.enabled && !labelWaitForTimeCorrection.visible
        enabled: !form.requireGPSTime || App.timeManager.corrected
        font.pixelSize: App.settings.font16
        font.capitalization: Font.MixedCase
        width: parent.width / 2
        x: (parent.width - width) / 2
        y: progressCircle.y + progressCircle.height + 8
        text: qsTr("Skip GPS")
        Material.background: Material.accentColor
        onClicked: {
            if (form.provider.allowManualGPS) {
                form.replaceLastPage("qrc:/imports/CyberTracker.1/FieldLocationPage.qml", {
                    replacePageOnConfirmUrl: "qrc:/SMART/LocationPage.qml",
                    replacePageOnConfirmParams: { tag: page.tag },
                    recordUid: form.rootRecordUid,
                    fieldUid: Globals.modelLocationFieldUid
                } )
            } else {
                positionSource.active = false
                page.instantGPS = true
                closeTimer.interval = 100
                closeTimer.start()
            }
        }

        Timer {
            id: skipTimer
            repeat: false
            onTriggered: skipVisible = true
        }
    }

    ProgressBar {
        id: progressBar
        anchors.horizontalCenter: parent.horizontalCenter
        y: labelStopMessage.y + labelStopMessage.height + 32
        width: progressCircle.width
        visible: false

        Connections {
            target: App

            function onProgressEvent(projectUid, operationName, index, count) {
                progressBar.value = index / count
                progressBar.visible = true
            }
        }
    }

    PositionSource {
        id: positionSource
        active: false
        name: App.positionInfoSourceName
        updateInterval: 1000

        property int fixCounter: 0

        onPositionChanged: {
            if (!active) {
                return
            }

            if (form.requireGPSTime && !App.timeManager.corrected) {
                console.log("Waiting for time correction")
                return
            }

            if (!App.lastLocationAccurate) {
                showToast(App.locationAccuracyText)
                return
            }

            page.lastFix = App.lastLocation.toMap

            if (fixCounter < page.positionMax) {
                fixCounter++
                progressCircle.progressPercent = (fixCounter / page.positionMax) * 100
                return
            }

            active = false
            page.success(page.lastFix)
            closeTimer.interval = page.positionMax < 3 ? 2000 : 500
            closeTimer.start()
        }
    }

    Timer {
        id: closeTimer
        repeat: false
        onTriggered: {
            if (instantGPS) {
                page.success(page.lastFix)
            }

            page.close()
        }
    }
}
