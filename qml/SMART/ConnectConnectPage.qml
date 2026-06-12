import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C
import ".."

C.ContentPage {
    id: page

    QtObject {
        id: internal

        property var server
        property var packages
    }

    header: C.PageHeader {
        text: "SMART " + qsTr("Connect")
    }

    C.LoginItem {
        id: loginItem
        anchors.fill: parent
        iconSource: App.settings.darkTheme ? "qrc:/SMART/logoDark.svg" : "qrc:/SMART/logo.svg"
        provider: "SMART"
        cacheKey: "SMARTConnect"

        onLoginClicked: {
            internal.server = Utils.makeUrlPath(loginItem.server, "/server/api/cybertracker/packages")
            busyCover.doWork = getPackages
            busyCover.start()
        }
    }

    Component {
        id: packagesPage

        ConnectFormPage {
            title: qsTr("Select packages")
            forms: internal.packages
            setFormSelected: function(index, value) { internal.packages[index].__selected = value }
            getFormName: function(form) { return form.name; }
            getFormSubText: function(form) { return App.formatDateTime(form.uploadedDate); }
            connectForms: page.connectPackages
        }
    }

    function getPackages() {
        var response = Utils.httpGet(internal.server, loginItem.username, loginItem.password)

        switch (response.status) {
        case 200:
            internal.packages = JSON.parse(response.data)
            if (internal.packages.length > 0) {
                internal.packages.sort(sortCompareByName)
                appPageStack.push(packagesPage)
            } else {
                loginItem.errorText = qsTr("No packages found")
            }
            break

        case 500:
            loginItem.errorText = qsTr("SMART 7+ server not found")
            break

        default:
            loginItem.errorText = loginItem.badUsernameOrPassword
            break
        }
    }

    function connectPackages() {
        for (let i = 0; i < internal.packages.length; i++) {
            let p = internal.packages[i]
            if (p.__selected !== true) {
                continue
            }

            let params = {
                server: internal.server + "/" + p.uuid,
                username: loginItem.username,
                password: loginItem.password
            }

            let result = App.projectManager.bootstrap("SMART", params)
            if (!result.success) {
                return
            }

            let project = App.projectManager.load("SMART_" + p.uuid)

            if (!getRoutes(project)) {
                App.showError(qsTr("Error installing routes"))
                return
            }
        }

        postPopToRoot()
    }

    function getRoutes(project) {
        let navigationUrl = project.connectorParams.navigation_url
        if (navigationUrl === undefined || navigationUrl === "") {
            return true
        }

        let response = Utils.httpGet(navigationUrl + "?api_key=" + project.accessToken)
        let routes = []

        switch (response.status) {
        case 200:
            routes = JSON.parse(response.data)
            break
        default:
            console.log("Failed to get navigation packages: " + response.status)
            return false
        }

        let api_key = project.accessToken
        for (let i = 0; i < routes.length; i++) {
            let route = routes[i]

            let zipUrl = navigationUrl + "/" + route.uuid + "?api_key=" + api_key
            let zipFile = App.downloadFile(zipUrl)
            if (zipFile !== "") {
                App.installPackage(zipFile, false)
            } else {
                console.log("Bootstrap route: " + route.uuid)
            }
        }

        return true
    }
}
