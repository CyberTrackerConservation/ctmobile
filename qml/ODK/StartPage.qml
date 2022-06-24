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
        text: form.project.title
        menuIcon: "qrc:/icons/settings.svg"
        menuVisible: true

        onMenuClicked: {
            form.pushPage("qrc:/ODK/SettingsPage.qml")
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
                width: parent.width
                height: 2
                color: Material.theme === Material.Dark ? "#FFFFFF" : "#000000"
                opacity: 0.12
            }
        }

        RowLayout {
            id: buttonRow
            spacing: 0
            Layout.fillWidth: true
            property int buttonCount: 5
            property int buttonWidth: page.width / buttonCount
            property var buttonColor: Material.theme === Material.Dark ? Material.foreground : Material.primary

            ToolButton {
                id: submitButton
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("Submit")
                enabled: getSubmitEnabled()
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/upload_multiple.svg"
                Material.foreground: buttonRow.buttonColor
                onClicked: popupSubmit.open()
            }

            ToolButton {
                id: mapButton
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

            ToolButton {
                id: newButton
                Layout.preferredWidth: buttonRow.buttonWidth
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: qsTr("New")
                font.pixelSize: App.settings.font10
                font.capitalization: Font.MixedCase
                display: Button.TextUnderIcon
                icon.source: "qrc:/icons/plus.svg"
                Material.foreground: buttonRow.buttonColor
                enabled: form.provider.parserError === ""
                onClicked: {
                    form.newSighting()
                    form.snapCreateTime()
                    form.saveSighting()
                    if (form.project.wizardMode) {
                        form.pushWizardPage(form.rootRecordUid)
                    } else {
                        form.pushPage("qrc:/ODK/FormPage.qml")
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
            source: App.projectManager.getFileUrl(form.project.uid, form.project.icon)
            fillMode: Image.PreserveAspectFit
        }
    }

    C.ListViewV {
        id: listView
        anchors.fill: parent
        model: C.SightingListModel {
            id: sightingListModel
            onCountChanged: submitButton.enabled = getSubmitEnabled()
            onDataChanged: submitButton.enabled = getSubmitEnabled()
        }

        delegate: ItemDelegate {
            id: d

            Binding { target: background; property: "color"; value: Utils.changeAlpha(C.Style.colorContent, 50) }
            width: ListView.view.width
            enabled: !modelData.submitted
            opacity: enabled ? 1.0 : 0.5

            contentItem: RowLayout {
                id: row

                ColumnLayout {
                    id: col

                    Label {
                        Layout.fillWidth: true
                        text: modelData.summary
                        font.bold: true
                        font.pixelSize: App.settings.font16
                        elide: Label.ElideRight
                        maximumLineCount: 1
                    }

                    Label {
                        Layout.fillWidth: true
                        text: App.formatDateTime(modelData.updateTime)
                        opacity: 0.5
                        font.pixelSize: App.settings.font10
                        fontSizeMode: Label.Fit
                    }
                }

                Image {
                    width: col.height * 0.5
                    sourceSize.width: width
                    source: {
                        if (modelData.submitted) {
                            return "qrc:/icons/cloud_upload_outline.svg"
                        } else if (modelData.completed) {
                            return "qrc:/icons/check_bold.svg"
                        } else {
                            return "qrc:/icons/dots_horizontal.svg"
                        }
                    }
                    opacity: 0.8
                    layer {
                        enabled: true
                        effect: ColorOverlay {
                            color: Material.foreground
                        }
                    }
                }
            }

            onPressAndHold: {
                if (!modelData.submitted) {
                    popupLongPress.open({ sighting: modelData })
                }
            }

            onClicked: {
                if (!modelData.submitted) {
                    editSighting(modelData)
                }
            }
        }
    }

    C.PopupLoader {
        id: popupLongPress

        popupComponent: Component {
            C.OptionsPopup {
                property var sighting

                text: getSightingTitle(sighting)
                model: [
                    { text: qsTr("Continue editing"), icon: "qrc:/icons/pencil_outline.svg" },
                    { text: qsTr("Delete sighting"), icon: "qrc:/icons/delete_outline.svg", delay: true }
                ]

                onClicked: function (index) {
                    switch (index) {
                    case 0:
                        editSighting(sighting)
                        break

                    case 1:
                        deleteSighting(sighting)
                        showToast(qsTr("Sighting deleted"))
                        break
                    }
                }
            }
        }
    }

    C.PopupLoader {
        id: popupSubmit
        popupComponent: Component {
            C.ConfirmPopup {
                icon: "qrc:/icons/upload_multiple.svg"
                text: qsTr("Submit data?")
                onConfirmed: {
                    form.provider.submitData()
                    submitButton.enabled = getSubmitEnabled()
                }
            }
        }
    }

    Component.onCompleted: {
        if (form.provider.parserError !== "") {
            showError(qsTr("Form error") + ": " + form.provider.parserError)
        }
    }

    function getSubmitEnabled() {
        let result = false
        for (let i = 0; i < sightingListModel.count; i++) {
            if (!sightingListModel.get(i).submitted && sightingListModel.get(i).completed) {
                result = true
                break
            }
        }

        return result
    }

    function getSightingTitle(sighting) {
        let title = sighting === undefined ? "" : sighting.summary
        return title === "" ? "??" : title
    }

    function editSighting(sighting) {
        popupLongPress.close()
        form.loadSighting(sighting.rootRecordUid)
        form.setPageStack([{ pageUrl: form.startPage}])

        if (form.project.wizardMode) {
            form.pushWizardPage(form.rootRecordUid)
        } else {
            form.wizard.reset()
            form.pushPage("qrc:/ODK/FormPage.qml")
        }
    }

    function deleteSighting(sighting) {
        popupLongPress.close()
        form.removeSighting(sighting.rootRecordUid)
    }
}
