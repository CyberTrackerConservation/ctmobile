import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

ContentPage {
    property alias recordUid: calendar.recordUid
    property alias fieldUid: calendar.fieldUid

    header: PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
        text: form.getFieldName(recordUid, fieldUid)
    }

    FieldCalendar {
        id: calendar
        anchors.fill: parent
    }
}
