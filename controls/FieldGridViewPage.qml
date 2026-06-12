import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    id: page

    property alias recordUid: gridView.recordUid
    property alias fieldUid: gridView.fieldUid
    property alias listElementUid: gridView.listElementUid
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

    FieldGridView {
        id: gridView
        anchors.fill: parent

        params: form.getFieldParameter(recordUid, fieldUid, "content", ({}))
        sideBorder: false

        onItemClicked: function (elementUid) {
            if (form.getElement(elementUid).hasChildren) {
                form.pushPage(Qt.resolvedUrl("FieldGridViewPage.qml"), { recordUid: recordUid, fieldUid: fieldUid, listElementUid: elementUid, depth: depth + 1 } )
            } else {
                form.setFieldValue(recordUid, fieldUid, elementUid)
                form.popPages(depth)
            }
        }
    }
}
