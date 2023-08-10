import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

ItemDelegate {
    id: root

    property string recordUid
    property string fieldUid

    C.FieldBinding {
        id: fieldBinding
        recordUid: root.recordUid
        fieldUid: root.fieldUid
    }

    padding: 0

    contentItem: Item {
        width: root.width
        height: root.height

        Image {
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            source: buildPhotoModel()[0] !== "" ? App.getMediaFileUrl(buildPhotoModel()[0]) : ""
            visible: buildPhotoModel()[0] !== ""
            autoTransform: true
            cache: false
        }

        RoundButton {
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }

            visible: buildPhotoModel()[0] !== ""
            icon.source: "qrc:/icons/delete_outline.svg"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.color: Utils.changeAlpha(Material.foreground, 128)
            onClicked: {
                popupConfirmDelete.open()
            }
        }

        SquareIcon {
            anchors.centerIn: parent
            visible: buildPhotoModel()[0] === ""
            source: "qrc:/icons/camera_alt.svg"
            size: Style.iconSize64
            opacity: 0.5
        }
    }

    onClicked: {
        fieldBinding.setValue(buildPhotoModel())
        form.pushPage(Qt.resolvedUrl("FieldCameraPage.qml"), { photoIndex: 0, recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
    }

    ConfirmPopup {
        id: popupConfirmDelete
        text: qsTr("Delete the image?")
        confirmText: qsTr("Yes, delete it")
        onConfirmed: {
            App.removeMedia(buildPhotoModel()[0])
            fieldBinding.resetValue()
        }
    }

    function buildPhotoModel() {
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
