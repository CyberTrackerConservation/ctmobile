import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    property string recordUid
    property string elementUid
    property bool saved: form.getFieldValue(recordUid, Globals.modelTreeFieldUid) !== undefined

    Connections {
        target: form

        function onFieldValueChanged(recordUid, fieldUid, oldValue, newValue) {
            pageHeader.backPrompt = !saved
        }
    }

    header: C.PageHeader {
        id: pageHeader
        text: title
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true

        backPromptText: qsTr("Discard changes?")
        backPromptConfirmText: qsTr("Yes, discard them")

        onMenuClicked: {
            if (listView.validate()) {
                form.setFieldValue(recordUid, Globals.modelTreeFieldUid, elementUid)
                form.popPagesToStart()
            }
        }
    }

    C.FieldListViewEditor {
        id: listView
        anchors.fill: parent
        recordUid: page.recordUid
        filterElementUid: page.elementUid
    }

    Component.onCompleted: {
        let element = form.getElement(page.elementUid)
        form.attachmentTags = element.getTagValue("attachmentTags", [])
    }
}
