import QtQml 2.12
import QtMultimedia 5.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

RowLayout {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    enabled: fieldBinding.isValid

    Component.onCompleted: {
        ensureValue()
        updateState()
    }

    C.FieldBinding {
        id: fieldBinding
        onValueChanged: {
            if (fieldBinding.value === undefined) {
                ensureValue()
            }

            updateState()
        }
    }

    ToolButton {
        id: recordButton
        icon.source: "qrc:/icons/record.svg"
        icon.color: enabled ? "red" : "gray"
        Layout.fillHeight: true
        onClicked: {
            ensureValue()

            if (fieldBinding.value.filename !== "") {
                confirmDeletePopup.open()
                return
            }

            let value = fieldBinding.value
            value.filename = Utils.generateUuid() + ".wav"
            fieldBinding.setValue(value)

            recorder.start(App.getMediaFilePath(fieldBinding.value.filename), fieldBinding.field.sampleRate)
            updateState()
        }
    }

    ProgressBar {
        id: progressBar

        value: 0
        Layout.fillWidth: true
        height: progressLabel.implicitHeight * 1.5

        background: Rectangle {
            anchors.fill: parent
            color: Material.accent
            radius: 3
            border.color: Material.foreground
            border.width: 1
            opacity: 0.1
        }

        contentItem: Rectangle {
            id: progressIndicator
            anchors.left: progressBar.left
            anchors.verticalCenter: progressBar.verticalCenter
            width: 0
            height: progressBar.height
            radius: 3
            color: Material.accent
            opacity: 0.75

            Label {
                id: progressLabel
                width: progressBar.width
                height: progressBar.height
                horizontalAlignment: Label.AlignHCenter
                verticalAlignment: Label.AlignVCenter
                font.pixelSize: App.settings.font10
            }
        }
    }

    ToolButton {
        id: playButton
        icon.source: "qrc:/icons/play.svg"
        icon.color: enabled ? "green" : "gray"
        onClicked: {
            stop()
            player.source = App.getMediaFileUrl(fieldBinding.value.filename)
            player.play()
            updateState()
        }
    }

    ToolButton {
        id: stopButton
        icon.source: "qrc:/icons/stop.svg"
        icon.color: enabled ? Material.foreground : "gray"
        onClicked: {
            stop()
            updateState()
        }
    }

    C.ConfirmPopup {
        id: confirmDeletePopup
        text: qsTr("Delete the recording?")
        confirmText: qsTr("Yes, delete it")
        onConfirmed: {
            reset()
            updateState()
            recordButton.clicked()
        }
    }

    MediaPlayer {
        id: player
        onPositionChanged: updateProgress()
        onStopped: updateState()
    }

    C.WaveFileRecorder {
        id: recorder

        onDurationChanged: {
            if (recording) {
                let value = fieldBinding.value
                value.duration = recorder.duration
                fieldBinding.setValue(value)

                if (recorder.duration >= fieldBinding.field.maxSeconds * 1000) {
                    recorder.stop()
                    updateState()
                } else {
                    updateProgress()
                }
            }
        }
    }

    function updateState() {
        if (!fieldBinding.isValid) {
            recordButton.enabled = false
            playButton.enabled = false
            stopButton.enabled = false
            progressBar.value = 0
            return
        }

        let playing = player.playbackState === MediaPlayer.PlayingState
        recordButton.enabled = !recorder.recording && !playing
        playButton.enabled = !recorder.recording && !playing && fieldBinding.value.duration > 0
        stopButton.enabled = recorder.recording || playing
        updateProgress()
    }

    function updateProgress() {
        let position = 0
        let duration = fieldBinding.value.duration
        let durationText = parseInt(duration / 1000, 10) + " " + qsTr("seconds")

        if (player.playbackState === MediaPlayer.PlayingState && duration !== 0) {
            progressBar.value = player.position / duration
        } else if (recorder.recording) {
            progressBar.value = recorder.duration / (fieldBinding.field.maxSeconds * 1000)
        } else if (duration !== 0) {
            progressBar.value = 1.0
        } else {
            progressBar.value = 0
            durationText = ""
        }

        progressIndicator.width = progressBar.value * progressBar.width
        progressLabel.text = durationText
    }

    function reset() {
        if (fieldBinding.value !== undefined) {
            App.removeMedia(fieldBinding.value.filename)
        }
        fieldBinding.setValue({ filename: "", duration: 0 })
        updateState()
        form.saveState()
    }

    function stop() {
        recorder.stop()
        player.stop()
        updateState()
        form.saveState()
    }

    function ensureValue() {
        if (fieldBinding.isValid && fieldBinding.value === undefined) {
            fieldBinding.setValue({ filename: "", duration: 0 })
        }
    }
}
