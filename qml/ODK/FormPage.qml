import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: form.project.title

        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true

        onMenuClicked: {
            if (listView.validate()) {
                form.markSightingCompleted()
                form.saveSighting()
                form.popPage(StackView.Immediate)
            }
        }

        formBack: false
        onBackClicked: {
            form.markSightingCompleted()
            form.saveSighting()
            form.popPage(StackView.Immediate)
        }
    }

    C.FieldListViewEditor {
        id: listView
        recordUid: form.rootRecordUid
    }
}
