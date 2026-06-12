import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    QtObject {
        id: internal

        property var projection: App.settings.projection
        property string filterText: ""
        property var items: Utils.variantListFromJsonFile(":/config/projections.json")
        property var filteredItems: {
            if (filterText.trim() === "") {
                return items
            }

            let epsg = parseInt(filterText)
            if (!isNaN(epsg)) {
                let result = items.filter(item => item.i === epsg)
                if (result.length !== 0) {
                    return result
                }
            }

            let ft = filterText.toLowerCase()
            return items.filter(item => item.d.toLowerCase().indexOf(ft) !== -1)
        }
    }

    header: C.PageHeader {
        text: qsTr("Map projection")
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            App.settings.projection = internal.projection
            popPageImmediate(page)
        }
    }

    ColumnLayout {
        anchors.fill: parent

        ItemDelegate {
            Layout.fillWidth: true

            contentItem: TextField {
                placeholderText: qsTr("Search") + "..."
                onTextChanged: internal.filterText = text
                font.pixelSize: App.settings.font16
            }
        }

        C.ListViewV {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true

            model: internal.filteredItems

            delegate: C.HighlightDelegate {
                id: d
                width: ListView.view.width
                highlighted: modelData.i === internal.projection.i

                contentItem: RowLayout {
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Label.WordWrap
                            text: App.translateText(modelData.d)
                            font.pixelSize: App.settings.font18
                            elide: Label.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "EPSG: " + modelData.i
                            font.pixelSize: App.settings.font12
                            opacity: 0.5
                            elide: Label.ElideRight
                        }
                    }
                }

                onClicked: {
                    internal.projection = modelData
                }

                C.HorizontalDivider {}
            }
        }
    }
}
