import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Layouts 1.12
import CyberTracker 1.0 as C

C.Form {
    id: form

    onConnectedChanged: {
        if (!connected) {
            return
        }

        appPageStackList.push( { stackView: formPageStack, form: form } )
        formPageStack.setProjectColors(form)
        formPageStack.setProjectColors(kioskPopup)
        form.loadPages()

        // Activate kiosk mode if needed.
        if (form.project.kioskMode === true) {
            App.settings.kioskModeProjectUid = form.project.uid
        }
    }

    StackView {
        id: formPageStack

        property var form: form
        property bool loading: false

        anchors.fill: parent
        visible: !loginPage.visible

        C.PasscodePopup {
            id: kioskPopup
            closeOnBack: false
            onSuccess: {
                App.settings.kioskModeProjectUid = ""
                form.popPage()
            }
        }

        // Handle Android back button.
        Connections {
            target: App

            function onBackPressed() {
                // Popups are closed automatically.
                if (appWindow.popupCount !== 0) {
                    return
                }

                // Only handle back click for topmost form.
                if (form !== appPageStackList[appPageStackList.length - 1].form) {
                    return
                }

                // Allow for overrides from the header.
                let lastStackView = appPageStackList[appPageStackList.length - 1].stackView
                let lastStackPage = lastStackView.get(lastStackView.depth - 1, StackView.DontLoad)
                if (lastStackPage.header !== null && lastStackPage.header.sendBackClick !== undefined) {
                    lastStackPage.header.sendBackClick()
                    return
                }

                // Normal pop.
                form.popPage()
            }
        }

        Connections {
            target: form

            function onPagePush(pageUrl, params, transition) {
                formPageStack.push(Qt.resolvedUrl(pageUrl), params, transition)
            }

            function onPagePop(transition) {
                formPageStack.pop(transition)
            }

            function onPagesPopToParent(transition) {
                appPageStackList.pop()
                let currPageStack = appPageStackList[appPageStackList.length - 1].stackView
                currPageStack.pop(transition)
            }

            function onPageReplaceLast(pageUrl, params, transition) {
                formPageStack.replace(formPageStack.currentItem, Qt.resolvedUrl(pageUrl), params, transition)
            }

            function onPagesLoad(pageStack) {
                formPageStack.clear()

                for (let i = 0; i < pageStack.length; i++) {
                    formPageStack.loading = true
                    try {
                        formPageStack.push(Qt.resolvedUrl(pageStack[i].pageUrl), pageStack[i].params, StackView.Immediate)
                    } catch (err) {
                        console.log("Error loading page: " + err.message)
                        return
                    } finally {
                        formPageStack.loading = false
                    }
                }
            }

            function onTriggerKioskPopup() {
                if (!kioskPopup.opened) {
                    kioskPopup.passcode = form.project.kioskModePin
                    kioskPopup.open()
                }
            }

            function onTriggerLogin() {
                loginPage.show()
            }

            function onLoggedInChanged(loggedIn) {
                if (loggedIn) {
                    App.taskManager.resumePausedTasks(form.project.uid)
                }
            }
        }

        function setProjectColors(item) {
            var colors = form.project.colors
            if (colors.primary !== undefined) {
                item.Material.primary = colors.primary.toString()
            }

            if (colors.accent !== undefined) {
                item.Material.accent = colors.accent.toString()
            }

            if (colors.foreground !== undefined) {
                //item.Material.foreground = colors.foreground.toString()
            }

            if (colors.background !== undefined) {
                //item.Material.background = colors.background.toString()
            }
        }
    }

    C.LoginPage {
        id: loginPage
        anchors.fill: parent
        visible: false
        defaultServer: ""
        serverVisible: false
        serverEnabled: false
        skipAllowed: false

        onBackClicked: {
            loginPage.visible = false
        }

        onLoginClicked: {
            busyCover.doWork = getLoginToken
            busyCover.start()
        }

        onSkipClicked: {
            loginPage.visible = false
        }

        function show() {
            loginPage.title = project.connector
            loginPage.iconSource = project.loginIcon

            loginPage.cacheKey = ""
            loginPage.cacheKey = form.project.connector

            loginPage.username = form.project.username
            loginPage.visible = true
        }

        function getLoginToken() {
            let server = loginPage.server.trim()
            if (server === "" || server === "https://") {
                server = form.project.connectorParams.server
            }

            if (form.project.login(server, loginPage.username, loginPage.password)) {
                loginPage.visible = false
                return
            }

            showError(qsTr("Bad username or password"))
        }
    }

    C.UsernamePage {
        anchors.fill: parent
        visible: form.requireUsername
        required: true
        backOnSet: false
    }
}
