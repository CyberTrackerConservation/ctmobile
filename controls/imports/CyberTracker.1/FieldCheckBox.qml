import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

CheckableBox {
    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

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
    }

    onCheckedChanged: {
        switch (fieldBinding.fieldType) {
        case "CheckField":
        case "AcknowledgeField":
            fieldBinding.setValue(checked)
            break

        case "StringField":
            fieldBinding.setValue(elementListModel.get(checked ? 0 : 1).uid)
            break
        }
    }
}
