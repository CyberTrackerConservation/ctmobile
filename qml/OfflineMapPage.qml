import QtCore
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.platform as Labs
import QtMultimedia
import CyberTracker as C

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
                let layer = layerListModel.get(listView.currentIndex)
                let shareUrl = App.offlineMapManager.getShareUrl(layer.id)

                if (shareUrl !== "") {
                    pushPageImmediate(page, "qrc:/ShareUrlPage.qml", {
                        title: layer.name,
                        url: shareUrl,
                    })
                    return
                }

                let packageFile = App.offlineMapManager.createPackage(layer.id)
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

        C.FooterButton {
            text: qsTr("Update")
            icon.source: "qrc:/icons/autorenew.svg"
            enabled: App.offlineMapManager.hasUpdateableMaps
            highlight: App.offlineMapManager.updatesAvailable
            onClicked: {
                appWindow.updateMaps()
            }
        }
    }

    C.MapLayerListModel {
        id: layerListModel
        addTypeHeaders: false
        addBaseLayers: false
        addDataLayers: false
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
                        text: modelData.timestamp ? modelData.timestamp : ""
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
                    enabled: {
                        let i = listView.currentIndex
                        if (i <= 0 || layerListModel.count < 2) return false
                        // The model partitions vectors above rasters; don't let the move cross that boundary.
                        return layerListModel.get(i - 1).vector === layerListModel.get(i).vector
                    }
                    onClicked: {
                        let index = listView.currentIndex
                        App.offlineMapManager.moveLayer(layerListModel.get(index).id, -1)
                        listView.currentIndex = index - 1
                    }
                }

                ToolButton {
                    width: C.Style.toolButtonSize
                    height: C.Style.toolButtonSize
                    icon.source: "qrc:/icons/chevron_down.svg"
                    visible: d.highlighted
                    enabled: {
                        let i = listView.currentIndex
                        if (i < 0 || i >= layerListModel.count - 1) return false
                        return layerListModel.get(i + 1).vector === layerListModel.get(i).vector
                    }
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

    Settings {
        location: App.iniUrl
        category: "OfflineMapsDialog"
        property alias fileDialogFolder: fileDialog.folder
        property alias fileDialogPackageFolder: fileDialogPackage.currentFolder
    }

    Labs.FileDialog {
        id: fileDialog

        folder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        nameFilters: [ "Offline maps (*.zip *.tpk *.vtpk *.mbtiles *.kml *.geojson)" ]
        onAccepted: {
            let filePath = App.copyContent(fileDialog.file, App.workPath)
            if (filePath !== "") {
                if (App.offlineMapManager.canInstallPackage(filePath)) {
                    App.installPackage(filePath, true)
                } else {
                    showError(qsTr("Not a map package"))
                }

                Utils.removeFile(filePath)
            }
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
        currentFolder: StandardPaths.writableLocation(StandardPaths.DesktopLocation)
        nameFilters: [ "CyberTracker map packages (*.zip)" ]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let targetFile = Utils.urlToLocalFile(fileDialogPackage.selectedFile)
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

    C.OptionsPopup {
        id: confirmDelete

        property string layerId
        property string packageId
        property bool packageHasMultipleLayers: false

        text: qsTr("Delete?")
        icon: "qrc:/icons/alert_circle_outline.svg"

        model: [
            { text: qsTr("Delete layer"), icon: "qrc:/icons/delete_outline.svg", delay: true },
            { text: qsTr("Delete package"), icon: "qrc:/icons/delete_outline.svg", delay: true,
              visible: confirmDelete.packageHasMultipleLayers },
            { text: qsTr("Cancel"), icon: "qrc:/icons/cancel.svg" },
        ]

        onClicked: function (index) {
            switch (index) {
            case 0:
                listView.currentIndex = -1
                App.offlineMapManager.deleteLayer(confirmDelete.layerId)
                break
            case 1:
                listView.currentIndex = -1
                App.offlineMapManager.deleteLayersByPackageId(confirmDelete.packageId)
                break
            }
            confirmDelete.layerId = ""
            confirmDelete.packageId = ""
        }

        function show(layer) {
            confirmDelete.layerId = layer.id
            confirmDelete.packageId = App.offlineMapManager.getLayerPackageId(layer.id)
            confirmDelete.packageHasMultipleLayers = App.offlineMapManager.getPackageLayerCount(confirmDelete.packageId) > 1
            open()
        }
    }
}
