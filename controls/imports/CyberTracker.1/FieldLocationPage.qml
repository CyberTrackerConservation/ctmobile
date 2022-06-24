import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

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
