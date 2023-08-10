import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtMultimedia 5.12
import QtQuick.Controls.Material 2.12
import QZXing 3.2
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    Component.onCompleted: {
        App.requestPermissionCamera()
        zxingFilter.active = true
    }

    header: C.PageHeader {
        text: qsTr("Scan QR code")
    }

    Item {
        anchors.fill: parent

        Camera {
            id: camera
            focus {
                focusMode: CameraFocus.FocusContinuous
                focusPointMode: CameraFocus.FocusPointAuto
            }
        }

        VideoOutput {
            id: videoOutput
            source: camera
            anchors.fill: parent
            autoOrientation: true
            fillMode: VideoOutput.PreserveAspectCrop
            filters: [zxingFilter]

// Just turns off auto-focus which does not work well.
//            MouseArea {
//                anchors.fill: parent
//                onClicked: {
//                    camera.focus.customFocusPoint = Qt.point(mouse.x / width, mouse.y / height)
//                    camera.focus.focusMode = CameraFocus.FocusMacro
//                    camera.focus.focusPointMode = CameraFocus.FocusPointCustom
//                }
//            }
        }

        QZXingFilter {
            id: zxingFilter

            captureRect: {
                videoOutput.contentRect
                videoOutput.sourceRect
                return videoOutput.mapRectToSource(videoOutput.mapNormalizedRectToItem(Qt.rect(0.0, 0.0, 1.0, 1.0)))
            }

            decoder {
                enabledDecoders: QZXing.DecoderFormat_QR_CODE
                onTagFound: function (tag) {
                    postPopToRoot()
                    App.runQRCode(tag)
                }

                tryHarder: true
            }
        }
    }
}
