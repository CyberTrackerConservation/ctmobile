import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property alias recordUid: recordNumberGridView.recordUid
    property alias fieldUid: recordNumberGridView.fieldUid

    header: PageHeader {
        topText: form.getFieldName(recordUid, fieldUid)
        text: form.getFieldName(recordUid, fieldUid)
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPages(1)
    }

    FieldRecordNumberGridView {
        id: recordNumberGridView
        anchors.fill: parent
        params: form.getFieldParameter(recordUid, fieldUid, "content", ({}))
        sideBorder: false
        onItemClicked: function (recordUid, fieldUid) {
            form.pushPage(Qt.resolvedUrl("FieldKeypadPage.qml"), { recordUid: recordUid, fieldUid: fieldUid } )
        }
    }
}
