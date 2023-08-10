import QtQuick 2.12
import QtQuick.Controls 2.12

PopupBase {
    id: popup

    property string title: ""
    property string message1: ""
    property string message2: ""
    property string message3: ""
    property string icon: "qrc:/icons/info.svg"
    property var buttons: [{icon: "qrc:/icons/ok.svg", text: qsTr("OK") } ]

    signal clicked(var index)

    width: parent.width * 0.75
    height: Math.min(parent.height * 0.75, column.implicitHeight + footer.height + (width - availableWidth))

    Component.onCompleted: {
        if (typeof(formPageStack) !== "undefined") {
            formPageStack.setProjectColors(popup)
        }

        footerButtons.model = popup.buttons
    }

    contentItem: Column {
        width: popup.availableWidth
        spacing: 0

        Flickable {
            id: flickable
            width: parent.width
            contentHeight: column.implicitHeight
            ScrollBar.vertical: ScrollBar { interactive: false }
            height: Math.min(column.implicitHeight, popup.availableHeight - footer.height)
            clip: true

            Column {
                id: column
                width: parent.width
                spacing: App.scaleByFontSize(8)

                SquareIcon {
                    anchors.horizontalCenter: column.horizontalCenter
                    size: Style.iconSize48
                    source: popup.icon
                    visible: popup.icon !== ""
                }

                Label {
                    width: column.width
                    text: popup.title
                    font.pixelSize: App.settings.font18
                    font.bold: true
                    wrapMode: Label.Wrap
                    verticalAlignment: Label.AlignVCenter
                    horizontalAlignment: Label.AlignHCenter
                    visible: popup.title !== ""
                    elide: Label.ElideRight
                }

                Label {
                    width: column.width
                    text: popup.message1
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.Wrap
                    visible: popup.message1 !== ""
                    elide: Label.ElideRight
                    textFormat: Text.MarkdownText
                    horizontalAlignment: (popup.message2 === "" && popup.message3 === "") ? Label.AlignHCenter : Label.AlignLeft
                }

                Label {
                    width: column.width
                    text: popup.message2
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.Wrap
                    visible: popup.message2 !== ""
                    elide: Label.ElideRight
                    textFormat: Text.MarkdownText
                }

                Label {
                    width: column.width
                    text: popup.message3
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.Wrap
                    visible: popup.message3 !== ""
                    elide: Label.ElideRight
                    textFormat: Text.MarkdownText
                }

                Item { width: 1; height: 1 }
            }
        }

        HorizontalDivider {}

        Row {
            id: footer
            width: parent.width
            spacing: 0

            Repeater {
                id: footerButtons

                delegate: ToolButton {
                    width: popup.availableWidth / popup.buttons.length
                    text: modelData.text
                    font.pixelSize: App.settings.font12
                    font.capitalization: Font.AllUppercase
                    icon.source: modelData.icon
                    icon.width: Style.toolButtonSize
                    icon.height: Style.toolButtonSize
                    onClicked: {
                        popup.close()
                        popup.clicked(model.index)
                    }
                }
            }
        }
    }
}
