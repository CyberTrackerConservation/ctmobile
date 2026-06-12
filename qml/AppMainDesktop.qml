import QtCore
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Window
import Qt.labs.platform as Labs
import QtQuick.Dialogs as Dialogs

Item {
    
    Settings {
        id: desktopSettings
        location: App.iniUrl
        category: "System"
        property alias saveAsDialogFolder: saveAsDialog.currentFolder
        property alias installPackageDialogFolder: installPackageDialog.currentFolder
        property bool consoleWindowVisible: false
    }

    // Console window.
    Loader {
        id: consoleWindowLoader

        Connections {
            target: consoleWindowLoader.item

            function onClosing(close) {
                desktopSettings.consoleWindowVisible = false
            }
        }

        Connections {
            target: desktopSettings

            function onConsoleWindowVisibleChanged() {
                if (desktopSettings.consoleWindowVisible) {
                    consoleWindowLoader.source = "qrc:/ConsoleWindow.qml"
                    consoleWindowLoader.active = true
                } else {
                    consoleWindowLoader.source = ""
                }
            }
        }
    }

    Dialogs.FileDialog {
        id: saveAsDialog
        title: qsTr("Choose a file")
        currentFolder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: [qsTr("Image files") + " (*.png)"]
        onAccepted: {
            App.snapScreenshotToFile(saveAsDialog.selectedFile)
        }
    }

    Dialogs.FileDialog {
        id: installPackageDialog
        title: qsTr("Choose a file")
        currentFolder: StandardPaths.writableLocation(StandardPaths.DesktopLocation)
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [qsTr("Package files") + " (*.zip)"]
        onAccepted: {
            App.installPackage(installPackageDialog.selectedFile)
        }
    }

    // Create ESRI Service window.
    Loader {
        id: createEsriServiceWindowLoader

        function open() {
            createEsriServiceWindowLoader.source = ""
            createEsriServiceWindowLoader.source = "qrc:/CreateEsriServiceWindow.qml"
        }
    }

    Labs.MenuBar {
        id: menuBar
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
                text: qsTr("Android back button")
                onTriggered: {
                    App.backPressed()
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
                onTriggered: createEsriServiceWindowLoader.open()
            }
        }
        Labs.Menu {
            title: qsTr("&Window")
            Labs.MenuItem {
                text: qsTr("Reset window size")
                onTriggered: {
                    desktopSettings.consoleWindowVisible = false
                    appWindow.setGeometry(appWindow.x, appWindow.y, 480, 640)
                }
            }
            Labs.MenuSeparator { }
            Labs.MenuItem {
                text: qsTr("Toggle developer console")
                onTriggered: {
                    desktopSettings.consoleWindowVisible = !desktopSettings.consoleWindowVisible
                }
            }
        }
    }
}
