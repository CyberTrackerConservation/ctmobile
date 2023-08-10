import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Connect")
        backVisible: !App.config.showConnectPage
    }

    Loader {
        id: loader
        anchors.fill: parent
        source: App.config.connectPane
    }
}
