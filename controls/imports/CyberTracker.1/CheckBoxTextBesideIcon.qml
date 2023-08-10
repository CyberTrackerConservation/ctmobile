import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls.Material 2.12

Item {
    id: root

    property bool checked: false
    property string icon: ""
    property alias text: label.text
    property alias font: label.font

    signal clicked(var checked)

    QtObject {
        id: internal
        property int checkSize: Math.min(root.width / 3 - row.spacing * 2, root.height)
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.checked = !root.checked
            root.clicked(root.checked)
        }
    }

    Row {
        id: row
        width: root.width
        height: root.height
        spacing: App.scaleByFontSize(4)

        SquareIcon {
            size: internal.checkSize
            height: root.height
            source: root.icon
        }

        Text {
            id: label
            width: root.width - (internal.checkSize + row.spacing) * 2
            height: root.height
            verticalAlignment: Image.AlignVCenter
            elide: Text.ElideRight
            clip: true
            color: Material.foreground
        }

        Rectangle {
            y: (root.height - internal.checkSize) / 2
            width: internal.checkSize
            height: internal.checkSize
            color: "transparent"
            border.color: root.checked ? (colorHighlight || Style.colorHighlight) : Style.colorHighlight
            border.width: internal.checkSize * 0.1

            Rectangle {
                x: (internal.checkSize - width) / 2
                y: (internal.checkSize - width) / 2
                width: internal.checkSize * 0.6
                height: width
                color: colorHighlight || Style.colorHighlight
                visible: root.checked
            }
        }
    }
}
