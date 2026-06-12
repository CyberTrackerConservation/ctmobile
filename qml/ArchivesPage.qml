import QtCore
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import CyberTracker as C

C.ContentPage {
    id: page

    property bool active: false

    Component.onCompleted: {
        listView.filter.load(App.settings.sightingFilter)
        listView.filter.completedOnly = true
    }

    Connections {
        target: App.settings
        function onSightingFilterChanged() {
            listView.filter.load(App.settings.sightingFilter)
            listView.filter.completedOnly = true
        }
    }

    header: C.PageHeader {
        text: qsTr("Data")
        backVisible: false
    }

    state: "single"
    states: [
        State {
            name: "single"
            StateChangeScript {
                script: {
                }
            }
        },
        State {
            name: "multiple"
            StateChangeScript {
                script: {
                }
            }
        }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ItemDelegate {
            Layout.fillWidth: true
            Layout.minimumHeight: C.Style.minRowHeight
            contentItem: RowLayout {
                ToolButton {
                    id: multiSelectButton
                    checkable: true
                    icon.source: checked ? "qrc:/icons/checkbox_multiple_marked.svg" : "qrc:/icons/checkbox_multiple_outline.svg"
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    opacity: 0.5
                    Material.accent: Material.foreground
                    enabled: listView.count > 0
                    checked: page.state === "multiple"
                    onCheckedChanged: {
                        listView.clearChecked()
                        page.state = checked ? "multiple" : "single"
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: listView.filter.summary
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.WordWrap
                    visible: page.state === "single"
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("%1 selected").arg(listView.checkedCount)
                    font.pixelSize: App.settings.font16
                    font.bold: true
                    wrapMode: Label.WordWrap
                    visible: page.state === "multiple"
                }

                ToolButton {
                    id: groupByButton
                    icon.source: "qrc:/icons/format_list_group.svg"
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    opacity: 0.5
                    visible: page.state === "single"
                    onClicked: {
                        pushPageImmediate(page, "qrc:/SightingGroupByPage.qml")
                    }
                }

                ToolButton {
                    id: filterButton
                    icon.source: "qrc:/icons/filter_outline.svg"
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    opacity: 0.5
                    visible: page.state === "single"
                    onClicked: {
                        pushPageImmediate(page, "qrc:/SightingFilterPage.qml")
                    }
                }

                ToolButton {
                    id: shareButton
                    icon.source: "qrc:/icons/share_variant_outline.svg"
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    opacity: 0.5
                    visible: page.state === "multiple"
                    enabled: listView.checkedCount > 0
                    onClicked: {
                        page.sendArchive()
                    }
                }

                ToolButton {
                    id: deleteButton
                    icon.source: "qrc:/icons/delete_outline.svg"
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    opacity: 0.5
                    visible: page.state === "multiple"
                    enabled: listView.checkedCount > 0
                    onClicked: {
                        confirmDelete.open()
                    }
                }
            }

            onClicked: {
                filterButton.clicked()
            }

            C.HorizontalDivider {
                height: C.Style.lineWidth2
            }

            background: Rectangle {
                anchors.fill: parent
                opacity: 0.25
                color: "black"
            }
        }

        C.SightingsListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            width: parent.width
            height: parent.height
            showNoData: true
            filter.projectUid: "*"
            filter.stateSpace: "*"
            showNullGroup: true
            checkable: page.state === "multiple"
            active: page.active

            onClicked: function (sighting, index) {
                if (sighting.trackOnly) {
                    showTrackFile(page, sighting.trackFileName, sighting.trackFileSummary)
                    return
                }

                appPageStack.pushPageImmediate("qrc:/InspectorPage.qml", { inspectList: model, initialIndex: index })
            }

            onLongPress: function (sighting, index) {
                if (page.state === "single") {
                    listView.clearChecked()
                    page.state = "multiple"
                }
            }

            onCountChanged: {
                if (listView.count === 0) {
                    page.state = "single"
                }
            }
        }
    }

    C.PopupLoader {
        id: confirmDelete

        popupComponent: Component {
            C.ConfirmPopup {
                confirmText: qsTr("Yes, delete %1").arg(listView.checkedCount === 1 ? qsTr("it") : qsTr("them"))
                onConfirmed: {
                    listView.deleteChecked()
                }

                Component.onCompleted: {
                    text = qsTr("Delete %1 %2?").arg(listView.checkedCount).arg(listView.checkedCount === 1 ? qsTr("sighting") : qsTr("sightings"))
                }
            }
        }
    }

    FileDialog {
        id: fileDialogShare

        property string packageFile: ""

        defaultSuffix: "zip"
        title: qsTr("Shared data")
        currentFolder: StandardPaths.writableLocation(StandardPaths.DesktopLocation)
        nameFilters: [ "Zip files (*.zip)" ]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let success = Utils.copyFile(packageFile, Utils.urlToLocalFile(fileDialogShare.selectedFile))
            if (success) {
                showToast(qsTr("Shared data saved"))
            } else {
                showError(qsTr("Failed to copy package"))
            }

            Utils.removeFile(packageFile)
        }

        onRejected: {
            Utils.removeFile(packageFile)
        }
    }

    function sendArchive() {

        let packageFile = App.projectManager.createArchivePackage("ShareData.zip", listView.getChecked())
        if (packageFile === "") {
            showError(qsTr("Packaging failed"))
            return
        }

        if (App.desktopOS) {
            fileDialogShare.packageFile = packageFile
            fileDialogShare.open()
            return
        }

        App.sendFile(packageFile, "Shared data")
    }
}
