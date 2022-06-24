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
        property string accessToken
        property string refreshToken
        property var forms: []
    }

    header: C.PageHeader {
        text: qsTr("Connect to") + " ESRI"
    }

    C.LoginItem {
        id: loginItem
        anchors.fill: parent
        provider: "Esri"
        serverVisible: false
        cacheKey: "Esri"
        iconSource: "qrc:/Esri/logo.svg"
        onLoginClicked: {
            busyCover.doWork = getToken
            busyCover.start()
        }
    }

    Component {
        id: formsPage

        ConnectFormPage {
            forms: internal.forms
            title: qsTr("Select surveys")
            getFormName: function(form) { return form.title; }
            getFormSubText: function(form) { return App.formatDateTime(form.modified); }
            connectForms: page.connectForms
        }
    }

    function getToken() {
        let result = Utils.esriAcquireOAuthToken(loginItem.username, loginItem.password)
        if (!result.success) {
            console.log(result.errorString)
            showError(result.errorString)
            return
        }

        internal.accessToken = result.accessToken
        internal.refreshToken = result.refreshToken

        getForms()
    }

    function getForms() {
        let result = Utils.esriFetchSurveys(internal.accessToken)
        if (!result.success) {
            console.log(result.errorString)
            showError(result.errorString)
            return
        }

        internal.forms = result.content.results
        if (internal.forms.length == 0) {
            showError(qsTr("No surveys found"))
            return
        }

        appPageStack.push(formsPage)
    }

    function connectForms() {
        for (let i = 0; i < internal.forms.length; i++) {
            let form = internal.forms[i]
            if (form.__selected !== true) {
                continue
            }

            let params = {
                "username": loginItem.username,
                accessToken: internal.accessToken,
                refreshToken: internal.refreshToken,
                surveyId: form.id
            }

            let result = App.projectManager.bootstrap("Esri", params, false)
            if (!result.success && !result.expected) {
                showError(result.errorString)
                return
            }
        }

        App.projectManager.projectsChanged()

        postPopToRoot()
    }
}
