import QtQuick 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Controls 2.12

Item {
    id: root
    width: parent.width
    height: button.implicitHeight

    property alias text: button.text
    property alias color: background.color
    property alias icon: button.icon

    signal clicked

    Rectangle {
        id: background
        anchors.fill: parent
        color: Material.accent
    }

    ToolButton {
        id: button
        anchors.fill: parent
        onClicked: root.clicked()
        icon.color: Material.foreground
    }
}
