import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: "CyberTracker"
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
                subText: qsTr("Scan for projects installed by CyberTracker desktop")
                wrapSubText: true
                iconSource: "qrc:/icons/monitor.svg"
                iconOpacity: 0.5
                iconColor: Material.foreground
                visible: Qt.platform.os !== "ios"
                onClicked: {
                    if (App.requestPermissionReadExternalStorage() && App.requestPermissionWriteExternalStorage()) {
                        busyCover.doWork = connectClassicDesktop
                        busyCover.start()
                    }
                }
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                text: qsTr("Web install")
                subText: qsTr("Install a project from a web source. Requires a web link from the project publisher.")
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
