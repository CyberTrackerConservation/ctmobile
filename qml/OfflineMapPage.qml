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
        text: qsTr("Offline maps")
    }

    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            text: qsTr("Share")
            icon.source: "qrc:/icons/share_variant_outline.svg"
            enabled: listView.currentIndex !== -1
            onClicked: {
                let packageFile = App.offlineMapManager.createPackage(layerListModel.get(listView.currentIndex).id)
                if (packageFile === "") {
                    showError(qsTr("Packaging failed"))
                    return
                }

                if (App.desktopOS) {
                    fileDialogPackage.packageFile = packageFile
                    fileDialogPackage.open()
                    return
                }

                App.sendFile(packageFile, qsTr("Offline map"))
            }
        }

        C.FooterButton {
            text: qsTr("Delete")
            icon.source: "qrc:/icons/delete_outline.svg"
            enabled: listView.currentIndex !== -1
            onClicked: {
                confirmDelete.show(layerListModel.get(listView.currentIndex))
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

    C.MapLayerListModel {
        id: layerListModel
        addTypeHeaders: false
        addBaseLayers: false
        addFormLayers: false
        addProjectLayers: false
        addOfflineLayers: true
    }

    Label {
        anchors.fill: parent
        text: qsTr("No layers")
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        visible: layerListModel.count === 0
        opacity: 0.5
        font.pixelSize: App.settings.font20
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent
        visible: layerListModel.count !== 0

        currentIndex: -1

        model: layerListModel

        delegate: C.HighlightDelegate {
            id: d
            width: ListView.view.width
            highlighted: ListView.isCurrentItem

            contentItem: RowLayout {
                C.SquareIcon {
                    size: C.Style.iconSize48
                    source: modelData.vector ? "qrc:/icons/vector_polyline.svg" : "qrc:/icons/checkerboard.svg"
                    opacity: 0.5
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Label.WordWrap
                        text: modelData.name
                        font.pixelSize: App.settings.font18
                        elide: Label.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData.timestamp
                        font.pixelSize: App.settings.font12
                        opacity: 0.5
                        elide: Label.ElideRight
                    }
                }


                ToolButton {
                    width: C.Style.toolButtonSize
                    height: C.Style.toolButtonSize
                    icon.source: "qrc:/icons/chevron_up.svg"
                    visible: d.highlighted
                    enabled: listView.currentIndex > 0
                    onClicked: {
                        let index = listView.currentIndex
                        App.offlineMapManager.moveLayer(layerListModel.get(index).id, -1)
                        listView.currentIndex = index - 1
                    }
                }

                ToolButton {
                    width: C.Style.toolButtonSize
                    height: C.Style.toolButtonSize
                    enabled: listView.currentIndex >= 0 && listView.currentIndex < layerListModel.count - 1
                    icon.source: "qrc:/icons/chevron_down.svg"
                    visible: d.highlighted
                    onClicked: {
                        let index = listView.currentIndex
                        App.offlineMapManager.moveLayer(layerListModel.get(index).id, 1)
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
        category: "OfflineMapsDialog"
        property alias fileDialogFolder: fileDialog.folder
        property alias fileDialogPackageFolder: fileDialogPackage.folder
    }

    FileDialog {
        id: fileDialog

        folder: shortcuts.home
        nameFilters: [ "Offline maps (*.zip *.tpk *.vtpk *.mbtiles *.kml *.geojson)" ]
        onAccepted: {
            let filePath = Utils.urlToLocalFile(fileDialog.fileUrl)
            if (!App.offlineMapManager.canInstallPackage(filePath)) {
                showError(qsTr("Not a map package"))
                return
            }

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
        title: qsTr("Save map package")
        folder: shortcuts.desktop
        nameFilters: [ "CyberTracker map packages (*.zip)" ]
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

    C.ConfirmPopup {
        id: confirmDelete
        property string layerId
        confirmText: qsTr("Yes, delete it")
        onConfirmed: {
            listView.currentIndex = -1
            App.offlineMapManager.deleteLayer(layerId)
            layerId = ""
        }

        function show(layer) {
            text = qsTr("Delete layer?")
            layerId = layer.id
            open()
        }
    }
}
