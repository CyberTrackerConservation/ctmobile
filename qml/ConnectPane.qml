import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

Pane {
    id: pane
    padding: 0
    contentWidth: width
    contentHeight: height
    Material.background: C.Style.colorContent
    clip: true

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
            icon: "qrc:/Google/forms.svg"
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
        clip: true

        flickableDirection: Flickable.VerticalFlick
        contentHeight: column.height
        contentWidth: column.width

        ColumnLayout {
            id: column
            width: root.width
            spacing: 0

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
                            let params = {}
                            if (model.account !== "") {
                                params.accountName = model.account
                            }
                            appPageStack.push(model.page, params)
                        } else {
                            showToast(qsTr("Coming soon..."))
                        }
                    }
                }
            }

            C.FooterSpacer {}
        }
    }
}
