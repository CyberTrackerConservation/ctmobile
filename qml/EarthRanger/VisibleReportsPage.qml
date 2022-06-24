import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    header: C.PageHeader {
        text: qsTr("Visible reports")
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent

        property int checkedChange: 0
        property var visibleReports: []
        property var locationReports: []

        Component.onCompleted: {
            let i

            // Initialize with all reports visible.
            visibleReports = form.getSetting("visibleReports", {})
            if (Object.keys(visibleReports).length === 0) {
                for (i = 0; i < model.count; i++) {
                    visibleReports[model.get(i).elementUid] = true
                }
            }

            // Initialize with all reports location enabled.
            locationReports = form.getSetting("locationReports", {})
            if (Object.keys(locationReports).length === 0) {
                for (i = 0; i < model.count; i++) {
                    locationReports[model.get(i).elementUid] = true
                }
            }

            checkedChange++
        }

        model: C.ElementTreeModel {
            elementUid: "categories"
        }

        delegate: ItemDelegate {
            width: ListView.view.width

            contentItem: RowLayout {
                id: row

                Image {
                    id: reportImage
                    Layout.preferredWidth: C.Style.minRowHeight
                    source: form.getElementIcon(modelData.elementUid)
                    fillMode: Image.Pad
                    visible: modelData.depth > 0
                    opacity: {
                        listView.checkedChange
                        return listView.visibleReports[modelData.elementUid] ? 1.0 : 0.5
                    }
                }

                Label {
                    Layout.fillWidth: true
                    wrapMode: Label.WordWrap
                    font.bold: modelData.depth === 0
                    font.pixelSize: App.settings.font14
                    text: form.getElementName(modelData.elementUid)
                    verticalAlignment: Text.AlignVCenter
                    opacity: reportImage.opacity
                }

                ToolButton {
                    checkable: true
                    enabled: {
                        listView.checkedChange
                        return listView.visibleReports[modelData.elementUid]
                    }
                    icon.source: checked ? "qrc:/icons/gps_fixed.svg" : "qrc:/icons/gps_not_fixed.svg"

                    checked: {
                        listView.checkedChange
                        return listView.locationReports[modelData.elementUid] === true
                    }

                    onClicked: {
                        listView.locationReports[modelData.elementUid] = checked

                        let i
                        if (modelData.depth === 0) {
                            // Turn on/off all reports in this category.
                            for (i = model.index + 1; i < listView.model.count; i++) {
                                if (listView.model.get(i).depth === 0) break
                                listView.locationReports[listView.model.get(i).elementUid] = checked
                            }
                        } else {
                            // Turn off "all" mode for this category.
                            for (i = model.index - 1; i >= 0; i--) {
                                if (listView.model.get(i).depth === 0) {
                                    listView.locationReports[listView.model.get(i).elementUid] = false
                                    break
                                }
                            }
                        }

                        listView.checkedChange++

                        form.setSetting("locationReports", listView.locationReports)
                    }
                }

                Switch {
                    id: checkBox
                    Layout.alignment: Qt.AlignRight
                    checked: {
                        listView.checkedChange
                        return listView.visibleReports[modelData.elementUid]
                    }

                    onClicked: {
                        listView.visibleReports[modelData.elementUid] = checked

                        let i
                        if (modelData.depth === 0) {
                            // Turn on/off all reports in this category.
                            for (i = model.index + 1; i < listView.model.count; i++) {
                                if (listView.model.get(i).depth === 0) break
                                listView.visibleReports[listView.model.get(i).elementUid] = checked
                            }
                        } else {
                            // Turn off "all" mode for this category.
                            for (i = model.index - 1; i >= 0; i--) {
                                if (listView.model.get(i).depth === 0) {
                                    listView.visibleReports[listView.model.get(i).elementUid] = false
                                    break
                                }
                            }
                        }

                        listView.checkedChange++

                        form.setSetting("visibleReports", listView.visibleReports)
                    }
                }
            }

            onClicked: {
                checkBox.checked = !checkBox.checked
                checkBox.clicked()
            }

            background: Rectangle {
                id: backColor
                anchors.fill: parent
                opacity: 0.5
                color: modelData.depth === 0 ? Material.primary : "transparent"
            }

            C.HorizontalDivider {
                visible: modelData.depth > 0
            }
        }
    }
}

