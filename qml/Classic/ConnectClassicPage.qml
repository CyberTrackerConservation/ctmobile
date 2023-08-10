import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Connect to") + " CyberTracker Classic"
    }

    Flickable {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: column.height

        ColumnLayout {
            id: column
            width: parent.width
            spacing: 0

            C.ListRowDelegate {
                Layout.fillWidth: true
                text: qsTr("Desktop install")
                subText: qsTr("Scan for %1 installed by CyberTracker desktop").arg(App.alias_projects)
                wrapSubText: true
                iconSource: "qrc:/icons/monitor.svg"
                iconOpacity: 0.5
                iconColor: Material.foreground
                visible: Qt.platform.os !== "ios"
                onClicked: {
                    busyCover.doWork = connectClassicDesktop
                    busyCover.start()
                }
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                text: qsTr("Web install")
                subText: qsTr("Install a %1 from the web. Requires a link from the %2 publisher.").arg(App.alias_project).arg(App.alias_project)
                wrapSubText: true
                iconSource: "qrc:/icons/web.svg"
                iconOpacity: 0.5
                iconColor: Material.foreground
                onClicked: appPageStack.push("ConnectWebInstallPage.qml")
                chevronRight: true
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                text: qsTr("Samples")
                subText: qsTr("Install a sample to see CyberTracker in action. Saved data will not be kept.")
                wrapSubText: true
                iconSource: "qrc:/icons/heart.svg"
                iconOpacity: 0.5
                iconColor: Material.foreground
                onClicked: appPageStack.push("ConnectSamplesPage.qml")
                chevronRight: true
            }
        }
    }

    function connectClassicDesktop() {
        App.projectManager.bootstrap("Classic", {})
        postPopToRoot()
    }
}
