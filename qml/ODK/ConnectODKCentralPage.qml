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
        property string token
        property var projects
        property var selectedProject
        property var forms
    }

    header: C.PageHeader {
        text: "ODK Central Connect"
    }

    C.LoginItem {
        id: loginItem
        anchors.fill: parent
        iconSource: "qrc:/ODK/logo.svg"
        cacheKey: "ODK"

        onLoginClicked: {
            busyCover.doWork = getToken
            busyCover.start()
        }
    }

    Component {
        id: projectsPage

        C.ContentPage {
            header: C.PageHeader {
                text: qsTr("Select project")
            }

            C.ListViewV {
                anchors.fill: parent
                model: internal.projects
                delegate: C.ListRowDelegate {
                    width: ListView.view.width
                    text: modelData.name
                    subText: App.formatDateTime(modelData.createdAt)
                    onClicked: {
                        internal.selectedProject = modelData
                        busyCover.doWork = getForms
                        busyCover.start()
                    }
                    chevronRight: true
                }
            }
        }
    }

    Component {
        id: formsPage

        ConnectFormPage {
            forms: internal.forms
            getFormName: function(form) { return form.name; }
            getFormSubText: function(form) { return App.formatDateTime(form.updatedAt); }
            connectForms: page.connectForms
        }
    }

    function getToken() {
        let response = Utils.httpPostJson(Utils.makeUrlPath(loginItem.server, "/v1/sessions"), { email: loginItem.username, password: loginItem.password })

        if (response.status === 200) {
            internal.token = JSON.parse(response.data).token
            if (internal.token.length > 0) {
                getProjects()
                return
            }
        }

        showError(loginItem.badUsernameOrPassword)
    }

    function getProjects() {
        let response = Utils.httpGetWithToken(Utils.makeUrlPath(loginItem.server, "/v1/projects"), internal.token)

        if (response.status === 200) {
            internal.projects = JSON.parse(response.data)
            if (internal.projects.length > 0) {
                appPageStack.push(projectsPage)
                return
            }
        }

        showError(qsTr("No projects found"))
    }

    function getForms() {
        let response = Utils.httpGetWithToken(Utils.makeUrlPath(loginItem.server, "/v1/projects/" + internal.selectedProject.id + "/forms"), internal.token)

        if (response.status === 200) {
            let forms = JSON.parse(response.data)

            internal.forms = []
            for (let i = 0; i < forms.length; i++) {
                let form = forms[i]
                if (form.state === "open") {
                    internal.forms.push(form)
                }
            }

            if (internal.forms.length > 0) {
                appPageStack.push(formsPage)
                return
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

            let params = {
                server: loginItem.server,
                token: internal.token,
                projectId: internal.selectedProject.id,
                formId: form.xmlFormId
            }

            let result = App.projectManager.bootstrap("ODK", params, false)
            if (!result.success && !result.expected) {
                showError(result.errorString)
                return
            }
        }

        App.projectManager.projectsChanged()

        postPopToRoot()
    }
}
