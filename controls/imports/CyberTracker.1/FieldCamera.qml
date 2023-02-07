import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.0 as Labs
import QtMultimedia 5.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Item {
    id: root

    property int photoIndex: 0
    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    property int resolutionX: -1
    property int resolutionY: -1

    property bool confirmDelete: false
    property bool allowVideo: true
    property string filename: "" // Filename without path.

    Labs.Settings {
        fileName: App.iniPath
        category: "FieldCameraDialog"
        property alias fileDialogFolder: fileDialog.folder
    }

    Component.onCompleted: {
        confirmDelete = getFilename() !== ""

        if (Qt.platform.os !== "windows") {
            if (fieldBinding.fieldUid !== "") {
                resolutionX = fieldBinding.field.resolutionX
                resolutionY = fieldBinding.field.resolutionY
            }

            if (resolutionX > 0) {
                camera.imageCapture.resolution.width = resolutionX
            }

            if (resolutionY > 0) {
                camera.imageCapture.resolution.height = resolutionY
            }
        }

        camera.flash.mode = App.settings.cameraFlashMode
        camera.imageProcessing.whiteBalanceMode = App.settings.cameraWhiteBalanceMode
        camera.position = App.settings.cameraPosition

        updatePreview()
    }

    C.FieldBinding {
        id: fieldBinding
    }

    FileDialog {
        id: fileDialog

        property string lastState

        folder: shortcuts.pictures
        nameFilters: [ "Image files (*.jpg *.jpeg *.png)" ]
        onAccepted: {
            let filename = App.copyToMedia(fileDialog.fileUrl)
            if (filename !== "") {
                root.setFilename(filename, false)
                root.updatePreview()
                return
            }

            showError(qsTr("Failed to copy"))
            root.state = lastState
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
                    camera.captureMode = Camera.CaptureStillImage
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
                    photoPreview.source = App.getMediaFileUrl(getFilename())
                    cameraAction.icon.color = "black"
                    cameraAction.icon.source = root.confirmDelete ? "qrc:/icons/delete_outline.svg" : "qrc:/icons/refresh.svg"
                    cameraActionBackground.color = root.confirmDelete ? Material.color(Material.Red) : Material.color(Material.Grey)
                }
            }
        },
        State {
            name: "VideoCapture"
            StateChangeScript {
                script: {
                    camera.captureMode = Camera.CaptureVideo
                    camera.start()

                    let recording = camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus
                    cameraAction.icon.color = recording ? "black" : "red"
                    cameraAction.icon.source = recording ? "qrc:/icons/stop.svg" : "qrc:/icons/record.svg"
                    cameraActionBackground.color = "white"
                }
            }
        },
        State {
            name: "VideoPreview"
            StateChangeScript {
                script: {
                    camera.stop()
                    videoPreview.source = App.getMediaFileUrl(getFilename())
                    cameraAction.icon.color = "black"
                    cameraAction.icon.source = root.confirmDelete ? "qrc:/icons/delete_outline.svg" : "qrc:/icons/refresh.svg"
                    cameraActionBackground.color = root.confirmDelete ? Material.color(Material.Red) : Material.color(Material.LightGreen)
                }
            }
        }
    ]

    Camera {
        id: camera

        captureMode: Camera.CaptureStillImage

        focus {
            focusPointMode: CameraFocus.FocusPointAuto
            focusMode: CameraFocus.FocusContinuous
        }

        imageCapture {
            onImageSaved: {
                setFilename(path, true)
                updatePreview()
            }
        }

        videoRecorder {
            mediaContainer: "mp4"
        }
    }

    VideoOutput {
        id: videoOutput
        source: camera
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        autoOrientation: true
        visible: {
            fieldBinding.isValid
            return getFilename() === ""
        }
        focus: visible
        scale: camera.digitalZoom

// Just turns off auto-focus which does not work well.
//        MouseArea {
//            anchors.fill: parent
//            anchors.bottomMargin: toolBar.height
//            onClicked: {
//                camera.focus.customFocusPoint = Qt.point(mouse.x / width, mouse.y / height)
//                camera.focus.focusMode = CameraFocus.FocusMacro
//                camera.focus.focusPointMode = CameraFocus.FocusPointCustom
//            }
//        }

        PinchArea {
            id: zoomPinch
            anchors.fill: parent
            anchors.bottomMargin: toolBar.height
            focus: true
            enabled: true
            pinch.minimumScale: 1
            pinch.maximumScale: camera.maximumDigitalZoom
            scale: camera.digitalZoom
            onPinchStarted: scale = camera.digitalZoom;
            onPinchUpdated: camera.digitalZoom = pinch.scale;
        }
    }

    Image {
        id: photoPreview
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        cache: false
        autoTransform: false
        visible: root.state === "PhotoPreview"
    }

    Item {
        property alias source: mediaPlayer.source

        id: videoPreview
        visible: root.state === "VideoPreview"
        anchors.fill: parent

        MediaPlayer {
            id: mediaPlayer
            autoPlay: true
            source: videoPreview.visible ? camera.videoRecorder.actualLocation : ""
        }

        VideoOutput {
            source: mediaPlayer
            anchors.fill : parent
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
            property var model: [
                { mode: Camera.FlashAuto, icon: "qrc:/icons/flash_auto.svg" },
                { mode: Camera.FlashOn, icon: "qrc:/icons/flash_on.svg" },
                { mode: Camera.FlashOff, icon: "qrc:/icons/flash_off.svg" }
            ]
            Layout.fillWidth: true
            enabled: getFilename() === ""
            opacity: enabled ? 1.0 : 0.5
            icon.color: "white"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.source: {
                for (let i = 0; i < model.length; i++) {
                    if (model[i].mode === camera.flash.mode) {
                        return model[i].icon
                    }
                }
                return model[0].icon
            }

            onClicked: {
                for (let i = 0; i < model.length; i++) {
                    if (model[i].mode === camera.flash.mode) {
                        camera.flash.mode = model[(i + 1) % model.length].mode
                        App.settings.cameraFlashMode = camera.flash.mode
                        break
                    }
                }
            }
        }

        ToolButton {
            property var model: [
                { mode: CameraImageProcessing.WhiteBalanceAuto, icon: "qrc:/icons/wb_auto.svg" },
                { mode: CameraImageProcessing.WhiteBalanceSunlight, icon: "qrc:/icons/wb_sunny.svg" },
                { mode: CameraImageProcessing.WhiteBalanceCloudy, icon: "qrc:/icons/wb_cloudy.svg" },
                { mode: CameraImageProcessing.WhiteBalanceTungsten, icon: "qrc:/icons/wb_incandescent.svg" },
                { mode: CameraImageProcessing.WhiteBalanceFluorescent, icon: "qrc:/icons/wb_iridescent.svg" }
            ]
            Layout.fillWidth: true
            enabled: getFilename() === ""
            opacity: enabled ? 1.0 : 0.5
            icon.color: "white"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.source: {
                for (let i = 0; i < model.length; i++) {
                    if (model[i].mode === camera.imageProcessing.whiteBalanceMode) {
                        return model[i].icon
                    }
                }
                return model[0].icon
            }

            onClicked: {
                for (let i = 0; i < model.length; i++) {
                    if (model[i].mode === camera.imageProcessing.whiteBalanceMode) {
                        camera.imageProcessing.whiteBalanceMode = model[(i + 1) % model.length].mode
                        App.settings.cameraWhiteBalanceMode = camera.imageProcessing.whiteBalanceMode
                        break
                    }
                }
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
                if (getFilename() !== "") {
                    if (confirmDelete) {
                        confirm.open()
                    } else {
                        root.reset()
                    }
                    return
                }

                switch (root.state) {
                case "PhotoCapture":
                    return camera.imageCapture.capture()

                case "VideoCapture":
                    switch (camera.videoRecorder.recorderStatus) {
                    case CameraRecorder.LoadedStatus:
                        return camera.videoRecorder.record()

                    case CameraRecorder.RecordingStatus:
                        camera.videoRecorder.stop()
                        setFilename(camera.videoRecorder.actualLocation, false)
                        updatePreview()
                        return
                    }
                }
            }
        }

//        ToolButton {
//            property var model: [
//                { mode: Camera.CaptureVideo, icon: "qrc:/icons/camera_alt.svg" },
//                { mode: Camera.CaptureStillImage, icon: "qrc:/icons/videocam.svg" }
//            ]
//            Layout.fillWidth: true
//            enabled: false
//            opacity: enabled ? 1.0 : 0.5
//            icon.color: "white"
//            icon.source: {
//                for (let i = 0; i < model.length; i++) {
//                    if (model[i].mode === camera.captureMode) {
//                        return model[i].icon
//                    }
//                }
//                return model[0].icon
//            }

//            onClicked: {
//                for (let i = 0; i < model.length; i++) {
//                    if (model[i].mode === camera.captureMode) {
//                        camera.captureMode = model[(i + 1) % model.length].mode
//                        break
//                    }
//                }
//            }
//        }

        ToolButton {
            Layout.fillWidth: true
            enabled: getFilename() === ""
            opacity: enabled ? 1.0 : 0.5
            icon.source: "qrc:/icons/folder.svg"
            icon.color: "white"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            onClicked: {
                fileDialog.show()
            }
        }

        ToolButton {
            property var model: [
                { mode: Camera.BackFace, icon: "qrc:/icons/camera_rear.svg" },
                { mode: Camera.FrontFace, icon: "qrc:/icons/camera_front.svg" }
            ]
            Layout.fillWidth: true
            enabled: getFilename() === ""
            opacity: enabled ? 1.0 : 0.5
            icon.color: "white"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            icon.source: {
                for (let i = 0; i < model.length; i++) {
                    if (model[i].mode === camera.position) {
                        return model[i].icon
                    }
                }
                return model[0].icon
            }

            onClicked: {
                for (let i = 0; i < model.length; i++) {
                    if (model[i].mode === camera.position) {
                        camera.position = model[(i + 1) % model.length].mode
                        App.settings.cameraPosition = camera.position
                        break
                    }
                }
            }
        }
    }

    function getFilename() {
        if (fieldBinding.fieldUid === "") {
            return root.filename
        }

        let value = fieldBinding.getValue([""])
        if (value.length <= photoIndex) {
            return ""
        }

        return value[photoIndex]
    }

    function setFilename(sourcePath, reorient) {
        let filename = getFilename()
        if (filename !== "") {
            App.removeMedia(filename)
            filename = ""
        }

        if (sourcePath !== "") {
            // Move to media unless already there. 
            filename = !sourcePath.includes("/") ? sourcePath : App.moveToMedia(sourcePath)
            if (filename === "") {
                showError(qsTr("Failed to move"))
            }
        }

        if (filename !== "" && !filename.endsWith(".mp4") && reorient) {
            Utils.reorientImageFile(App.getMediaFilePath(filename), root.resolutionX, root.resolutionY, camera.position === Camera.BackFace, camera.orientation)
        }

        root.filename = filename

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
    }

    function updatePreview() {
        let filename = getFilename()
        if (filename === "") {
            root.state = "PhotoCapture"
            return
        }

        root.state = filename.endsWith(".mp4") ? "VideoPreview" : "PhotoPreview"
    }

    function reset() {
        setFilename("", false)
        confirmDelete = false
        updatePreview()
    }
}
