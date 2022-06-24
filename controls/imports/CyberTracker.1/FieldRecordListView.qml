import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ListViewV {
    id: root

    property string recordUid
    property string fieldUid
    property bool wizardMode: false
    property var highlightInvalid: false

    C.FieldBinding {
        id: fieldBinding
        recordUid: root.recordUid
        fieldUid: root.fieldUid
    }

    model: C.RecordListModel {
        id: recordListModel
        recordUid: root.recordUid
        fieldUid: root.fieldUid
    }

    delegate: SwipeDelegate {
        id: swipeDelegate
        width: ListView.view.width

        Binding { target: background; property: "color"; value: C.Style.colorContent }

        contentItem: RowLayout {
            Item { height: Style.minRowHeight }
            ColumnLayout {
                id: content
                
                property color textColor: (highlightInvalid && !modelData.recordValid) ? Style.colorInvalid : Material.foreground

                RowLayout {
                    Label {
                        id: labelTitle
                        text: modelData.recordName
                        Layout.fillWidth: true
                        font.pixelSize: App.settings.font16
                        wrapMode: Label.Wrap
                        opacity: 0.6
                        color: content.textColor
                    }

                    Image {
                        fillMode: Image.PreserveAspectFit
                        source: modelData.recordIcon
                        sourceSize.width: Style.minRowHeight
                        sourceSize.height: Style.minRowHeight
                        visible: modelData.recordIcon !== ""
                        verticalAlignment: Image.AlignVCenter
                    }
                }

                Repeater {
                    id: repeater
                    visible: count !== 0
                    Layout.fillWidth: true

                    model: modelData.recordData

                    delegate: ColumnLayout {

                        RowLayout {
                            Label {
                                Layout.preferredWidth: (root.width / 2) - 32
                                text: modelData.fieldName
                                font.pixelSize: App.settings.font14
                                wrapMode: Label.Wrap
                                opacity: 0.6
                                color: content.textColor
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.fieldValue
                                font.pixelSize: App.settings.font14
                                wrapMode: Label.Wrap
                                color: content.textColor
                            }
                        }
                    }
                }
            }
        }

        HorizontalDivider {}

        swipe.right: Rectangle {
            width: parent.width
            height: parent.height
            color: C.Style.colorGroove

            RowLayout {
                anchors.fill: parent

                Label {
                    text: fieldBinding.field.dynamic ? qsTr("Delete record?") : qsTr("Reset record?")
                    leftPadding: 20
                    font.pixelSize: App.settings.font16
                    fontSizeMode: Label.Fit
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }

                ToolButton {
                    flat: true
                    text: qsTr("Yes")
                    onClicked: {
                        swipeDelegate.swipe.close()
                        if (fieldBinding.field.dynamic) {
                            form.deleteRecord(modelData.recordUid)
                        } else {
                            form.resetRecord(modelData.recordUid)
                        }
                    }
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: parent.height
                    Layout.maximumWidth: parent.width / 5
                    font.pixelSize: App.settings.font14
                }

                ToolButton {
                    flat: true
                    text: qsTr("No")
                    onClicked: swipeDelegate.swipe.close()
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: parent.height
                    Layout.maximumWidth: parent.width / 5
                    font.pixelSize: App.settings.font14
                }
            }
        }

        onClicked: {
            if (root.wizardMode) {
                form.wizard.edit(modelData.recordUid)
            } else {
                form.pushPage(Qt.resolvedUrl("FieldRecordEditorPage.qml"), { title: modelData.recordName, recordUid: modelData.recordUid, fieldUid: root.fieldUid, deleteOnBack: false })
            }
        }
    }

    footer: ItemDelegate {
        width: parent.width
        visible: {
            // Prevent adding more than max records.
            if (fieldBinding.field.maxCount > 0 && fieldBinding.field.maxCount === recordListModel.count) {
                return false
            } else {
                return fieldBinding.field.dynamic
            }
        }

        contentItem: Label {
            horizontalAlignment: Label.AlignHCenter
            verticalAlignment: Label.AlignVCenter
            text: qsTr("Add record")
            font.pixelSize: App.settings.font14
            font.bold: true
            opacity: 0.7
        }

        C.HorizontalDivider {}

        onClicked: {
            let recordUid = form.newRecord(root.recordUid, root.fieldUid)

            if (root.wizardMode) {
                form.wizard.edit(recordUid)
            } else {
                let recordName = form.getRecordName(recordUid)
                form.pushPage(Qt.resolvedUrl("FieldRecordEditorPage.qml"), { title: recordName, recordUid: recordUid, fieldUid: root.fieldUid, deleteOnBack: fieldBinding.field.dynamic })
            }
        }
    }

    function validate() {
        for (let i = 0; i < recordListModel.count; i++) {
            if (!recordListModel.get(i).recordValid) {
                highlightInvalid = true
                return false
            }
        }

        return true
    }
}
