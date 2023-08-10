import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Item {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property string listElementUid: fieldBinding.field.listElementUid
    property bool iconVisible: true

    signal itemClicked(string elementUid)

    C.FieldBinding {
        id: fieldBinding
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ItemDelegate {
            Layout.fillWidth: true
            padding: App.scaleByFontSize(6) // Same as HorizontalDivider.
            contentItem: TextField {
                id: searchFilter
                Layout.fillWidth: true
                placeholderText: qsTr("Search")
                font.pixelSize: App.settings.font18
            }

            visible: (elementListModel.count > 10) || (searchFilter.displayText !== "")
        }

        ListViewV {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true

            Component.onCompleted: {
                let index = fieldBinding.getControlState(listElementUid + "_CurrentIndex", -1)
                if (index !== -1) {
                    listView.positionViewAtIndex(index, ListView.Contain)
                }
            }

            model: C.ElementListModel {
                id: elementListModel
                elementUid: listElementUid
                flatten: fieldBinding.field.listFlatten
                sorted: fieldBinding.field.listSorted
                other: fieldBinding.field.listOther
                searchFilter: searchFilter.displayText
                filterByFieldRecordUid: fieldBinding.recordUid
                filterByField: fieldBinding.field.listFilterByField
                filterByTag: fieldBinding.field.listFilterByTag
            }

            delegate: HighlightDelegate {
                width: ListView.view.width
                highlighted: modelData.uid === fieldBinding.value

                Component {
                    id: itemComponent

                    RowLayout {

                        Item { height: Style.minRowHeight }

                        SquareIcon {
                            source: form.getElementIcon(modelData.uid)
                            size: Style.minRowHeight
                            visible: root.iconVisible && source.toString() !== ""
                        }

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Label.Wrap
                            text: form.getElementName(modelData.uid)
                            font.pixelSize: App.settings.font18
                        }

                        C.ChevronRight {
                            visible: form.getElement(modelData.uid).hasChildren
                        }
                    }
                }

                contentItem: Loader {
                    height: item.height
                    sourceComponent: itemComponent
                }

                C.HorizontalDivider {
                    visible: !highlighted
                }

                onClicked: {
                    fieldBinding.setControlState(listElementUid + "_CurrentIndex", model.index)
                    itemClicked(modelData.uid)
                }
            }
        }
    }
}
