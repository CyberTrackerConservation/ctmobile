import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: "EarthRanger"
        menuIcon: "qrc:/icons/settings.svg"
        menuVisible: true
        onMenuClicked: {
            form.pushPage("qrc:/EarthRanger/SettingsPage.qml")
        }
    }

    footer: ColumnLayout {
        spacing: 0
        width: parent.width

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "transparent"

            Rectangle {
                x: 0
                y: 0
                width: parent.width - saveHighlight.width
                height: 2
                color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
                opacity: 0.12
            }

            Rectangle {
                x: saveHighlight.x
                y: 0
                width: saveHighlight.width
                height: 2
                color: saveButton.enabled ? Material.accent : (Material.theme === Material.Dark ? "#FFFFFF" : "#000000")
                opacity: saveButton.enabled ? 0.5 : 0.12
            }
        }

        RowLayout {
            id: buttonRow
            spacing: 0
            Layout.fillWidth: true
            property int buttonCount: 4
            property int buttonWidth: page.width / buttonCount
            property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Sync")
                enabled: listView.model.count !== 0
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/autorenew.svg"
                Material.foreground: buttonRow.buttonColor
                onClicked: App.taskManager.resumePausedTasks(form.project.uid)
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                enabled: listView.model.count !== 0
                text: qsTr("Clear")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/delete_outline.svg"
                Material.foreground: buttonRow.buttonColor
                onClicked: confirmClear.open()
            }

            ToolButton {
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Map")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/map_outline.svg"
                Material.foreground: buttonRow.buttonColor
                onClicked: form.pushPage("qrc:/MapsPage.qml")
            }

            Rectangle {
                id: saveHighlight
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: saveButton.enabled ? Material.accent : Material.background

                ToolButton {
                    id: saveButton
                    anchors.fill: parent
                    text: qsTr("New")
                    font.pixelSize: App.settings.font10
                    font.capitalization: Font.MixedCase
                    display: Button.TextUnderIcon
                    icon.source: "qrc:/icons/plus.svg"
                    Material.foreground: "white"
                    Material.background: Material.accent
                    flat: true
                    onClicked: {
                        form.newSighting()
                        form.pushPage("qrc:/EarthRanger/EventTypePage.qml")
                    }
                }
            }
        }
    }

    // Watermark.
    Control {
        anchors.fill: parent
        padding: parent.width / 4
        opacity: 0.15
        contentItem: Image {
            source: "qrc:/EarthRanger/logo.svg"
            fillMode: Image.PreserveAspectFit
        }
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent

        model: C.SightingListModel {}
        delegate: ItemDelegate {
            width: ListView.view.width
            Binding { target: background; property: "color"; value: Utils.changeAlpha(C.Style.colorContent, 50) }

            contentItem: RowLayout {
                id: row
                Image {
                    source: form.getElementIcon(getReportElementUid(modelData))
                    sourceSize.width: col.height
                }

                ColumnLayout {
                    id: col
                    Label {
                        id: categoryLabel
                        Layout.fillWidth: true
                        text: form.getElementName(form.getElement(getReportElementUid(modelData)).parentElement.uid)
                        font.bold: true
                        font.pixelSize: App.settings.font16
                    }
                    Label {
                        id: reportLabel
                        Layout.fillWidth: true
                        text: form.getElementName(getReportElementUid(modelData))
                        font.pixelSize: App.settings.font14
                    }

                    RowLayout {
                        spacing: 6
                        Layout.fillWidth: true
                        Label {
                            id: uploadDate
                            text: App.formatDateTime(modelData.updateTime)
                            opacity: 0.5
                            font.pixelSize: App.settings.font10
                            fontSizeMode: Label.Fit
                        }
                        Image {
                            visible: modelData.readonly === true
                            width: uploadDate.height * 1.2
                            sourceSize.width: width
                            source: "qrc:/icons/edit_off.svg"
                            opacity: 0.8
                            layer {
                                enabled: true
                                effect: ColorOverlay {
                                    color: Material.foreground
                                }
                            }
                         }
                     }
                }

                Image {
                    width: col.height * 0.5
                    sourceSize.width: width
                    source: modelData.uploaded === true ? "qrc:/icons/cloud_done.svg" : "qrc:/icons/cloud_queue.svg"
                    opacity: 0.8
                    layer {
                        enabled: true
                        effect: ColorOverlay {
                            color: Material.foreground
                        }
                    }
                }
            }

            onClicked: {
                if (modelData.readonly) {
                    showToast(qsTr("Report cannot be changed"))
                    return
                }

                listView.currentIndex = model.index
                form.loadSighting(modelData.rootRecordUid)
                let pageStack = form.getPageStack()
                pageStack = [pageStack[0], { pageUrl: "qrc:/EarthRanger/AttributesPage.qml", params: { elementUid: form.getFieldValue("reportUid") } }]
                form.setPageStack(pageStack)
                form.loadPages()
            }
        }
    }

    function getReportElementUid(sighting) {
        return sighting.records[0].fieldValues["reportUid"]
    }

    C.ConfirmPopup {
        id: confirmClear
        text: qsTr("Clear uploaded data?")
        confirmText: qsTr("Yes, clear it")
        onConfirmed: {
            App.taskManager.waitForTasks(form.project.uid)
            form.removeUploadedSightings()
        }
    }
}
