import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    id: page

    property alias recordUid: listView.recordUid
    property alias fieldUid: listView.fieldUid
    property alias listElementUid: listView.listElementUid
    property int depth: 1

    header: PageHeader {
        topText: page.depth === 1 ? "" : form.getFieldName(recordUid, fieldUid)
        text: page.depth === 1 ? form.getFieldName(recordUid, fieldUid) : form.getElementName(page.listElementUid)
        menuVisible: depth > 1
        menuIcon: "qrc:/icons/check.svg"
        onMenuClicked: {
            form.setFieldValue(recordUid, fieldUid, listElementUid)
            form.popPages(depth)
        }
    }

    FieldListView {
        id: listView
        anchors.fill: parent

        onItemClicked: function (elementUid) {
            if (form.getElement(elementUid).hasChildren) {
                form.pushPage(Qt.resolvedUrl("FieldListViewPage.qml"), { recordUid: recordUid, fieldUid: fieldUid, listElementUid: elementUid, depth: depth + 1 } )
            } else {
                form.setFieldValue(recordUid, fieldUid, elementUid)
                form.popPages(depth)
            }
        }
    }
}
