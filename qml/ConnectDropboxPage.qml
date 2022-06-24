import QtQml 2.12
import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import CyberTracker 1.0 as C

C.ContentPage {
    header: C.PageHeader {
        text: qsTr("Connect to Dropbox")
    }

    C.OAuthItem {
        anchors.fill: parent
        provider: "Dropbox"
        redirectUri: "https://dropboxauth"

        onAuthComplete: {
            console.debug("Dropbox AuthComplete: " + authCode)
            visible = false
            busyCover.doWork = connectDropbox
            busyCover.start()
        }

        Component.onCompleted: {
            url = "https://www.dropbox.com/oauth2/authorize?client_id=gjp2br2ejke1qi0&response_type=code" + "&redirect_uri=" + redirectUri
            visible = true
        }
    }

    function connectDropbox() {
        let result = App.projectManager.bootstrap("Dropbox", { "authCode": authCode })
        if (!result.success) {
            return
        }

        postPopToRoot()
    }
}
