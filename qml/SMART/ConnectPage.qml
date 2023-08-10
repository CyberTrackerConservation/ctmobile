import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Connect to %1").arg("SMART")
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
                subText: qsTr("Scan for %1 installed by SMART Desktop").arg(App.alias_projects)
                wrapSubText: true
                iconSource: "qrc:/icons/monitor.svg"
                iconOpacity: 0.5
                iconColor: Material.foreground
                visible: Qt.platform.os !== "ios"
                onClicked: {
                    busyCover.doWork = boostrapDesktop
                    busyCover.start()
                }
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                text: qsTr("Connect")
                subText: qsTr("Add a %1 from SMART Connect").arg(App.alias_project)
                wrapSubText: true
                iconSource: "qrc:/icons/cellphone_wireless.svg"
                iconOpacity: 0.5
                iconColor: Material.foreground
                onClicked: appPageStack.push("ConnectConnectPage.qml")
                chevronRight: true
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                text: qsTr("Collect")
                subText: qsTr("Add a %1 from SMART Collect").arg(App.alias_project)
                wrapSubText: true
                iconSource: "qrc:/icons/nature_people.svg"
                iconOpacity: 0.5
                iconColor: Material.foreground
                onClicked: appPageStack.push("ConnectCollectPage.qml")
                chevronRight: true
            }
        }
    }

    function boostrapDesktop() {
        App.projectManager.bootstrap("SMART", {})
        App.projectManager.projectsChanged()
        postPopToRoot()
    }
}
