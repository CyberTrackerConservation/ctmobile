import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

PopupBase {
    property alias provider: oauthItem.provider
    property alias url: oauthItem.url
    property alias redirectUri: oauthItem.redirectUri
    signal authComplete(var provider, var authCode)

    width: parent.width * 0.8
    height: parent.height * 0.8

    OAuthItem {
        id: oauthItem
        onAuthComplete: {
            authComplete(provider, authCode)
            close()
        }
    }
}
