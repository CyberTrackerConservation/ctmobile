import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property string recordUid
    property string elementUid

    header: C.PageHeader {
        text: title
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true

        onMenuClicked: {
            if (listView.validate()) {
                form.setFieldValue(recordUid, Globals.modelTreeFieldUid, elementUid)
                form.recordComplete(recordUid)
                form.popPagesToStart()
            }
        }
    }

    C.FieldListViewEditor {
        id: listView
        recordUid: page.recordUid
        filterElementUid: page.elementUid
    }
}
