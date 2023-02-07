import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var inspectList: []
    property int currentIndex: 0
    property var currentItem: ({})

    property var forms: ({})

    Component.onCompleted: {
        page.rebuild(0)
    }

    Connections {
        target: listViewSighting.sightingForm

        function onSightingSaved(sightingUid) {
            rebuild(0)
        }
    }

    C.FormSightingListView {
        id: listViewSighting
        anchors.fill: parent
        visible: page.currentItem.overlayId === "sighting"
    }

    ColumnLayout {
        id: listViewGoto
        width: page.width
        spacing: 0
        clip: true

        ItemDelegate {
            Layout.fillWidth: true
            contentItem: Label {
                text: page.currentItem.source
                font.pixelSize: App.settings.font18
                horizontalAlignment: Label.AlignHCenter
            }
        }

        ItemDelegate {
            Layout.fillWidth: true
            contentItem: Label {
                text: {
                    var location = page.currentItem.path[page.currentItem.pathIndex]
                    return location === undefined ? "" : App.getLocationText(location[0], location[1])
                }
                font.pixelSize: App.settings.font16
                horizontalAlignment: Label.AlignHCenter
            }
        }

        ItemDelegate {
            Layout.fillWidth: true
            contentItem: Label {
                text: page.currentItem.name
                font.pixelSize: App.settings.font16
                horizontalAlignment: Label.AlignHCenter
            }
        }

        visible: page.currentItem.overlayId !== "sighting"
    }

    header: C.PageHeader {
        text: qsTr("Identify") + " - " + currentItem.name
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
            enabled: page.currentIndex > 0
            onClicked: rebuild(-1)
        }

        C.FooterButton {
            text: qsTr("Next")
            icon.source: "qrc:/icons/chevron_right.svg"
            enabled: page.currentIndex < page.inspectList.length - 1
            onClicked: rebuild(+1)
        }

        C.FooterButton {
            text: qsTr("Set goto")
            icon.source: "qrc:/icons/arrow_goto.svg"
            onClicked: {
                App.gotoManager.setTarget(page.currentItem.name, page.currentItem.path, page.currentItem.pathIndex)
                if (typeof(formPageStack) !== "undefined") {
                    form.popPage()
                } else {
                    appPageStack.pop()
                }
            }
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
            icon.source: "qrc:/icons/pencil_outline.svg"
            visible: form.supportSightingEdit
            onClicked: {
                form.saveState()
                form.pushFormPage({ projectUid: form.project.uid, stateSpace: listViewSighting.sightingForm.stateSpace, editSightingUid: listViewSighting.sightingUid })
            }
        }
    }

    C.PopupLoader {
        id: confirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete sighting?")
                onConfirmed: {
                    form.removeSighting(listViewSighting.sightingUid)
                    form.popPage()
                }
            }
        }
    }

    function rebuild(indexDelta) {
        currentIndex += indexDelta
        if (currentIndex < 0 || currentIndex >= inspectList.length) {
            console.log("Error bad index")
            return
        }

        currentItem = inspectList[currentIndex]

        switch (currentItem.overlayId) {
        case "sighting":
            if (forms[currentItem.stateSpace] === undefined) {
                forms[currentItem.stateSpace] = App.createForm(form.project.uid, currentItem.stateSpace, true, page)
            }

            listViewSighting.sightingForm = forms[currentItem.stateSpace]
            listViewSighting.sightingUid = ""   // Ensure change event
            listViewSighting.sightingUid = currentItem.sightingUid

            deleteButton.enabled = editButton.enabled = listViewSighting.sightingForm.canEditSighting(currentItem.sightingUid)
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
