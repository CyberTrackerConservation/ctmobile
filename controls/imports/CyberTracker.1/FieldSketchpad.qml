import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

C.Sketchpad {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    baseLine: false
    penColor: Material.foreground

    Component.onCompleted: {
        if (fieldBinding.value) {
            root.sketch = fieldBinding.value.sketch
        } else {
            root.clear()
        }
    }

    onSketchChanged: commitToBinding()

    C.FieldBinding {
        id: fieldBinding
    }

    RoundButton {
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }

        icon.source: "qrc:/icons/delete_outline.svg"
        icon.width: Style.toolButtonSize
        icon.height: Style.toolButtonSize
        icon.color: Utils.changeAlpha(Material.foreground, 128)
        enabled: !fieldBinding.isEmpty
        onClicked: {
            root.clear()
            resetField();
        }
    }

    function commitToBinding() {
        let sketch = root.sketch
        if (sketch.length === 0) {
            resetField()
            return
        }

        let filename = ""
        if (fieldBinding.value !== undefined) {
            filename = fieldBinding.value.filename
        }

        if (filename === "") {
            filename = Utils.generateUuid() + '.png'
        }

        Utils.renderSketchToPng(sketch, App.getMediaFilePath(filename), "black", 2)
        fieldBinding.setValue({ sketch: sketch, filename: filename })
    }

    function resetField() {
        if (fieldBinding.value) {
            App.removeMedia(fieldBinding.value.filename)
            fieldBinding.resetValue()
        }
    }
}
