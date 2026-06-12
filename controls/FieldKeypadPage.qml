import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    property alias recordUid: keypad.recordUid
    property alias fieldUid: keypad.fieldUid
    property alias listElementUid: keypad.listElementUid

    header: PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
        text: listElementUid !== "" ? form.getElementName(listElementUid) : form.getFieldName(recordUid, fieldUid)
    }

    FieldKeypad {
        id: keypad
        anchors.fill: parent
    }
}
