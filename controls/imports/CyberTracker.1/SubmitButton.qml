import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

RoundButton {
    id: root

    property bool showPopup: false

    implicitWidth: Style.iconSize64
    implicitHeight: Style.iconSize64
    radius: width / 2

    QtObject {
        id: internal
        property int initialPendingUploadCount: 0
        property int currentPendingUploadCount: 0
    }

    state: ""
    states: [
        State {
            name: "done"
            StateChangeScript {
                script: {
                    root.enabled = false
                    timerWiFi.running = timerProgress.running = timerBusy.running = false
                    centerIcon.rotation = 0
                    centerIcon.source = "qrc:/icons/cloud_done.svg"
                    progressCircle.visible = false
                    progressCircle.arcEnd = 0
                    internal.initialPendingUploadCount = internal.currentPendingUploadCount = 0
                }
            }
        },
        State {
            name: "noWiFi"
            StateChangeScript {
                script: {
                    root.enabled = false
                    timerWiFi.running = true
                    timerProgress.running = timerBusy.running = false
                    centerIcon.rotation = 0
                    centerIcon.source = "qrc:/icons/wifi_strength_off_outline.svg"
                    progressCircle.visible = false
                    progressCircle.arcEnd = 0
                    internal.initialPendingUploadCount = internal.currentPendingUploadCount = 0
                }
            }
        },

        State {
            name: "submit"
            StateChangeScript {
                script: {
                    root.enabled = true
                    timerWiFi.running = timerProgress.running = timerBusy.running = false
                    centerIcon.rotation = 0
                    centerIcon.source = "qrc:/icons/upload_multiple.svg"
                    progressCircle.visible = false
                    progressCircle.arcEnd = 0
                    internal.initialPendingUploadCount = internal.currentPendingUploadCount = 0
                }
            }
        },
        State {
            name: "busy"
            StateChangeScript {
                script: {
                    root.enabled = false
                    timerWiFi.running = false
                    timerProgress.running = true
                    timerBusy.running = true
                    centerIcon.rotation = 0
                    centerIcon.source = "qrc:/icons/sync.svg"
                    progressCircle.visible = true
                    progressCircle.arcEnd = 0
                    internal.initialPendingUploadCount = internal.currentPendingUploadCount = form.getPendingUploadCount()
                }
            }
        },
        State {
            name: "fail"
            StateChangeScript {
                script: {
                    root.enabled = true
                    timerWiFi.running = timerProgress.running = timerBusy.running = false
                    centerIcon.rotation = 0
                    centerIcon.source = "qrc:/icons/upload_multiple.svg"
                    progressCircle.visible = false
                    progressCircle.arcEnd = 0
                    internal.initialPendingUploadCount = internal.currentPendingUploadCount = 0
                    App.showError(qsTr("Upload failed"))
                }
            }
        }
    ]

    SquareIcon {
        id: centerIcon
        anchors.fill: parent
        size: parent.width / 2
        opacity: root.enabled ? 0.8 : 0.2
        recolor: true
    }

    ProgressCircle {
        id: progressCircle
        anchors.fill: parent
        size: parent.width
        lineWidth: App.scaleByFontSize(5)
        colorCircle: Material.accent
        colorBackground: Style.colorGroove
        showBackground: true
        arcBegin: 0
        arcEnd: 0
    }

    Timer {
        id: timerWiFi
        interval: 10000
        repeat: true
        onTriggered: {
            updateState()
        }
    }

    Timer {
        id: timerProgress
        interval: 1000
        repeat: true
        onTriggered: {
            if (internal.initialPendingUploadCount !== 0) {
                progressCircle.arcEnd = (360 * (internal.initialPendingUploadCount - internal.currentPendingUploadCount)) / internal.initialPendingUploadCount
            }

            updateState()
        }
    }

    Timer {
        id: timerBusy
        interval: 100
        repeat: true
        onTriggered: {
            centerIcon.rotation += 10
        }
    }

    PopupLoader {
        id: popupSubmit
        popupComponent: Component {
            ConfirmPopup {
                icon: "qrc:/icons/upload_multiple.svg"
                text: qsTr("Submit data?")
                onConfirmed: {
                    submitData()
                }
            }
        }
    }

    Connections {
        target: form

        function onSightingRemoved(sightingUid) {
            updateState()
        }

        function onSightingFlagsChanged(sightingUid) {
            updateState()
        }
    }

    Component.onCompleted: {
        App.taskManager.resumePausedTasks(form.projectUid)
        updateState()
    }

    onClicked: {
        if (root.showPopup) {
            popupSubmit.open()
            return
        }

        submitData()
    }

    function submitData() {
        form.submitData()
        updateState()
    }

    function updateState() {
        let canSubmit = form.canSubmitData()
        let pendingUploadCount = form.getPendingUploadCount()

        if (!canSubmit && pendingUploadCount === 0) {
            root.state = "done"
            return
        }

        if (!App.uploadAllowed()) {
            root.state = "noWiFi"
            return
        }

        if (canSubmit) {
            root.state = "submit"
            return
        }

        if (App.taskManager.getRunningCount(form.projectUid) === 0) {
            root.state = "fail"
            return
        }

        if (pendingUploadCount !== 0) {
            internal.currentPendingUploadCount = pendingUploadCount
            root.state = "busy"
            return
        }

        console.log("How did we get here: " + canSubmit + " " + pendingUploadCount)
    }
}
