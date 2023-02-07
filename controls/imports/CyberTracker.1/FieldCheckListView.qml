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

    signal itemClicked(string elementUid)

    C.FieldBinding {
        id: fieldBinding
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ItemDelegate {
            Layout.fillWidth: true
            padding: 6 // Same as HorizontalDivider.
            contentItem: TextField {
                id: searchFilter
                Layout.fillWidth: true
                placeholderText: qsTr("Search")
                font.pixelSize: App.settings.font18
            }

            visible: (elementListModel.count > 10) || (searchFilter.displayText !== "")
        }

        C.ListViewV {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true

            currentIndex: fieldBinding.getControlState(listElementUid + "_CurrentIndex", -1)

            model: C.ElementListModel {
                id: elementListModel
                elementUid: listElementUid
                flatten: fieldBinding.field.listFlatten
                sorted: fieldBinding.field.listSorted
                other: fieldBinding.field.listOther
                searchFilter: searchFilter.displayText
                searchFilterIgnore: fieldBinding.value
            }

            delegate: Item {
                width: ListView.view.width
                height: d.height

                ItemDelegate {
                    id: d
                    width: parent.width
                    contentItem: RowLayout {

                        Item { height: Style.minRowHeight }

                        SquareIcon {
                            source: form.getElementIcon(modelData.uid)
                            size: Style.minRowHeight
                        }

                        Label {
                            id: controlLabel
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            text: form.getElementName(modelData.uid)
                            font.pixelSize: App.settings.font16
                        }

                        C.CheckableBox {
                            id: checkBox
                            size: Style.minRowHeight
                            visible: !form.getElement(modelData.uid).hasChildren
                            checked: {
                                fieldBinding.value  // Pick up changes
                                return fieldBinding.getValue({})[modelData.uid] === true
                            }
                            onCheckedChanged: {
                                let value = fieldBinding.getValue({})
                                let wasChecked = value[modelData.uid] === true
                                let changed
                                if (checkBox.checked) {
                                    value[modelData.uid] = true
                                    changed = !wasChecked
                                } else {
                                    delete value[modelData.uid]
                                    changed = wasChecked
                                }

                                if (changed) {
                                    fieldBinding.setValue(value)
                                }
                            }
                        }

                        C.ChevronRight {
                            visible: form.getElement(modelData.uid).hasChildren
                        }
                    }

                    onClicked: {
                        if (form.getElement(modelData.uid).hasChildren) {
                            itemClicked(modelData.uid)
                        } else if (App.config.fullRowCheck) {
                            checkBox.checked = !checkBox.checked
                        }
                    }
                }

                C.HorizontalDivider {}
            }
        }
    }
}
