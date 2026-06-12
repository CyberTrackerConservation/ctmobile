import QtCore
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.platform as Labs
import QtMultimedia
import CyberTracker as C

Item {
    id: root

    property int photoIndex: 0
    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    property int resolutionX: -1
    property int resolutionY: -1
    property bool simpleMode: form.project.simpleCamera

    property bool confirmDelete: false

    QtObject {
        id: internal

        property string filename: "" // Filename without path.
        property var attachmentTags: ({})
    }

    Component.onCompleted: {
        root.confirmDelete = root.getFilename() !== ""

        if (Qt.platform.os !== "windows") {
            if (fieldBinding.fieldUid !== "") {
                resolutionX = fieldBinding.field.resolutionX
                resolutionY = fieldBinding.field.resolutionY
            }
        }

        root.updatePreview()
    }

    C.FieldBinding {
        id: fieldBinding
    }

    ImagePicker {
        id: imagePicker

        property string lastState

        onAccepted: (fileUrl) => {
            try {
                let filePath = App.copyContent(fileUrl, App.workPath)
                if (filePath !== "") {
                    Utils.resizeImageFile(filePath, root.resolutionX, root.resolutionY)
                    let filename = App.moveToMedia(filePath)
                    if (filename !== "") {
                        filename = App.convertMediaToJpeg(filename)
                        root.setFilename(filename)
                    }
                }
            } finally {
                root.state = lastState
                root.updatePreview()
            }
        }

        onRejected: {
            root.state = lastState
        }

        function show() {
            lastState = root.state
            root.state = "Gallery"
            open()
        }
    }

    state: "PhotoCapture"
    states: [
        State {
            name: "Gallery"
            StateChangeScript {
                script: {
                    camera.stop()
                    cameraAction.icon.color = "white"
                    cameraAction.icon.source = "qrc:/icons/camera.svg"
                    cameraActionBackground.color = "white"
                }
            }
        },
        State {
            name: "PhotoCapture"
            StateChangeScript {
                script: {
                    camera.start()
                    cameraAction.icon.color = "white"
                    cameraAction.icon.source = "qrc:/icons/camera.svg"
                    cameraActionBackground.color = "white"
                }
            }
        },
        State {
            name: "PhotoPreview"
            StateChangeScript {
                script: {
                    camera.stop()
                    photoPreview.source = App.getMediaFileUrl(root.getFilename())
                    cameraAction.icon.color = "black"
                    cameraAction.icon.source = root.confirmDelete ? "qrc:/icons/delete_outline.svg" : "qrc:/icons/refresh.svg"
                    cameraActionBackground.color = root.confirmDelete ? Material.color(Material.Red) : Material.color(Material.Grey)
                }
            }
        }
    ]

    C.CameraManager {
        id: camera
        videoOutput: videoOutput

        onPhotoCaptured: (filename) =>
        {
            Utils.resizeImageFile(App.getMediaFilePath(filename), root.resolutionX, root.resolutionY)

            setFilename(filename)

            if (App.settings.replicateMedia) {
                if (!App.replicateMedia(filename)) {
                    App.showError(qsTr("Copy failed"))
                }
            }

            root.updatePreview()
       }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        visible: {
            fieldBinding.isValid
            return root.getFilename() === ""
        }
        focus: visible
        clip: true

        PinchArea {
            id: zoomPinch
            anchors.fill: parent
            anchors.bottomMargin: toolBar.height
            focus: true
            enabled: true
            pinch.minimumScale: 1
            pinch.maximumScale: camera.maximumZoomFactor
            scale: camera.zoomFactor
            onPinchStarted: function(pinch) {
                scale = camera.zoomFactor
            }
            onPinchUpdated: function(pinch) {
                camera.zoomFactor = pinch.scale
            }
        }
    }

    Image {
        id: photoPreview
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        cache: false
        autoTransform: true
        visible: root.state === "PhotoPreview"
    }

    // Attachment tags.
    Flow {
        anchors {
            leftMargin: App.scaleByFontSize(8)
            rightMargin: App.scaleByFontSize(8)
            left: parent.left
            right: parent.right
            bottom: toolBar.top
        }

        anchors.margins: App.scaleByFontSize(8)
        spacing: App.scaleByFontSize(8)
        visible: root.state === "PhotoPreview" && form.attachmentTags.length !== 0

        Repeater {
            model: form.attachmentTags

            delegate: ColorButton {
                text: form.getElementName(modelData)
                font.pixelSize: App.settings.font14
                font.capitalization: Font.MixedCase
                checkable: true
                checked: internal.attachmentTags[modelData] === true
                onCheckedChanged: internal.attachmentTags[modelData] = checked
                color: checked ? Material.accent : Style.colorContent
            }
        }
    }

    ConfirmPopup {
        id: confirm
        text: qsTr("Delete the image?")
        confirmText: qsTr("Yes, delete it")
        onConfirmed: root.reset()
    }

    // Footer.
    Rectangle {
        anchors.fill: toolBar
        color: "black"
        opacity: 0.2
    }

    RowLayout {
        id: toolBar
        width: parent.width
        anchors.bottom: parent.bottom
        spacing: 0

        ToolButton {
            id: flashModeButton
            Layout.fillWidth: true
            enabled: root.getFilename() === "" && !root.simpleMode
            opacity: enabled ? 1.0 : 0.5
            icon.color: "white"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.source: root.simpleMode ? "" : camera.flashModeIcon
            onClicked: {
                camera.switchFlashMode()
            }
        }

        ToolButton {
            id: whiteBalanceButton
            Layout.fillWidth: true
            enabled: root.getFilename() === "" && !root.simpleMode
            opacity: enabled ? 1.0 : 0.5
            icon.color: "white"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.source: root.simpleMode ? "" : camera.whiteBalanceIcon
            onClicked: {
                camera.switchWhiteBalanceMode()
            }
        }

        RoundButton {
            id: cameraAction

            icon.height: Style.iconSize48
            icon.width: Style.iconSize48
            icon.color: "white"
            radius: width / 2
            background: Item {
                id: cameraActionBackground

                property color color

                Rectangle {
                    anchors.fill: parent
                    radius: cameraAction.radius
                    color: Utils.changeAlpha(cameraActionBackground.color, 128)
                }

                Rectangle {
                    anchors.centerIn: parent
                    color: cameraActionBackground.color
                    radius: cameraAction.radius
                    width: parent.width - 10
                    height: parent.height - 10
                }
            }

            onClicked: {
                if (root.getFilename() !== "") {
                    if (root.confirmDelete) {
                        confirm.open()
                    } else {
                        root.reset()
                    }
                    return
                }

                switch (root.state) {
                case "PhotoCapture":
                    if (!camera.readyForCapture()) {
                        App.showError(qsTr("Not ready"))
                        return
                    }

                    camera.capture()
                    return
                }
            }
        }

        ToolButton {
            Layout.fillWidth: true
            enabled: root.getFilename() === "" && !root.simpleMode
            opacity: enabled ? 1.0 : 0.5
            icon.source: !root.simpleMode ? "qrc:/icons/folder.svg" : ""
            icon.color: "white"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            onClicked: {
                imagePicker.open()
            }
        }

        ToolButton {
            Layout.fillWidth: true
            enabled: root.getFilename() === ""
            opacity: enabled ? 1.0 : 0.5
            icon.color: "white"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.source: camera.cameraIcon
            onClicked: {
                camera.switchCamera()
            }
        }
    }

    function getFilename() {
        if (fieldBinding.fieldUid === "") {
            return internal.filename
        }

        let valueChange = fieldBinding.value // So this function triggers a change.

        let value = fieldBinding.getValue([""])
        if (value.length <= photoIndex) {
            return ""
        }

        return value[photoIndex]
    }

    function setFilename(sourcePath) {
        let filename = getFilename()
        if (filename !== "") {
            App.removeMedia(filename)
            filename = ""
        }

        filename = sourcePath
        internal.filename = filename

        if (fieldBinding.fieldUid !== "") {
            let value = fieldBinding.getValue([""])
            while (value.length < fieldBinding.field.maxCount && value.length <= photoIndex) {
                value.push("")
            }
            value[photoIndex] = filename
            fieldBinding.setValue(value)
        }

        // Persist the state in case of abnormal shutdown.
        form.saveState()

        reloadAttachmentTags(filename)
    }

    function updatePreview() {
        let filename = getFilename()
        if (filename === "") {
            root.state = "PhotoCapture"
            reloadAttachmentTags("")
            return
        }

        root.state = "PhotoPreview"
        reloadAttachmentTags(filename)
    }

    function reset() {
        root.setFilename("")
        root.confirmDelete = false
        root.updatePreview()
    }

    function reloadAttachmentTags(filename) {
        internal.attachmentTags = Utils.unpackTags(Utils.getImageTags(App.getMediaFilePath(filename)), form.attachmentTags)
    }

    function commitAttachmentTags(filename) {
        if (filename === "") {
            return
        }

        let tags = Utils.packTags(internal.attachmentTags, form.attachmentTags)
        let result = Utils.setImageTags(App.getMediaFilePath(filename), tags)
        if (!result) {
            App.showError(qsTr("Failed to write attachment tags"))
        }

        fieldBinding.sendValueChange();
    }
}
