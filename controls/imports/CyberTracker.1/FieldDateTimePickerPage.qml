import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

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

    C.FieldDateTimePicker {
        id: dateTimePicker
        anchors.fill: parent
        fontPixelSize: 32
    }
}
