import QtMultimedia
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

RowLayout {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property var fieldValue: undefined
    property bool readonly: fieldValue !== undefined
    property alias recording: recorder.recording
    property bool playing: false

    property alias progressFrom: progressBar.from
    property alias progressTo: progressBar.to
    property alias progressValue: progressBar.value

    property string fillRecordUid: ""
    property var fillFieldUids: []

    state: "Stop"
    states: [
        State {
            name: "Stop"
            StateChangeScript {
                script: {
                    if (root.recording) {
                        root.setDuration(recorder.stop())

                        if (fieldBinding.field.transcribeTo !== "" || fieldBinding.field.fill === true) {
                            let speechPrompt = form.buildSpeechPrompt(root.fillRecordUid, root.fillFieldUids)
                            let speechSchema = form.buildSpeechSchema(root.fillRecordUid, root.fillFieldUids)
                            form.requestTranscribe(getValue().filename, speechPrompt, speechSchema, root.fillRecordUid, root.fillFieldUids)
                        }
                    }

                    if (root.playing) {
                        player.stop()
                        root.playing = false
                    }

                    if (!root.readonly) {
                        form.saveState()
                    }

                    root.updateProgress()
                }
            }
        },
        State {
            name: "Record"
            StateChangeScript {
                script: {
                    if (root.getValue().filename !== "") {
                        resetValue()
                    }

                    let filename = Utils.generateUuid() + ".wav";
                    fieldBinding.setValue({ filename: filename, duration: 0 })
                    recorder.start(App.getMediaFilePath(filename))

                    root.updateProgress()
                }
            }
        },
        State {
            name: "Play"
            StateChangeScript {
                script: {
                    root.playing = true
                    player.source = App.getMediaFileUrl(root.getValue().filename)
                    player.play()

                    root.updateProgress()
                }
            }
        }
    ]

    C.FieldBinding {
        id: fieldBinding
    }

    ToolButton {
        id: recordButton
        icon.source: "qrc:/icons/record.svg"
        icon.width: Style.toolButtonSize
        icon.height: Style.toolButtonSize
        icon.color: enabled ? "red" : "gray"
        Layout.fillHeight: true
        visible: !root.readonly
        enabled: !root.readonly && !root.recording && !root.playing

        onClicked: {
            startRecording()
        }
    }

    ProgressBar {
        id: progressBar

        value: 0
        Layout.fillWidth: true
        Layout.preferredHeight: progressLabel.implicitHeight * 1.5

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
                font.pixelSize: App.settings.font12
                elide: Text.ElideRight
            }
        }
    }

    ToolButton {
        id: playButton
        icon.source: "qrc:/icons/play.svg"
        icon.color: enabled ? "green" : "gray"
        icon.width: Style.toolButtonSize
        icon.height: Style.toolButtonSize
        enabled: !root.recording && !root.playing && root.getValue().duration > 0
        onClicked: {
            root.state = "Play"
        }
    }

    ToolButton {
        id: stopButton
        icon.source: "qrc:/icons/stop.svg"
        icon.color: enabled ? Material.foreground : "gray"
        icon.width: Style.toolButtonSize
        icon.height: Style.toolButtonSize
        enabled: root.recording || root.playing
        onClicked: {
            root.state = "Stop"
        }
    }

    ConfirmPopup {
        id: confirmDeletePopup
        text: qsTr("Delete the recording?")
        confirmText: qsTr("Yes, delete it")
        onConfirmed: {
            if (root.getValue().filename !== "") {
                App.removeMedia(getValue().filename)
                fieldBinding.resetValue()
                form.saveState()
            }
            root.state = "Stop"
            root.updateProgress()
        }
    }

    MediaPlayer {
        id: player
        audioOutput: AudioOutput {}
        onPositionChanged: {
            root.updateProgress()
        }

        onPlaybackStateChanged: {
            if (player.playbackState === MediaPlayer.StoppedState) {
                root.state = "Stop"
            }
        }
    }

    C.WaveFileRecorder {
        id: recorder

        onDurationChanged: function() {
            root.setDuration(recorder.duration)
            root.updateProgress()

            if (recorder.duration >= fieldBinding.field.maxSeconds * 1000) {
                root.state = "Stop"
            }
        }

        onStartFailed: function(reason) {
            resetValue()
            Qt.callLater(() => { root.state = "Stop"; })
        }
    }

    function startRecording() {
        let value = root.getValue()
        if (value.filename !== "") {
            confirmDeletePopup.open()
            return
        }

        App.requestPermissionMicrophone(() => {
            Qt.callLater(() => { root.state = "Record"; })
        })
    }

    function stop() {
        root.state = "Stop"
    }

    function resetValue() {
        App.removeMedia(getValue().filename)
        fieldBinding.resetValue()
        form.saveState()
    }

    function getValue() {
        let defaultValue = { filename: "", duration: 0 }
        if (!root.readonly) {
            fieldBinding.value // change notifications
            return fieldBinding.getValue(defaultValue)
        } else {
            return root.fieldValue !== undefined ? root.fieldValue : defaultValue
        }
    }

    function setDuration(duration) {
        let value = getValue()
        value.duration = duration
        fieldBinding.setValue(value)
    }

    function updateProgress() {
        let position = 0
        let duration = getValue().duration
        let durationText = parseInt(duration / 1000, 10) + " " + qsTr("seconds")

        if (root.playing && duration !== 0) {
            progressBar.value = player.position / duration
        } else if (root.recording) {
            progressBar.value = duration / (fieldBinding.field.maxSeconds * 1000)
        } else if (duration !== 0) {
            progressBar.value = 1.0
        } else {
            progressBar.value = 0.0
            durationText = ""
        }

        progressIndicator.width = progressBar.value * progressBar.width
        progressIndicator.visible = progressBar.value !== 0
        progressLabel.text = durationText
    }
}
