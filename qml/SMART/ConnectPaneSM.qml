import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Pane {
    id: pane
    anchors.fill: parent

    Material.background: C.Style.colorContent
    contentWidth: flickable.contentWidth
    contentHeight: Math.min(height, flickable.contentHeight)
    leftPadding: (width - flickable.contentWidth) / 2
    topPadding: flickable.contentHeight < pane.height ? (height - flickable.contentHeight) / 2 : App.scaleByFontSize(8)

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
                subText: qsTr("Scan for desktop %1").arg(App.alias_projects)
                wrapSubText: true
                visible: Qt.platform.os !== "ios"
                onClicked: {
                    busyCover.doWork = bootstrapSMARTDesktop
                    busyCover.start()
                }
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                iconSource: "qrc:/icons/cellphone_wireless.svg"
                text: "SMART Connect"
                divider: false
                iconOpacity: 0.5
                iconColor: Material.foreground
                subText: qsTr("Connect to an online %1").arg(App.alias_project)
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
                subText: qsTr("Connect to a community %1").arg(App.alias_project)
                wrapSubText: true
                onClicked: appPageStack.push("qrc:/SMART/ConnectCollectPage.qml")
            }

            C.ListRowDelegate {
                Layout.fillWidth: true
                iconSource: "qrc:/icons/qrcode_scan.svg"
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
