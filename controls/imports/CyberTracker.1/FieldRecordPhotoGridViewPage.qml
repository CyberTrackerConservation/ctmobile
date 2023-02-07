import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property alias recordUid: recordPhotoGridView.recordUid
    property alias fieldUid: recordPhotoGridView.fieldUid

    header: PageHeader {
        topText: form.getFieldName(recordUid, fieldUid)
        text: form.getFieldName(recordUid, fieldUid)
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPages(1)
    }

    FieldRecordPhotoGridView {
        id: recordPhotoGridView
        anchors.fill: parent
        params: form.getFieldParameter(recordUid, fieldUid, "content", ({}))
        sideBorder: false
    }
}
