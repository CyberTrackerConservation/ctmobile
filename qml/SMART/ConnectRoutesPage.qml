import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    property var routes

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
        anchors.fill: parent
        model: page.routes
        delegate: C.ListRowDelegate {
            width: ListView.view.width
            checkable: true
            text: modelData.name
            subText: App.formatDateTime(modelData.uploadedDate)
            onCheckedChanged: {
                page.routes = JSON.parse(JSON.stringify(page.routes))
                page.routes[model.index].checked = checked
            }
        }
    }

    function connectRoutes() {
        let connectorParams = form.project.connectorParams
        let api_key = form.project.accessToken
        let connectedCount = 0
        for (let i = 0; i < page.routes.length; i++) {
            let route = page.routes[i]
            if (route.checked === true) {
                let zipUrl = connectorParams.navigation_url + "/" + route.uuid + "?api_key=" + api_key
                let zipFile = App.downloadFile(zipUrl)
                if (zipFile !== "") {
                    if (App.installPackage(zipFile, false).success) {
                        connectedCount++
                    }
                } else {
                    console.log("Bootstrap route: " + route.uuid)
                }
            }
        }

        showToast(connectedCount + " " + (connectedCount === 1 ? qsTr("route installed") : qsTr("routes installed")))

        popPageImmediate(page)
    }
}
