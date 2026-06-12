import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Create location service")
    }

    Flickable {
        anchors.fill: parent
        contentHeight: column.height

        ColumnLayout {
            id: column
            spacing: 0
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.9

            Item { height: App.scaleByFontSize(16) }

            Label {
                Layout.fillWidth: true
                text: qsTr("Create an ArcGIS Online hosted feature service which will receive location data.")
                font.pixelSize: App.settings.font16
                wrapMode: Label.WordWrap
            }

            Item { height: App.scaleByFontSize(16) }

            C.HorizontalDivider {}

            Item { height: App.scaleByFontSize(16) }

            TextField {
                id: username
                Layout.fillWidth: true
                font.pixelSize: App.settings.font14
                inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhPreferLowercase
                placeholderText: qsTr("ArcGIS Online user name")
            }

            Item { height: App.scaleByFontSize(8) }

            TextField {
                id: password
                Layout.fillWidth: true
                font.pixelSize: App.settings.font14
                echoMode: TextInput.Password
                placeholderText: qsTr("ArcGIS Online password")
                inputMethodHints: Qt.ImhHiddenText | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhSensitiveData
            }

            Item { height: App.scaleByFontSize(8) }

            TextField {
                id: serviceName
                Layout.fillWidth: true
                font.pixelSize: App.settings.font14
                placeholderText: qsTr("The name of service to be created")
            }

            Item { height: App.scaleByFontSize(16) }

            Button {
                id: buttonStart

                Layout.fillWidth: true
                text: qsTr("Create service")
                font.pixelSize: App.settings.font14
                enabled: username.text.trim() !== "" && password.text.trim() !== "" && serviceName.text.trim() !== ""
                onClicked: {
                    if (Utils.networkAvailable()) {
                        busyCover.doWork = createService
                        busyCover.start()
                    } else {
                        showToast(qsTr("Offline"));
                    }
                }
            }
        }
    }

    function createService() {
        let result = Utils.esriAcquireOAuthToken(username.text.trim(), password.text.trim())
        if (!result.success) {
            console.log(result.errorString)
            showError(result.errorString)
            return
        }

        let accessToken = result.accessToken

        result = App.esriCreateLocationService(username.text.trim(), serviceName.text.trim(), ""/*serviceDescription.text.trim()*/, accessToken)
        if (!result.success) {
            console.log(result.errorString)
            showError(result.errorString)
            return
        }

        form.project.locationServiceUrl = result.serviceUrl
        form.project.esriLocationServiceState = result.serviceState
        showToast(qsTr("Service created"))
        popPageImmediate(page)
    }
}
