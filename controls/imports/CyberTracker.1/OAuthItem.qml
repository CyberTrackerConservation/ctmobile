import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

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
