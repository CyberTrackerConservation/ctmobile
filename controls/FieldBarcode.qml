import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

Item {
    id: root

    property string recordUid
    property string fieldUid

    signal tagFound(string tag)

    C.FieldBinding {
        id: fieldBinding
        recordUid: root.recordUid
        fieldUid: root.fieldUid
    }

    BarcodeScanner {
        anchors.fill: parent

        onTagFound: function (tag) {

            if (fieldBinding.fieldUid !== "") {
                fieldBinding.setValue(tag)
            }

            root.tagFound(tag)
        }
    }
}
