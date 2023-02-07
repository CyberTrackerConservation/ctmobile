import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var sightingList: []
    property int currentIndex: 0
    property var forms: ({})

    header: C.PageHeader {
        text: qsTr("Sighting")
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

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("First")
            icon.source: "qrc:/icons/page_first.svg"
            enabled: page.currentIndex > 0
            visible: !deleteButton.visible && !editButton.visible
            onClicked: page.rebuild(-page.currentIndex)
        }

        C.FooterButton {
            text: qsTr("Previous")
            icon.source: "qrc:/icons/chevron_left.svg"
            enabled: page.currentIndex > 0
            onClicked: page.rebuild(-1)
        }

        C.FooterButton {
            text: qsTr("Next")
            icon.source: "qrc:/icons/chevron_right.svg"
            enabled: page.currentIndex < page.sightingList.length - 1
            onClicked: page.rebuild(+1)
        }

        C.FooterButton {
            text: qsTr("Last")
            icon.source: "qrc:/icons/page_last.svg"
            enabled: page.currentIndex < page.sightingList.length - 1
            visible: !deleteButton.visible && !editButton.visible
            onClicked: page.rebuild(page.sightingList.length - page.currentIndex - 1)
        }

        C.FooterButton {
            id: deleteButton
            text: qsTr("Delete")
            icon.source: "qrc:/icons/delete_outline.svg"
            visible: form.supportSightingDelete
            onClicked: {
                confirmDelete.open()
            }
        }

        C.FooterButton {
            id: editButton
            text: qsTr("Edit")
            icon.source: "qrc:/icons/edit.svg"
            visible: form.supportSightingEdit
            onClicked: {
                form.saveState()
                form.pushFormPage({ projectUid: form.project.uid, stateSpace: listView.sightingForm.stateSpace, editSightingUid: listView.sightingUid })
            }
        }
    }

    C.PopupLoader {
        id: confirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete sighting?")
                onConfirmed: {
                    form.removeSighting(listView.sightingUid)
                    form.popPage()
                }
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
        deleteButton.enabled = editButton.enabled = listView.sightingForm.canEditSighting(listView.sightingUid)
    }
}
