import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property alias recordUid: listView.recordUid
    property alias fieldUid: listView.fieldUid
    property alias listElementUid: listView.listElementUid
    property int depth: 1

    header: C.PageHeader {
        topText: page.depth === 1 ? "" : form.getFieldName(recordUid, fieldUid)
        text: page.depth === 1 ? form.getFieldName(recordUid, fieldUid) : form.getElementName(page.listElementUid)
        menuVisible: depth > 1
        menuIcon: "qrc:/icons/check.svg"
        onMenuClicked: {
            form.setFieldValue(recordUid, fieldUid, listElementUid)
            form.popPages(depth)
        }
    }

    C.FieldListView {
        id: listView
        anchors.fill: parent

        onItemClicked: {
            if (form.getElement(elementUid).hasChildren) {
                form.pushPage(Qt.resolvedUrl("FieldListViewPage.qml"), { recordUid: recordUid, fieldUid: fieldUid, listElementUid: elementUid, depth: depth + 1 } )
            } else {
                form.setFieldValue(recordUid, fieldUid, elementUid)
                form.popPages(depth)
            }
        }
    }
}
