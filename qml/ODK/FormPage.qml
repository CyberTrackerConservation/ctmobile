import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: form.project.title
        menuIcon: form.editing ? C.Style.okIconSource : C.Style.checkIconSource
        menuVisible: true

        onMenuClicked: {
            if (!listView.validate()) {
                return
            }

            if (form.editing) {
                form.saveSighting()
                form.markSightingCompleted()
                form.popPagesToParent()
                return
            }

            form.applyTrackCommand()
            form.saveSighting()
            form.markSightingCompleted()
            form.popPage(StackView.Immediate)
        }

        formBack: false
        backIcon: form.editing ? C.Style.cancelIconSource : C.Style.homeIconSource
        onBackClicked: {
            if (form.editing) {
                form.popPagesToParent()
                return
            }

            form.saveSighting()
            form.popPage(StackView.Immediate)
        }
    }

    C.FieldListViewEditor {
        id: listView
        anchors.fill: parent
        recordUid: form.rootRecordUid
    }
}
