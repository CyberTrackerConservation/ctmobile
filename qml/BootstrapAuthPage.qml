import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var params: ({})

    header: C.PageHeader {
        text: qsTr("Connect")
    }

    C.LoginItem {
        id: loginItem
        anchors.fill: parent
        iconSource: params.icon
        provider: "SMART"
        serverVisible: params.server === ""
        server: params.server
        username: params.username
        password: params.password
        cacheKey: params.connector + "_BootstrapAuth"
        cachePassword: params.cachePassword === true

        onLoginClicked: {
            busyCover.doWork = bootstrap
            busyCover.start()
        }
    }

    function bootstrap() {
        params.username = loginItem.username
        params.password = loginItem.password

        let result = App.projectManager.bootstrap(params.connector, params)
        if (result.success) {
            let projectUid = App.projectManager.lastNewProjectUid
            let launch = params.launch === true && projectUid !== "" && App.projectManager.exists(projectUid)
            if (launch) {
                changeToProject(projectUid)
            } else {
                postPopToRoot()
            }
        }
    }
}
