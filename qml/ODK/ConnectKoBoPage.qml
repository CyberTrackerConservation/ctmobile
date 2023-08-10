import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C
import ".."

C.ContentPage {
    id: page

    QtObject {
        id: internal
        property var userData
        property string token
        property var forms: []
    }

    header: C.PageHeader {
        text: qsTr("Connect to %1").arg("KoBoToolbox")
    }

    C.LoginItem {
        id: loginItem
        anchors.fill: parent
        iconSource: "qrc:/KoBo/logo.svg"
        cacheKey: "KoBo"
        servers: ListModel {
            ListElement { name: qsTr("Humanitarian server"); server: "https://kobo.humanitarianresponse.info" }
            ListElement { name: qsTr("Global server"); server: "https://kf.kobotoolbox.org" }
            ListElement { name: qsTr("Custom"); server: "https://"; custom: true }
        }

        onLoginClicked: {
            busyCover.doWork = getToken
            busyCover.start()
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
        let response = Utils.httpGet(loginItem.server + "/token/?format=json", loginItem.username, loginItem.password)

        if (response.status === 200) {
            internal.token = JSON.parse(response.data).token
            if (internal.token.length > 0) {
                getForms()
                return
            }
        }

        showError(loginItem.badUsernameOrPassword)
    }

    function getForms() {
        let response = Utils.httpGetWithToken(loginItem.server + "/api/v2/assets/?q=asset_type%3Asurvey", internal.token, "Token")

        if (response.status === 200) {
            let result = JSON.parse(response.data)
            if (result.results !== undefined && result.results.length > 0) {
                let items = result.results
                internal.forms = []
                for (let i = 0; i < items.length; i++) {
                    let item = items[i]
                    if (item.deployment__active) {
                        internal.forms.push(item)
                    }
                }

                if (internal.forms.length > 0) {
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

            let response = Utils.httpGetWithToken(loginItem.server + "/api/v2/assets/" + form.uid + "/", internal.token, "Token")

            if (response.status !== 200) {
                showError(qsTr("Error retrieving form"))
                return
            }

            let formObj = JSON.parse(response.data)

            let params = {
                server: loginItem.server,
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
