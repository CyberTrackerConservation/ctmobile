import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

ListViewH {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property int imageSize: 48
    property bool highlightInvalid: false

    spacing: App.scaleByFontSize(4)

    C.FieldBinding {
        id: fieldBinding

        property int changeCounter: 0

        onValueChanged: {
            changeCounter++
            rebuild()
        }
    }

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

                SquareIcon {
                    anchors.centerIn: parent
                    source: "qrc:/icons/camera_alt.svg"
                    size: Style.iconSize48
                }

                layer.enabled: true
                layer.effect: SimpleColorOverlay {
                    color: root.highlightInvalid && !fieldBinding.isValid ? Style.colorInvalid : Material.foreground
                }
            }

            Rectangle {
                color: "blue"
                x: 0
                y: 0
                width: parent.height / 12
                height: width
                visible: {
                    fieldBinding.changeCounter
                    return Utils.getImageTags(App.getMediaFilePath(modelData)).length > 0
                }
            }
        }

        onClicked: {
            form.pushPage(Qt.resolvedUrl("FieldCameraPage.qml"), { photoIndex: model.index, recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
        }
    }

    function rebuild() {
        let value = fieldBinding.getValue([""])
        if (value[value.length - 1] !== "" && value.length < fieldBinding.field.maxCount) {
            value.push("")
        }

        root.model = value
    }

    Component.onCompleted: {
        rebuild()
    }
}
