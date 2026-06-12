import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

RoundButton {
    id: root

    property string fillRecordUid: ""
    property var fillFieldUids: []

    implicitWidth: Style.iconSize64
    implicitHeight: Style.iconSize64
    radius: width / 2

    QtObject {
        id: internal

        property alias recording: recorder.recording
        property string filename: ""
        property int duration: 0
    }

    C.WaveFileRecorder {
        id: recorder

        onDurationChanged: function() {
            internal.duration = recorder.duration

            if (internal.duration >= 60 * 1000) {
                stopRecording()
            }
        }
    }

    contentItem: ProgressCircle {
        id: progressCircle
        anchors.fill: parent
        size: root.width
        lineWidth: App.scaleByFontSize(5)
        colorCircle: Material.accent
        colorBackground: Utils.changeAlpha(Style.colorGroove, 80)
        showBackground: true
        arcBegin: 0
        arcEnd: internal.recording ? (360 * (internal.duration)) / 60000 : 0
        endAnimation: false

        Rectangle {
            id: r
            x: root.leftInset
            y: root.topInset
            width: root.implicitWidth - root.leftInset * 2
            height: root.implicitHeight - root.topInset * 2
            radius: root.radius
            color: internal.recording ? Material.accentColor : Material.backgroundColor
            opacity: 0.5
        }

        SquareIcon {
            anchors.fill: parent
            size: root.width / 2
            opacity: 0.8
            recolor: true
            source: internal.recording ? "qrc:/icons/stop.svg" : "qrc:/icons/microphone.svg"
        }
    }

    Rectangle {
        id: redDot
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: "green"
        x: App.scaleByFontSize(4)
        y: x
        radius: width / 2
        width: parent.height / 8
        height: width
        visible: App.transcribeQueue.working

        Timer {
            interval: 750
            running: App.transcribeQueue.working
            repeat: true
            onTriggered: {
                redDot.color = Qt.rgba(redDot.color.r, redDot.color.g, redDot.color.b, redDot.color.a ^ 1)
            }
        }
    }

    onClicked: {
        if (internal.recording) {
            stopRecording()
        } else {
            startRecording()
        }
    }

    function startRecording() {
        if (internal.recording) {
            return
        }

        App.requestPermissionMicrophone(() => {
            internal.duration = 0
            internal.filename = Utils.generateUuid() + ".wav";
            recorder.start(App.getMediaFilePath(internal.filename))
        })
    }

    function stopRecording() {
        if (!internal.recording) {
            return
        }

        let duration = recorder.stop()
        if (duration > 1000) {
            let speechPrompt = form.buildSpeechPrompt(fillRecordUid, fillFieldUids)
            let speechSchema = form.buildSpeechSchema(fillRecordUid, fillFieldUids)
            form.requestTranscribe(internal.filename, speechPrompt, speechSchema, fillRecordUid, fillFieldUids)
        }

        internal.duration = 0
        internal.filename = ""
    }
}
