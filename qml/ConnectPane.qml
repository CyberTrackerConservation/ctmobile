import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Pane {
    id: pane
    padding: 0
    contentWidth: width
    contentHeight: height
    Material.background: C.Style.colorContent

    ListModel {
        id: connectors
        ListElement {
            name: "CyberTracker Online"
            subText: "https://online.cybertracker.org"
            icon: "qrc:/app/appicon.svg"
            page: "qrc:/Classic/ConnectOnlinePage.qml"
        }

        ListElement {
            name: "CyberTracker Classic"
            subText: "https://cybertrackerwiki.org/classic"
            icon: "qrc:/Classic/logo.svg"
            page: "qrc:/Classic/ConnectClassicPage.qml"
        }

        ListElement {
            name: "SMART"
            subText: "https://smartconservationtools.org"
            icon: "qrc:/SMART/logo.svg"
            iconDark: "qrc:/SMART/logoDark.svg"
            page: "qrc:/SMART/ConnectPage.qml"
        }

        ListElement {
            name: "EarthRanger"
            subText: "https://earthranger.com"
            icon: "qrc:/EarthRanger/logo.svg"
            iconDark: "qrc:/EarthRanger/logoDark.svg"
            page: "qrc:/EarthRanger/ConnectPage.qml"
        }

        ListElement {
            name: "Google Forms"
            subText: "https://rillaforms.com"
            icon: "qrc:/Google/logo.svg"
            page: "qrc:/Google/ConnectPage.qml"
        }

        ListElement {
            name: "KoBoToolbox"
            subText: "https://kobotoolbox.org"
            icon: "qrc:/KoBo/logo.svg"
            page: "qrc:/ODK/ConnectKoBoPage.qml"
        }

        ListElement {
            name: "ODK Central"
            subText: "https://getodk.org"
            icon: "qrc:/ODK/logo.svg"
            page: "qrc:/ODK/ConnectODKCentralPage.qml"
        }

        ListElement {
            name: "Survey123"
            subText: "https://survey123.com"
            icon: "qrc:/Esri/Survey123.svg"
            page: "qrc:/Esri/ConnectPage.qml"
        }

        ListElement {
            name: "Trillion Trees"
            subText: "https://trilliontrees.org"
            icon: "qrc:/TrillionTrees/logo.svg"
            page: "qrc:/ODK/ConnectKoBoSharedPage.qml"
            account: "trilliontrees"
        }

        ListElement {
            name: "ConSoSci"
            subText: "https://consosci.org"
            icon: "qrc:/ConSoSci/logo.svg"
            page: "qrc:/ODK/ConnectKoBoSharedPage.qml"
            account: "consosci"
        }
    }

    Flickable {
        id: root
        anchors.fill: parent

        flickableDirection: Flickable.VerticalFlick
        contentHeight: column.height
        contentWidth: column.width

        ColumnLayout {
            id: column
            width: root.width
            spacing: 0

            C.ListRowDelegate {
                Layout.fillWidth: true
                iconSource: "qrc:/icons/qrcode_scan.svg"
                text: qsTr("Scan QR code")
                subText: qsTr("Scan QR code to install a %1").arg(App.alias_Project)
                iconOpacity: 0.5
                iconColor: Material.foreground
                wrapSubText: true
                chevronRight: true
                onClicked: {
                    appPageStack.push("qrc:/ConnectQRCodePage.qml")
                }
            }

            Repeater {
                Layout.fillWidth: true

                model: connectors

                C.ListRowDelegate {
                    Layout.fillWidth: true
                    text: model.name
                    subText: model.subText
                    iconSource: model.icon
                    chevronRight: true

                    onClicked: {
                        if (model.page !== undefined) {
                            appPageStack.push(model.page, { accountName: model.account })
                        } else {
                            showToast(qsTr("Coming soon..."))
                        }
                    }
                }
            }
        }
    }
}
