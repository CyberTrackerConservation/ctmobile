import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

ListViewV {
    id: root

    property alias active: sightingListModel.active
    property alias canSubmitData: sightingListModel.canSubmit
    property alias count: sightingListModel.count
    property alias filter: sightingListModel.filter
    property alias showNullGroup: sightingListModel.showNullGroup
    property bool checkable: false

    property alias checkedCount: sightingListModel.checkedCount

    property bool showNoData: true
    property bool showStatusIcon: true

    QtObject {
        id: internal
        property int checkedChange: 0
    }

    signal clicked(var sighting, int index)
    signal longPress(var sighting, int index)

    model: C.SightingListProxyModel {
        id: sightingListModel
    }

    section.property: "section"
    section.criteria: ViewSection.FullString
    section.delegate: ItemDelegate {
        id: sd
        width: ListView.view.width
        contentItem: RowLayout {
            CheckableBox {
                id: sectionCheck
                visible: root.checkable
                checked: {
                    internal.checkedChange
                    return sightingListModel.getSectionChecked(section)
                }

                onCheckedChanged: {
                    if (checked !== sightingListModel.getSectionChecked(section)) {
                        sightingListModel.setSectionChecked(section, checked)
                    }
                }
            }

            Label {
                id: controlLabel
                Layout.fillWidth: true
                wrapMode: Label.WordWrap
                text: section
                font.bold: true
                font.pixelSize: App.settings.font16
                verticalAlignment: Text.AlignVCenter
            }

            SquareIcon {
                Layout.alignment: Qt.AlignRight
                source: sightingListModel.getExpanded(section) ? "qrc:/icons/chevron_up.svg" : "qrc:/icons/chevron_down.svg"
                size: Style.minRowHeight
                opacity: 0.5
            }
        }

        background: Rectangle {
            anchors.fill: parent
            opacity: 0.25
            color: Material.primary
        }

        onClicked: {
            let expanded = sightingListModel.getExpanded(section)
            sightingListModel.setExpanded(section, !expanded)
        }
    }

    delegate: ItemDelegate {
        id: d

        Binding { target: background; property: "color"; value: Utils.changeAlpha(colorContent || Style.colorContent, 50) }
        width: ListView.view.width
        height: model.expanded ? implicitHeight : 0
        enabled: !model.submitted
        opacity: enabled ? 1.0 : 0.5
        clip: true

        contentItem: RowLayout {
            id: row

            CheckableBox {
                id: sightingCheck
                visible: root.checkable
                checked: model.checked
                onCheckedChanged: {
                    sightingListModel.setChecked(model.index, checked)
                    internal.checkedChange++
                }
            }

            SquareIcon {
                size: Style.minRowHeight
                source: model.summaryIcon || ""
                recolor: model.summaryIcon !== undefined && model.summaryIcon.startsWith("qrc:/icons/")
                opacity: recolor ? 0.5 : 1.0
            }

            ColumnLayout {
                id: col

                Label {
                    Layout.fillWidth: true
                    text: model.trackOnly ? qsTr("Track") : (model.summaryText !== "" ? model.summaryText : "??")
                    font.bold: true
                    font.pixelSize: App.settings.font16
                    elide: Label.ElideRight
                    maximumLineCount: 1
                }

                Label {
                    Layout.fillWidth: true
                    text: model.trackOnly ? model.trackFileSummary : qsTr("Track") + ": " + model.trackFileSummary
                    font.pixelSize: App.settings.font14
                    elide: Label.ElideRight
                    maximumLineCount: 1
                    visible: model.trackFileSummary !== ""
                }

                RowLayout {
                    spacing: App.scaleByFontSize(6)
                    Layout.fillWidth: true
                    SquareIcon {
                        visible: model.readonly === true
                        size: updateDate.height * 1.2
                        source: "qrc:/icons/edit_off.svg"
                        opacity: 0.8
                    }
                    SquareIcon {
                        visible: model.archived === true
                        size: updateDate.height * 1.2
                        source: "qrc:/icons/archive_outline.svg"
                        opacity: 0.8
                    }
                    SquareIcon {
                        visible: model.note !== ""
                        size: updateDate.height * 1.2
                        source: "qrc:/icons/note_text_outline.svg"
                        opacity: 0.8
                    }
                    Label {
                        id: updateDate
                        Layout.fillWidth: true
                        text: App.formatDateTime(model.updateTime)
                        opacity: 0.5
                        font.pixelSize: App.settings.font10
                        fontSizeMode: Label.Fit
                        elide: Label.ElideRight
                    }
                }
            }

            SquareIcon {
                id: statusIcon
                source: model.statusIcon
                opacity: 0.5
                visible: root.showStatusIcon

                Timer {
                    running: App.transcribeQueue.workingDataUid === model.sightingUid
                    interval: 500
                    repeat: true
                    onTriggered: statusIcon.opacity = statusIcon.opacity > 0 ? 0.0 : 0.5
                    onRunningChanged: if (!running) statusIcon.opacity = 0.5
                }
            }
        }

        HorizontalDivider {
            height: {
                if (model.index === -1 || model.index >= sightingListModel.count - 1) {
                    return Style.lineWidth1
                }

                let curr = model
                let next = sightingListModel.get(model.index + 1)
                if (curr.type !== next.type) {
                    return Style.lineWidth2
                }

                if (curr.type !== "sighting") {
                    return Style.lineWidth1
                }

                if (curr.completed !== next.completed || curr.submitted !== next.submitted || curr.exported !== next.exported) {
                    return Style.lineWidth2
                }

                return Style.lineWidth1
            }
        }

        onPressAndHold: {
            if (!root.checkable && !model.submitted && !model.readonly && model.canEdit && model.canDelete) {
                popupLongPress.open({ sighting: model, trackOnly: model.trackOnly, index: model.index })
            } else {
                root.longPress(model, model.index)
            }
        }

        onClicked: {
            if (root.checkable) {
                return
            }

            if (!model.submitted) {
                popupLongPress.close()
                root.clicked(model, model.index)
            }
        }
    }

    Label {
        anchors.fill: parent
        text: qsTr("No data")
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        visible: root.showNoData && (sightingListModel.count === 0)
        opacity: 0.5
        font.pixelSize: App.settings.font20
    }

    PopupLoader {
        id: popupLongPress

        popupComponent: Component {
            OptionsPopup {
                property var sighting
                property bool trackOnly
                property int index

                text: getSightingTitle(sighting)
                model: [
                    { text: trackOnly ? qsTr("View track") : qsTr("Continue editing"), icon: trackOnly ? "qrc:/icons/map_marker_multiple_outline.svg" : "qrc:/icons/pencil_outline.svg"  },
                    { text: trackOnly ? qsTr("Delete track") : qsTr("Delete sighting"), icon: "qrc:/icons/delete_outline.svg", delay: true }
                ]

                onClicked: function (index) {
                    switch (index) {
                    case 0:
                        root.clicked(sighting, index)
                        break

                    case 1:
                        popupLongPress.close()
                        form.deleteSighting(sighting.sightingUid)
                        showToast(trackOnly ? qsTr("Track deleted") : qsTr("Sighting deleted"))
                        break
                    }
                }
            }
        }
    }

    PopupLoader {
        id: popupSubmit
        popupComponent: Component {
            ConfirmPopup {
                icon: "qrc:/icons/upload_multiple.svg"
                text: qsTr("Submit data?")
                confirmDelay: false
                onConfirmed: {
                    form.submitData()
                }
            }
        }
    }

    function getSightingTitle(sighting) {
        let title = !sighting ? "" : (sighting.trackOnly ? qsTr("Track") : sighting.summaryText)
        return title === "" ? "??" : title
    }

    function submit() {
        if (!form.canSubmitData()) {
            App.showError(qsTr("No supported in demo mode"))
            return
        }

        popupSubmit.open()
    }

    function clearChecked() {
        sightingListModel.clearChecked()
    }

    function deleteChecked() {
        sightingListModel.deleteChecked()
    }

    function getChecked() {
        return sightingListModel.getChecked()
    }
}
