import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

ItemDelegate {
    id: root

    property bool sideBorder: true
    property int borderWidth: 1
    property bool lines: true
    property int columns: 1
    property int itemPadding: 0
    property var sourceComponent

    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    Binding {
        target: background
        property: "color"
        value: root.highlighted ? (Style.darkTheme ? Material.background : Material.foreground) : (colorContent || Style.colorContent)
    }

    Binding {
        target: background
        property: "opacity"
        value: root.highlighted ? 0.5 : 1.0
    }

    Rectangle {
        id: lineY1
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: root.borderWidth
        color: Style.colorGroove
        visible: root.lines && (model.index / root.columns < 1)
    }

    Rectangle {
        id: lineX1
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: root.borderWidth
        color: Style.colorGroove
        visible: root.lines && root.sideBorder && ((model.index + root.columns) % root.columns === 0)
    }

    Rectangle {
        id: lineX2
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: root.borderWidth
        color: Style.colorGroove
        visible: root.lines && (root.sideBorder || ((model.index + 1) % root.columns !== 0))
    }

    Rectangle {
        id: lineY2
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: root.borderWidth
        color: Style.colorGroove
        visible: root.lines
    }

    contentItem: Item {
        implicitWidth: root.width - (lineX1.visible ? lineX1.width : 0) - (lineX2.visible ? lineX2.width : 0) - root.itemPadding * 2
        implicitHeight: root.height - (lineY1.visible ? lineY1.height : 0) - (lineY2.visible ? lineY2.height : 0) - root.itemPadding * 2
        Loader {
            x: root.itemPadding + (lineX1.visible ? lineX1.width : 0)
            y: root.itemPadding + (lineY1.visible ? lineY1.height : 0)
            width: parent.implicitWidth
            height: parent.implicitHeight
            sourceComponent: root.sourceComponent
        }
    }
}
