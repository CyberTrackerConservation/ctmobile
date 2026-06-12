import QtQuick
import QtQuick.Controls.Material

Button {
    id: root

    property color color

    Material.background: color

    contentItem: Text {
        text: root.text
        font: root.font
        color: Utils.lightness(root.color) < 128 ? "white" : "black"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
