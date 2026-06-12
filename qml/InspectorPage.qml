import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    property var inspectList
    property int initialIndex: 0

    QtObject {
        id: internal
        property int currentIndex: page.initialIndex
        property string note: currentItem().note
    }

    Component.onCompleted: {
        page.rebuild(0)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: page.inspectList.count > 0

        C.SightingListView {
            id: listViewSighting
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        C.HorizontalDivider {
            visible: internal.note !== ""
        }

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            Layout.maximumHeight: C.Style.minRowHeight * 2
            clip: true
            contentItem: RowLayout {
                C.SquareIcon {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    source: "qrc:/icons/note_text_outline.svg"
                    size: C.Style.iconSize24
                    opacity: 0.5
                }

                Label {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    verticalAlignment: Qt.AlignVCenter
                    text: internal.note
                    font.pixelSize: App.settings.font14
                    font.bold: true
                    wrapMode: Label.WordWrap
                    elide: Label.ElideRight
                }
            }

            background: Rectangle {
                opacity: 0.5
                color: Material.accent
            }

            visible: internal.note !== ""

            onClicked: {
                noteButton.clicked()
            }
        }
    }

    header: C.PageHeader {
        text: qsTr("Sighting")
    }

    Label {
        anchors.fill: parent
        text: qsTr("No selection")
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        visible: page.inspectList.count === 0
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
            enabled: internal.currentIndex < page.inspectList.count - 1
            onClicked: rebuild(+1)
        }

        C.FooterButton {
            text: qsTr("Set goto")
            icon.source: "qrc:/icons/arrow_goto.svg"
            enabled: currentItem().location !== undefined
            onClicked: {
                App.gotoManager.setTarget(qsTr("Sighting"), [currentItem().location], 0)
                showToast(qsTr("Goto target updated"))
                popPageImmediate(page)
            }
        }

        C.FooterButton {
            id: deleteButton
            text: qsTr("Delete")
            icon.source: "qrc:/icons/delete_outline.svg"
            onClicked: {
                confirmDelete.open()
            }
        }

        C.FooterButton {
            id: noteButton
            text: qsTr("Note")
            icon.source: "qrc:/icons/note_edit_outline.svg"
            onClicked: {
                let item = currentItem()
                pushPageImmediate(page, "qrc:/SightingNotePage.qml", { projectUid: item.projectUid, sightingUid: item.sightingUid, note: item.note })
            }
        }
    }

    C.PopupLoader {
        id: confirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete sighting?")
                onConfirmed: {
                    let item = currentItem()
                    App.deleteSighting(item.projectUid, item.sightingUid, true)
                    showToast(qsTr("Sighting deleted"))
                    popPageImmediate(page)
                }
            }
        }
    }

    Connections {
        target: inspectList

        function onDataChanged() {
            internal.note = currentItem().note
        }

        function onRowsRemoved() {
            if (inspectList.count === 0) {
                popPageImmediate(page)
                return
            }

            if (internal.currentIndex >= inspectList.count) {
                internal.currentIndex = inspectList.count - 1
            }

            rebuild(0)
        }
    }

    function currentItem() {
        return internal.currentIndex >= 0 && internal.currentIndex < inspectList.count ? inspectList.get(internal.currentIndex) : ({})
    }

    function currentNote() {
        return currentItem().note
    }

    function rebuild(indexDelta) {
        internal.currentIndex += indexDelta
        if (internal.currentIndex < 0 || internal.currentIndex >= inspectList.count) {
            console.log("Error bad index")
            return
        }

        listViewSighting.sightingUid = currentItem().sightingUid
        internal.note = currentItem().note
    }
}
