import QtCore
import QtQuick
import QtQuick.Controls.Material
import QtSensors
import QtPositioning
import QtMultimedia
import QtQuick.Window
import QtWebView
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs
import CyberTracker as C
import CyberTracker.Classic

ApplicationWindow {
    id: appWindow

    property bool videoMode: false

    property var appPageStackList: [ { stackView: appPageStack, form: undefined } ]
    property int popupCount: 0
    property bool frameless: videoMode

    flags: {
        if (App.desktopOS) {
            return Qt.Window | (frameless ? Qt.FramelessWindowHint : 0)
        }

        return Qt.Window | Qt.MaximizeUsingFullscreenGeometryHint
    }

    width: 480
    height: 640
    visible: true

    Material.theme: App.settings.darkTheme ? Material.Dark : Material.Light
    Material.primary: C.Style.colorPrimary
    Material.accent: C.Style.colorAccent

    // Menu.
    Loader {
        id: desktopMenuLoader
        active: App.desktopOS && !frameless
        source: "AppMainDesktop.qml"
    }

    Settings {
        id: settings
        location: App.iniUrl
        category: "AppWindow"
        property alias x: appWindow.x
        property alias y: appWindow.y
        property alias width: appWindow.width
        property alias height: appWindow.height
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
    onClosing: function(close) {
        if (Qt.platform.os !== "android") {
            return
        }

        close.accepted = false

        if (popupCount === 0 && appPageStackList.length === 1) {
            if (appPageStack.depth > 1) {
                appPageStack.pop()
                return
            }

            if (!App.kioskMode)
            {
                App.backToHomeScreen()
            }

            return
        }

        App.backPressed()
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        visible: !videoMode
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
                    if (App.settings.fullScreen) {
                        return 0
                    }

                    return parent.SafeArea.margins.bottom
                }

                // Older Android (API < 30) reports the keyboard via SafeArea
                // insets, so the manual math below is not needed.
                if (Qt.platform.os === "android" && Utils.androidSdkVersion() < 30) {
                    return parent.SafeArea.margins.bottom
                }

                let keyboardHeight = Qt.inputMethod.keyboardRectangle.height

                if (Qt.platform.os === "android") {
                    // Adjust if the keyboard is not at the bottom.
                    keyboardHeight += (appWindow.height * Screen.devicePixelRatio - Qt.inputMethod.keyboardRectangle.bottom)

                    // Scale to screen coordinates.
                    keyboardHeight /= Screen.devicePixelRatio

                    keyboardHeight += parent.SafeArea.margins.bottom
                }

                if (Qt.platform.os === "ios") {
                    keyboardHeight += parent.SafeArea.margins.bottom
                }

                return keyboardHeight
            }
        }

        initialItem: Loader {
            source: App.config.mainPage
        }

        Component.onCompleted: {
            visible = !App.projectManager.exists(App.settings.autoLaunchProjectUid)

            App.garbageCollectStreamers()

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

        function pushPageDefault(page, params = ({})) {
            appPageStack.push(page, params)
        }

        function pushPageImmediate(page, params = ({})) {
            appPageStack.push(page, params, StackView.Immediate)
        }

        function popPageDefault() {
            appPageStack.pop()
        }

        function popPageImmediate() {
            appPageStack.pop(StackView.Immediate)
        }
    }

    function pushPageDefault(context, page, params = ({})) {
        App.parentStackView(context).pushPageDefault(page, params)
    }

    function pushPageImmediate(context, page, params = ({})) {
        App.parentStackView(context).pushPageImmediate(page, params, StackView.Immediate)
    }

    function popPageDefault(context) {
        App.parentStackView(context).popPageDefault()
    }

    function popPageImmediate(context) {
        let pageStack = App.parentStackView(context)
        pageStack.popPageImmediate()
        pageStack.currentItem.x = 0
        pageStack.currentItem.opacity = 1.0
    }

    function setProjectColors(item) {
        // Stub fallback to simplify callers.
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

    function showTrackFile(context, filename, summary) {
        let points = App.trackFileToPoints(filename)
        if (points.length === 0) {
            showError(qsTr("Bad track file"))
            return
        }

        let params = {
            title: qsTr("Track"),
            footerText: summary,
            hideDataLayers: true,
            mode: C.MapController.ModeField,
            initialPanMode: C.MapController.AutoPanModeOff,
            initialExtent: Utils.computeExtent(points),
            fieldValue: { fieldType: "TrackFile", fieldValue: points },
        }

        pushPageImmediate(context, "qrc:/imports/CyberTracker/EsriMapViewPage.qml", params)
    }

    Timer {
        id: popToRootTimer
        interval: 10
        onTriggered: {
            appPageStack.pop(null, StackView.Immediate)
            appPageStack.currentItem.x = 0
            appPageStack.currentItem.opacity = 1.0
            appPageStackList = [appPageStackList[0]]
        }
    }

    function postPopToRoot() {
        popToRootTimer.start()
    }

    Connections {
        target: App
        
        function onPermissionGranted(callback) {
            if (callback !== undefined) {
                callback()
            }
        }
    }

    function changeToProject(projectUid, transition = StackView.Immediate) {
        let callback = () => changeToProject(projectUid, transition)

        if (App.promptForLocation(projectUid)) {
            popupRequestLocation.open({ callback: callback })
            return
        }

        if (App.promptForBackgroundLocation(projectUid)) {
            popupRequestBackgroundLocation.open({ callback: callback })
            return
        }

        if (App.promptForCamera(projectUid)) {
            App.requestPermissionCamera(callback)
            return
        }

        if (App.promptForMicrophone(projectUid)) {
            App.requestPermissionMicrophone(callback)
            return
        }

        App.clearComponentCache()
        appPageStack.visible = false
        appPageStack.pop(null, StackView.Immediate)
        appPageStack.currentItem.x = 0
        appPageStack.currentItem.opacity = 1.0
        appPageStackList = [appPageStackList[0]]

        try {
            appPageStack.push(FormViewUrl, { projectUid: projectUid }, transition)
        }
        finally {
            appPageStack.visible = true
        }
    }

    function pushConnectQRCodePage() {
        App.requestPermissionCamera(() => { appPageStack.push("qrc:/ConnectQRCodePage.qml") })
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
            if (App.projectManager.exists(projectUid)) {
                changeToProject(projectUid)
            }
        }
    }

    Connections {
        target: App

        function onCommandLineChanged() {
            if (App.commandLine === "") {
                return
            }

            appPageStack.pop(null, StackView.Immediate)
            appPageStack.currentItem.x = 0
            appPageStack.currentItem.opacity = 1.0
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

        function onChangeToProject(projectUid) {
            changeToProject(projectUid, StackView.Immediate)
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

        if (result.webhook === true) {
            confirmSetWebhook.open(({ params: result.params }))
            return
        }

        if (result.setGoto === true) {
            appPageStack.push("qrc:/MapsPage.qml", StackView.Immediate)
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

    // Once-per-session guard for the auto-popup that prompts when offline maps have updates.
    property bool offlineMapsUpdatePopupShown: false

    function updateMaps() {
        if (Utils.networkAvailable()) {
            busyCover.doWork = () => App.offlineMapManager.updateAll()
            busyCover.start()
            return
        }

        showError(qsTr("Offline"))
    }

    Connections {
        target: App.offlineMapManager

        function onUpdatesAvailableChanged() {
            if (offlineMapsUpdatePopupShown) {
                return
            }

            if (!App.offlineMapManager.updatesAvailable) {
                return
            }

            offlineMapsUpdatePopupShown = true
            popupOfflineMapsUpdate.open()
        }
    }

    C.PopupLoader {
        id: popupOfflineMapsUpdate

        popupComponent: Component {
            C.ConfirmPopup {
                icon: "qrc:/icons/autorenew.svg"
                text: qsTr("Offline maps update available")
                confirmText: qsTr("Update now")
                onConfirmed: {
                    updateMaps()
                }
            }
        }
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
        id: popupRequestBackgroundLocation

        popupComponent: Component {
            C.MessagePopup {
                property var callback

                icon: "qrc:/icons/map_marker_multiple_outline.svg"
                title: qsTr("Background location required")
                message1: qsTr("You must select %1 on the next screen.").arg("**'" + qsTr("Allow all the time") + "'**")
                message2: qsTr("This app collects your location in the background so it can create an accurate track for mapping and field reporting. Location may be collected even when the app is closed or you are not actively using it.")
                onClicked: function (index) {
                    App.requestPermissionBackgroundLocation(callback)
                }
            }
        }
    }

    C.PopupLoader {
        id: popupRequestLocation

        popupComponent: Component {
            C.MessagePopup {
                property var callback

                icon: "qrc:/icons/map_marker.svg"
                title: qsTr("Location required")
                message1: qsTr("This app uses your precise location to show where you are on the map and to record location points during data collection. Location is accessed only while you are actively using the app.")
                message2: qsTr("Tap %1 below and allow it to be used.").arg("**" + qsTr("OK") + "**")
                onClicked: function (index) {
                    App.requestPermissionLocation(callback)
                }
            }
        }
    }

    C.PopupLoader {
        id: confirmSetWebhook
        popupComponent: Component {
            C.ConfirmPopup {
                property var params
                text: qsTr("Install webhook")
                subText: qsTr("Data will be shared with %1").arg("'" + params.entity + "'")
                confirmText: qsTr("Yes, share my data")
                onConfirmed: {
                    App.settings.webhookUrl = params.url.trim()
                    if (App.settings.webhookUrl !== "") {
                        showToast(qsTr("Webhook installed"))
                    }
                }
            }
        }
    }
}
