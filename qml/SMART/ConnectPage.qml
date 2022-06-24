import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: "SMART"
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
                subText: qsTr("Scan for projects installed by SMART Desktop")
                wrapSubText: true
                iconSource: "qrc:/icons/monitor.svg"
                iconOpacity: 0.5
                iconColor: Material.foreground
                visible: Qt.platform.os !== "ios"
                onClicked: {
                    if (App.requestPermissionReadExternalStorage() && App.requestPermissionWriteExternalStorage()) {
                        busyCover.doWork = boostrapDesktop
                        busyCover.start()
                    }
                }
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                text: qsTr("Connect")
                subText: qsTr("Install a project from SMART Connect")
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
                subText: qsTr("Install a project from SMART Collect")
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
