import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page
    property string recordUid
    property string listElementUid: Globals.modelTreeElementUid

    header: C.PageHeader {
        text: form.getElementName(listElementUid)
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent

        currentIndex: form.getControlState(recordUid, "CM", listElementUid, -1)

        model: C.ElementListModel {
            elementUid: listElementUid
        }

        delegate: ItemDelegate {
            width: ListView.view.width
            highlighted: ListView.isCurrentItem

            contentItem: RowLayout {

                Item { height: C.Style.minRowHeight }

                Image {
                    fillMode: Image.PreserveAspectFit
                    source: form.getElementIcon(modelData.uid)
                    sourceSize.width: C.Style.minRowHeight
                    sourceSize.height: C.Style.minRowHeight
                    verticalAlignment: Image.AlignVCenter
                }

                Label {
                    id: controlLabel
                    Layout.fillWidth: true
                    wrapMode: Label.WordWrap
                    text: form.getElementName(modelData.uid)
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: App.settings.font16
                }

                C.ChevronRight {
                    visible: form.getElement(modelData.uid).hasChildren
                }
            }

            C.HorizontalDivider {}

            onClicked: {
                listView.currentIndex = model.index
                form.setControlState(recordUid, "CM", listElementUid, model.index)

                if (form.getElement(modelData.uid).hasChildren) {
                    form.pushPage("qrc:/SMART/ModelPage.qml", { recordUid: recordUid, listElementUid: modelData.uid } )
                } else {
                    form.pushPage("qrc:/SMART/AttributesPage.qml", { title: form.getElementName(modelData.uid), recordUid: recordUid, elementUid: modelData.uid } )
                }
            }
        }
    }
}
