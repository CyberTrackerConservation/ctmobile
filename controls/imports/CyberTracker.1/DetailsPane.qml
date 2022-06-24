import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import CyberTracker 1.0 as C

Pane {
    id: root

    property var model: []

    padding: 0

    contentItem: C.ListViewV {
        id: listView
        model: root.model

        delegate: ItemDelegate {
            width: listView.width
            height: modelData.name === "-" ? 2 : implicitContentHeight + padding

            contentItem: RowLayout {
                spacing: 4
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

            C.HorizontalDivider {
                height: modelData.name === "-" ? 2 : 1
            }
        }
    }
}
