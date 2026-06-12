import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    id: page

    property alias recordUid: textEdit.recordUid
    property alias fieldUid: textEdit.fieldUid

    header: PageHeader {
        text: form.getFieldName(recordUid, page.fieldUid)
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            Qt.inputMethod.hide()
            form.popPage(StackView.Immediate)
        }
    }

    FieldTextEdit {
        id: textEdit
        anchors.fill: parent
        anchors.margins: 10
        forceFocus: true
    }
}
