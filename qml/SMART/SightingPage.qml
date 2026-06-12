import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    property var sightingList: []
    property int currentIndex: 0

    QtObject {
        id: internal
        property var currentItem: ({})
    }

    header: C.PageHeader {
        text: qsTr("Sighting")
    }

    C.SightingListView {
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
            visible: form && form.connected
            onClicked: {
                confirmDelete.open()
            }
        }

        C.FooterButton {
            id: editButton
            text: qsTr("Edit")
            icon.source: "qrc:/icons/edit.svg"
            visible: form && form.connected
            onClicked: {
                form.editSighting(internal.currentItem.stateSpace, internal.currentItem.sightingUid)
            }
        }
    }

    C.PopupLoader {
        id: confirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete sighting?")
                onConfirmed: {
                    form.deleteSighting(listView.sightingUid)
                    showToast(qsTr("Sighting deleted"))
                    popPageImmediate(page)
                }
            }
        }
    }

    function rebuild(indexDelta) {
        currentIndex += indexDelta
        if (currentIndex >= sightingList.length) {
            return
        }

        internal.currentItem = JSON.parse(JSON.stringify(sightingList[currentIndex]))
        listView.sightingUid = internal.currentItem.sightingUid
        editButton.enabled = internal.currentItem.canEdit
        deleteButton.enabled = internal.currentItem.canDelete
    }
}
