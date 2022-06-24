import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Pane {
    padding: 4
    contentWidth: width - 8
    contentHeight: height - 8

    GridView {
        id: root
        anchors.fill: parent

        cellWidth: width / 3
        cellHeight: width / 3

        model: ListModel {
            ListElement {
                name: "QR code"
                icon: "qrc:/icons/qrcode_scan.svg"
                page: "qrc:/ConnectQRCodePage.qml"
            }

            ListElement {
                name: "CyberTracker"
                icon: "qrc:/Classic/logo.svg"
                page: "qrc:/Classic/ConnectPage.qml"
            }

            ListElement {
                name: "SMART"
                icon: "qrc:/SMART/logo.svg"
                iconDark: "qrc:/SMART/logoDark.svg"
                page: "qrc:/SMART/ConnectPage.qml"
            }

            ListElement {
                name: "EarthRanger"
                icon: "qrc:/EarthRanger/logo.svg"
                iconDark: "qrc:/EarthRanger/logoDark.svg"
                page: "qrc:/EarthRanger/ConnectPage.qml"
            }

            ListElement {
                name: "ODK"
                icon: "qrc:/ODK/logo.svg"
                page: "qrc:/ODK/ConnectODKCentralPage.qml"
            }

            ListElement {
                name: "KoBoToolbox"
                icon: "qrc:/KoBo/logo.svg"
                page: "qrc:/ODK/ConnectKoBoPage.qml"
            }

            ListElement {
                name: "Survey123"
                icon: "qrc:/Esri/Survey123.svg"
                page: "qrc:/Esri/ConnectPage.qml"
            }

            ListElement {
                name: "Trillion Trees"
                icon: "qrc:/TrillionTrees/logo.svg"
                page: "qrc:/ODK/ConnectKoBoSharedPage.qml"
                account: "trilliontrees"
            }

            ListElement {
                name: "ConSoSci"
                icon: "qrc:/ConSoSci/logo.svg"
                page: "qrc:/ODK/ConnectKoBoSharedPage.qml"
                account: "consosci"
            }
        }

        delegate: RoundButton {
            flat: true
            width: root.cellWidth
            height: root.cellHeight
            icon.source: App.settings.darkTheme && model.iconDark !== undefined ? model.iconDark : model.icon
            icon.width: root.cellWidth / 2
            icon.height: root.cellWidth / 2
            icon.color: model.icon.startsWith("qrc:/icons/") ? Utils.changeAlpha(Material.foreground, 75) : "transparent"
            display: RoundButton.TextUnderIcon
            text: model.name
            font.preferShaping: !text.includes("Cy") // Hack to fix iOS bug
            font.pixelSize: App.settings.font12
            font.capitalization: Font.MixedCase
            radius: 0

            Binding { target: background; property: "color"; value: Utils.changeAlpha("black", 50) }

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
