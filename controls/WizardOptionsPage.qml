import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

ContentPage {
    id: page

    property string recordUid: ""
    property string fieldUid: ""
    property var filterFieldUids: ([])

    header: PageHeader {
        text: {
            switch (stackLayout.currentIndex) {
            case 0:
                return qsTr("Current sighting")

            case 1:
                return qsTr("Saved sightings")

            case 2:
                return qsTr("Submit")
            }

            return "??"
        }

        menuIcon: "qrc:/icons/settings.svg"
        menuVisible: true
        onMenuClicked: {
            form.pushPage("qrc:/imports/CyberTracker/FormSettingsPage.qml")
        }
    }

    footer: RowLayout {
        spacing: 0
        width: page.width

        ButtonGroup { id: buttonGroup }

        // Index.
        FooterButton {
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: stackLayout.currentIndex === 0
            icon.source: checked ? "qrc:/icons/file.svg" : "qrc:/icons/file_outline.svg"
            onClicked: stackLayout.currentIndex = 0
        }

        // All sightings.
        FooterButton {
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: stackLayout.currentIndex === 1
            icon.source: checked ? "qrc:/icons/file_multiple.svg" : "qrc:/icons/file_multiple_outline.svg"
            onClicked: stackLayout.currentIndex = 1
        }

        // Submit
        C.FooterButton {
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: stackLayout.currentIndex === 2
            icon.source: "qrc:/icons/upload_multiple.svg"
            onClicked: stackLayout.currentIndex = 2
        }
    }

    StackLayout {
        id: stackLayout

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: locationTrackingState.visible ? locationTrackingState.top : parent.bottom
        }

        clip: true

        WizardIndex {
            id: wizardIndexView
            Layout.fillWidth: true
            Layout.fillHeight: true

            recordUid: page.recordUid
            fieldUid: page.fieldUid
            filterFieldUids: page.filterFieldUids

            highlightFollowsCurrentItem: false
        }

        SightingsListView {
            id: sightingsListView

            Layout.fillWidth: true
            Layout.fillHeight: true
            active: true
            filter.projectUid: form.project.uid
            filter.stateSpace: form.stateSpace

            onClicked: function (sighting, index) {
                form.pushFormPage({ projectUid: form.project.uid, stateSpace: form.stateSpace, editSightingUid: sighting.sightingUid })
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            SubmitButton {
                anchors.centerIn: parent
            }
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

        visible: stackLayout.currentIndex === 0 && (form.trackStreamer.running || form.pointStreamer.running)

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
            icon.source: App.locationLogger.stateIcon
            text: form.pointStreamer.running ? form.pointStreamer.rateFullText : form.trackStreamer.rateFullText
        }
    }

}
