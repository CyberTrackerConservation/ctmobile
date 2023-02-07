import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property bool configMode: false

    header: C.PageHeader {
        id: title
        text: qsTr("Layers")
    }

    C.MapLayerListModel {
        id: layerListModel
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
                        icon.source: "qrc:/icons/settings.svg"
                        icon.width: C.Style.toolButtonSize
                        icon.height: C.Style.toolButtonSize
                        opacity: 0.5
                        visible: modelData.config || false
                        onClicked: {
                            if (typeof(formPageStack) !== "undefined") {
                                form.pushPage("qrc:/OfflineMapPage.qml", {})
                            } else {
                                appPageStack.push("qrc:/OfflineMapPage.qml", {}, StackView.Immediate)
                            }
                        }
                    }
                }
            }

            Component {
                id: layerComponent
                RowLayout {
                    Switch {
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

                case "form":
                case "goto":
                case "project":
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
