import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12

Item {
    id: root

    implicitHeight: layout.implicitHeight

    property string provider: ""
    property string defaultServer: "https://"
    property alias server: server.text
    property alias username: username.text
    property alias password: password.text
    property bool serverVisible: true
    property bool serverEnabled: true
    property string cacheKey: ""
    property bool cachePassword: false
    property string errorText: ""
    property alias iconSource: iconImage.source
    property bool skipAllowed: false

    property var servers: undefined

    property string badServer: qsTr("Bad server")
    property string badUsernameOrPassword: qsTr("Bad user name or password")

    signal loginClicked(var provider, var server, var username, var password)
    signal skipClicked()

    onErrorTextChanged: {
        if (errorText !== "") {
            showError(errorText)
        }
    }

    onCacheKeyChanged: {
        if (cacheKey === "") {
            serverSelect.currentIndex = -1
            server.text = defaultServer
            username.text = ""
            password.text = ""
            return
        }

        let cacheValue = App.getLogin(cacheKey, defaultServer)
        if (servers !== undefined && cacheValue.serverIndex !== undefined) {
            serverSelect.currentIndex = cacheValue.serverIndex
        }

        server.text = cacheValue.server !== "" ? cacheValue.server : defaultServer
        username.text = cacheValue.username
        password.text = cacheValue.password
    }

    ColumnLayout {
        id: layout
        
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.8

        Item {
            height: 8
            visible: iconSource !== ""
        }

        Image {
            id: iconImage
            Layout.alignment: Qt.AlignHCenter
            sourceSize.width: root.width * 0.2
            sourceSize.height: root.width * 0.2
            fillMode: Image.PreserveAspectFit
            width: root.width * 0.2
            height: root.width * 0.2
            visible: iconSource !== ""
        }

        Item {
            height: 8
            visible: iconSource !== ""
        }

        ComboBox {
            id: serverSelect
            font.pixelSize: App.settings.font14
            property bool customServer: false
            Layout.fillWidth: true
            visible: serverVisible && root.servers !== undefined
            enabled: serverEnabled
            model: root.servers
            textRole: "name"
            onCurrentIndexChanged: {
                if (currentIndex !== -1) {
                    let curr = model.get(currentIndex)
                    server.text = curr.server
                    customServer = curr.custom === true
                }
            }

            Component.onCompleted: {
                if (root.servers !== undefined && currentIndex === -1) {
                    currentIndex = 0
                }
            }
        }

        TextField {
            id: server
            Layout.fillWidth: true
            enabled: serverEnabled
            font.pixelSize: App.settings.font14
            visible: serverVisible && (root.servers === undefined || serverSelect.customServer)
            inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
            placeholderText: qsTr("Server")
        }

        Item {
            height: 8
            visible: iconSource !== ""
        }

        TextField {
            id: username
            Layout.fillWidth: true
            font.pixelSize: App.settings.font14
            inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhPreferLowercase
            placeholderText: qsTr("User name")
        }

        Item {
            height: 8
            visible: iconSource !== ""
        }

        TextField {
            id: password
            Layout.fillWidth: true
            font.pixelSize: App.settings.font14
            echoMode: TextInput.Password
            placeholderText: qsTr("Password")
        }

        ColorButton {
            Layout.fillWidth: true
            text: qsTr("Login")
            font.pixelSize: App.settings.font12
            font.bold: true
            font.capitalization: Font.MixedCase
            color: Material.accent
            enabled: {
                if (serverVisible && server.text.trim() === "") {
                    return false
                }

                if (username.text.trim() === "" || password.text.trim() === "") {
                    return false
                }

                return true
            }

            onClicked: {
                root.errorText = ""

                let s = server.text.trim()
                let u = username.text.trim()
                let p = password.text.trim()

                if (cacheKey !== "") {
                    App.setLogin(cacheKey, { server: s, serverIndex: serverSelect.currentIndex, username: u, password: p, cachePassword: cachePassword })
                }

                loginClicked(provider, s, u, p)
            }
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Skip and login later")
            font.pixelSize: App.settings.font12
            font.bold: true
            font.capitalization: Font.MixedCase
            visible: skipAllowed
            onClicked: skipClicked()
        }
    }
}
