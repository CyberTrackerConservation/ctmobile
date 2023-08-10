import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls.Material 2.12

Item {
    id: root

    property bool checked: false
    property string icon: ""
    signal clicked(var checked)
    signal pressAndHold()

    QtObject {
        id: internal
        property int checkSize: Math.min(root.width, root.height)
        property int iconSize: checkSize * 0.7
    }

    Rectangle {
        id: checkRect
        anchors.centerIn: parent
        width: internal.checkSize
        height: internal.checkSize
        color: "transparent"
        border.color: root.checked ? (colorHighlight || Style.colorHighlight) : "transparent"
        border.width: internal.checkSize * 0.1
    }

    SquareIcon {
        anchors.centerIn: checkRect
        source: root.icon
        size: internal.iconSize
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.checked = !root.checked
            root.clicked(root.checked)
        }
        onPressAndHold: {
            root.pressAndHold()
        }
    }
}
