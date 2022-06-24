import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    C.MapLayerListModel {
        id: layerListModel
    }

    header: C.PageHeader {
        id: title
        text: qsTr("Layers")
    }

    ButtonGroup {
        id: basemapButtonGroup
    }

    C.ListViewV {
        id: mapLayerListView
        anchors.fill: parent

        model: layerListModel

        delegate: ItemDelegate {
            id: d
            width: ListView.view.width
            implicitHeight: di.implicitHeight

            Component {
                id: formLayerComponent
                RowLayout {
                    Switch {
                        checked: modelData.checked === true
                        onCheckedChanged: {
                            if (modelData.checked !== checked) {
                                layerListModel.setDataLayerState(modelData.id, checked)
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: modelData.name
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/magnify_expand.svg"
                        onClicked: {
                            App.zoomToMapOverlay(modelData.id)
                            page.header.sendBackClick()
                        }
                        opacity: enabled ? 0.8 : 0
                        enabled: modelData.checked
                    }
                }
            }

            Component {
                id: gotoLayerComponent
                RowLayout {
                    Switch {
                        checked: modelData.checked === true
                        onCheckedChanged: {
                            if (modelData.checked !== checked) {
                                layerListModel.setProjectLayerState(modelData.id, checked)
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: modelData.name
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/magnify_expand.svg"
                        onClicked: {
                            App.zoomToMapOverlay(modelData.id)
                            page.header.sendBackClick()
                        }
                        opacity: enabled ? 0.8 : 0
                        enabled: modelData.checked
                    }
                }
            }

            Component {
                id: projectLayerComponent
                RowLayout {
                    Switch {
                        checked: modelData.checked === true
                        onCheckedChanged: {
                            if (modelData.checked !== checked) {
                                layerListModel.setProjectLayerState(modelData.id, checked)
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: modelData.name
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/magnify_expand.svg"
                        onClicked: {
                            App.zoomToMapLayer(modelData.id)
                            page.header.sendBackClick()
                        }
                        opacity: enabled ? 0.8 : 0
                        enabled: modelData.checked
                    }
                }
            }

            Component {
                id: basemapLayerComponent
                RowLayout {
                    Item {
                        width: 8
                    }

                    C.CheckableBox {
                        id: checkableBox
                        size: App.settings.font14 * 1.42
                        radioIcon: true
                        font.pixelSize: App.settings.font14
                        ButtonGroup.group: basemapButtonGroup
                        checked: modelData.checked === true
                        onCheckedChanged: {
                            if (modelData && modelData.checked !== checkableBox.checked) {
                                if (checkableBox.checked) {
                                    layerListModel.setBasemap(modelData.id)
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: modelData.name
                    }
                }
            }

            SwipeDelegate {
                id: di
                anchors.fill: parent

                Binding { target: background; property: "color"; value: C.Style.colorContent }

                function getComponent() {
                    switch (modelData.type) {
                    case "form":
                        return formLayerComponent

                    case "goto":
                        return gotoLayerComponent

                    case "project":
                        return projectLayerComponent

                    case "basemap":
                        return basemapLayerComponent

                    default: console.log(modelData.type)
                    }

                    return undefined
                }

                contentItem: getComponent().createObject(d)

                swipe.right: Rectangle {
                    width: modelData.type === "goto" ? parent.width : 0
                    height: parent.height
                    color: C.Style.colorGroove

                    RowLayout {
                        anchors.fill: parent

                        Label {
                            text: qsTr("Delete layer?")
                            leftPadding: 20
                            font.pixelSize: App.settings.font16
                            fontSizeMode: Label.Fit
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                        }

                        ToolButton {
                            flat: true
                            text: qsTr("Yes")
                            onClicked: {
                                di.swipe.close()
                                confirmDelete.gotoLayerId = modelData.id
                                confirmDelete.gotoLayerIndex = model.index
                                confirmDelete.open()
                            }
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredWidth: parent.height
                            Layout.maximumWidth: parent.width / 5
                            font.pixelSize: App.settings.font14
                        }

                        ToolButton {
                            flat: true
                            text: qsTr("No")
                            onClicked: di.swipe.close()
                            Layout.alignment: Qt.AlignRight
                            Layout.preferredWidth: parent.height
                            Layout.maximumWidth: parent.width / 5
                            font.pixelSize: App.settings.font14
                        }
                    }
                }
            }

            // Divider.
            Rectangle {
                x: 6
                y: parent.height - 1
                width: parent.width - 12
                height: 1
                color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
                opacity: Material.theme === Material.Dark ? 0.12 : 0.12
                visible: modelData.divider === true
            }
        }
    }

    C.ConfirmPopup {
        id: confirmDelete
        property var gotoLayerIndex
        property var gotoLayerId
        text: qsTr("Delete map layer?")
        confirmText: qsTr("Yes, delete it")
        onConfirmed: {
            layerListModel.removeGotoLayer(gotoLayerId)
        }
    }
}
