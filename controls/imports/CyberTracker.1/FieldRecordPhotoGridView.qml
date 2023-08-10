import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

GridViewV {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    C.FieldBinding {
        id: fieldBinding
    }

    clip: true
    cellWidth: root.width / columns
    cellHeight: root.width / columns

    delegate: GridFrameDelegate {
        id: d

        width: root.cellWidth
        height: root.cellHeight
        sideBorder: root.sideBorder
        borderWidth: root.borderWidth
        lines: root.lines
        columns: root.columns
        itemPadding: root.padding

        Component {
            id: delegatePhotoButton

            FieldPhotoButton {
                width: parent.width
                height: parent.height
                recordUid: modelData.recordUid
                fieldUid: modelData.fieldUid
            }
        }

        Component {
            id: delegateAddButton

            ToolButton {
                width: parent.width
                height: parent.height
                icon.source: "qrc:/icons/camera_plus_outline.svg"
                icon.width: Style.iconSize48
                icon.height: Style.iconSize48
                opacity: 0.5
                padding: 0
                onClicked: {
                    triggerPhotoTimer.fieldUid = modelData.fieldUid
                    triggerPhotoTimer.start()
                }
            }
        }

        sourceComponent: {
            if (modelData.recordUid !== undefined) {
                return delegatePhotoButton
            }

            return delegateAddButton
        }
    }

    Component.onCompleted: {
        root.model = rebuild()
    }

    Timer {
        id: triggerPhotoTimer
        interval: 10
        repeat: false

        property string fieldUid

        onTriggered: {
            let recordUid = form.newRecord(fieldBinding.recordUid, fieldBinding.fieldUid)
            form.setFieldValue(recordUid, fieldUid, [""])
            form.pushPage(Qt.resolvedUrl("FieldCameraPage.qml"), { photoIndex: 0, recordUid: recordUid, fieldUid: fieldUid } )
            root.model = rebuild()
        }
    }

    function buildGroupModel(recordUid) {
        let result = []
        let recordFieldUid = form.getRecordFieldUid(recordUid)
        let recordField = form.getField(recordFieldUid)

        for (let i = 0; i < recordField.fields.length; i++) {
            let field = recordField.fields[i]

            if (!form.getFieldVisible(recordUid, field.uid)) {
                continue
            }

            result.push({ recordUid: recordUid, fieldUid: field.uid })
        }

        return result
    }

    function buildDynamicModel(recordUid, fieldUid) {
        let result = []
        let recordField = form.getField(fieldUid)
        let photoField = recordField.fields[0]

        // Add all the records.
        let recordUids = form.getFieldValue(recordUid, fieldUid, [])
        for (let i = 0; i < recordUids.length; i++) {
            result.push({ recordUid: recordUids[i], fieldUid: photoField.uid })
        }

        // Add the "+" button.
        if (recordField.dynamic && (recordField.maxCount === 0 || recordUids.length < recordField.maxCount)) {
            result.push({ fieldUid: photoField.uid })
        }

        return result
    }

    function rebuild() {
        // Check the Record to make sure it is only PhotoFields.
        let recordField = fieldUid === "" ? form.getField(form.getRecordFieldUid(recordUid)) : form.getField(fieldUid)
        if (recordField.onlyFieldType !== "PhotoField") {
            App.showError(qsTr("Expected photo field"))
            return []
        }

        // Group RecordField containing multiple photo fields.
        if (recordUid !== "" && fieldUid === "") {
            return buildGroupModel(recordUid)
        }

        // Dynamic RecordField containing a single photo field.
        if (fieldBinding.fieldType === "RecordField") {
            return buildDynamicModel(recordUid, fieldUid)
        }

        App.showError(qsTr("photoGrid not a group or repeat"))
        return ({})
    }
}
