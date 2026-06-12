import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    property alias recordUid: range.recordUid
    property alias fieldUid: range.fieldUid

    header: PageHeader {
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPage(StackView.Immediate)
        text: form.getFieldName(recordUid, fieldUid)
    }

    FieldRange {
        id: range
        anchors.fill: parent
        params: form.getFieldParameter(recordUid, fieldUid, "content", ({}))
        sideBorder: false

        onItemClicked: {
            form.setFieldValue(recordUid, fieldUid, value)
            form.popPage(StackView.Immediate)
        }
    }
}
