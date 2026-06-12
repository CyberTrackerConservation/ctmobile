import QtCore
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Upload queue")
        menuVisible: App.uploadQueue.processing
        menuEnabled: false
        menuIcon: "qrc:/icons/sync.svg"
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Share")
            icon.source: "qrc:/icons/share_variant_outline.svg"
            enabled: uploadQueueModel.count !== 0
            onClicked: {
                sendSnapshot()
            }
        }

        C.FooterButton {
            text: qsTr("Reset")
            icon.source: "qrc:/icons/delete_outline.svg"
            enabled: uploadQueueModel.count !== 0
            onClicked: {
                confirmResetQueue.open()
            }
        }

        C.FooterButton {
            text: qsTr("Clear sent")
            icon.source: "qrc:/icons/notification_clear_all.svg"
            enabled: uploadQueueModel.count !== 0
            onClicked: {
                confirmClearQueue.open()
            }
        }

        C.FooterButton {
            text: qsTr("Start")
            icon.source: "qrc:/icons/upload_multiple.svg"
            enabled: uploadQueueModel.count !== 0 && !App.uploadQueue.processing
            onClicked: {
                App.uploadQueue.kick()
            }
        }
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent

        model: C.UploadQueueModel {
            id: uploadQueueModel
        }

        delegate: ItemDelegate {
            width: ListView.view.width
            opacity: modelData.complete ? 0.5 : 1.0

            contentItem: RowLayout {
                ColumnLayout {
                    Layout.fillWidth: true

                    Label {
                        Layout.fillWidth: true
                        text: modelData.projectTitle
                        font.pixelSize: App.settings.font10
                        elide: Label.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData.summary === "" ? "??" : modelData.summary
                        font.pixelSize: App.settings.font14
                        elide: Label.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: {
                            let result = modelData.subtitle
                            if (modelData.working) {
                                result = qsTr("Uploading")
                            } else if (modelData.error === "") {
                                result = qsTr("Queued")
                            }

                            return result
                        }
                        font.pixelSize: App.settings.font12
                        wrapMode: Label.NoWrap
                        elide: Label.ElideRight
                    }
                }

                Item {
                    property real angle: 0

                    width: C.Style.iconSize32
                    height: C.Style.iconSize32
                    rotation: modelData.working ? angle : 0

                    NumberAnimation on angle {
                        from: 0
                        to: 360
                        duration: 2000
                        loops: Animation.Infinite
                        running: modelData.working
                    }

                    C.SquareIcon {
                        size: C.Style.iconSize32
                        visible: modelData.complete || modelData.working
                        source: modelData.complete ?  "qrc:/icons/check_all.svg" : "qrc:/icons/progress_upload.svg"
                        opacity: modelData.complete ? 1.0 : 0.5
                    }
                }
            }

            C.HorizontalDivider {}
        }
    }

    Label {
        anchors.centerIn: listView
        visible: uploadQueueModel.count === 0
        font.pixelSize: App.settings.font16
        text: qsTr("Empty")
        opacity: 0.5
    }

    C.PopupLoader {
        id: confirmResetQueue

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Reset queue?")
                subText: qsTr("Delete all unsent data")
                onConfirmed: {
                    App.uploadQueue.resetQueue()
                }
            }
        }
    }

    C.PopupLoader {
        id: confirmClearQueue

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Clear queue?")
                subText: qsTr("Remove successfully sent items")
                confirmDelay: false
                onConfirmed: {
                    App.uploadQueue.clearQueue()
                }
            }
        }
    }

    FileDialog {
        id: fileDialogSnapshot

        property string snapshotFile: ""

        defaultSuffix: "zip"
        title: qsTr("Upload queue snapshot")
        currentFolder: StandardPaths.writableLocation(StandardPaths.DesktopLocation)
        nameFilters: [ "Zip files (*.zip)" ]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let success = Utils.copyFile(snapshotFile, Utils.urlToLocalFile(fileDialogSnapshot.selectedFile))
            if (success) {
                showToast(qsTr("Snapshot saved"))
            } else {
                showError(qsTr("Failed to copy archive"))
            }

            Utils.removeFile(snapshotFile)
        }

        onRejected: {
            Utils.removeFile(snapshotFile)
        }
    }

    function sendSnapshot() {
        let snapshotFile = App.uploadQueue.createSnapshot()
        if (snapshotFile === "") {
            return
        }

        if (App.desktopOS) {
            fileDialogSnapshot.snapshotFile = snapshotFile
            fileDialogSnapshot.open()
            return
        }

        App.sendFile(snapshotFile, "Upload queue snapshot")
    }
}
