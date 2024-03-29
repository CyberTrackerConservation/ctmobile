import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.0 as Labs
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var project

    Component.onCompleted: {
        // Clone so changes to the caller model do not affect us.
        page.project = App.projectManager.load(page.project.uid)
    }

    header: C.PageHeader {
        text: qsTr("%1 options").arg(App.alias_Project)
    }

    property var options: ([
        {
            text: qsTr("Exported data"),
            subText: qsTr("%1 data waiting for import").arg(App.alias_Project),
            icon: "qrc:/icons/tray_full.svg",
            enabled: App.projectManager.canExportData(project.uid),
            chevronRight: true,
            handler: "exportData"
        },
        {
            text: qsTr("QR code"),
            subText: qsTr("Share a link to this %1").arg(App.alias_project),
            icon: "qrc:/icons/qrcode.svg",
            enabled: App.projectManager.canShareLink(project.uid),
            chevronRight: true,
            handler: "shareLink"
        },
        {
            text: qsTr("Send package"),
            subText: qsTr("Share this %1 with others").arg(App.alias_project),
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
            text: qsTr("Reset %1").arg(App.alias_project),
            subText: qsTr("Reset %1 state and data").arg(App.alias_project),
            icon: "qrc:/icons/clipboard_remove_outline.svg",
            enabled: true,
            handler: "resetProject"
        },
        {
            text: qsTr("Delete %1").arg(App.alias_project),
            subText: qsTr("Delete the %1 and all data").arg(App.alias_project),
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
                height: page.project.displayIcon.toString() !== "" ? 84 : 8

                Image {
                    anchors.centerIn: parent
                    height: 64
                    sourceSize.height: 64
                    source: page.project.displayIcon
                }
            }

            Label {
                Layout.fillWidth: true
                font.pixelSize: App.settings.font18
                font.bold: true
                wrapMode: Label.WordWrap
                text: project ? project.title : ""
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
        let packageFile = App.projectManager.createPackage(project.uid, true, true, false)
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

    Labs.Settings {
        fileName: App.iniPath
        category: "PackageFileDialog"
        property alias fileDialogPackageFolder: fileDialogPackage.folder
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
            C.OptionsPopup {
                id: popupConfirmReset2
                text: qsTr("Reset %1?").arg(App.alias_project)
                subText: qsTr("Data will be permanently removed from the device.")
                icon: "qrc:/icons/alert_circle_outline.svg"

                model: [
                    { text: qsTr("State only"), icon: "qrc:/icons/ok.svg", delay: true },
                    { text: qsTr("State and data"), icon: "qrc:/icons/ok.svg", delay: true },
                    { text: qsTr("Cancel"), icon: "qrc:/icons/cancel.svg" }
                ]

                onClicked: function (index) {
                    switch (index) {
                    case 0:
                        App.backupDatabase("Project state reset: " + project.uid)
                        App.projectManager.reset(project.uid, true)
                        showToast(qsTr("%1 state reset").arg("'" + project.title + "'"))
                        break

                    case 1:
                        App.backupDatabase("Project reset: " + project.uid)
                        App.projectManager.reset(project.uid)
                        showToast(qsTr("%1 reset").arg("'" + project.title + "'"))
                        break

                    case 2:
                        popupConfirmReset2.cancel()
                        break
                    }
                }
            }
        }
    }

    C.PopupLoader {
        id: popupConfirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete %1?").arg(App.alias_project)
                subText: qsTr("The %1 and all of its data will be permanently removed.").arg(App.alias_project)
                confirmText: qsTr("Yes, delete it")
                confirmDelay: true
                onConfirmed: {
                    App.backupDatabase("Project delete: " + project.uid)
                    App.projectManager.remove(project.uid)
                    showToast(qsTr("%1 deleted").arg("'" + project.title + "'"))
                    appPageStack.pop()
                }
            }
        }
    }

    Connections {
        target: App.projectManager

        function onProjectUpdateComplete(projectUid, result) {
            if (projectUid === page.project.uid) {
                page.project = App.projectManager.load(projectUid)
            }
        }
    }
}
