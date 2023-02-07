import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Layouts 1.12
import CyberTracker 1.0 as C

C.Form {
    id: form

    signal loadComplete

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

        // Send load complete.
        loadComplete()
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
            target: App.settings

            function onDarkThemeChanged() {
                formPageStack.setProjectColors(form)
                formPageStack.setProjectColors(kioskPopup)
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

            if (App.settings.darkTheme) {
                if (colors.primaryDark !== undefined) {
                    item.Material.primary = colors.primaryDark.toString()
                } else if (colors.primary !== undefined) {
                    item.Material.primary = colors.primary.toString()
                } else {
                    item.Material.primary = appWindow.Material.primary
                }

                if (colors.accentDark !== undefined) {
                    item.Material.accent = colors.accentDark.toString()
                } else if (colors.accent !== undefined) {
                    item.Material.accent = colors.accent.toString()
                } else {
                    item.Material.accent = appWindow.Material.accent
                }

                item.Material.foreground = (colors.foregroundDark !== undefined) ? colors.foregroundDark.toString() : appWindow.Material.foreground
                item.Material.background = (colors.backgroundDark !== undefined) ? colors.backgroundDark.toString() : appWindow.Material.background

                return
            }

            item.Material.primary = (colors.primary !== undefined) ? colors.primary.toString() : appWindow.Material.primary
            item.Material.accent = (colors.accent !== undefined) ? colors.accent.toString() : appWindow.Material.accent
            item.Material.foreground = (colors.foreground !== undefined) ? colors.foreground.toString() : appWindow.Material.foreground
            item.Material.background = (colors.background !== undefined) ? colors.background.toString() : appWindow.Material.background
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
