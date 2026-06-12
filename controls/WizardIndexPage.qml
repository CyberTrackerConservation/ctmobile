import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    id: page

    property string recordUid: form.rootRecordUid
    property string fieldUid: form.wizard.lastPageFieldUid
    property var filterFieldUids: ([])
    property string topText: ""
    property bool highlightInvalid: false

    header: PageHeader {
        topText: page.topText
        text: form.wizard.header !== "" ? form.wizard.header : form.project.title
        menuVisible: page.recordUid === form.rootRecordUid
        menuIcon: "qrc:/icons/skip_next.svg"
        onMenuClicked: {
            listView.gotoLastField()
        }
    }

    WizardIndex {
        id: listView
        anchors.fill: parent

        recordUid: page.recordUid
        fieldUid: page.fieldUid
        filterFieldUids: page.filterFieldUids
        highlightInvalid: page.highlightInvalid
    }
}
