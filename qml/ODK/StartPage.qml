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
        text: form.project.title
        menuIcon: "qrc:/icons/settings.svg"
        menuVisible: true
        onMenuClicked: {
            form.pushPage("qrc:/imports/CyberTracker.1/FormSettingsPage.qml")
        }
    }

    footer: RowLayout {
        spacing: 0
        width: page.width

        C.FooterButton {
            text: qsTr("Submit")
            enabled: listView.canSubmit
            icon.source: "qrc:/icons/upload_multiple.svg"
            onClicked: listView.submit()
        }

        C.FooterButton {
            text: qsTr("Map")
            icon.source: "qrc:/icons/map_outline.svg"
            onClicked: form.pushPage("qrc:/MapsPage.qml")
        }

        C.FooterButton {
            text: qsTr("New")
            icon.source: "qrc:/icons/plus.svg"
            enabled: form.provider.parserError === ""
            onClicked: {
                form.newSighting()
                form.snapCreateTime()
                form.saveSighting()
                if (form.project.wizardMode) {
                    form.pushWizardPage(form.rootRecordUid, {})
                } else {
                    form.pushPage("qrc:/ODK/FormPage.qml")
                }
            }
        }
    }

    // Watermark.
    Control {
        anchors.fill: parent
        padding: parent.width / 4
        opacity: 0.15
        contentItem: Image {
            source: form.project.icon !== "" ? App.projectManager.getFileUrl(form.project.uid, form.project.icon) : ""
            fillMode: Image.PreserveAspectFit
        }
    }

    C.FormSightingsListView {
        id: listView

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: locationTrackingState.visible ? locationTrackingState.top : parent.bottom
        }

        showNoData: false

        onClicked: function (sighting, index) {
            editSighting(sighting)
        }
    }

    ColumnLayout {
        id: locationTrackingState
        spacing: 0
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        visible: form.trackStreamer.running || form.pointStreamer.running

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
            opacity: 0.125
        }

        Button {
            Layout.fillWidth: true
            enabled: false
            flat: true
            display: Button.TextBesideIcon
            font.capitalization: Font.MixedCase
            font.pixelSize: App.settings.font14
            icon.source: form.trackStreamer.running ? form.trackStreamer.rateIcon : form.pointStreamer.rateIcon
            text: form.trackStreamer.running ? form.trackStreamer.rateFullText : form.pointStreamer.rateFullText
        }
    }

    Component.onCompleted: {
        if (form.provider.parserError !== "") {
            showError(qsTr("Form error") + ": " + form.provider.parserError)
        }
    }

    function editSighting(sighting) {
        form.loadSighting(sighting.rootRecordUid)
        form.setPageStack([{ pageUrl: form.startPage}])

        form.wizard.reset()
        if (form.project.wizardMode) {
            form.pushWizardIndexPage(form.rootRecordUid, {})
        } else {
            form.pushPage("qrc:/ODK/FormPage.qml")
        }
    }
}
