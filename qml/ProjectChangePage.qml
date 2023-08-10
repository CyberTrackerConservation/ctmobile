import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Change %1").arg(App.alias_project)
    }

    C.ListViewV {
        anchors.fill: parent

        currentIndex: -1
        model: C.ProjectListModel {}
        delegate: C.ListRowDelegate {
            width: ListView.view.width
            iconSource: modelData.icon !== "" ? App.projectManager.getFileUrl(modelData.uid, modelData.icon) : ""
            text: modelData.title
            subText: modelData.subtitle
            onClicked: {
                let projectUid = modelData.uid
                let app = appWindow
                form.popPagesToStart()
                app.changeToProject(projectUid)
            }
        }
    }
}
