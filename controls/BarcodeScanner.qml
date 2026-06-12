import QtQuick
import QtQuick.Controls.Material
import QtMultimedia
import ZXing

Item {
    id: pane

    property bool qrCodeOnly: false

    signal tagFound(var tag)

    QtObject {
        id: internal
        property bool first: true
    }

    BarcodeReader {
        id: barcodeReader
        videoSink: videoOutput.videoSink

        formats: qrCodeOnly ? ZXing.QRCode : ZXing.LinearCodes | ZXing.MatrixCodes

        onFoundBarcode: function (barcode) {
            if (internal.first) {
                internal.first = false
                barcodeReader.stop()
                pane.tagFound(barcode.text)
            }
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        focus: visible
        fillMode: VideoOutput.PreserveAspectCrop
    }

    Item {
        id: captureZone
        anchors.fill: parent

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: gismo.top
            color: "black"
            opacity: 0.4
        }

        Rectangle {
            anchors.top: gismo.top
            anchors.left: parent.left
            anchors.right: gismo.left
            anchors.bottom: gismo.bottom
            color: "black"
            opacity: 0.4
        }

        Rectangle {
            anchors.top: gismo.top
            anchors.left: gismo.right
            anchors.right: parent.right
            anchors.bottom: gismo.bottom
            color: "black"
            opacity: 0.4
        }

        Rectangle {
            anchors.top: gismo.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: "black"
            opacity: 0.4
        }

        Item {
            id: gismo
            x: parent.width * barcodeReader.factorsRect.x
            y: parent.height * barcodeReader.factorsRect.y
            width: parent.width * barcodeReader.factorsRect.width
            height: parent.height * barcodeReader.factorsRect.height

            property int value4: App.scaleByFontSize(4)
            property int value16: App.scaleByFontSize(16)

            property int gismowidth: gismo.width

            // Borders
            Item {
                anchors.top: parent.top
                anchors.left: parent.left
                width: gismo.value16; height: gismo.value16;

                Rectangle { width: gismo.value16; height: gismo.value4; color: "white"; }
                Rectangle { width: gismo.value4; height: gismo.value16; color: "white"; }
            }
            Item {
                anchors.top: parent.top
                anchors.right: parent.right
                width: gismo.value16; height: gismo.value16;

                Rectangle { width: gismo.value16; height: gismo.value4; color: "white"; }
                Rectangle { width: gismo.value4; height: gismo.value16; color: "white";
                            anchors.right: parent.right; }
            }
            Item {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                width: gismo.value16; height: gismo.value16;

                Rectangle { width: gismo.value16; height: gismo.value4; color: "white";
                            anchors.bottom: parent.bottom; }
                Rectangle { width: gismo.value4; height: gismo.value16; color: "white";
                            anchors.bottom: parent.bottom; }
            }
            Item {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                width: gismo.value16; height: gismo.value16;

                Rectangle { width: gismo.value16; height: gismo.value4; color: "white";
                            anchors.bottom: parent.bottom; }
                Rectangle { width: gismo.value4; height: gismo.value16; color: "white";
                            anchors.bottom: parent.bottom; anchors.right: parent.right; }
            }

            // Scanner (horizontal)
            Rectangle {
                id: scanBar
                width: 2
                height: parent.height
                anchors.verticalCenter: parent.verticalCenter

                color: "white"
                opacity: 0.66

                SequentialAnimation {
                    running: visible
                    loops: -1
                    NumberAnimation { target: scanBar; property: "x"; from: 0; to: gismo.gismowidth; duration: 750; }
                    NumberAnimation { target: scanBar; property: "x"; from: gismo.gismowidth; to: 0; duration: 750; }
                }
            }
        }
    }
}
