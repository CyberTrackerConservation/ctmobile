import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Item {
    id: confirmButton
    implicitWidth: button.implicitWidth
    implicitHeight: button.implicitHeight

    property alias buttonOpacity: button.opacity
    property alias buttonIcon: button.icon
    property alias buttonCheckable: button.checkable
    property alias buttonChecked: button.checked
    property alias buttonEnabled: button.enabled

    signal clicked()

    RoundButton {
        id: button
        anchors.fill: parent
        icon.source: "qrc:/icons/check.svg"
        icon.color: enabled ? Material.accent : Style.colorGroove
        icon.width: Style.toolButtonSize
        icon.height: Style.toolButtonSize
        onClicked: confirmButton.clicked()
    }

    Rectangle {
        x: button.leftInset
        y: button.topInset
        width: button.implicitWidth - button.leftInset * 2
        height: button.implicitHeight - button.topInset * 2
        radius: (button.implicitWidth - button.leftInset * 2) / 2
        color: "transparent"
        border.color: button.icon.color
        border.width: 1
    }
}
