import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property alias recordUid: listView.recordUid
    property alias fieldUid: listView.fieldUid
    property alias highlightInvalid: listView.highlightInvalid

    header: C.PageHeader {
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

    C.FieldRecordListView {
        id: listView
        anchors.fill: parent
    }
}
