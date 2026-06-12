import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

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
            title: form.provider.hasDataServer ? qsTr("Upload data") : qsTr("Export data")
            image: "qrc:/icons/export.svg"
            onClicked: {
                if (form.provider.hasDataServer) {
                    popupConfirmUpload.open()
                } else {
                    popupConfirmExport.open()
                }
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
            enabled: form.provider.allowHistory
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
            onClicked: {
                if (App.settings.patrolProjectUid !== "" && App.settings.patrolProjectUid !== form.project.uid) {                    
                    let patrolProject = App.projectManager.load(App.settings.patrolProjectUid)
                    if (patrolProject) {
                        showError(qsTr("%1 already started - %2").arg(form.provider.getPatrolText()).arg(patrolProject.title))
                        return
                    }
                }

                form.pushFormPage({ projectUid: form.project.uid, stateSpace: Globals.formSpacePatrol })
            }
            enabled: form.provider.hasPatrol
        }
    }

    C.PopupLoader {
        id: popupConfirmExport

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Export data?")
                subText: qsTr("Exported data can no longer be changed.")
                confirmText: qsTr("Yes, export it")
                confirmDelay: false
                onConfirmed: {
                    busyCover.doWork = completeExport
                    busyCover.start()
                }
            }
        }
    }

    C.PopupLoader {
        id: popupConfirmUpload

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Upload data?")
                subText: form.provider.manualUpload ?
                    qsTr("Uploaded data can no longer be changed.") :
                    qsTr("Ensure all data is uploaded")
                confirmText: qsTr("Yes, upload it")
                confirmDelay: false
                onConfirmed: {
                    busyCover.doWork = completeUpload
                    busyCover.start()
                }
            }
        }
    }

    C.MessagePopup {
        id: popupExportComplete
    }

    function completeUpload() {
        let success = false
        if (form.provider.manualUpload) {
            success = form.provider.uploadData()
        } else {
            success = App.uploadQueue.waitForQueue(form.project.uid)
        }

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
