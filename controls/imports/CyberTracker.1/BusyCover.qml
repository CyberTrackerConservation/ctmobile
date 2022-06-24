import QtQml 2.12
import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12

Popup {
    id: busyCover

    property var params: ({})

    width: parent.width
    height: parent.height
    parent: Overlay.overlay

    Component.onCompleted: {
        if (typeof(formPageStack) !== "undefined") {
            formPageStack.setProjectColors(busyCover)
        }
    }

    property var doWork

    background: Rectangle {
        implicitWidth: busyCover.width
        implicitHeight: busyCover.height
        opacity: 0.5
        color: "white"
    }

    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        running: false
    }

    ProgressBar {
        id: progressBar
        x: busyIndicator.x - 32
        y: busyIndicator.y + busyIndicator.height + 32
        width: busyIndicator.width + 64
        visible: false
    }

    onOpened: {
        busyIndicator.running = true
        if (busyCover.doWork !== undefined) {
            workTimer.restart()
        }
    }

    onClosed: {
        progressBar.visible = false

        // Clear params in case it contains references to objects
        busyCover.params = ({})
    }

    Timer {
        id: workTimer
        interval: 100
        repeat: false
        running: false
        onTriggered: {
            busyCover.doWork()
            busyCover.doWork = undefined
            busyIndicator.running = false
            busyCover.close()
        }
    }

    function start() {
        open()
    }

    function stop() {
        busyIndicator.running = false
        busyCover.close();
    }

    Connections {
        target: App

        function onProgressEvent(projectUid, operationName, index, count) {
            if (busyCover.opened) {
                progressBar.value = index / count
                progressBar.visible = true
            }
        }
    }
}
