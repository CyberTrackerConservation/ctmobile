import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import CyberTracker as C

C.ContentPage {
    id: page

    property string url

    header: C.PageHeader {
        text: page.title
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
                let size = Math.min(width, height)
                return size !== 0 ? "data:image/png;base64," + Utils.renderQRCodeToPng(page.url, size, size) : ""
            }
        }
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Share link")
            icon.source: "qrc:/icons/link.svg"
            onClicked: {
                if (App.desktopOS) {
                    App.copyToClipboard(page.url)
                    showToast(qsTr("Copied to clipboard"))
                    return
                }

                App.share(page.url, page.title)
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

                let qrCodeFile = App.createQRCodeFile(page.url)
                App.sendFile(qrCodeFile, page.title)
            }
        }
    }

    FileDialog {
        id: fileDialogQRCode

        defaultSuffix: "png"
        title: qsTr("Save QR code")
        currentFolder: StandardPaths.writableLocation(StandardPaths.DesktopLocation)
        nameFilters: [ "PNG files (*.png)" ]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let qrCodeFile = App.createQRCodeFile(page.url)
            let success = Utils.copyFile(qrCodeFile, Utils.urlToLocalFile(fileDialogQRCode.selectedFile))
            if (success) {
                showToast(qsTr("QR code saved"))
            } else {
                showError(qsTr("Failed to save code"))
            }

            Utils.removeFile(qrCodeFile)
        }
    }
}
