import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

GridViewV {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    C.FieldBinding {
        id: fieldBinding
    }

    clip: true
    cellWidth: root.width / columns
    cellHeight: root.width / columns

    model: {
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

    delegate: GridFrameDelegate {
        id: d

        width: root.cellWidth
        height: root.cellHeight
        sideBorder: root.sideBorder
        borderWidth: root.borderWidth
        lines: root.lines
        columns: root.columns
        itemPadding: root.padding

        contentItem: FieldPhotoButton {
            width: parent.width
            height: parent.height
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            photoIndex: model.index
            emptyIcon: model.index === root.model.length - 1 ? "qrc:/icons/camera_plus_outline.svg" : "qrc:/icons/camera_alt.svg"
        }
    }
}
