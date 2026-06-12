import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

C.ContentPage {
    id: page

    property bool isExportMode: false
    property var historyPoints: []

    QtObject {
        id: internal

        property var currentSighting
        property string currentSightingUid
        property bool canDelete: false
        property int activeSightingIndex: -1
    }

    header: C.PageHeader {
        text: form.project.title
        menuIcon: "qrc:/icons/cancel.svg"
        menuVisible: true
        onMenuClicked: form.popPagesToParent()
    }

    footer: RowLayout {
        id: tabBar

        spacing: 0

        ButtonGroup { id: buttonGroup }

        C.FooterButton {
            id: tabSightings
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: stackLayout.currentIndex === 0
            text: qsTr("Sightings")
            icon.source: checked ? "qrc:/icons/file_multiple.svg" : "qrc:/icons/file_multiple_outline.svg"
            onClicked: stackLayout.currentIndex = 0
        }

        C.FooterButton {
            id: tabActive
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: stackLayout.currentIndex === 1
            text: qsTr("Active")
            icon.source: checked ? "qrc:/icons/file.svg" : "qrc:/icons/file_outline.svg"
            onClicked: stackLayout.currentIndex = 1
        }

        C.FooterButton {
            id: tabSubmit
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: stackLayout.currentIndex === 2
            text: page.isExportMode ? qsTr("Export") : qsTr("Submit")
            icon.source: checked ? "qrc:/icons/upload.svg" : "qrc:/icons/upload_outline.svg"
            onClicked: stackLayout.currentIndex = 2
        }

        C.FooterButton {
            id: tabHistory
            ButtonGroup.group: buttonGroup
            checkable: true
            checked: stackLayout.currentIndex === 3
            text: qsTr("History")
            icon.source: "qrc:/icons/history.svg"
            visible: page.historyPoints.length > 0
            onClicked: stackLayout.currentIndex = 3
        }
    }

    C.SightingListModel {
        id: sightingListModel
        projectUid: form.project.uid
        stateSpace: form.stateSpace
        active: true
    }

    StackLayout {
        id: stackLayout
        anchors.fill: parent
        clip: true

        onCurrentIndexChanged: {
            if (tabBar.children[stackLayout.currentIndex].checkable) {
                tabBar.children[stackLayout.currentIndex].checked = true
            }
        }

        // Tab 0: All Sightings
        C.SightingsListView {
            id: sightingsListView
            active: true
            filter.projectUid: form.project.uid

            header: ItemDelegate {
                width: parent.width

                contentItem: RowLayout {
                    id: row

                    ColumnLayout {
                        id: col

                        Label {
                            Layout.fillWidth: true
                            text: form.provider.currentSightingSummaryText !== "" ? form.provider.currentSightingSummaryText : "??"
                            font.bold: true
                            font.pixelSize: App.settings.font16
                            elide: Label.ElideRight
                            maximumLineCount: 1
                        }

                        Label {
                            id: updateDate
                            Layout.fillWidth: true
                            text: qsTr("Active sighting")
                            opacity: 0.5
                            font.pixelSize: App.settings.font10
                            fontSizeMode: Label.Fit
                            elide: Label.ElideRight
                        }
                    }

                    C.SquareIcon {
                        id: statusIcon
                        source: "qrc:/icons/dots_horizontal.svg"
                        opacity: 0.5
                    }
                }

                onClicked: {
                    setActiveSightingIndex(-1)
                    stackLayout.currentIndex = 1
                }

                C.HorizontalDivider {
                    x: 0
                    width: parent.width
                    height: C.Style.lineWidth2
                }
            }

            onClicked: function (sighting, index) {
                setActiveSightingIndex(index)

                stackLayout.currentIndex = 1
            }
        }

        // Tab 1: Current sighting
        C.SightingListView {
            id: activeSightingView

            Label {
                anchors.fill: parent
                text: qsTr("No data")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: parent.model.count === 0
                opacity: 0.5
                font.pixelSize: App.settings.font20
            }

            RowLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: App.scaleByFontSize(4)
                anchors.rightMargin: App.scaleByFontSize(4)
                anchors.bottomMargin: App.scaleByFontSize(8)
                spacing: 0

                C.CircleButton {
                    icon.source: "qrc:/icons/chevron_left.svg"
                    icon.width: C.Style.iconSize40
                    icon.height: C.Style.iconSize40
                    opacity: enabled ? 0.75 : 0.25
                    enabled: internal.activeSightingIndex >= 0
                    onClicked: {
                        setActiveSightingIndex(internal.activeSightingIndex - 1)
                    }
                }

                C.CircleButton {
                    icon.source: "qrc:/icons/chevron_right.svg"
                    icon.width: C.Style.iconSize40
                    icon.height: C.Style.iconSize40
                    opacity: enabled ? 0.75 : 0.25
                    enabled: internal.activeSightingIndex < sightingsListView.count - 1
                    onClicked: {
                        setActiveSightingIndex(internal.activeSightingIndex + 1)
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                C.CircleButton {
                    icon.source: "qrc:/icons/delete_outline.svg"
                    icon.width: C.Style.iconSize40
                    icon.height: C.Style.iconSize40
                    opacity: enabled ? 0.75 : 0.25
                    enabled: internal.canDelete
                    onClicked: confirmDeletePopup.open()
                }

                C.CircleButton {
                    icon.source: "qrc:/icons/edit.svg"
                    icon.width: C.Style.iconSize40
                    icon.height: C.Style.iconSize40
                    opacity: enabled ? 0.75 : 0.25
                    enabled: !internal.trackOnly
                    onClicked: {
                        if (activeSightingView.sighting !== null) {
                            popPageImmediate(page)
                        } else {
                            form.editSighting(form.stateSpace, activeSightingView.sightingUid)
                        }
                    }
                }
            }
        }

        // Tab 2: Submit
        Item {
            id: submitTab

            property int submitButtonSize: Math.min(C.Style.iconSize64 * 2, submitTab.height / 2)

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Item {
                    height: showQueueButton.height
                }

                // Export mode: simple round button.
                ToolButton {
                    id: exportButton
                    Layout.alignment: Qt.AlignHCenter
                    visible: page.isExportMode
                    width: submitTab.submitButtonSize
                    height: submitTab.submitButtonSize
                    enabled: form.canSubmitData()
                    opacity: enabled ? 0.8 : 0.2
                    icon.source: form.canSubmitData() ? "qrc:/icons/upload.svg" : "qrc:/icons/upload_outline.svg"
                    icon.width: submitTab.submitButtonSize
                    icon.height: submitTab.submitButtonSize

                    onClicked: {
                        form.submitData()

                        setActiveSightingIndex(-1)

                        exportButton.enabled = form.canSubmitData()
                        exportButton.icon.source = form.canSubmitData() ? "qrc:/icons/upload.svg" : "qrc:/icons/upload_outline.svg"
                    }
                }

                // Upload mode: SubmitButton with progress tracking.
                C.SubmitButton {
                    Layout.alignment: Qt.AlignHCenter
                    visible: !page.isExportMode
                    implicitWidth: submitTab.submitButtonSize
                    implicitHeight: submitTab.submitButtonSize
                }

                ToolButton {
                    id: showQueueButton
                    Layout.alignment: Qt.AlignHCenter
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    font.pixelSize: App.settings.font16
                    font.capitalization: Font.AllUppercase
                    text: page.isExportMode ? qsTr("Show exported data") : qsTr("Show upload queue")
                    opacity: 0.8
                    onClicked: {
                        if (page.isExportMode) {
                            form.pushPage("qrc:/ExportsPage.qml", { projectUid: form.project.uid })
                        } else {
                            form.pushPage("qrc:/UploadQueuePage.qml")
                        }
                    }
                }
            }
        }

        ListView {
            id: historyListView
            model: page.historyPoints
            clip: true

            delegate: ItemDelegate {
                width: ListView.view.width
                contentItem: ColumnLayout {
                    spacing: 0
                    width: parent.width
                    Repeater {
                        Layout.fillWidth: true
                        model: modelData.properties
                        Row {
                            Layout.fillWidth: true
                            Label {
                                width: parent.width / 2
                                text: modelData.name
                                font.pixelSize: App.settings.font14
                                font.bold: true
                                wrapMode: Label.WordWrap
                                elide: Label.ElideRight
                            }
                            Label {
                                width: parent.width / 2
                                text: modelData.value
                                font.pixelSize: App.settings.font14
                                wrapMode: Label.WordWrap
                                elide: Label.ElideRight
                            }
                        }
                    }
                }

                C.HorizontalDivider { height: App.scaleByFontSize(2) }
            }
        }
    }

    C.PopupLoader {
        id: confirmDeletePopup

        popupComponent: Component {
            C.ConfirmPopup {
                text: qsTr("Delete sighting?")
                onConfirmed: {
                    form.deleteSighting(activeSightingView.sightingUid)
                    showToast(qsTr("Sighting deleted"))
                    setActiveSightingIndex(internal.activeSightingIndex - 1)
                }
            }
        }
    }

    Component.onCompleted: {
        internal.currentSighting = form.provider.currentSighting
        internal.currentSightingUid = ""
        setActiveSightingIndex(-1)

        stackLayout.currentIndex = 1
    }

    function setActiveSightingIndex(index) {
        if (index === sightingsListView.count) {
            index -= 1
        }

        if (index < 0 || index >= sightingsListView.count) {
            activeSightingView.sighting = internal.currentSighting
            activeSightingView.sightingUid = ""
            internal.activeSightingIndex = -1
            internal.canDelete = false
            return
        }

        activeSightingView.sighting = null
        activeSightingView.sightingUid = sightingsListView.model.get(index).sightingUid
        internal.activeSightingIndex = index
        internal.canDelete = true
    }
}
