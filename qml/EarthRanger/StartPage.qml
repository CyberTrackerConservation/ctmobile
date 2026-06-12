import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

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
            enabled: listView.count !== 0
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

    C.SightingsListView {
        id: listView
        anchors.fill: parent
        showNoData: false
        active: true
        filter.projectUid: form.project.uid
        filter.stateSpace: form.stateSpace

        onClicked: function (sighting, index) {
            if (sighting.readonly) {
                showToast(qsTr("Report cannot be changed"))
                return
            }

            listView.currentIndex = index
            let pageStack = [form.getPageStack()[0]]
            form.loadSighting(sighting.sightingUid)
            form.setPageStack(pageStack)
            let reportUid = form.getFieldValue("reportUid")

            if (form.project.wizardMode) {
                form.pushWizardIndexPage(sighting.sightingUid, { fieldUids: form.getElement(reportUid).fieldUids, header: form.getElementName(reportUid), banner: form.provider.banner() })
            } else {
                form.pushPage("qrc:/EarthRanger/AttributesPage.qml")
            }
        }
    }

    Component.onCompleted: {
        if (!form.project.loggedIn) {
            form.triggerLogin()
        }
    }

    C.ConfirmPopup {
        id: confirmClear
        text: qsTr("Clear uploaded data?")
        confirmText: qsTr("Yes, clear it")
        onConfirmed: {
            App.uploadQueue.waitForQueue(form.project.uid)
            form.deleteUploadedSightings()
            form.newSessionId()
        }
    }
}
