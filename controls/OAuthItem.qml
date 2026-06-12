import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    property string provider: ""
    property alias url: webView.url
    property string redirectUri: ""
    signal authComplete(var provider, var authCode)

    anchors.fill: parent

    WebView {
        id: webView
        anchors.fill: parent

        onUrlChanged: {
            var newURL = String(url);
            if (newURL.startsWith(redirectUri)) {
                authComplete(provider, newURL.split("code=")[1]);
            }
        }
    }
}
