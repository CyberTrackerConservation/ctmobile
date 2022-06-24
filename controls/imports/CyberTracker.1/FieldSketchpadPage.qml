import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    property alias recordUid: sketchpad.recordUid
    property alias fieldUid: sketchpad.fieldUid

    header: PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
        text: form.getFieldName(recordUid, fieldUid)
    }

    FieldSketchpad {
        id: sketchpad
        width: parent.width
        height: Math.min(parent.width * 0.6666, parent.height)
    }
}
