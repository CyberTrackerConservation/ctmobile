import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import CyberTracker 1.0 as C

Item {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property bool dateMode: fieldBinding.fieldType === "DateField" || fieldBinding.fieldType === "DateTimeField"
    property bool timeMode: fieldBinding.fieldType === "TimeField" || fieldBinding.fieldType === "DateTimeField"
    property int fontPixelSize: App.settings.font20
    property string value: App.timeManager.currentDateTimeISO()

    signal valueUpdated(var value)

    C.FieldBinding {
        id: fieldBinding
    }

    GridLayout {
        anchors.fill: parent
        clip: true
        columns: parent.width > parent.height ? 2 : 1
        rows: parent.width > parent.height ? 1 : 2

        C.FieldDatePicker {
            id: datePicker
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            Layout.alignment: Qt.AlignHCenter
            fontPixelSize: root.fontPixelSize
            visible: root.dateMode
            padding: 0
            onValueUpdated: {
                root.value = value
                root.valueUpdated(value)
            }
        }

        C.FieldTimePicker {
            id: timePicker
            recordUid: fieldBinding.recordUid
            fieldUid: fieldBinding.fieldUid
            Layout.alignment: Qt.AlignHCenter
            fontPixelSize: root.fontPixelSize
            visible: root.timeMode
            padding: 0
            onValueUpdated: {
                root.value = value
                root.valueUpdated(value)
            }
        }
    }
}
