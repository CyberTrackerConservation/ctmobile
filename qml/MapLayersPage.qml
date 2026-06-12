import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    property bool configMode: false
    property bool hideDataLayers: false

    header: C.PageHeader {
        id: title
        text: qsTr("Layers")
    }

    C.MapLayerListModel {
        id: layerListModel
        addDataLayers: !page.hideDataLayers
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

            Component {
                id: titleComponent

                RowLayout {
                    Label {
                        Layout.fillWidth: true
                        font.bold: true
                        font.pixelSize: App.settings.font14
                        text: modelData.name
                        elide: Label.ElideRight
                    }

                    ToolButton {
                        icon.source: modelData.configIcon
                        icon.width: C.Style.toolButtonSize
                        icon.height: C.Style.toolButtonSize
                        opacity: 0.5
                        visible: modelData.configIcon !== ""
                        onClicked: {
                            pushPageImmediate(page, modelData.configPage)
                        }
                    }
                }
            }

            Component {
                id: layerComponent
                RowLayout {
                    Switch {
                        scale: C.Style.switchScale
                        checked: modelData.checked === true
                        onCheckedChanged: {
                            if (modelData.checked !== checked) {
                                layerListModel.setLayerState(modelData, checked)
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: modelData.name
                        elide: Label.ElideRight
                    }

                    ToolButton {
                        icon.source: "qrc:/icons/magnify_expand.svg"
                        icon.width: C.Style.toolButtonSize
                        icon.height: C.Style.toolButtonSize
                        onClicked: {
                            App.zoomToMapLayer(modelData)
                            page.header.sendBackClick()
                        }
                        opacity: enabled ? 0.5 : 0
                        enabled: modelData.checked
                    }
                }
            }

            Component {
                id: basemapComponent
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
                        elide: Label.ElideRight
                    }
                }
            }

            contentItem: {
                switch (modelData.type) {
                case "title":
                    return titleComponent.createObject(d)

                case "data":
                case "goto":
                case "offline":
                    return layerComponent.createObject(d)

                case "basemap":
                    return basemapComponent.createObject(d)

                default: console.log(modelData.type)
                }

                return undefined
            }

            C.HorizontalDivider {
                visible: modelData.divider === true
            }
        }
    }
}
