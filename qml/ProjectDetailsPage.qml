import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtQuick.Dialogs 1.2
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var project

    header: C.PageHeader {
        text: qsTr("Project details")
    }

    property var options: ([
        {
            text: qsTr("Exported data"),
            subText: qsTr("Project data waiting for import"),
            icon: "qrc:/icons/tray_full.svg",
            enabled: App.projectManager.canExportData(project.uid),
            chevronRight: true,
            handler: "exportData"
        },
        {
            text: qsTr("QR code"),
            subText: qsTr("Share a link to this project"),
            icon: "qrc:/icons/qrcode.svg",
            enabled: App.projectManager.canShareLink(project.uid),
            chevronRight: true,
            handler: "shareLink"
        },
        {
            text: qsTr("Send package"),
            subText: qsTr("Share this project with others"),
            icon: "qrc:/icons/share_variant_outline.svg",
            enabled: App.projectManager.canSharePackage(project.uid),
            handler: "sharePackage"
        },
        {
            text: qsTr("Update"),
            subText: qsTr("Check for updates"),
            icon: "qrc:/icons/autorenew.svg",
            enabled: project.uid === "" ? false : App.projectManager.canUpdate(project.uid),
            handler: "updateProject"
        },
        {
            text: qsTr("Reset project"),
            subText: qsTr("Delete all project data"),
            icon: "qrc:/icons/clipboard_remove_outline.svg",
            enabled: true,
            handler: "resetProject"
        },
        {
            text: qsTr("Delete project"),
            subText: qsTr("Delete the project and all data"),
            icon: "qrc:/icons/delete_outline.svg",
            enabled: true,
            handler: "deleteProject"
        }
    ])

    Flickable {
        anchors.fill: parent

        ScrollBar.vertical: ScrollBar { interactive: false }
        contentWidth: parent.width
        contentHeight: column.height

        ColumnLayout {
            id: column
            width: parent.width
            spacing: 0

            Item {
                Layout.fillWidth: true
                height: 84

                Image {
                    anchors.centerIn: parent
                    height: 64
                    sourceSize.height: 64
                    source: {
                        let result = App.settings.darkTheme && project.iconDark !== "" ? project.iconDark : project.icon
                        return result !== "" ? App.projectManager.getFileUrl(project.uid, result) : ""
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                font.pixelSize: App.settings.font18
                font.bold: true
                wrapMode: Label.WordWrap
                text: project.title
                horizontalAlignment: Qt.AlignHCenter
            }

            Item { height: 8 }

            C.HorizontalDivider {
                Layout.fillWidth: true
            }

            Repeater {
                model: page.options
                delegate: C.ListRowDelegate {
                    Layout.fillWidth: true
                    text: modelData.text
                    subText: modelData.subText
                    wrapSubText: true
                    iconSource: modelData.icon
                    iconOpacity: 0.5
                    iconColor: Material.foreground
                    enabled: modelData.enabled
                    chevronRight: modelData.chevronRight === true
                    onClicked: page[modelData.handler]()
                }
            }
        }
    }

    // Handlers.
    function exportData() {
        appPageStack.push(Qt.resolvedUrl("ExportsPage.qml"), { projectUid: project.uid })
    }

    function shareLink() {
        appPageStack.push(Qt.resolvedUrl("ShareProjectPage.qml"), { project: project })
    }

    function sharePackage() {
        busyCover.doWork = createPackage
        busyCover.start()
    }

    function updateProject() {
        appWindow.updateProject(project.uid)
    }

    function resetProject(project) {
        popupConfirmReset.open()
    }

    function deleteProject(project) {
        popupConfirmDelete.open()
    }

    // Helpers.
    function createPackage() {
        let packageFile = App.projectManager.createPackage(project.uid, true, false, false)
        if (packageFile === "") {
            showError(qsTr("Packaging failed"))
            return
        }

        if (App.desktopOS) {
            fileDialogPackage.packageFile = packageFile
            fileDialogPackage.open()
            return
        }

        App.sendFile(packageFile, project.title)
    }

    // Dialogs.
    FileDialog {
        id: fileDialogPackage

        property string packageFile: ""

        defaultSuffix: "zip"
        title: qsTr("Save package")
        folder: shortcuts.desktop
        nameFilters: [ "CyberTracker packages (*.zip)" ]
        selectExisting: false
        onAccepted: {
            let targetFile = Utils.urlToLocalFile(fileDialogPackage.fileUrl)
            let success = Utils.copyFile(packageFile, targetFile)
            if (success) {
                App.copyToClipboard(targetFile)
                showToast(qsTr("Copied to clipboard"))
            } else {
                showError(qsTr("Failed to save package"))
            }

            Utils.removeFile(packageFile)
        }
    }

    C.PopupLoader {
        id: popupConfirmReset

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Reset project?")
                subText: qsTr("All project data will be permanently removed from the device.")
                confirmText: qsTr("Yes, reset it")
                confirmDelay: true
                onConfirmed: {
                    App.backupDatabase("Project reset: " + project.uid)
                    App.projectManager.reset(project.uid)
                    showToast(qsTr("%1 reset").arg("'" + project.title + "'"))
                }
            }
        }
    }

    C.PopupLoader {
        id: popupConfirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete project?")
                subText: qsTr("The project and all of its data will be permanently removed.")
                confirmText: qsTr("Yes, delete it")
                confirmDelay: true
                onConfirmed: {
                    App.backupDatabase("Project delete: " + project.uid)
                    let projectTitle = project.title
                    App.projectManager.remove(project.uid)
                    showToast(qsTr("%1 deleted").arg("'" + projectTitle + "'"))
                    appPageStack.pop()
                }
            }
        }
    }
}
