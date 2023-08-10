import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C
import ".."

Item {
    id: root

    property string scope: "https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/forms https://www.googleapis.com/auth/drive.readonly https://www.googleapis.com/auth/script.external_request"

    signal loggedIn(string username)

    state: "loggedOut"
    states: [
        State {
            name: "loggedOut"
            StateChangeScript {
                script: {
                    internal.email = internal.refreshToken = internal.accessToken = ""
                    paneSignIn.visible = true
                    labelMessage.text = ""
                }
            }
        },
        State {
            name: "requestAuth"
            StateChangeScript {
                script: {
                    paneSignIn.visible = false
                    labelMessage.text = qsTr("Requesting access...")
                    oauth2.start()
                }
            }
        },
        State {
            name: "refreshToken"
            StateChangeScript {
                script: {
                    paneSignIn.visible = false
                    labelMessage.text = qsTr("Refreshing token...")
                    busyCover.doWork = refreshToken
                    busyCover.start()
                }
            }
        },
        State {
            name: "loggedIn"
            StateChangeScript {
                script: {
                    App.settings.googleAccessToken = internal.accessToken
                    App.settings.googleRefreshToken = internal.refreshToken
                    App.settings.googleEmail = internal.email

                    root.loggedIn(internal.email)

                    paneSignIn.visible = true
                    labelMessage.text = ""
                }
            }
        }
    ]

    QtObject {
        id: internal
        property var forms: []
        property string email: ""
        property string accessToken: ""
        property string refreshToken: ""
    }

    Label {
        id: labelMessage

        anchors.centerIn: parent
        width: parent.width * 0.8
        text: ""
        font.pixelSize: App.settings.font14
        wrapMode: Label.WordWrap
        horizontalAlignment: Label.AlignHCenter
    }

    Pane {
        id: paneSignIn
        anchors.fill: parent
        Material.background: C.Style.colorContent

        spacing: 0
        bottomPadding: 0
        horizontalPadding: root.width * 0.1
        topPadding: flickable.contentHeight < root.height ? (root.height - flickable.contentHeight) / 2 : App.scaleByFontSize(8)

        contentItem: Flickable {
            id: flickable
            contentHeight: column.implicitHeight

            Column {
                id: column
                width: parent.width
                spacing: App.scaleByFontSize(8)

                SquareIcon {
                    anchors.horizontalCenter: column.horizontalCenter
                    source: "qrc:/Google/logo.svg"
                    size: C.Style.iconSize64
                }

                Label {
                    horizontalAlignment: Label.AlignHCenter
                    width: column.width
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                    text: qsTr("Login to a Google account. Form access is required to retrieve forms.")
                }

                GoogleLoginButton {
                    anchors.horizontalCenter: column.horizontalCenter
                    width: Math.min(implicitWidth, column.width)
                    onClicked: {
                        root.state = App.settings.googleRefreshToken !== "" ? "refreshToken" : "requestAuth"
                    }
                }
            }
        }
    }

    C.OAuth2 {
        id: oauth2

        scope: root.scope
        authorizationUrl: "https://accounts.google.com/o/oauth2/auth"
        accessTokenUrl: "https://oauth2.googleapis.com/token"
        clientId: App.googleCredentials.clientId
        clientSecret: App.googleCredentials.clientSecret
        redirectUri: App.googleCredentials.redirectUri
        googleNative: true

        onGranted: function(accessToken, refreshToken) {
            internal.accessToken = accessToken
            internal.refreshToken = refreshToken

            busyCover.doWork = findEmail
            busyCover.start()
        }

        onFailed: {
            console.log("OAuth2 flow failed: " + errorString)
            App.showError(errorString)
            root.state = "loggedOut"
        }
    }

    function refreshToken() {
        let result = Utils.googleRefreshOAuthToken(App.settings.googleRefreshToken)
        if (!result.success) {
            console.log(result.errorString)
            root.state = "requestAuth"
            return
        }

        internal.accessToken = result.accessToken
        internal.refreshToken = result.refreshToken

        busyCover.doWork = findEmail
        busyCover.start()
    }

    function findEmail() {
        let result = Utils.googleFetchEmail(internal.accessToken)
        if (!result.success) {
            console.log(result.errorString)
            showError(result.errorString)
            root.state = "loggedOut"
            return
        }

        internal.email = result.email
        root.state = "loggedIn"
    }
}
