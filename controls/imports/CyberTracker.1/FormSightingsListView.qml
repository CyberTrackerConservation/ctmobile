import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

ListViewV {
    id: root

    property alias canSubmit: sightingListModel.canSubmit
    property bool showNoData: true

    signal clicked(var sighting, int index)

    model: C.SightingListModel {
        id: sightingListModel
    }

    delegate: ItemDelegate {
        id: d

        Binding { target: background; property: "color"; value: Utils.changeAlpha(colorContent || Style.colorContent, 50) }
        width: ListView.view.width
        enabled: !modelData.submitted
        opacity: enabled ? 1.0 : 0.5

        contentItem: RowLayout {
            id: row

            SquareIcon {
                size: Style.minRowHeight
                source: modelData.summaryIcon || ""
                recolor: modelData.summaryIcon !== undefined && modelData.summaryIcon.startsWith("qrc:/")
                opacity: recolor ? 0.5 : 1.0
            }

            ColumnLayout {
                id: col

                Label {
                    Layout.fillWidth: true
                    text: modelData.summaryText !== "" ? modelData.summaryText : "??"
                    font.bold: true
                    font.pixelSize: App.settings.font16
                    elide: Label.ElideRight
                    maximumLineCount: 1
                }

                Label {
                    Layout.fillWidth: true
                    text: modelData.trackFile
                    font.pixelSize: App.settings.font14
                    elide: Label.ElideRight
                    maximumLineCount: 1
                    visible: modelData.trackFile !== ""
                }

                RowLayout {
                    spacing: 6
                    Layout.fillWidth: true
                    C.SquareIcon {
                        visible: modelData.readonly === true
                        size: updateDate.height * 1.2
                        source: "qrc:/icons/edit_off.svg"
                        opacity: 0.8
                        recolor: true
                    }
                    Label {
                        id: updateDate
                        Layout.fillWidth: true
                        text: App.formatDateTime(modelData.updateTime)
                        opacity: 0.5
                        font.pixelSize: App.settings.font10
                        fontSizeMode: Label.Fit
                        elide: Label.ElideRight
                    }
                }
            }

            SquareIcon {
                recolor: true
                opacity: 0.5
                source: modelData.statusIcon
            }
        }

        HorizontalDivider {
            height: {
                if (model.index === -1 || model.index >= sightingListModel.count - 1) {
                    return Style.lineWidth1
                }

                let curr = modelData
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
            if (!modelData.submitted) {
                popupLongPress.open({ sighting: modelData, index: model.index })
            }
        }

        onClicked: {
            if (!modelData.submitted) {
                popupLongPress.close()
                root.clicked(modelData, model.index)
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
            C.OptionsPopup {
                property var sighting
                property int index

                text: getSightingTitle(sighting)
                model: [
                    { text: qsTr("Continue editing"), icon: "qrc:/icons/pencil_outline.svg" },
                    { text: qsTr("Delete sighting"), icon: "qrc:/icons/delete_outline.svg", delay: true }
                ]

                onClicked: function (index) {
                    switch (index) {
                    case 0:
                        root.clicked(sighting, index)
                        break

                    case 1:
                        popupLongPress.close()
                        form.removeSighting(sighting.rootRecordUid)
                        showToast(qsTr("Sighting deleted"))
                        break
                    }
                }
            }
        }
    }

    PopupLoader {
        id: popupSubmit
        popupComponent: Component {
            C.ConfirmPopup {
                icon: "qrc:/icons/upload_multiple.svg"
                text: qsTr("Submit data?")
                onConfirmed: {
                    form.submitData()
                }
            }
        }
    }

    function getSightingTitle(sighting) {
        let title = sighting === undefined ? "" : sighting.summaryText
        return title === "" ? "??" : title
    }

    function submit() {
        if (!form.canSubmitData()) {
            App.showError(qsTr("No supported in demo mode"))
            return
        }

        popupSubmit.open()
    }
}
