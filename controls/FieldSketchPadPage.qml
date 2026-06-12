import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    property alias recordUid: sketchPad.recordUid
    property alias fieldUid: sketchPad.fieldUid

    header: PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
        text: form.getFieldName(recordUid, fieldUid)
    }

    FieldSketchPad {
        id: sketchPad
        anchors.fill: parent
    }
}
