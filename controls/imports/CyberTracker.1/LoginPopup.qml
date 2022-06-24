import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Dialog {
    property alias provider: loginItem.provider
    property alias server: loginItem.server
    property alias username: loginItem.username
    property alias password: loginItem.password
    property alias serverVisible: loginItem.serverVisible
    property alias serverEnabled: loginItem.serverEnabled
    property alias cacheKey: loginItem.cacheKey
    property alias iconSource: loginItem.iconSource

    signal loginClicked(var provider, var server, var username, var password)

    parent: Overlay.overlay
    x: (parent.width - width) / 2
    y: 40

    modal: true
    width: parent.width * 0.8
    height: loginItem.implicitHeight + padding * 2

    LoginItem {
        id: loginItem
        anchors.fill: parent
    }

    Component.onCompleted: {
        loginItem.loginClicked.connect(loginClicked)
    }
}
