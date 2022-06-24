import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtSensors 5.12
import QtPositioning 5.12
import QtMultimedia 5.12
import QtQuick.Window 2.12
import Qt.labs.platform 1.0 as Labs
import Esri.ArcGISRuntime 100.14
import QtQuick.Controls.Material 2.12
import QtQuick.Dialogs 1.2 as Dialogs
import CyberTracker 1.0 as C

ApplicationWindow {
    id: appWindow

    property var appPageStackList: [ { stackView: appPageStack, form: undefined } ]
    property int popupCount: 0

    //flags: Qt.FramelessWindowHint | Qt.Window

    width: 480
    height: 640

    Material.theme: C.Style.darkTheme ? Material.Dark : Material.Light
    Material.primary: C.Style.colorPrimary
    Material.accent: C.Style.colorAccent

    // Menu.
    Loader {
        id: desktopMenuLoader
        sourceComponent: App.desktopOS ? desktopMenu : undefined
    }

    Dialogs.FileDialog {
        id: fileDialog
        title: qsTr("Choose a file")
        folder: shortcuts.pictures
        selectExisting: false
        selectMultiple: false
        selectFolder: false
        nameFilters: [qsTr("Image files") + " (*.png)"]
        onAccepted: App.snapScreenshotToFile(fileDialog.fileUrl)
    }

    Component {
        id: desktopMenu

        Labs.MenuBar {

            Labs.Menu {
                title: qsTr("&File")
                Labs.MenuItem {
                    text: qsTr("&Copy to Clipboard")
                    onTriggered: App.snapScreenshotToClipboard()
                }
                Labs.MenuItem {
                    text: qsTr("Snap &As...")
                    onTriggered: {
                        fileDialog.open()
                    }
                }
                Labs.MenuSeparator { }
                Labs.MenuItem {
                    text: qsTr("Connect using clipboard link")
                    onTriggered: {
                        App.runLinkOnClipboard()
                    }
                }
                Labs.MenuSeparator { }
                Labs.MenuItem {
                    text: qsTr("&Quit")
                    onTriggered: Qt.quit()
                }
            }
        }
    }

    // Message dialog.
    Dialogs.MessageDialog {
        id: messageDialog
        icon: Dialogs.StandardIcon.Information
        standardButtons: Dialogs.StandardButton.Ok
        onAccepted: close()
        modality: App.mobileOS ? Qt.ApplicationModal : Qt.WindowModal
    }

    Connections {
        target: App

        function onShowMessageBox(title, text, details) {
            messageDialog.title = title
            messageDialog.text = text
            messageDialog.detailedText = details
            messageDialog.open()
        }
    }

    // Setting the state declaratively does not stick.
    Timer {
        interval: 100
        running: App.settings.fullScreen
        onTriggered: appWindow.showFullScreen()
    }

    // Handle Android back button.
    onClosing: {
        if (Qt.platform.os !== "android") {
            return
        }

        close.accepted = false

        if (popupCount === 0 && appPageStackList.length === 1) {
            if (appPageStack.depth > 1) {
                appPageStack.pop()
                return
            }

            Qt.quit()
            return
        }

        App.backPressed()
    }

    StackView {
        id: appPageStack

        anchors {
            fill: parent

            bottomMargin: {
                if (!Qt.inputMethod.visible) {
                    return 0
                }

                let keyboardHeight = Qt.inputMethod.keyboardRectangle.height
                if (Qt.platform.os !== "ios") {
                    keyboardHeight /= Screen.devicePixelRatio
                }
                
                return keyboardHeight
            }
        }

        initialItem: Loader {
            source: App.config.mainPage
        }
    }

    Connections {
        target: Qt.inputMethod

        function onVisibleChanged() {
            if (!Qt.inputMethod.visible && App.settings.fullScreen) {
                Utils.androidHideNavigationBar()
            }
        }
    }

    function showError(info) {
        popupError.start(appPageStack, info)
    }

    C.PopupError {
        id: popupError
    }

    function showToast(info) {
        popupToast.start(appPageStack, info)
    }

    C.PopupToast {
        id: popupToast
    }

    Timer {
        id: popToRootTimer
        interval: 10
        onTriggered: {
            appPageStack.pop(null)
            appPageStackList = [appPageStackList[0]]
        }
    }

    function postPopToRoot() {
        popToRootTimer.start()
    }

    function changeToProject(projectUid) {
        appPageStack.pop(null)
        appPageStackList = [appPageStackList[0]]

        if (App.requestPermissions(projectUid)) {
            appPageStack.push(FormViewUrl, { projectUid: projectUid })
        }
    }

    function sortCompareByName(a, b) {
        let nameA = a.name.toUpperCase() // ignore upper and lowercase
        let nameB = b.name.toUpperCase() // ignore upper and lowercase
        if (nameA < nameB) {
            return -1
        } else if (nameA > nameB) {
            return 1
        } else {
            return 0
        }
    }

    // Commandline handling.
    Timer {
        interval: 100
        repeat: false
        running: true
        onTriggered: {
            if (App.kioskMode) {
                appPageStack.push(FormViewUrl, { projectUid: App.settings.kioskModeProjectUid })
                return
            }

            if (App.commandLine !== "") {
                busyCover.doWork = runCommandLine
                busyCover.start()
                return
            }
        }
    }

    Connections {
        target: App

        function onCommandLineChanged() {
            if (App.commandLine === "") {
                return
            }

            appPageStack.pop(null)
            appPageStackList = [appPageStackList[0]]
            busyCover.doWork = runCommandLine
            busyCover.start()
        }

        function onShowToast(info) {
            showToast(info)
        }

        function onShowError(info) {
            showError(info)
        }
    }

    function runCommandLine() {
        let result = App.runCommandLine()
        if (!result.success) {
            if (result.expected) {
                showToast(result.errorString)
            } else {
                showError(result.errorString)
            }

            return
        }

        if (result.auth === true) {
            appPageStack.push(Qt.resolvedUrl("BootstrapAuthPage.qml"), { params: result.params })
            return
        }

        let projectUid = App.projectManager.lastNewProjectUid
        if (result.launch === true && projectUid !== "" && App.projectManager.exists(projectUid)) {
            if (App.requestPermissions(projectUid)) {
                appPageStack.push(FormViewUrl, { projectUid: projectUid })
            }
        }
    }

    function updateProject(projectUid) {
        if (Utils.networkAvailable()) {
            busyCover.doWork = () => App.projectManager.update(projectUid)
            busyCover.start()
            return
        }

        showError(qsTr("Offline"));
    }

    C.BusyCover {
        id: busyCover
    }
}
