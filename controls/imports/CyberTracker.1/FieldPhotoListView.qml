import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ListViewH {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property var imageSize: 24
    property bool highlightInvalid: false

    spacing: 4

    C.FieldBinding {
        id: fieldBinding
    }

    model: buildModel()

    delegate: ItemDelegate {
        id: control
        width: imageSize
        height: imageSize
        padding: 0

        contentItem: Item {
            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: modelData !== "" ? App.getMediaFileUrl(modelData) : ""
                sourceSize.width: imageSize
                sourceSize.height: imageSize
                visible: modelData !== ""
                autoTransform: true
                cache: false
            }

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: "black"
                border.width: 2
                opacity: 0.5
                visible: modelData === ""

                Image {
                    anchors.fill: parent
                    fillMode: Image.Pad
                    source: "qrc:/icons/camera_alt.svg"
                    sourceSize.width: imageSize / 2
                    sourceSize.height: imageSize / 2
                }

                layer {
                    enabled: true
                    effect: ColorOverlay {
                        color: root.highlightInvalid && !fieldBinding.isValid ? Style.colorInvalid : Material.foreground
                    }
                }
            }
        }

        onClicked: {
            fieldBinding.setValue(buildModel())
            form.pushPage(Qt.resolvedUrl("FieldCameraPage.qml"), { photoIndex: model.index, recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
        }
    }

    function buildModel() {
        var v = fieldBinding.value
        if (v === undefined || v === null) {
            v = [""]
        } else {
            v = v.filter(e => e !== "")
            if (v.length < fieldBinding.field.maxCount) {
                v.push("")
            }
        }

        return v
    }
}
