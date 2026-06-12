import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

LocationPage {
    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    hideDataLayers: true

    C.FieldBinding {
        id: fieldBinding
    }

    flagLocation: fieldBinding.value
    onFlagLocationChanged: {
        fieldBinding.setValue(flagLocation)
    }
}
