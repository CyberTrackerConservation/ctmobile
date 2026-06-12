import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    property alias recordUid: dateTimePicker.recordUid
    property alias fieldUid: dateTimePicker.fieldUid
    property alias dateMode: dateTimePicker.dateMode
    property alias timeMode: dateTimePicker.timeMode

    header: PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
        text: form.getFieldName(recordUid, fieldUid)
    }

    FieldDateTimePicker {
        id: dateTimePicker
        anchors.fill: parent
        fontPixelSize: 32
    }
}
