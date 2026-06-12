import QtQuick
import QtQuick.Controls.Material
import QtPositioning
import CyberTracker as C

Label {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property alias active: positionSource.active
    property int fixCounter: 0
    property int fixCount: 4
    property string placeholderText: ""

    opacity: fieldBinding.value ? 1.0 : 0.5

    Component.onCompleted: {
        positionSource.active = root.active
        root.fixCount = App.locationLogger.state === C.LocationLogger.StateFix ? 1 : fieldBinding.field.fixCount
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

            if (fixCounter < root.fixCount) {
                fixCounter++
                return
            }

            active = false
            fieldBinding.setValue(App.lastLocation.toMap)
        }

        onActiveChanged: fixCounter = 0
    }
}
