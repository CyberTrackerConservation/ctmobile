import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

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
            onCheckedChanged: page.routes[model.index].checked = checked
        }
    }

    function connectRoutes() {
        let connectorParams = form.project.connectorParams
        let connectedCount = 0
        for (let i = 0; i < page.routes.length; i++) {
            let route = page.routes[i]
            if (route.checked === true) {
                let zipUrl = connectorParams.navigation_url + "/" + route.uuid + "?api_key=" + connectorParams.api_key
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

        form.popPage()
    }
}
