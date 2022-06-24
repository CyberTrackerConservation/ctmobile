import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

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
