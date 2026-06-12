import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

CheckableBox {
    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    QtObject {
        id: internal
        property bool mutex: false
    }

    Component.onCompleted: {
        switch (fieldBinding.fieldType) {
        case "CheckField":
        case "AcknowledgeField":
            checked = fieldBinding.value === undefined ? false : fieldBinding.value
            break

        case "StringField":
            if (elementListModel.count === 2) {
                checked = fieldBinding.value === elementListModel.get(0).uid
            } else {
                console.log("Need two list items to use checkbox for a StringField")
            }

            break

        default:
            console.log("Bad field type for FieldCheckBox")
        }
    }

    C.ElementListModel {
        id: elementListModel
        elementUid: fieldBinding.field.listElementUid
    }

    C.FieldBinding {
        id: fieldBinding

        onValueChanged: {
            if (internal.mutex) {
                return
            }

            internal.mutex = true
            try {
                checked = fieldBinding.value === undefined ? false : fieldBinding.value
            } finally {
                internal.mutex = false
            }
        }
    }

    onCheckedChanged: {
        if (internal.mutex) {
            return;
        }

        internal.mutex = true
        try {
            // We false and untouched to be the same. If you specifically want false for unchecked, then 
            // make the field required and this will trigger FieldCheckBox3.
            if (checked === false && fieldBinding.fieldDefaultValue === undefined) {
                fieldBinding.resetValue()
                return
            }

            switch (fieldBinding.fieldType) {
            case "CheckField":
            case "AcknowledgeField":
                fieldBinding.setValue(checked)
                break

            case "StringField":
                fieldBinding.setValue(elementListModel.get(checked ? 0 : 1).uid)
                break
            }
        } finally {
            internal.mutex = false
        }
    }
}
