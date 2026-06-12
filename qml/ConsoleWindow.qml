import QtCore
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Window
import CyberTracker as C

Window {
    id: window

    width: 800
    height: 200
    color: "black"
    visible: true
    title: qsTr("Developer console")

    Settings {
        location: App.iniUrl
        category: "ConsoleWindow"
        property alias x: window.x
        property alias y: window.y
        property alias width: window.width
        property alias height: window.height
        property alias visibility: window.visibility
    }

    Flickable {
        id: flickable

        anchors.fill: parent
        anchors.margins: 8
        flickableDirection: Flickable.VerticalFlick
        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            interactive: true
            policy: ScrollBar.AlwaysOn
            anchors.rightMargin: 10
            contentItem: Rectangle {
                color: textEdit.color
            }
        }

        contentWidth: parent.width
        contentHeight: textEdit.contentHeight
        clip: true

        TextEdit {
            id: textEdit
            width: flickable.width
            textFormat: TextEdit.RichText
            focus: true
            wrapMode: TextEdit.WrapAnywhere
            font.family: App.fixedFontFamily
            font.pixelSize: 16
            onTextChanged: scrollBar.setPosition(1.0 - scrollBar.size)
            selectByMouse: true
            color: "#CECECE"
        }
    }

    Button {
        anchors.right: flickable.right
        anchors.bottom: flickable.bottom
        anchors.rightMargin: scrollBar.width + 4
        text: qsTr("Clear")
        onClicked: {
            textEdit.clear()
        }
    }

    onVisibilityChanged: function(visibility) {
        if (visibility === Window.Minimized) {
            close()
        }
    }

    Connections {
        target: Log

        function onConsoleMessage(t) {
            textEdit.append(t)
        }
    }
}
