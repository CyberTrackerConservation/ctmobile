import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import CyberTracker 1.0 as C

ContentPage {
    property alias recordUid: calendar.recordUid
    property alias fieldUid: calendar.fieldUid

    header: C.PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
        text: form.getFieldName(recordUid, fieldUid)
    }

    C.FieldCalendar {
        id: calendar
        anchors.fill: parent
    }
}
