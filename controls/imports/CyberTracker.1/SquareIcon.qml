import QtQuick 2.15
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.SquareImage  {
    id: root

    property bool recolor: source.toString().startsWith("qrc:/icons/")
    property color color: Material.foreground

    size: Style.iconSize24
    visible: source !== ""

    layer {
        enabled: root.recolor
        effect: ColorOverlay {
            color: root.color
        }
    }
}
