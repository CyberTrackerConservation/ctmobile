import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var sightingList: []
    property var currentIndex: 0

    property var forms: ({})

    header: C.PageHeader {
        id: title
        text: qsTr("Sighting")
        menuIcon: "qrc:/icons/edit.svg"
        onMenuClicked: {
            form.saveState()
            form.pushFormPage({ projectUid: form.project.uid, stateSpace: listView.sightingForm.stateSpace, editSightingUid: listView.sightingUid })
        }
    }

    Connections {
        target: listView.sightingForm

        function onSightingSaved(sightingUid) {
            rebuild(0)
        }
    }

    C.FormSightingListView {
        id: listView
        anchors.fill: parent
        visible: page.sightingList.length > 0
        Component.onCompleted: page.rebuild(0)
    }

    Label {
        anchors.fill: parent
        text: qsTr("No sighting selected")
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        visible: page.sightingList.length === 0
        opacity: 0.5
        font.pixelSize: App.settings.font20
    }

    footer: ColumnLayout {
        spacing: 0
        width: parent.width

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
            opacity: Material.theme === Material.Dark ? 0.12 : 0.12
        }

        RowLayout {
            id: buttonRow
            spacing: 0
            Layout.fillWidth: true
            property int buttonCount: 4
            property int buttonWidth: page.width / buttonCount
            property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("First")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/page_first.svg"
                Material.foreground: buttonRow.buttonColor
                enabled: page.currentIndex > 0
                onClicked: page.rebuild(-page.currentIndex)
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Previous")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/chevron_left.svg"
                Material.foreground: buttonRow.buttonColor
                enabled: page.currentIndex > 0
                onClicked: page.rebuild(-1)
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Next")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/chevron_right.svg"
                Material.foreground: buttonRow.buttonColor
                enabled: page.currentIndex < page.sightingList.length - 1
                onClicked: page.rebuild(+1)
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Last")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/page_last.svg"
                Material.foreground: buttonRow.buttonColor
                enabled: page.currentIndex < page.sightingList.length - 1
                onClicked: page.rebuild(page.sightingList.length - page.currentIndex - 1)
            }
        }
    }

    function rebuild(indexDelta) {
        currentIndex += indexDelta
        if (currentIndex >= sightingList.length) {
            return
        }

        let sighting = sightingList[currentIndex]
        if (forms[sighting.stateSpace] === undefined) {
            forms[sighting.stateSpace] = App.createForm(form.project.uid, sighting.stateSpace, true, page)
        }

        listView.sightingForm = forms[sighting.stateSpace]
        listView.sightingUid = ""   // Ensure change event
        listView.sightingUid = sighting.sightingUid
        title.menuVisible = listView.sightingForm.canEditSighting(listView.sightingUid)
    }
}
