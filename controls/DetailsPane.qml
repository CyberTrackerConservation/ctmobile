import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

Pane {
    id: root

    property var model: []

    padding: 0
    Material.background: Style.colorContent

    contentItem: ListViewV {
        id: listView
        model: root.model

        delegate: ItemDelegate {
            width: listView.width
            height: modelData.name === "-" ? 2 : implicitContentHeight + padding

            contentItem: RowLayout {
                spacing: App.scaleByFontSize(4)
                opacity: 0.75
                visible: modelData.name !== "-"

                Label {
                    Layout.preferredWidth: parent.width / 2
                    text: modelData.name
                    font.pixelSize: App.settings.font16
                    elide: Label.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: typeof(modelData.value) !== "undefined" ? modelData.value : ""
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    elide: Label.ElideRight
                }
            }

            HorizontalDivider {
                height: modelData.name === "-" ? 2 : 1
            }
        }
    }
}
