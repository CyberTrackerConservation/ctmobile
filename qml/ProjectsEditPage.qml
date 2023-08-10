import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.0 as Labs
import QtMultimedia 5.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Edit %1").arg(App.alias_projects)
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Options")
            icon.source: "qrc:/icons/application_settings_outline.svg"
            enabled: listView.currentIndex !== -1
            onClicked: {
                appPageStack.push(Qt.resolvedUrl("ProjectDetailsPage.qml"), { project: getProject() } )
            }
        }

        C.FooterButton {
            text: qsTr("Delete")
            icon.source: "qrc:/icons/delete_outline.svg"
            enabled: listView.currentIndex !== -1
            onClicked: {
                popupConfirmDelete.open()
            }
        }

        C.FooterButton {
            text: qsTr("Add")
            icon.source: "qrc:/icons/plus.svg"
            onClicked: {
                fileDialog.show()
            }
        }
    }

    C.ProjectListModel {
        id: projectListModel
    }

    Label {
        anchors.fill: parent
        text: qsTr("No %1").arg(App.alias_projects)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        visible: projectListModel.count === 0
        opacity: 0.5
        font.pixelSize: App.settings.font20
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent
        visible: projectListModel.count !== 0

        currentIndex: -1

        model: projectListModel

        delegate: C.HighlightDelegate {
            id: d
            width: ListView.view.width
            highlighted: ListView.isCurrentItem

            contentItem: RowLayout {
                C.SquareIcon {
                    size: C.Style.iconSize48
                    source: {
                        let result = App.settings.darkTheme && modelData.iconDark !== "" ? modelData.iconDark : modelData.icon
                        return result !== "" ? App.projectManager.getFileUrl(modelData.uid, result) : ""
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Label.WordWrap
                        text: modelData.title
                        font.pixelSize: App.settings.font18
                        elide: Label.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData.subtitle
                        font.pixelSize: App.settings.font12
                        opacity: 0.5
                        elide: Label.ElideRight
                        visible: text !== ""
                    }
                }

                ToolButton {
                    width: C.Style.toolButtonSize
                    height: C.Style.toolButtonSize
                    icon.source: "qrc:/icons/chevron_up.svg"
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    enabled: listView.currentIndex > 0
                    visible: d.highlighted
                    onClicked: {
                        let index = listView.currentIndex
                        App.projectManager.moveProject(projectListModel.get(index).uid, -1)
                        listView.currentIndex = index - 1
                    }
                }

                ToolButton {
                    width: C.Style.toolButtonSize
                    height: C.Style.toolButtonSize
                    icon.source: "qrc:/icons/chevron_down.svg"
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    enabled: listView.currentIndex >= 0 && listView.currentIndex < projectListModel.count - 1
                    visible: d.highlighted
                    onClicked: {
                        let index = listView.currentIndex
                        App.projectManager.moveProject(projectListModel.get(index).uid, 1)
                        listView.currentIndex = index + 1
                    }
                }
            }

            onClicked: {
                listView.currentIndex = model.index
            }

            C.HorizontalDivider {}
        }
    }

    Labs.Settings {
        fileName: App.iniPath
        category: "ProjectsDialog"
        property alias fileDialogFolder: fileDialog.folder
        property alias fileDialogPackageFolder: fileDialogPackage.folder
    }

    FileDialog {
        id: fileDialog

        folder: shortcuts.home
        nameFilters: [ "Project packages (*.zip)" ]
        onAccepted: {
            App.installPackage(fileDialog.fileUrl, true)
        }

        function show() {
            open()
        }
    }

    // Dialogs.
    FileDialog {
        id: fileDialogPackage

        property string packageFile: ""

        defaultSuffix: "zip"
        title: qsTr("Save %1 package").arg(App.alias_project)
        folder: shortcuts.desktop
        nameFilters: [ "CyberTracker project packages (*.zip)" ]
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
        id: popupConfirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete %1?").arg(App.alias_project)
                subText: qsTr("The %1 and all of its data will be permanently removed.").arg(App.alias_project)
                confirmText: qsTr("Yes, delete it")
                confirmDelay: true
                onConfirmed: {
                    let project = getProject()
                    let title = project.title
                    App.backupDatabase("Project delete: " + project.uid)
                    App.projectManager.remove(project.uid)
                    showToast(qsTr("%1 deleted").arg("'" + title + "'"))
                }
            }
        }
    }

    function getProject() {
        let index = listView.currentIndex
        if (index === -1) {
            return undefined
        }

        return projectListModel.get(index)
    }
}
