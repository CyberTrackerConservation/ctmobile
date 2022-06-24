import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    property alias recordUid: keypad.recordUid
    property alias fieldUid: keypad.fieldUid
    property alias listElementUid: keypad.listElementUid

    header: C.PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
        text: listElementUid !== "" ? form.getElementName(listElementUid) : form.getFieldName(recordUid, fieldUid)
    }

    C.FieldKeypad {
        id: keypad
        anchors.fill: parent
    }
}
