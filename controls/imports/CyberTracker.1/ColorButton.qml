import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

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
