import QtQuick 2.12
import QtQuick.Controls 2.12

Page {
    id: root

    Rectangle {
        x: -root.padding
        y: -root.padding
        width: root.width
        height: root.height - (root.header ? root.header.height : 0) - ((root.footer && root.footer.visible) ? root.footer.height : 0)
        color: Style.colorContent
    }

    Rectangle {
        x: -root.padding
        y: root.height - root.padding - (root.header ? root.header.height : 0) - ((root.footer && root.footer.visible) ? root.footer.height : 0)
        width: root.width
        height: root.footer ? root.footer.height : 0
        color: Style.colorToolbar
    }
}
