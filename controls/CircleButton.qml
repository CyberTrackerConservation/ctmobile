import QtQuick
import QtQuick.Controls.Material

RoundButton {
    id: root

    property bool outline: true
    property alias backgroundColor: r.color

    icon.width: Style.toolButtonSize
    icon.height: Style.toolButtonSize
    icon.color: Material.foreground

    radius: (implicitWidth - leftInset * 2) / 2

    Rectangle {
        id: r
        x: root.leftInset
        y: root.topInset
        width: root.implicitWidth - root.leftInset * 2
        height: root.implicitHeight - root.topInset * 2
        radius: root.radius
        color: "transparent"
        border.color: root.icon.color
        border.width: 1
        visible: root.outline
    }
}
