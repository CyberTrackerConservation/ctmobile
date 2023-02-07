import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    QtObject {
        id: internal

        property var server
        property var packages
        property var selectedPackage
        property var routes
        property var selectedProject
    }

    header: C.PageHeader {
        text: "SMART " + qsTr("Connect")
    }

    C.LoginItem {
        id: loginItem
        anchors.fill: parent
        iconSource: App.settings.darkTheme ? "qrc:/SMART/logoDark.svg" : "qrc:/SMART/logo.svg"
        provider: "SMART"
        //server: "https://connect7.refractions.net:8443"
        cacheKey: "SMARTConnect"

        onLoginClicked: {
            internal.server = Utils.makeUrlPath(loginItem.server, "/server/api/cybertracker/packages")
            busyCover.doWork = getPackages
            busyCover.start()
        }
    }

    Component {
        id: packagesPage

        C.ContentPage {
            header: C.PageHeader {
                text: qsTr("Select package")
            }

            C.ListViewV {
                anchors.fill: parent
                model: internal.packages
                delegate: C.ListRowDelegate {
                    width: ListView.view.width
                    text: modelData.name
                    subText: App.formatDateTime(modelData.uploadedDate)
                    onClicked: {
                        internal.selectedPackage = modelData
                        busyCover.doWork = connectPackage
                        busyCover.start()
                    }
                }
            }
        }
    }

    Component {
        id: routesPage

        C.ContentPage {
            header: C.PageHeader {
                text: qsTr("Select routes")
                menuIcon: "qrc:/icons/check.svg"
                menuVisible: true
                onMenuClicked: {
                    busyCover.doWork = connectRoutes
                    busyCover.start()
                }
            }

            C.ListViewV {
                property var selectedPackage
                anchors.fill: parent

                model: internal.routes
                delegate: C.ListRowDelegate {
                    width: ListView.view.width
                    checkable: true
                    text: modelData.name
                    subText: App.formatDateTime(modelData.uploadedDate)
                    onCheckedChanged: internal.routes[model.index].checked = checked
                }
            }
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

    function connectPackage() {
        let params = {
            server: internal.server + "/" + internal.selectedPackage.uuid,
            username: loginItem.username,
            password: loginItem.password
        }

        let result = App.projectManager.bootstrap("SMART", params)
        if (!result.success) {
            return
        }

        internal.selectedProject = App.projectManager.load("SMART_" + internal.selectedPackage.uuid)

        if (getRoutes()) {
            return
        }

        postPopToRoot()
    }

    function getRoutes() {
        let project = internal.selectedProject
        var response = Utils.httpGet(project.connectorParams.navigation_url + "?api_key=" + project.connectorParams.api_key)

        switch (response.status) {
        case 200:
            internal.routes = JSON.parse(response.data)
            if (internal.routes.length > 0) {
                internal.routes.sort(sortCompareByName)
                appPageStack.push(routesPage)
                return true
            }
            break
        default:
            console.log("Failed to get navigation packages: " + response.status)
            break
        }

        return false
    }

    function connectRoutes() {
        let connectorParams = internal.selectedProject.connectorParams
        for (let i = 0; i < internal.routes.length; i++) {
            let route = internal.routes[i]
            if (route.checked === true) {
                let zipUrl = connectorParams.navigation_url + "/" + route.uuid + "?api_key=" + connectorParams.api_key
                let zipFile = App.downloadFile(zipUrl)
                if (zipFile !== "") {
                    App.installPackage(zipFile, false)
                } else {
                    console.log("Bootstrap route: " + route.uuid)
                }
            }
        }

        postPopToRoot()
    }
}
