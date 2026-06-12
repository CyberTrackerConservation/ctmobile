import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

RoundButton {
    id: root

    property alias sourceFilePath: recorder.sourceFilePath

    property string fillRecordUid: ""
    property var fillFieldUids: []

    implicitWidth: Style.iconSize64
    implicitHeight: Style.iconSize64
    radius: width / 2

    QtObject {
        id: internal

        property alias listening: recorder.listening
        property alias recording: recorder.recording
        property int duration: 0
    }

    C.VoiceRecorder {
        id: recorder

        onVoiceStarted: function(filePath) {
            console.log("Voice started: " + filePath)
        }

        onVoiceCompleted: function(filePath, duration, deleted) {
            console.log("Voice completed: " + duration + "ms, deleted: " + deleted)
        }

        onDurationChanged: function() {
            internal.duration = recorder.duration
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
            color: internal.recording ? Material.color(Material.Red) : (internal.listening ? Material.color(Material.Blue) : Material.backgroundColor)
            opacity: 0.5
        }

        SquareIcon {
            anchors.fill: parent
            size: root.width / 2
            opacity: 0.8
            recolor: true
            source: "qrc:/icons/microphone.svg"
        }
    }

    Rectangle {
        id: redDot
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: "orange"
        x: App.scaleByFontSize(4)
        y: x
        radius: width / 2
        width: parent.height / 8
        height: width
        visible: internal.listening

        Timer {
            interval: 750
            running: internal.listening
            repeat: true
            onTriggered: {
                redDot.color = Qt.rgba(redDot.color.r, redDot.color.g, redDot.color.b, redDot.color.a ^ 1)
            }
        }
    }


    onClicked: {
        if (internal.listening) {
            stopListening()
        } else {
            startListening()
        }
    }

    function startListening() {
        if (internal.listening) {
            return
        }

        App.requestPermissionMicrophone(() => {
            internal.duration = 0
            recorder.start()
        })
    }

    function stopListening() {
        if (!internal.listening) {
            return
        }

        recorder.stop()

        internal.duration = 0
    }
}
