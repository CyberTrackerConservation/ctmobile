import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import Qt.labs.settings 1.0 as Labs
import CyberTracker 1.0 as C

Window {
    id: window

    width: 640
    height: pane.implicitHeight

    visible: true
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint | Qt.Dialog
    modality: Qt.ApplicationModal

    title: qsTr("Create ArcGIS location service")

    minimumWidth: 640
    maximumWidth: 640
    minimumHeight: pane.implicitHeight
    maximumHeight: pane.implicitHeight

    Labs.Settings {
        fileName: App.iniPath
        category: "CreateEsriService"
        property alias username: username.text
        property alias password: password.text
        property alias serviceName: serviceName.text
        property alias serviceDescription: serviceDescription.text
    }

    Pane {
        id: pane
        width: window.width
        opacity: enabled ? 1.0 : 0.5

        contentItem: ColumnLayout {
            spacing: 0
            width: pane.width

            Label {
                Layout.fillWidth: true
                text: qsTr("Create an ArcGIS Online hosted feature service which will receive location track data from Survey123 %1.").arg(App.alias_projects)
                font.pixelSize: App.settings.font14
                wrapMode: Label.WordWrap
            }

            Item { height: 16 }

            C.HorizontalDivider { Layout.fillWidth: true }

            Item { height: 16 }

            Label {
                Layout.fillWidth: true
                font.pixelSize: App.settings.font10
                text: "User name"
            }

            TextField {
                id: username
                Layout.fillWidth: true
                font.pixelSize: App.settings.font14
                inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhPreferLowercase
                placeholderText: "ArcGIS Online user name"
            }

            Label {
                Layout.fillWidth: true
                font.pixelSize: App.settings.font10
                text: "Password"
            }

            TextField {
                id: password
                Layout.fillWidth: true
                font.pixelSize: App.settings.font14
                echoMode: TextInput.Password
                placeholderText: "ArcGIS Online password"
            }

            Label {
                text: qsTr("Service name")
                Layout.fillWidth: true
                font.pixelSize: App.settings.font10
            }

            TextField {
                id: serviceName
                Layout.fillWidth: true
                font.pixelSize: App.settings.font14
                placeholderText: qsTr("The name of service to be created")
            }

            Label {
                text: qsTr("Service description")
                Layout.fillWidth: true
                font.pixelSize: App.settings.font10
            }

            TextField {
                id: serviceDescription
                Layout.fillWidth: true
                font.pixelSize: App.settings.font14
                placeholderText: qsTr("The service description")
            }

            Item { height: 8 }

            Label {
                id: progressLabel
                Layout.fillWidth: true
                font.pixelSize: App.settings.font14
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                text: qsTr("Click 'START' to create service")
            }

            Item { height: 16 }
            C.HorizontalDivider { Layout.fillWidth: true }
            Item { height: 8 }

            Button {
                id: buttonStart

                Layout.fillWidth: true
                text: qsTr("Start")
                font.pixelSize: App.settings.font14
                enabled: username.text.trim() !== "" && password.text.trim() !== "" && serviceName.text.trim() !== ""
                onClicked: {
                    pane.enabled = false
                    let result = createService(username.text.trim(), password.text.trim(), serviceName.text.trim(), serviceDescription.text.trim())
                    pane.enabled = true

                    if (!result.success) {
                        resultField.text = ""
                        return
                    }

                    resultField.text = result.serviceUrl
                    buttonStart.visible = false
                    pane.visible = false
                }
            }
        }
    }

    Pane {
        id: successPane
        visible: !pane.visible
        width: window.width
        height: window.height
        anchors.centerIn: parent
        contentItem: ColumnLayout {
            spacing: App.scaleByFontSize(8)

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Success - service created")
                font.pixelSize: App.settings.font16
                font.bold: true
                wrapMode: Label.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Create a column named 'bind::ct:esriLocationServiceUrl' in the 'settings' sheet of the XlsForm and set it to the content below. Republish and install or update the form in CyberTracker.")
                font.pixelSize: App.settings.font14
                wrapMode: Label.WordWrap
            }

            Image {
                Layout.alignment: Qt.AlignHCenter
                source: "qrc:/Esri/LocationTrackingXlsForm.png"
            }

            C.HorizontalDivider { Layout.fillWidth: true }
            Item { height: 4 }

            Label {
                id: resultField
                Layout.fillWidth: true
                font.pixelSize: App.settings.font14
                clip: true
                wrapMode: Label.WordWrap
                font.family: "Monospace"
            }

            Item { height: 4 }
            C.HorizontalDivider { Layout.fillWidth: true }

            Item { height: 4 }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr(qsTr("Copy to clipboard"))
                font.pixelSize: App.settings.font14
                onClicked: App.copyToClipboard(resultField.text)
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    function showProgress(str) {
        progressLabel.text = str
    }

    function makeNumber(str) {
        let result = parseInt(str.trim())
        if (result !== result) {
            result = 0
        }
        return result
    }

    function createService(username, password, serviceName, serviceDescription) {
        showProgress(qsTr("Logging in..."))

        let resultFail = { result: false }

        let result = Utils.esriAcquireOAuthToken(username, password)
        if (!result.success) {
            console.log(result.errorString)
            showProgress(result.errorString)
            return resultFail
        }

        showProgress(qsTr("Creating service..."))

        result = App.esriCreateLocationService(username, serviceName, serviceDescription, result.accessToken)
        if (!result.success) {
            console.log(result.errorString)
            showProgress(result.errorString)
            return resultFail
        }

        showProgress(qsTr("Success!"))

        return result
    }
}
