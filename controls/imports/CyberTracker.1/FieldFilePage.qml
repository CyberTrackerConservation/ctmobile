import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    property alias recordUid: filePane.recordUid
    property alias fieldUid: filePane.fieldUid

    header: C.PageHeader {
        text: fieldUid !== "" ? form.getFieldName(recordUid, fieldUid) : qsTr("File")
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        formBack: false
        onBackClicked: menuClicked()
        onMenuClicked: {
            form.popPage(StackView.Immediate)
        }
    }

    FieldFilePane {
        id: filePane
        anchors.fill: parent
    }
}
