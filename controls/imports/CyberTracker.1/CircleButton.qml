import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

RoundButton {
    id: root

    property bool outline: true

    icon.width: Style.toolButtonSize
    icon.height: Style.toolButtonSize
    icon.color: Material.foreground

    radius: (implicitWidth - leftInset * 2) / 2

    Rectangle {
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
