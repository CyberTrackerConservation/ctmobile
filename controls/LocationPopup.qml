import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtPositioning
import CyberTracker as C

PopupBase {
    id: popup

    enum State {
        StateInit,
        StateAcquire,
        StateTriangle,
        StateDone
    }

    property int fixCount: 4
    property int skipTimeout: 0
    property bool skipAllowed: true
    property bool showTriangle: false
    property color color: Style.colorContent
    property bool skipVisible: false
    property var contextData: ({})

    QtObject {
        id: internal
        property var lastLocation
        property int state: LocationPopup.StateInit
    }

    closePolicy: Popup.NoAutoClose
    closeOnBack: false

    onOpened: {
        App.locationLogger.updateState()
        //App.locationLogger.state === C.LocationLogger.StateFix ? 1 :

        internal.state = LocationPopup.StateAcquire
        positionSource.active = true

        if (skipTimeout > 0) {
            skipTimer.interval = skipTimeout * 1000
            skipTimer.start()
        } else {
            skipVisible = true
        }
    }

    insets: 0

    width: formPageStack.width
    height: formPageStack.height

    leftMargin: appWindow.SafeArea.margins.left
    topMargin: appWindow.SafeArea.margins.top
    rightMargin: appWindow.SafeArea.margins.right
    bottomMargin: App.settings.fullScreen ? 0 : appWindow.SafeArea.margins.bottom

    contentItem: Item {
        C.Skyplot {
            id: skyplot
            anchors.centerIn: parent
            visible: internal.state === LocationPopup.StateAcquire
            active: internal.state === LocationPopup.StateAcquire
            compassVisible: false
            width: Math.min(popup.width, popup.height) * 0.7
            height: Math.min(popup.width, popup.height) * 0.7
            progressVisible: true
            legendVisible: false
            satNumbersVisible: false
        }

        C.SquareIcon {
            id: triangle
            anchors.centerIn: parent
            visible: internal.state === LocationPopup.StateTriangle
            source: "qrc:/icons/triangle.svg"
            size: Math.min(popup.width, popup.height) * 0.5
        }

        ToolButton {
            id: buttonSkip
            visible: internal.state === LocationPopup.StateAcquire && popup.skipAllowed && popup.skipVisible && !doneTimer.running
            icon.source: "qrc:/icons/edit_location_outline.svg"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            font.pixelSize: App.settings.font16
            font.capitalization: Font.AllUppercase
            width: Math.max(implicitWidth, parent.width / 2)
            text: qsTr("Skip GPS")

            anchors {
                bottom: buttonCancel.top
                bottomMargin: App.scaleByFontSize(8)
                horizontalCenter: parent.horizontalCenter
            }

            onClicked: {
                positionSource.active = false
                doneTimer.stop()
                form.pushPage(Qt.resolvedUrl("LocationPage.qml"), { contextData: popup.contextData })
                popup.close()
            }

            Timer {
                id: skipTimer
                repeat: false
                onTriggered: popup.skipVisible = true
            }
        }

        ToolButton {
            id: buttonCancel
            visible: internal.state === LocationPopup.StateAcquire
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
        active: false
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

            internal.lastLocation = App.lastLocation.toMap

            if (fixCounter < popup.fixCount) {
                fixCounter++
                skyplot.progressPercent = (fixCounter * 100) / popup.fixCount
                return
            }

            popup.enabled = false
            active = false

            if (popup.showTriangle) {
                internal.state = LocationPopup.StateTriangle
                triangleTimer.start()
            } else {
                doneTimer.interval = 500
                doneTimer.start()
            }
        }
    }

    Timer {
        id: triangleTimer
        interval: 2000
        repeat: false
        onTriggered: {
            internal.state = LocationPopup.StateDone
            popup.close()
            sendLocationAcquired(internal.lastLocation)
        }
    }

    Timer {
        id: doneTimer
        repeat: false
        onTriggered: {
            internal.state = LocationPopup.StateDone
            popup.close()
            sendLocationAcquired(internal.lastLocation)
        }
    }

    function sendLocationAcquired(l) {
        App.locationAcquired(l.x, l.y, l.z, l.a, l.ts, popup.contextData)
    }
}
