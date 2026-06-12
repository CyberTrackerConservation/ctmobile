import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtMultimedia
import CyberTracker as C

ContentPage {
    id: page

    property alias text: pageHeader.text
    property alias source: image.source
    property var tags: ({})

    header: PageHeader {
        id: pageHeader
        text: page.title
    }

    Image {
        id: image
        cache: false
        autoTransform: true
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
    }

    // Attachment tags.
    Flow {
        anchors {
            leftMargin: App.scaleByFontSize(8)
            rightMargin: App.scaleByFontSize(8)
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        anchors.margins: App.scaleByFontSize(8)
        spacing: App.scaleByFontSize(8)
        visible: page.tags && page.tags.length > 0

        Repeater {
            model: page.tags

            delegate: Button {
                text: modelData
                font.pixelSize: App.settings.font14
                font.capitalization: Font.MixedCase
                background: Rectangle {
                    color: Material.accent
                }
            }
        }
    }
}
