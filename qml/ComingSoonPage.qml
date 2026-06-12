import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    header: C.PageHeader {
        text: qsTr("Coming soon")
    }

    ItemDelegate {
        anchors.fill: parent
        enabled: false
        font.pixelSize: App.settings.font16
        text: qsTr("In progress, expected during 2020!")
    }
}
