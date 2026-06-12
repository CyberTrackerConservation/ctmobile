import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ToolButton {
    id: button
    property alias image: icon.source
    property alias title: label.text
    font.bold: true
    font.pixelSize: App.settings.font16
    font.capitalization: Font.AllUppercase

    contentItem: Item {

        anchors.fill: parent

        Image {
            id: icon
            anchors.centerIn: parent
            height: 55
            fillMode: Image.PreserveAspectFit
            width: 55
            smooth: true
            opacity: button.enabled ? 1.0 : 0.5

            sourceSize {
                height: 55
                width: 55
            }

            layer.enabled: true
            layer.effect: C.SimpleColorOverlay {
                color: App.settings.darkTheme ? Utils.lighter(Material.primary) : Material.primary
            }
        }

        Label {
            id: label
            verticalAlignment: Label.AlignVCenter
            horizontalAlignment: Label.AlignHCenter

            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                bottomMargin: 5
            }

            font: button.font
            minimumPixelSize: 8
            wrapMode: Label.WordWrap
            maximumLineCount: 3
        }

        Rectangle {
            height: 1
            color: Material.primary
            anchors {
                left: parent.left
                leftMargin: 10
                right: parent.right
                rightMargin: 10
                bottom: parent.bottom
            }
        }
    }
}
