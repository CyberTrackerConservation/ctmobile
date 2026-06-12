import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

ContentPage {
    id: page

    property alias recordUid: listView.recordUid
    property alias fieldUid: listView.fieldUid
    property alias highlightInvalid: listView.highlightInvalid

    header: PageHeader {
        text: title
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            if (!listView.validate()) {
                return
            }

            form.popPage(StackView.Immediate)
        }
    }

    FieldRecordListView {
        id: listView
        anchors.fill: parent
    }
}
