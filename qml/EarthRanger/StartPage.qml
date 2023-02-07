import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: "EarthRanger"
        menuIcon: "qrc:/icons/settings.svg"
        menuVisible: true
        onMenuClicked: {
            form.pushPage("qrc:/EarthRanger/SettingsPage.qml")
        }
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Sync")
            enabled: listView.model.count !== 0
            icon.source: "qrc:/icons/autorenew.svg"
            onClicked: App.taskManager.resumePausedTasks(form.project.uid)
        }

        C.FooterButton {
            enabled: listView.model.count !== 0
            text: qsTr("Clear")
            icon.source: "qrc:/icons/delete_outline.svg"
            onClicked: confirmClear.open()
        }

        C.FooterButton {
            text: qsTr("Map")
            icon.source: "qrc:/icons/map_outline.svg"
            onClicked: form.pushPage("qrc:/MapsPage.qml")
        }

        C.FooterButton {
            text: qsTr("New")
            highlight: true
            icon.source: "qrc:/icons/plus.svg"
            onClicked: {
                form.newSighting()
                form.pushPage("qrc:/EarthRanger/EventTypePage.qml")
            }
        }
    }

    // Watermark.
    Control {
        anchors.fill: parent
        padding: parent.width / 4
        opacity: 0.15
        contentItem: Image {
            source: "qrc:/EarthRanger/logo.svg"
            fillMode: Image.PreserveAspectFit
        }
    }

    C.FormSightingsListView {
        id: listView
        anchors.fill: parent
        showNoData: false

        onClicked: function (sighting, index) {
            if (sighting.readonly) {
                showToast(qsTr("Report cannot be changed"))
                return
            }

            listView.currentIndex = index
            form.loadSighting(sighting.rootRecordUid)
            form.setPageStack([form.getPageStack()[0]])
            let reportUid = form.getFieldValue("reportUid")

            if (form.project.wizardMode) {
                form.pushWizardIndexPage(sighting.rootRecordUid, { fieldUids: form.getElement(reportUid).fieldUids, header: form.getElementName(reportUid), banner: form.provider.banner() })
            } else {
                form.pushPage("qrc:/EarthRanger/AttributesPage.qml", { elementUid: reportUid, banner: form.provider.banner() })
            }
        }
    }

    Component.onCompleted: {
        if (!form.project.loggedIn) {
            form.triggerLogin()
        }
    }

    function getReportElementUid(sighting) {
        return sighting.records[0].fieldValues["reportUid"]
    }

    C.ConfirmPopup {
        id: confirmClear
        text: qsTr("Clear uploaded data?")
        confirmText: qsTr("Yes, clear it")
        onConfirmed: {
            App.taskManager.waitForTasks(form.project.uid)
            form.removeUploadedSightings()
        }
    }
}
