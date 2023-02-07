import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

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
            form.popPage()
        }
    }
}
