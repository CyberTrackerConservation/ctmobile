import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    property bool deleteOnBack: false

    QtObject {
        id: internal
        property bool somethingChanged: false
    }

    C.FieldBinding {
        id: fieldBinding
    }

    header: C.PageHeader {
        id: pageHeader
        text: title
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        formBack: false
        onBackClicked: {
            if (deleteOnBack && !internal.somethingChanged) {
                let _recordUid = recordUid
                fieldBinding.reset()
                form.deleteRecord(_recordUid)
            }
                
            form.popPage(StackView.PopTransition)
        }

        onMenuClicked: {
            if (listView.validate()) {
                form.popPage(StackView.Immediate)
            }
        }
    }

    C.FieldListViewEditor {
        id: listView
        recordUid: fieldBinding.recordUid
    }

    Connections {
        target: form

        function onFieldValueChanged(recordUid, fieldUid, oldValue, newValue) {
            internal.somethingChanged = internal.somethingChanged || form.getFieldType(fieldUid) !== "LocationField";
        }
    }
}
