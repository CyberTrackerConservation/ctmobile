import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtSensors 5.12
import QtPositioning 5.12
import QtMultimedia 5.12
import QtQuick.Window 2.12
import QtWebView 1.1
import Qt.labs.platform 1.0 as Labs
import Qt.labs.settings 1.0 as Labs
import Esri.ArcGISRuntime 100.15
import QtQuick.Controls.Material 2.12
import QtQuick.Dialogs 1.2 as Dialogs
import CyberTracker 1.0 as C

ApplicationWindow {
    id: appWindow

    property var appPageStackList: [ { stackView: appPageStack, form: undefined } ]
    property int popupCount: 0
    property bool frameless: false

    flags: frameless ? Qt.FramelessWindowHint | Qt.Window : Qt.Window

    width: 480
    height: 640

    Labs.Settings {
        fileName: App.iniPath
        category: "AppWindow"
        property alias saveAsDialogFolder: saveAsDialog.folder
        property alias installPackageDialogFolder: installPackageDialog.folder
        property alias x: appWindow.x
        property alias y: appWindow.y
        property alias width: appWindow.width
        property alias height: appWindow.height
        property alias consoleVisible: consoleWindowLoader.active
    }

    Material.theme: C.Style.darkTheme ? Material.Dark : Material.Light
    Material.primary: C.Style.colorPrimary
    Material.accent: C.Style.colorAccent

    // Menu.
    Loader {
        id: desktopMenuLoader
        sourceComponent: App.desktopOS && !frameless ? desktopMenu : undefined
    }

    Dialogs.FileDialog {
        id: saveAsDialog
        title: qsTr("Choose a file")
        folder: shortcuts.pictures
        selectExisting: false
        selectMultiple: false
        selectFolder: false
        nameFilters: [qsTr("Image files") + " (*.png)"]
        onAccepted: {
            App.snapScreenshotToFile(saveAsDialog.fileUrl)
        }
    }

    Dialogs.FileDialog {
        id: installPackageDialog
        title: qsTr("Choose a file")
        folder: shortcuts.desktop
        selectExisting: true
        selectMultiple: false
        selectFolder: false
        nameFilters: [qsTr("Package files") + " (*.zip)"]
        onAccepted: {
            App.installPackage(installPackageDialog.fileUrl)
        }
    }

    Component {
        id: desktopMenu

        Labs.MenuBar {

            Labs.Menu {
                title: qsTr("&File")
                Labs.MenuItem {
                    text: qsTr("Save &As...")
                    onTriggered: {
                        saveAsDialog.open()
                    }
                }
                Labs.MenuSeparator { }
                Labs.MenuItem {
                    text: qsTr("Install package")
                    onTriggered: {
                        installPackageDialog.open()
                    }
                }
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
            Labs.Menu {
                title: qsTr("&Edit")
                Labs.MenuItem {
                    text: qsTr("&Copy to clipboard")
                    onTriggered: App.snapScreenshotToClipboard()
                }
            }
            Labs.Menu {
                title: qsTr("&Tools")
                Labs.MenuItem {
                    text: qsTr("Create ArcGIS location service")
                    onTriggered: createEsriServiceWindowLoader.active = true
                }
            }
            Labs.Menu {
                title: qsTr("&Window")
                Labs.MenuItem {
                    text: qsTr("Reset window size")
                    onTriggered: {
                        consoleWindowLoader.active = false
                        appWindow.setGeometry(appWindow.x, appWindow.y, 480, 640)
                    }
                }
                Labs.MenuSeparator { }
                Labs.MenuItem {
                    text: qsTr("Toggle developer console")
                    onTriggered: consoleWindowLoader.active = !consoleWindowLoader.active
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
                    // Adjust if the keyboard is not at the bottom.
                    keyboardHeight += (appWindow.height * Screen.devicePixelRatio - Qt.inputMethod.keyboardRectangle.bottom)

                    // Scale to screen coordinates.
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

    // Console window.
    Loader {
        id: consoleWindowLoader
        active: false
        sourceComponent: ConsoleWindow {
            onClosing: consoleWindowLoader.active = false
        }
    }

    // Create ESRI Service window.
    Loader {
        id: createEsriServiceWindowLoader
        active: false
        sourceComponent: CreateEsriServiceWindow {
            onClosing: createEsriServiceWindowLoader.active = false
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

        function onShowMessageBox(title, text, details) {
            messageDialog.title = title
            messageDialog.text = text
            messageDialog.detailedText = details
            messageDialog.open()
        }

        function onPopPageStack() {
            appPageStackList.pop()
            appPageStack.pop()
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

    function getParam(params, key, defaultValue = undefined) {
        return params !== undefined ? Utils.getParam(params, key, defaultValue) : defaultValue
    }

    function getParamColor(params, key, defaultValue = undefined) {
        return params !== undefined ? Utils.getParam(params, key + (App.settings.darkTheme ? "Dark" : ""), defaultValue) : defaultValue
    }
}
