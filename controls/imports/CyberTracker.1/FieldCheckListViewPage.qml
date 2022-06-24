import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property alias recordUid: checkListView.recordUid
    property alias fieldUid: checkListView.fieldUid
    property alias listElementUid: checkListView.listElementUid
    property int depth: 1

    header: C.PageHeader {
        topText: page.depth === 1 ? "" : form.getFieldName(recordUid, fieldUid)
        text: page.depth === 1 ? form.getFieldName(recordUid, fieldUid) : form.getElementName(page.listElementUid)
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: form.popPages(depth)
    }

    C.FieldCheckListView {
        id: checkListView
        anchors.fill: parent

        onItemClicked: {
            if (form.getElement(elementUid).hasChildren) {
                form.pushPage(Qt.resolvedUrl("FieldCheckListViewPage.qml"), { recordUid: recordUid, fieldUid: fieldUid, listElementUid: elementUid, depth: depth + 1 } )
            } else if (App.config.fullRowCheck) {
                checkBox.checked = !checkBox.checked
            }
        }
    }
}
