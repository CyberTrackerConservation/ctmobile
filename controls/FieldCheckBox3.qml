import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

CheckableBox3 {
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
            if (fieldBinding.value === undefined) {
                checkState = CheckableBox3.Indeterminate
            } else {
                checkState = fieldBinding.value ? CheckableBox3.Checked : CheckableBox3.Unchecked
            }
            break

        case "StringField":
            if (elementListModel.count === 2) {
                if (fieldBinding.value === undefined) {
                    checkState = CheckableBox3.Indeterminate
                } else {
                    checkState = fieldBinding.value === elementListModel.get(0).uid ? CheckableBox3.Checked : CheckableBox3.Unchecked
                }
            } else {
                console.log("Need two list items to use checkbox for a StringField")
            }
            break

        default:
            console.log("Bad field type for FieldCheckBox3")
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
                if (fieldBinding.value === undefined) {
                    checkState = CheckableBox3.Indeterminate
                } else {
                    checkState = fieldBinding.value ? CheckableBox3.Checked : CheckableBox3.Unchecked
                }
            } finally {
                internal.mutex = false
            }
        }
    }

    onCheckStateChanged: {
        if (internal.mutex) {
            return;
        }

        internal.mutex = true
        try {
            if (checkState === CheckableBox3.Indeterminate) {
                fieldBinding.resetValue()
            } else {
                switch (fieldBinding.fieldType) {
                case "CheckField":
                case "AcknowledgeField":
                    fieldBinding.setValue(checkState === CheckableBox3.Checked)
                    break

                case "StringField":
                    fieldBinding.setValue(elementListModel.get(checkState === CheckableBox3.Checked ? 0 : 1).uid)
                    break
                }
            }
        } finally {
            internal.mutex = false
        }
    }
}
