import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C
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
            ListElement { name: qsTr("Global server"); server: "https://kf.kobotoolbox.org" }
            ListElement { name: qsTr("European Union server"); server: "https://eu.kobotoolbox.org" }
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
            setFormSelected: function(index, value) { internal.forms[index].__selected = value }
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
        internal.forms = Utils.getKoBoForms(loginItem.server, internal.token)
        if (internal.forms.length > 0) {
            appPageStack.push(formsPage)
            return
        }

        showError(qsTr("No forms found"))
    }

    function connectForms() {
        for (let i = 0; i < internal.forms.length; i++) {
            let kform = internal.forms[i]
            if (kform.__selected !== true) {
                continue
            }

            let params = {
                server: loginItem.server,
                token: internal.token,
                formId: kform.uid,
                formhubUuid: kform.formhubUuid
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
