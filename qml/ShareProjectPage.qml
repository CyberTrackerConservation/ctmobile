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

    ColumnLayout {
        id: column
        anchors.centerIn: parent
        width: parent.width * 0.9
        height: parent.height * 0.9

        Image {
            id: qrCodeImage
            Layout.fillWidth: true
            Layout.fillHeight: true
            fillMode: Image.PreserveAspectFit
            source: {
                authSwitch.checked
                let data = App.projectManager.getShareUrl(project.uid, authSwitch.checked)
                let size = Math.min(width, height)
                return size !== 0 ? "data:image/png;base64," + Utils.renderQRCodeToPng(data, size, size) : ""
            }
        }

        Switch {
            id: authSwitch

            checked: true
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Require login")
            font.pixelSize: App.settings.font14
            visible: App.projectManager.canShareAuth(project.uid)
        }
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Share link")
            icon.source: "qrc:/icons/link.svg"
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

        C.FooterButton {
            text: qsTr("Share QR code")
            icon.source: "qrc:/icons/qrcode.svg"
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
}
