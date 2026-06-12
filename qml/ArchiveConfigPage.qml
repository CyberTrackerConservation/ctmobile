import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import Qt.labs.platform as Labs
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Data archive")
        backVisible: appPageStack.depth > 1
    }

    Flickable {
        anchors.fill: parent
        ScrollBar.vertical: ScrollBar { interactive: false }
        contentWidth: parent.width
        contentHeight: column.height

        ColumnLayout {
            id: column
            width: parent.width
            spacing: 0

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    spacing: 0

                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Archive")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Archive data after capture")
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }
                    }

                    Switch {
                        id: checkArchiveData
                        Layout.alignment: Qt.AlignRight
                        scale: C.Style.switchScale
                        checked: App.settings.archiveData
                        onCheckedChanged: {
                            App.settings.archiveData = checkArchiveData.checked
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Backup")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Create a backup file")
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/share_variant.svg"
                        opacity: 0.5
                    }
                }

                onClicked: {
                    sendArchive()
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Restore")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Restore data from a backup or share package")
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/file_restore_outline.svg"
                        opacity: 0.5
                    }
                }

                onClicked: {
                    fileDialogRestore.show()
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Reset")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Delete all archived data")
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }
                    }

                    C.SquareIcon {
                        source: "qrc:/icons/clipboard_remove_outline.svg"
                        opacity: 0.5
                    }
                }

                onClicked: {
                    confirmReset.open()
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                Layout.minimumHeight: C.Style.minRowHeight
                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Manage backups")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("View backups and set retention")
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }
                    }

                    C.ChevronRight {}
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/BackupsPage.qml")
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    ColumnLayout {
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Webhook")
                            font.pixelSize: App.settings.font14
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Post data to a webhook")
                            font.pixelSize: App.settings.font12
                            font.capitalization: Font.MixedCase
                            wrapMode: Label.WordWrap
                        }
                    }

                    C.ChevronRight {
                    }
                }

                onClicked: {
                    pushPageImmediate(page, "qrc:/WebhookPage.qml")
                }

                C.HorizontalDivider {}
            }

            C.FooterSpacer {}
        }
    }

    FileDialog {
        id: fileDialogBackup

        property string packageFile: ""

        defaultSuffix: "zip"
        title: qsTr("Data archive")
        currentFolder: StandardPaths.writableLocation(StandardPaths.DesktopLocation)
        nameFilters: [ "Zip files (*.zip)" ]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let success = Utils.copyFile(packageFile, Utils.urlToLocalFile(fileDialogBackup.selectedFile))
            if (success) {
                showToast(qsTr("Archive saved"))
            } else {
                showError(qsTr("Failed to copy archive"))
            }

            Utils.removeFile(packageFile)
        }

        onRejected: {
            Utils.removeFile(packageFile)
        }
    }

    Labs.FileDialog {
        id: fileDialogRestore

        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: [ "Archive packages (*.zip)" ]
        onAccepted: {
            let filePath = App.copyContent(fileDialogRestore.file, App.workPath)
            if (filePath !== "") {
                let result = App.installPackage(filePath, true)
                if (result.success) {
                    showToast(qsTr("Restore successful"))
                } else {
                    showError(result.errorString)
                }

                Utils.removeFile(filePath)
            }
        }

        function show() {
            open()
        }
    }

    C.PopupLoader {
        id: confirmReset

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Reset archive?")
                subText: qsTr("Delete all archived data")
                onConfirmed: {
                    App.projectManager.resetArchive()
                }
            }
        }
    }

    function sendArchive() {
        let packageFile = App.projectManager.createArchivePackage()
        if (packageFile === "") {
            showError(qsTr("Packaging failed"))
            return
        }

        if (App.desktopOS) {
            fileDialogBackup.packageFile = packageFile
            fileDialogBackup.open()
            return
        }

        App.sendFile(packageFile, "Data archive")
    }
}
