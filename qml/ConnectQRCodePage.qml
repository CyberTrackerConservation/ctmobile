import QtCore
import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Scan QR code")
    }

    C.BarcodeScanner {
        anchors.fill: parent

        qrCodeOnly: true

        onTagFound: function (tag) {
            postPopToRoot()
            App.runQRCode(tag)
        }
    }
}
