import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    property var inspectList: []

    QtObject {
        id: internal
        property var currentItem: ({})
        property int currentIndex: 0
    }

    Component.onCompleted: {
        page.rebuild(0)
    }

    C.SightingListView {
        id: listViewSighting
        anchors.fill: parent
        visible: internal.currentItem.overlayId === "sighting"
    }

    ColumnLayout {
        id: listViewGoto
        width: page.width
        spacing: 0
        clip: true

        ItemDelegate {
            Layout.fillWidth: true
            contentItem: Label {
                text: internal.currentItem.source
                font.pixelSize: App.settings.font18
                horizontalAlignment: Label.AlignHCenter
            }
        }

        ItemDelegate {
            Layout.fillWidth: true
            contentItem: Label {
                text: {
                    var location = internal.currentItem.path[internal.currentItem.pathIndex]
                    return location === undefined ? "" : App.getLocationText(location[0], location[1])
                }
                font.pixelSize: App.settings.font16
                horizontalAlignment: Label.AlignHCenter
            }
        }

        ItemDelegate {
            Layout.fillWidth: true
            contentItem: Label {
                text: internal.currentItem.name
                font.pixelSize: App.settings.font16
                horizontalAlignment: Label.AlignHCenter
            }
            visible: internal.currentItem.name !== undefined && internal.currentItem.name !== ""
        }

        Repeater {
            model: {
                var props = internal.currentItem.properties
                if (props === undefined || props === "") return []
                try { return JSON.parse(props) } catch(e) { return [] }
            }

            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: modelData.name
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: modelData.value
                        font.pixelSize: App.settings.font14
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                    }
                }
            }
        }

        visible: internal.currentItem.overlayId !== "sighting"
    }

    header: C.PageHeader {
        text: qsTr("Identify") + " - " + (internal.currentItem.name || internal.currentItem.source || "")
    }

    Label {
        anchors.fill: parent
        text: qsTr("No selection")
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        visible: page.inspectList.length === 0
        opacity: 0.5
        font.pixelSize: App.settings.font20
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Previous")
            icon.source: "qrc:/icons/chevron_left.svg"
            enabled: internal.currentIndex > 0
            onClicked: rebuild(-1)
        }

        C.FooterButton {
            text: qsTr("Next")
            icon.source: "qrc:/icons/chevron_right.svg"
            enabled: internal.currentIndex < page.inspectList.length - 1
            onClicked: rebuild(+1)
        }

        C.FooterButton {
            text: qsTr("Set goto")
            icon.source: "qrc:/icons/arrow_goto.svg"
            onClicked: {
                App.gotoManager.setTarget(internal.currentItem.name, internal.currentItem.path, internal.currentItem.pathIndex)
                popPageImmediate(page)
            }
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
            icon.source: "qrc:/icons/pencil_outline.svg"
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
                    App.deleteSighting(internal.currentItem.projectUid, internal.currentItem.sightingUid, true)
                    popPageImmediate(page)
                }
            }
        }
    }

    function rebuild(indexDelta) {
        internal.currentIndex += indexDelta
        if (internal.currentIndex < 0 || internal.currentIndex >= inspectList.length) {
            console.log("Error bad index")
            return
        }

        internal.currentItem = inspectList[internal.currentIndex]

        switch (internal.currentItem.overlayId) {
        case "sighting":
            listViewSighting.sightingUid = internal.currentItem.sightingUid
            editButton.enabled = internal.currentItem.canEdit
            deleteButton.enabled = internal.currentItem.canDelete
            break

        case "gotoPoints":
            deleteButton.enabled = editButton.enabled = false
            break

        case "gotoLines":
            deleteButton.enabled = editButton.enabled = false
            break
        }
    }
}
