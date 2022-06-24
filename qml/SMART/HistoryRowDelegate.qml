import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

SwipeDelegate {
    id: d
    width: parent ? parent.width : undefined

    property var form
    property var divider: true

    Binding { target: background; property: "color"; value: C.Style.colorContent }

    function getTitle(sighting) {
        var incidentTypes = []

        // Metadata change records only have 1 record.
        if (sighting.records.length === 1) {
            return form.provider.getSightingTypeText(sighting.records[0].fieldValues[Globals.modelIncidentTypeFieldUid])
        }

        // All other records are IncidentRecord types, so pick up their names.
        for (var i = 1; i < sighting.records.length; i++) {
            if (sighting.records[i].fieldUid === Globals.modelIncidentRecordFieldUid) {
                var value = sighting.records[i].fieldValues[Globals.modelTreeFieldUid]
                value = form.getElementName(value)
                if (value !== "") {
                    incidentTypes.push(value)
                }
            }
        }

        return incidentTypes.join(", ")
    }

    Component {
        id: componentItem

        RowLayout {
            ColumnLayout {
                Label {
                    Layout.fillWidth: true
                    text: getTitle(modelData)
                    font.bold: true
                    font.pixelSize: App.settings.font16
                    wrapMode: Label.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    visible: false
                    font.pixelSize: App.settings.font14
                }

                Label {
                    Layout.fillWidth: true
                    text: App.formatDateTime(modelData.createTime)
                    opacity: 0.5
                    font.pixelSize: App.settings.font10
                    wrapMode: Label.WordWrap
                }
            }

            Image {
                visible: form.provider.hasDataServer && !form.provider.manualUpload
                sourceSize.width: 32
                source: modelData.uploaded || modelData.exported ? "qrc:/icons/cloud_done.svg" : "qrc:/icons/cloud_queue.svg"
                opacity: 0.5
                layer {
                    enabled: true
                    effect: ColorOverlay {
                        color: Material.foreground
                    }
                }
            }
        }
    }

    contentItem: Loader {
        height: item.height
        sourceComponent: componentItem
    }

    C.HorizontalDivider {
        visible: divider
    }

    swipe.right: Rectangle {
        width: form.canEditSighting(modelData.rootRecordUid) ? parent.width : 0
        height: parent.height
        color: C.Style.colorGroove

        RowLayout {
            anchors.fill: parent
            Label {
                text: qsTr("Delete observation?")
                leftPadding: 20
                Layout.fillWidth: true
                font.pixelSize: App.settings.font16
            }

            ToolButton {
                flat: true
                text: qsTr("Yes")
                onClicked: form.removeSighting(modelData.rootRecordUid)
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: parent.height
                Layout.maximumWidth: parent.width / 5
                font.pixelSize: App.settings.font14
            }

            ToolButton {
                id: deleteNo
                flat: true
                text: qsTr("No")
                onClicked: d.swipe.close()
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: parent.height
                Layout.maximumWidth: parent.width / 5
                font.pixelSize: App.settings.font14
            }
        }
    }
}
