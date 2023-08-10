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
        property int checkSize: Math.min(root.width / 2 - row.spacing, root.height)
        property int iconSize: checkSize
    }

    MouseArea {
        anchors.fill: parent
        onPressAndHold: {
            root.pressAndHold()
        }
        onClicked: {
            root.checked = !root.checked
            root.clicked(root.checked)
        }
    }

    Row {
        id: row
        anchors.fill: parent
        spacing: App.scaleByFontSize(4)

        SquareIcon {
            width: root.width - internal.checkSize - row.spacing
            source: root.icon
            size: internal.iconSize
        }

        Rectangle {
            y: (root.height - internal.checkSize) / 2
            width: internal.checkSize
            height: internal.checkSize
            color: "transparent"
            border.color: root.checked ? (colorHighlight || Style.colorHighlight) : Style.colorHighlight
            border.width: internal.checkSize * 0.1

            Rectangle {
                x: (internal.checkSize - width) / 2
                y: (internal.checkSize - width) / 2
                width: internal.checkSize * 0.6
                height: width
                color: colorHighlight || Style.colorHighlight
                visible: root.checked
            }
        }
    }
}
