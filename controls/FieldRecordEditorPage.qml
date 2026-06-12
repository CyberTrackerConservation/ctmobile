import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
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

    header: PageHeader {
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

    FieldListViewEditor {
        id: listView
        anchors.fill: parent
        recordUid: fieldBinding.recordUid
    }

    Connections {
        target: form

        function onFieldValueChanged(recordUid, fieldUid, oldValue, newValue) {
            internal.somethingChanged = internal.somethingChanged || form.getFieldType(fieldUid) !== "LocationField";
        }
    }
}
