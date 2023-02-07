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
        Image {
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            source: buildPhotoModel()[0] !== "" ? App.getMediaFileUrl(buildPhotoModel()[0]) : ""
            visible: buildPhotoModel()[0] !== ""
            autoTransform: true
            cache: false
        }

        SquareIcon {
            anchors.centerIn: parent
            visible: buildPhotoModel()[0] === ""
            source: "qrc:/icons/camera_alt.svg"
            size: Style.iconSize48
            opacity: 0.5
            recolor: true
        }
    }

    onClicked: {
        fieldBinding.setValue(buildPhotoModel())
        form.pushPage(Qt.resolvedUrl("FieldCameraPage.qml"), { photoIndex: 0, recordUid: fieldBinding.recordUid, fieldUid: fieldBinding.fieldUid } )
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
