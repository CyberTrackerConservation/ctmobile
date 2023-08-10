import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

ContentPage {
    id: root

    property alias provider: loginItem.provider
    property alias defaultServer: loginItem.defaultServer
    property alias server: loginItem.server
    property alias username: loginItem.username
    property alias password: loginItem.password
    property alias serverVisible: loginItem.serverVisible
    property alias serverEnabled: loginItem.serverEnabled
    property alias cacheKey: loginItem.cacheKey
    property alias iconSource: loginItem.iconSource
    property alias skipAllowed: loginItem.skipAllowed

    signal loginClicked(var provider, var server, var username, var password)
    signal loggedIn(var username)
    signal skipClicked()
    signal backClicked()

    header: PageHeader {
        id: pageHeader
        formBack: false
        onBackClicked: root.backClicked
        text: root.title
    }

    LoginItem {
        id: loginItem
        anchors.fill: parent
    }

    Component.onCompleted: {
        loginItem.loginClicked.connect(loginClicked)
        loginItem.skipClicked.connect(skipClicked)
        pageHeader.backClicked.connect(backClicked)
    }
}
