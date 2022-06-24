import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property alias recordUid: textEdit.recordUid
    property alias fieldUid: textEdit.fieldUid

    header: C.PageHeader {
        text: form.getFieldName(recordUid, page.fieldUid)
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            Qt.inputMethod.hide()
            form.popPage(StackView.Immediate)
        }
    }

    C.FieldTextEdit {
        id: textEdit
        anchors.fill: parent
        anchors.margins: 10
        forceFocus: true
    }
}
