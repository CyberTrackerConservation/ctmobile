import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Connect")
        backVisible: !App.config.showConnectPage
    }

    Loader {
        anchors.fill: parent
        clip: true
        source: App.config.connectPane
    }
}
