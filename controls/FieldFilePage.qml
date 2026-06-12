import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    property alias recordUid: filePane.recordUid
    property alias fieldUid: filePane.fieldUid

    header: PageHeader {
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
