import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtWebView
import CyberTracker as C

C.ContentPage {
    id: page

    property double locationX
    property double locationY

    states: [
        State {
            name: "search"
            StateChangeScript {
                script: {
                    navBar.visible = listViewPath.visible = listViewDetails.visible = labelNotFound.visible = false
                    busyCover.doWork = runSearch
                    busyCover.start()
                }
            }
        },
        State {
            name: "notFound"
            StateChangeScript {
                script: {
                    navBar.visible = listViewPath.visible = listViewDetails.visible = false
                    labelNotFound.visible = true
                }
            }
        },
        State {
            name: "found"
            StateChangeScript {
                script: {
                    navBar.visible = listViewPath.visible = listViewDetails.visible = labelNotFound.visible = false

                    internal.stack = []
                    internal.firstStackItem = 0

                    // If only one country, default it.
                    let items = App.place.buildModel(internal.stack)
                    if (items.length === 1) {
                        internal.firstStackItem = 1
                        internal.stack.push(items[0].name)
                    }

                    Qt.callLater(() => { page.state = "tree"; })
                }
            }
        },
        State {
            name: "tree"
            StateChangeScript {
                script: {
                    navBar.visible = listViewPath.visible = true
                    listViewDetails.visible = labelNotFound.visible = false
                    listViewDetails.model = []

                    rebuild()
                }
            }
        },
        State {
            name: "details"
            StateChangeScript {
                script: {
                    listViewPath.visible = labelNotFound.visible = false
                    navBar.visible = listViewDetails.visible = true
                    internal.headerText = internal.details.title || ""
                    internal.detailsHeaderText = internal.details.subtitle || ""
                    listViewDetails.model = internal.details.blocks || []
                }
            }
        }
    ]

    QtObject {
        id: internal
        property var stack: []
        property int firstStackItem: 0
        property string headerText: ""
        property string detailsHeaderText: ""
        property bool canBack: false
        property var details: ({})
    }

    header: C.PageHeader {
        text: qsTr("Place")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ItemDelegate {
            id: navBar
            Layout.fillWidth: true
            visible: false
            enabled: internal.canBack
            onClicked: {
                if (!internal.canBack) {
                    return
                }

                if (page.state === "details") {
                    page.state = "tree"
                } else {
                    internal.stack.pop()
                    rebuild()
                }
            }
            contentItem: RowLayout {
                spacing: 0
                C.SquareIcon {
                    source: "qrc:/icons/chevron_left.svg"
                    opacity: internal.canBack ? 1.0 : 0.0
                }

                Label {
                    Layout.fillWidth: true
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    text: internal.headerText
                    clip: true
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    wrapMode: Label.Wrap
                }

                C.SquareIcon {
                    source: "qrc:/icons/chevron_right.svg"
                    opacity: 0.0
                }
            }

            C.HorizontalDivider {
                height: App.scaleByFontSize(2)
            }
        }

        C.ListViewV {
            id: listViewPath
            visible: false
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: []

            delegate: ItemDelegate {
                width: listViewPath.width
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font14
                        text: modelData.name
                        wrapMode: Label.Wrap
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    if (modelData.blocks !== undefined) {
                        internal.details = modelData
                        page.state = "details"
                    } else {
                        internal.stack.push(modelData.name)
                        rebuild()
                    }
                }

                C.HorizontalDivider {}
            }
        }

        C.ListViewV {
            id: listViewDetails
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: false

            header: ItemDelegate {
                width: parent.width
                height: internal.detailsHeaderText === "" ? 0 : implicitHeight

                contentItem: Label {
                    id: detailsSubtitle
                    width: parent.width
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.Wrap
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    text: internal.detailsHeaderText
                }

                opacity: 0.8
                visible: internal.detailsHeaderText !== ""

                C.HorizontalDivider {}
            }

            model: []

            delegate: ItemDelegate {
                id: detailsDelegate
                width: listViewDetails.width

                contentItem: ColumnLayout {
                    width: listViewDetails.width
                    spacing: App.scaleByFontSize(4)
                    Label {
                        id: labelBlock
                        Layout.fillWidth: true
                        text: modelData.label || ""
                        visible: text !== ""
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        wrapMode: Label.Wrap
                        font.pixelSize: App.settings.font14
                        font.bold: true
                    }

                    Item {
                        height: App.scaleByFontSize(2)
                        visible: labelBlock.visible
                    }

                    Loader {
                        id: detailsLoader
                        Layout.fillWidth: true
                        property var blockItem: modelData
                        sourceComponent: {
                            switch (modelData.type) {
                            case "body":
                                return detailsBodyComponent

                            case "image":
                                return imageComponent

                            case "link":
                                return linkComponent

                            case "notice":
                                return noticeComponent

                            case "attributes":
                                return attributesComponent

                            case "web":
                                return webComponent
                           }
                        }
                    }
                }

                C.HorizontalDivider {
                    visible: model.index < listViewDetails.count - 1
                }
            }
        }

        Label {
            id: labelNotFound
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            visible: false
            font.pixelSize: App.settings.font16
            text: qsTr("No items found")
            wrapMode: Label.WordWrap
            opacity: 0.5
        }
    }

    Component.onCompleted: {
        page.state = "search"
    }

    Component {
        id: detailsBodyComponent

        Label {
            readonly property var blockItem: parent.blockItem

            font.pixelSize: App.settings.font14
            wrapMode: Label.Wrap
            textFormat: blockItem.format === "html" ? Label.RichText : Label.PlainText
            text: blockItem.value !== undefined ? String(blockItem.value) : ""
        }
    }

    Component {
        id: imageComponent

        Item {
            readonly property var blockItem: parent.blockItem

            implicitHeight: image.status === Image.Ready && image.sourceSize.width > 0
                        ? width * image.sourceSize.height / image.sourceSize.width
                        : 0

            Image {
                id: image
                anchors.fill: parent
                source: parent.blockItem.source
                fillMode: Image.PreserveAspectFit
            }
        }
    }

    Component {
        id: linkComponent

        Label {
            readonly property var blockItem: parent.blockItem

            textFormat: Label.RichText
            horizontalAlignment: Qt.AlignHCenter
            font.pixelSize: App.settings.font14
            elide: Label.ElideRight
            text: "<a href=\"" + blockItem.url + "\">" + blockItem.text + "</a>"
            onLinkActivated: function(link) {
                Qt.openUrlExternally(link)
            }
        }
    }

    Component {
        id: noticeComponent

        Rectangle {
            readonly property var blockItem: parent.blockItem

            property var colors: ({
                note: {
                    textColor: "#1F2937",
                    backColor: "#E5E7EB"
                },

                important: {
                    textColor: "#1E3A8A",
                    backColor: "#DBEAFE"
                },

                caution: {
                    textColor: "#92400E",
                    backColor: "#FEF3C7"
                },

                restriction: {
                    textColor: "#991B1B",
                    backColor: "#FEE2E2"
                }
            })

            implicitHeight: label.implicitHeight + App.scaleByFontSize(16)
            color: colors[blockItem.tone].backColor
            border.color: colors[blockItem.tone].textColor
            border.width: App.scaleByFontSize(2)

            Label {
                id: label
                x: App.scaleByFontSize(8)
                y: x
                width: parent.width - x * 2
                height: implicitHeight + y * 2
                color: parent.colors[parent.blockItem.tone].textColor
                horizontalAlignment: Qt.AlignHCenter
                font.pixelSize: App.settings.font14
                wrapMode: Label.Wrap
                text: parent.blockItem.text
            }
        }
    }

    Component {
        id: attributesComponent

        ColumnLayout {
            readonly property var blockItem: parent.blockItem

            spacing: App.scaleByFontSize(4)

            Repeater {
                id: attributes
                model: blockItem.items
                Layout.fillWidth: true

                delegate: Row {
                    Layout.fillWidth: true

                    Label {
                        id: l1
                        width: parent.width / 2
                        font.pixelSize: App.settings.font12
                        wrapMode: Label.Wrap
                        text: modelData.label
                    }
                    Label {
                        id: l2
                        width: parent.width / 2
                        text: String(modelData.value)
                        font.pixelSize: App.settings.font12
                        wrapMode: Label.Wrap
                    }
                }
            }
        }
    }

    Component {
        id: webComponent

        Item {
            readonly property var blockItem: parent.blockItem

            implicitHeight: blockItem.orientation === "portrait" ? width * 4 / 3 : width * 3 / 4

            WebView {
                anchors.fill: parent

                url: (parent.blockItem.url || "")
                    .replace("<latitude>", page.locationY)
                    .replace("<longitude>", page.locationX)
            }
        }
    }

    function runSearch() {
        page.state = App.place.search(page.locationX, page.locationY) ? "found" : "notFound"
    }

    function rebuild() {
        listViewPath.model = App.place.buildModel(internal.stack)
        internal.headerText = internal.stack.length > 0 ? internal.stack[internal.stack.length - 1] :
            App.getLocationText(page.locationX, page.locationY, 0)
        internal.canBack = internal.stack.length > 0
    }
}
