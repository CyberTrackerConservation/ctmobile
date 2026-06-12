import QtQuick
import QtQuick.Effects
import QtQuick.Controls.Material
import CyberTracker as C

C.SquareImage  {
    id: root

    property bool recolor: source.toString().startsWith("qrc:/icons/")
    property color color: Material.foreground

    size: Style.iconSize24
    visible: source !== ""

    layer.enabled: root.recolor
    layer.effect: SimpleColorOverlay {
        color: root.color
    }
}
