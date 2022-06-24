import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import QtQuick.Dialogs 1.2
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var project

    header: C.PageHeader {
        text: project.title
    }

    Component.onCompleted: rebuild()

    ColumnLayout {
        id: column
        anchors.centerIn: parent
        width: parent.width * 0.85
        spacing: 32

        Image {
            id: qrCodeImage
            Layout.fillWidth: true
            height: width
        }

        Switch {
            id: authSwitch

            checked: true
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Require login")
            font.pixelSize: App.settings.font14
            visible: App.projectManager.canShareAuth(project.uid)
            onClicked: rebuild()
        }
    }

    footer: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
            opacity: Material.theme === Material.Dark ? 0.12 : 0.12
        }

        RowLayout {
            id: buttonRow
            spacing: 0
            Layout.fillWidth: true
            property int buttonCount: 2
            property int buttonWidth: page.width / buttonCount
            property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillHeight: true
                text: qsTr("Share link")
                icon.source: "qrc:/icons/link.svg"
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                Material.foreground: buttonRow.buttonColor
                onClicked: {
                    let qrCodeData = App.projectManager.getShareUrl(project.uid, authSwitch.checked)

                    if (App.desktopOS) {
                        App.copyToClipboard(qrCodeData)
                        showToast(qsTr("Copied to clipboard"))
                        return
                    }

                    App.share(qrCodeData, project.title)
                }
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillHeight: true
                text: qsTr("Share QR code")
                icon.source: "qrc:/icons/qrcode.svg"
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                Material.foreground: buttonRow.buttonColor
                onClicked: {
                    if (App.desktopOS) {
                        fileDialogQRCode.open()
                        return
                    }

                    let qrCodeFile = App.projectManager.createQRCode(project.uid, authSwitch.checked)
                    App.sendFile(qrCodeFile, project.title)
                }
            }
        }
    }

    FileDialog {
        id: fileDialogQRCode

        defaultSuffix: "png"
        title: qsTr("Save QR code")
        folder: shortcuts.desktop
        nameFilters: [ "PNG files (*.png)" ]
        selectExisting: false
        onAccepted: {
            let qrCodeFile = App.projectManager.createQRCode(project.uid, authSwitch.checked)
            let success = Utils.copyFile(qrCodeFile, Utils.urlToLocalFile(fileDialogQRCode.fileUrl))
            if (success) {
                showToast(qsTr("QR code saved"))
            } else {
                showError(qsTr("Failed to save code"))
            }

            Utils.removeFile(qrCodeFile)
        }
    }

    function rebuild() {
        let data = App.projectManager.getShareUrl(project.uid, authSwitch.checked)
        qrCodeImage.source = "data:image/png;base64," + Utils.renderQRCodeToPng(data, qrCodeImage.width, qrCodeImage.width)
    }
}
