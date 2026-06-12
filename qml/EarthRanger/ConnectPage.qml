import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    header: C.PageHeader {
        text: qsTr("Connect to") + " EarthRanger"
    }

    C.LoginItem {
        id: loginItem
        anchors.fill: parent
        provider: "EarthRanger"
        cacheKey: "EarthRanger"
        iconSource: App.settings.darkTheme ? "qrc:/EarthRanger/logoDark.svg" : "qrc:/EarthRanger/logo.svg"
        onLoginClicked: {
            busyCover.doWork = connectEarthRanger
            busyCover.start()
        }
    }

    function connectEarthRanger() {
        let params = {
            "server": loginItem.server,
            "username": loginItem.username,
            "password": loginItem.password
        }

        let result = App.projectManager.bootstrap("EarthRanger", params)
        if (!result.success) {
            return
        }

        postPopToRoot()
    }
}
