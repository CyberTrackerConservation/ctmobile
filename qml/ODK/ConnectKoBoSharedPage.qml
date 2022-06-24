import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C
import ".."

C.ContentPage {
    id: page

    property var accountName

    QtObject {
        id: internal

        property var account: page.accountName !== undefined ? App.config.accounts[page.accountName] : undefined
        property string landscape: ""
        property string token
        property var forms: []
    }

    header: C.PageHeader {
        text: qsTr("%1 Connect").arg(internal.account.title)
    }

    ColumnLayout {
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width * 0.8
        spacing: 0

        Item { height: 16 }

        Image {
            Layout.alignment: Qt.AlignHCenter
            sourceSize.width: parent.width * 0.2
            sourceSize.height: parent.width * 0.2
            fillMode: Image.PreserveAspectFit
            width: parent.width * 0.2
            height: parent.width * 0.2
            source: internal.account.icon
        }

        Item { height: 8 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Enter Landscape")
            font.pixelSize: App.settings.font14
            font.bold: true
        }

        Item { height: 4 }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Use %1 for templates").arg("'" + page.accountName + "'")
            font.pixelSize: App.settings.font10
            visible: page.accountName !== ""
        }

        Item { height: 4 }

        TextField {
            id: textFieldLandscape
            Layout.fillWidth: true
            font.pixelSize: App.settings.font14
            horizontalAlignment: TextField.AlignHCenter
            focus: true
            inputMethodHints: Qt.ImhNoAutoUppercase
        }

        Item { height: 8 }

        C.ColorButton {
            Layout.fillWidth: true
            text: qsTr("Choose forms")
            font.pixelSize: App.settings.font12
            font.bold: true
            font.capitalization: Font.MixedCase
            color: Material.accent
            enabled: textFieldLandscape.text.trim() !== ""
            onClicked: {
                internal.landscape = textFieldLandscape.text.trim()
                busyCover.doWork = getToken
                busyCover.start()
            }
        }
    }

    Component {
        id: formsPage

        ConnectFormPage {
            forms: internal.forms
            getFormName: function(form) { return form.name; }
            getFormSubText: function(form) { return App.formatDateTime(form.date_modified); }
            connectForms: page.connectForms
        }
    }

    function getToken() {
        let response = Utils.httpGet(internal.account.server + "/token/?format=json", internal.account.username, internal.account.password)

        if (response.status === 200) {
            internal.token = JSON.parse(response.data).token
            if (internal.token.length > 0) {
                getForms()
                return
            }
        }

        showError(qsTr("Connection failed"))
    }

    function getForms() {
        let response = Utils.httpGetWithToken(internal.account.server + "/api/v2/assets/?q=asset_type%3Asurvey", internal.token, "Token")

        if (response.status === 200) {
            let result = JSON.parse(response.data)
            if (result.results !== undefined && result.results.length > 0) {
                let items = result.results
                let forms = []
                for (let i = 0; i < items.length; i++) {
                    let item = items[i]
                    if (!item.deployment__active) {
                        continue
                    }

                    if (item.owner__username.toLowerCase() !== internal.landscape.toLowerCase()) {
                        continue
                    }

                    forms.push(item)
                }

                if (forms.length > 0) {
                    internal.forms = forms
                    appPageStack.push(formsPage)
                    return
                }
            }
        }

        showError(qsTr("No forms found"))
    }

    function connectForms() {
        for (let i = 0; i < internal.forms.length; i++) {
            let form = internal.forms[i]
            if (form.__selected !== true) {
                continue
            }

            let response = Utils.httpGetWithToken(internal.account.server + "/api/v2/assets/" + form.uid + "/", internal.token, "Token")

            if (response.status !== 200) {
                showError(qsTr("Error retrieving form"))
                return
            }

            let formObj = JSON.parse(response.data)

            let params = {
                account: page.accountName,
                token: internal.token,
                formId: formObj.uid
            }

            let result = App.projectManager.bootstrap("KoBo", params, false)
            if (!result.success && !result.expected) {
                showError(result.errorString)
                return
            }
        }

        App.projectManager.projectsChanged()

        postPopToRoot()
    }
}
