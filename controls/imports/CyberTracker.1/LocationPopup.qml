import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtPositioning 5.12
import CyberTracker 1.0 as C

PopupBase {
    id: popup

    property int fixCount: 4
    property color color: Style.colorContent

    signal locationFound(var value)

    width: parent.width
    height: parent.height

    Binding { target: background; property: "color"; value: color }

    contentItem: Item {
        C.Skyplot {
            anchors.centerIn: parent

            id: skyplot
            active: true
            compassVisible: false
            width: Math.min(popup.width, popup.height) * 0.7
            height: Math.min(popup.width, popup.height) * 0.7
            progressVisible: true
            legendVisible: false
            satNumbersVisible: false
        }

        ToolButton {
            anchors {
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }

            icon.source: Style.cancelIconSource
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.color: Material.foreground
            onClicked: {
                positionSource.active = false
                doneTimer.stop()
                popup.close()
            }
        }
    }

    PositionSource {
        id: positionSource
        active: true
        name: App.positionInfoSourceName
        updateInterval: 1000

        property int fixCounter: 0

        onPositionChanged: {
            if (!active) {
                return
            }

            if (form.requireGPSTime && !App.timeManager.corrected) {
                App.showToast(qsTr("Waiting for time correction"))
                return
            }

            if (!App.lastLocationAccurate) {
                App.showToast(App.locationAccuracyText)
                return
            }

            if (fixCounter < popup.fixCount) {
                fixCounter++
                skyplot.progressPercent = (fixCounter * 100) / popup.fixCount
                return
            }

            popup.enabled = false
            active = false
            doneTimer.interval = popup.fixCount < 3 ? 2000 : 500
            doneTimer.start()
        }
    }

    Timer {
        id: doneTimer
        repeat: false
        onTriggered: {
            popup.close()
            popup.locationFound(App.lastLocation.toMap)
        }
    }
}
