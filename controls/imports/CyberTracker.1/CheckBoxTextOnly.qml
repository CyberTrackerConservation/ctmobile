import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls.Material 2.12

Row {
    id: root

    property bool checked: false
    property alias text: label.text
    property alias font: label.font

    signal clicked(var checked)
    spacing: 4

    QtObject {
        id: internal
        property int checkSize: Math.min(root.width / 3 - root.spacing, root.height)
    }

    Text {
        id: label
        width: root.width - internal.checkSize - spacing
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
        border.color: Material.foreground
        border.width: internal.checkSize * 0.1
        opacity: 0.75

        Rectangle {
            x: (internal.checkSize - width) / 2
            y: (internal.checkSize - width) / 2
            width: internal.checkSize * 0.6
            height: width
            color: Material.foreground
            visible: root.checked
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.checked = !root.checked
                root.clicked(checked)
            }
        }
    }
}
