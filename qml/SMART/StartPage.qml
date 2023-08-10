import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: form.project.title
        menuVisible: Qt.platform.os !== "ios"
        menuIcon: App.batteryIcon
        onMenuClicked: App.showToast(App.batteryText)
    }

    Grid {
        anchors.fill: parent
        columns: 2
        rows: 3

        StartPageButton {
            width: parent.width / parent.columns
            height: parent.height / parent.rows
            title: qsTr("Map")
            image: "qrc:/icons/map_outline.svg"
            onClicked: form.pushPage("qrc:/MapsPage.qml")
        }

        StartPageButton {
            width: parent.width / parent.columns
            height: parent.height / parent.rows
            title: qsTr("Settings")
            image: "qrc:/icons/settings.svg"
            onClicked: form.pushPage("qrc:/SMART/SettingsPage.qml")
        }

        StartPageButton {
            id: exportDataButton
            width: parent.width / parent.columns
            height: parent.height / parent.rows
            title: qsTr("Export data")
            image: "qrc:/icons/export.svg"
            onClicked: {
                busyCover.doWork = form.provider.hasDataServer ? completeUpload : completeExport
                busyCover.start()
            }

            enabled: {
                Globals.patrolChangeCount
                return (form.provider.hasUploadData() || form.hasData()) && !form.provider.patrolStarted
            }
        }

        StartPageButton {
            width: parent.width / parent.columns
            height: parent.height / parent.rows
            title: qsTr("History")
            image: "qrc:/icons/history.svg"
            onClicked: form.pushPage("qrc:/SMART/HistoryPage.qml")
        }

        StartPageButton {
            width: parent.width / parent.columns
            height: parent.height / parent.rows
            title: qsTr("Report Incident")
            image: "qrc:/icons/assignment_turned_in_fill.svg"
            onClicked: form.pushFormPage({ projectUid: form.project.uid, stateSpace: Globals.formSpaceIncident })
            enabled: form.provider.hasIncident
        }

        StartPageButton {
            width: parent.width / parent.columns
            height: parent.height / parent.rows
            title: {
                Globals.patrolChangeCount
                return form.provider.getSightingTypeText(form.provider.patrolStarted ? "ResumePatrol" : "NewPatrol")
            }
            image: form.provider.patrolStarted ? "qrc:/icons/step_forward.svg" : "qrc:/icons/play.svg"
            onClicked: form.pushFormPage({ projectUid: form.project.uid, stateSpace: Globals.formSpacePatrol })
            enabled: form.provider.hasPatrol
        }
    }

    C.MessagePopup {
        id: popupExportComplete
    }

    function completeUpload() {
        let success = form.provider.manualUpload ? form.provider.uploadData() : App.taskManager.waitForTasks(form.project.uid)
        let successMessage = qsTr("All data has been uploaded.")
        let failureMessage = qsTr("Connection failed. Please try again later.")

        reportExportResult(success, "qrc:/icons/cloud_done.svg", successMessage, "qrc:/icons/cloud_off_outline.svg", failureMessage)
    }

    function completeExport() {
        let success = form.provider.exportData()
        let successMessage = qsTr("Data is ready for import from the desktop.")
        let failureMessage = qsTr("Export may have failed.")

        reportExportResult(success, "qrc:/icons/check_circle.svg", successMessage, "qrc:/icons/close_circle_outline.svg", failureMessage)
    }

    function reportExportResult(success, successIcon, successMessage, failureIcon, failureMessage) {
        if (success) {
            form.provider.clearCompletedData()
        }

        popupExportComplete.icon = success ? successIcon : failureIcon
        popupExportComplete.title = success ? qsTr("Success") : qsTr("Error")
        popupExportComplete.message1 = success ? successMessage : failureMessage
        popupExportComplete.open()

        Globals.patrolChangeCount++
    }
}
