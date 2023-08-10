import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtSensors 5.12
import QtPositioning 5.12
import QtMultimedia 5.12
import QtQuick.Window 2.12
import QtWebView 1.1
import QtQuick.Layouts 1.12
import Qt.labs.platform 1.0 as Labs
import Qt.labs.settings 1.0 as Labs
import Esri.ArcGISRuntime 100.15
import QtQuick.Controls.Material 2.12
import QtQuick.Dialogs 1.2 as Dialogs
import CyberTracker 1.0 as C

ApplicationWindow {
    id: appWindow

    property bool videoMode: false

    property var appPageStackList: [ { stackView: appPageStack, form: undefined } ]
    property int popupCount: 0
    property bool frameless: videoMode

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
                    text: qsTr("Reset cache")
                    onTriggered: {
                        App.clearDeviceCache()
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

    // Message popup.
    C.PopupLoader {
        id: messagePopup

        popupComponent: Component {
            C.MessagePopup {}
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

    Rectangle {
        anchors.fill: parent
        color: "#007bff"
        visible: videoMode

        Rectangle {
            anchors.fill: parent
            color: "black"
            radius: 8
        }

        RowLayout {
            y: 4
            anchors.horizontalCenter: parent.horizontalCenter

            C.SquareIcon {
                source: "qrc:/icons/apple.svg"
                color: Material.background
                size: 32
            }

            C.SquareIcon {
                source: App.config.logo
                size: 32
            }

            C.SquareIcon {
                source: "qrc:/icons/android.svg"
                color: Material.background
                size: 32
            }
        }
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

        Component.onCompleted: {
            visible = App.settings.autoLaunchProjectUid === ""

            if (videoMode) {
                appWindow.x = 2200
                appWindow.y = 200
                appWindow.width = 360 + 32
                appWindow.height = 800 + 56

                appPageStack.anchors.fill = ContingentNullValue
                x = 16
                y = 40
                width = 360
                height = 800
            }
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

    function changeToProject(projectUid, transition = StackView.Immediate) {
        App.clearComponentCache()

        if (App.promptForLocation(projectUid)) {
            popupRequestLocation.open({ callback: () => { changeToProject(projectUid, transition) } })
            return
        }

        if (App.promptForBackgroundLocation(projectUid)) {
            popupRequestBackgroundLocation.open({ projectUid: projectUid, transition: transition })
            return
        }

        if (!App.requestPermissions(projectUid)) {
            return
        }

        if (App.promptForBlackviewMessage(projectUid)) {
            App.settings.blackviewMessageShown = true
            popupBlackviewMessage.open({ projectUid: projectUid, transition: transition })
            return
        }

        appPageStack.visible = false
        appPageStack.pop(null)
        appPageStackList = [appPageStackList[0]]

        try {
            appPageStack.push(FormViewUrl, { projectUid: projectUid }, transition)
        }
        finally {
            appPageStack.visible = true
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

            let projectUid = App.settings.autoLaunchProjectUid
            if (projectUid !== "") {
                changeToProject(projectUid)
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

        function onShowMessageBox(title, message1, message2, message3) {
            messagePopup.open({ title: title, message1: message1, message2: message2, message3: message3 })
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
            changeToProject(projectUid)
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

    C.PopupLoader {
        id: popupBlackviewMessage

        popupComponent: Component {
            C.MessagePopup {
                property string projectUid: ""
                property int transition: StackView.Immediate

                icon: "qrc:/icons/alert_outline.svg"
                title: qsTr("%1 detected").arg("Blackview")
                message1: qsTr("Use the %1 app to allow background activity for %2.").arg("**'System Manager'**").arg(App.config.title)
                message2: qsTr("This is required to ensure that a proper track of your location is created.").arg(App.alias_project)
                onClicked: {
                    changeToProject(projectUid, transition)
                }
            }
        }
    }

    C.PopupLoader {
        id: popupRequestBackgroundLocation

        popupComponent: Component {
            C.MessagePopup {
                property string projectUid: ""
                property int transition: StackView.Immediate

                icon: "qrc:/icons/map_marker_multiple_outline.svg"
                title: qsTr("Background location required")
                message1: qsTr("You must select %1 on the next screen.").arg("**'" + qsTr("Allow all the time") + "'**")
                message2: qsTr("This %1 requires permission to capture location in the background. This is needed to ensure that a proper track of your location is created.").arg(App.alias_project)
                onClicked: function (index) {
                    if (App.requestPermissionBackgroundLocation(projectUid)) {
                        changeToProject(projectUid, transition)
                    }
                }
            }
        }
    }

    C.PopupLoader {
        id: popupRequestLocation

        popupComponent: Component {
            C.MessagePopup {
                property var callback: undefined

                icon: "qrc:/icons/map_marker.svg"
                title: qsTr("Location required")
                message1: qsTr("Permission to use your location is needed for mapping and data collection.")
                message2: qsTr("Tap %1 below and allow it to be used.").arg("**" + qsTr("OK") + "**")
                onClicked: function (index) {
                    if (App.requestPermissionLocation() && callback !== undefined) {
                        callback()
                    }
                }
            }
        }
    }
}
