import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtMultimedia 5.12
import QtQuick.Controls.Material 2.12
import QZXing 3.2
import CyberTracker 1.0 as C

Item {
    id: root

    property string recordUid
    property string fieldUid

    signal tagFound(string tag)

    Component.onCompleted: zxingFilter.active = true

    C.FieldBinding {
        id: fieldBinding
        recordUid: root.recordUid
        fieldUid: root.fieldUid
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
                return videoOutput.mapRectToSource(videoOutput.mapNormalizedRectToItem(Qt.rect(0.15, 0.15, 0.85, 0.85)))
            }

            decoder {
                enabledDecoders: QZXing.DecoderFormat_EAN_13 | QZXing.DecoderFormat_CODE_39 | QZXing.DecoderFormat_QR_CODE | QZXing.DecoderFormat_CODE_128 | QZXing.DecoderFormat_CODE_128_GS1
                onTagFound: {
                    zxingFilter.active = false

                    console.log(tag + " | " + decoder.foundedFormat() + " | " + decoder.charSet())

                    if (fieldBinding.fieldUid !== "") {
                        fieldBinding.setValue(tag)
                    }

                    root.tagFound(tag)
                }
                tryHarder: false
            }
        }
    }

    Rectangle {
        z: 1
        anchors.centerIn: parent
        width: parent.width * 0.7
        height: parent.height * 0.7
        color: "white"
        opacity: 0.15
    }
}
