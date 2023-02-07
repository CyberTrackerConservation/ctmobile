import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Pane {
    id: pane
    anchors.fill: parent

    contentWidth: flickable.contentWidth
    contentHeight: Math.min(height, flickable.contentHeight)
    leftPadding: (width - flickable.contentWidth) / 2
    topPadding: flickable.contentHeight < pane.height ? (height - flickable.contentHeight) / 2 : 4

    contentItem: Flickable {
        id: flickable

        flickableDirection: Flickable.VerticalFlick
        contentHeight: column.height
        contentWidth: column.width

        ColumnLayout {
            id: column
            width: implicitWidth < pane.width * 0.9 ? implicitWidth : pane.width * 0.9

            C.ListRowDelegate {
                Layout.fillWidth: true
                iconSource: "qrc:/icons/monitor.svg"
                text: "SMART Desktop"
                divider: false
                iconOpacity: 0.5
                iconColor: Material.foreground
                subText: qsTr("Scan for desktop projects")
                wrapSubText: true
                visible: Qt.platform.os !== "ios"
                onClicked: {
                    if (App.requestPermissionReadExternalStorage() && App.requestPermissionWriteExternalStorage()) {
                        busyCover.doWork = bootstrapSMARTDesktop
                        busyCover.start()
                    }
                }
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                iconSource: "qrc:/icons/cellphone_wireless.svg"
                text: "SMART Connect"
                divider: false
                iconOpacity: 0.5
                iconColor: Material.foreground
                subText: qsTr("Connect to an online project")
                wrapSubText: true
                onClicked: appPageStack.push("qrc:/SMART/ConnectConnectPage.qml")
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                iconSource: "qrc:/icons/nature_people.svg"
                text: "SMART Collect"
                divider: false
                iconOpacity: 0.5
                iconColor: Material.foreground
                subText: qsTr("Connect to a community project")
                wrapSubText: true
                onClicked: appPageStack.push("qrc:/SMART/ConnectCollectPage.qml")
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                iconSource: "qrc:/icons/qrcode.svg"
                text: qsTr("Scan QR code")
                divider: false
                iconOpacity: 0.5
                iconColor: Material.foreground
                subText: qsTr("Install from a shared QR code")
                wrapSubText: true
                onClicked: appPageStack.push("qrc:/ConnectQRCodePage.qml")
            }
        }
    }

    function bootstrapSMARTDesktop() {
        App.projectManager.bootstrap("SMART", {})
        App.projectManager.projectsChanged()
        postPopToRoot()
    }
}
