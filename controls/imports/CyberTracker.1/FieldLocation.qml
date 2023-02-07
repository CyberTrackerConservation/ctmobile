import QtQuick 2.12
import QtQuick.Controls 2.12
import QtPositioning 5.12
import CyberTracker 1.0 as C

Label {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property alias active: positionSource.active
    property int fixCounter: 0
    property string placeholderText: ""

    opacity: fieldBinding.value ? 1.0 : 0.5

    Component.onCompleted: {
        positionSource.active = root.active
    }

    wrapMode: Label.Wrap

    text: {
        let point = fieldBinding.value
        return point === undefined ? root.placeholderText : App.getLocationText(point.x, point.y)
    }

    C.FieldBinding {
        id: fieldBinding
    }

    PositionSource {
        id: positionSource
        active: false
        name: App.positionInfoSourceName
        updateInterval: 1000
        onPositionChanged: {
            if (!active) {
                return
            }

            if (form.requireGPSTime && !App.timeManager.corrected) {
                console.log("Waiting for time correction")
                return
            }

            if (!App.lastLocationAccurate) {
                showToast(App.locationAccuracyText)
                return
            }

            if (fixCounter < fieldBinding.field.fixCount) {
                fixCounter++
                return
            }

            active = false
            fieldBinding.setValue(App.lastLocation.toMap)
        }

        onActiveChanged: fixCounter = 0
    }
}
