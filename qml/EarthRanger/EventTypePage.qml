import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property string listElementUid: "categories"
    property bool flattenCategories: false

    header: C.PageHeader {}

    C.ElementListModel {
        id: elementListModel
    }

    C.ElementTreeModel {
        id: elementTreeModel
    }

    Component.onCompleted: {
        // Get the flatten setting.
        let flatten = form.getSetting("mergeCategories", false)
        page.flattenCategories = flatten

        // Use the filter only if specified - if the user has not gone to settings page and configured this, it will be empty.
        let filter = form.getSetting("visibleReports", {})
        let useFilter = Object.keys(filter).length > 0

        if (flatten) {
            page.header.text = qsTr("Report")
            elementListModel.init(listElementUid, flatten, false, useFilter, filter)
            gridView.currentIndex = form.getControlState("EventTypeStage", "", listElementUid, -1)

        } else {
            // Turn on a category when a child is active.
            page.header.text = form.getElementName(listElementUid)
            elementTreeModel.elementUid = listElementUid

            let lastParent = elementTreeModel.get(0)
            for (let i = 1; i < elementTreeModel.count; i++) {
                let item = elementTreeModel.get(i)
                if (item.depth === 0) {
                    lastParent = item
                    continue
                }

                if (filter[item.elementUid] === true) {
                    filter[lastParent.elementUid] = true
                }
            }

            elementListModel.init(listElementUid, flatten, false, useFilter, filter)
            listView.currentIndex = form.getControlState("EventTypeStage", "", listElementUid, -1)
        }
    }

    C.ListViewV {
        id: listView

        visible: !flattenCategories
        anchors.fill: parent

        model: elementListModel
        delegate: ItemDelegate {
            id: control
            width: ListView.view.width
            height: controlLabel.implicitHeight * 3
            highlighted: ListView.isCurrentItem
            contentItem: RowLayout {
                Image {
                    fillMode: Image.PreserveAspectFit
                    source: form.getElementIcon(modelData.uid)
                    sourceSize.width: controlLabel.implicitHeight * 1.4
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

            onClicked: {
                listView.currentIndex = model.index
                form.setControlState("EventTypeStage", "", listElementUid, model.index)

                if (form.getElement(modelData.uid).hasChildren) {
                    form.pushPage("qrc:/EarthRanger/EventTypePage.qml", { listElementUid: modelData.uid } )
                } else {
                    form.pushPage("qrc:/EarthRanger/AttributesPage.qml", { elementUid: modelData.uid } )
                }
            }

            C.HorizontalDivider {}
        }
    }

    Label {
        id: l
        text: "Wg"
        visible: false
    }

    GridView {
        id: gridView

        visible: flattenCategories
        anchors.fill: parent
        cellWidth: parent.width / 3
        cellHeight: cellWidth / 2 + l.implicitHeight * 3

        model: elementListModel
        delegate: ItemDelegate {
            id: d
            width: gridView.cellWidth
            height: gridView.cellHeight
            highlighted: GridView.isCurrentItem
            padding: 4
            contentItem: Column {
                clip: true
                Image {
                    fillMode: Image.PreserveAspectFit
                    source: form.getElementIcon(modelData.uid)
                    sourceSize.width: gridView.cellWidth
                    sourceSize.height: gridView.cellWidth / 2
                    horizontalAlignment: Image.AlignHCenter
                    verticalAlignment: Image.AlignVCenter
                    width: parent.width
                    height: parent.width / 2
                }

                Label {
                    horizontalAlignment: Label.AlignHCenter
                    verticalAlignment: Label.AlignVCenter
                    wrapMode: Label.WordWrap
                    fontSizeMode: Text.Fit
                    width: parent.width
                    height: parent.height - (parent.width / 2)
                    text: form.getElementName(modelData.uid)
                    font.pixelSize: App.settings.font14
                }
            }

            onClicked: {
                gridView.currentIndex = model.index
                form.setControlState("EventTypeStage", "", listElementUid, model.index)
                form.pushPage("qrc:/EarthRanger/AttributesPage.qml", { elementUid: modelData.uid } )
            }

            C.HorizontalDivider {}
        }
    }
}
