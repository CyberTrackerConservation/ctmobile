import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

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
                    centerIcon.angle = 0
                    centerIcon.source = "qrc:/icons/cloud_done.svg"
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
                    centerIcon.angle = 0
                    centerIcon.source = "qrc:/icons/wifi_strength_off_outline.svg"
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
                    centerIcon.angle = 0
                    centerIcon.source = "qrc:/icons/upload_multiple.svg"
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
                    centerIcon.angle = 0
                    centerIcon.source = "qrc:/icons/sync.svg"
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
                    centerIcon.angle = 0
                    centerIcon.source = "qrc:/icons/upload_multiple.svg"
                    progressCircle.arcEnd = 0
                    internal.initialPendingUploadCount = internal.currentPendingUploadCount = 0
                    App.showError(qsTr("Upload failed"))
                }
            }
        }
    ]

    contentItem: ProgressCircle {
        id: progressCircle
        anchors.fill: parent
        size: root.width
        lineWidth: App.scaleByFontSize(5)
        colorCircle: Material.accent
        colorBackground: Utils.changeAlpha(Style.colorGroove, 128)
        showBackground: true
        arcBegin: 0
        arcEnd: 0

        SquareIcon {
            id: centerIcon
            anchors.fill: parent
            size: root.width / 2
            opacity: root.enabled ? 0.8 : 0.2
            recolor: true
        }
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
        interval: 20
        repeat: true
        onTriggered: {
            centerIcon.angle += 2
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

    C.SightingListModel {
        id: sightingListModel
        projectUid: form.project.uid
        stateSpace: form.stateSpace
    }

    Connections {
        target: form

        function onSightingDeleted(sightingUid) {
            updateState()
        }

        function onSightingFlagsChanged(sightingUid) {
            updateState()
        }
    }

    Component.onCompleted: {
        App.uploadQueue.kick()
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
        let canSubmitData = sightingListModel.canSubmit
        let pendingUploadCount = form.getPendingUploadCount()

        if (!canSubmitData && pendingUploadCount === 0) {
            root.state = "done"
            return
        }

        if (!App.uploadAllowed()) {
            root.state = "noWiFi"
            return
        }

        if (canSubmitData) {
            root.state = "submit"
            return
        }

        if (!App.uploadQueue.processing) {
            root.state = "fail"
            return
        }

        if (pendingUploadCount !== 0) {
            internal.currentPendingUploadCount = pendingUploadCount
            root.state = "busy"
            return
        }

        console.log("How did we get here: " + canSubmitData + " " + pendingUploadCount)
    }
}
