import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

GridViewV {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property string listElementUid: fieldBinding.field.listElementUid

    signal itemClicked(string elementUid)

    Text {
        id: textHeightCheck
        text: "Wg"
        font.pixelSize: root.fontSize
        font.bold: root.fontBold
        visible: false
    }

    currentIndex: fieldBinding.getControlState(listElementUid + "_CurrentIndex", -1)
    clip: true
    cellWidth: {
        return width / root.columns
    }
    cellHeight: {
        switch (root.style.toLowerCase()) {
        case "iconinlay":
            return Math.min(cellWidth, root.itemHeight)

        case "icononly":
            return Math.min((cellWidth + root.padding) / 2, root.itemHeight)

        case "textonly":
            return Math.max(textHeightCheck.height + 8, root.itemHeight)

        case "textbesideicon":
            return Math.max(textHeightCheck.height + 8, root.itemHeight)

        default:
            return 64
        }
    }

    C.FieldBinding {
        id: fieldBinding
    }

    model: C.ElementListModel {
        id: elementListModel
        elementUid: listElementUid
        flatten: fieldBinding.field.listFlatten
        sorted: fieldBinding.field.listSorted
        other: fieldBinding.field.listOther
        filterByFieldRecordUid: fieldBinding.recordUid
        filterByField: fieldBinding.field.listFilterByField
        filterByTag: fieldBinding.field.listFilterByTag
    }

    delegate: GridFrameDelegate {
        id: d

        width: root.cellWidth
        height: root.cellHeight
        sideBorder: root.sideBorder
        borderWidth: root.borderWidth
        lines: root.lines
        columns: root.columns
        itemPadding: root.padding

        Component {
            id: delegateIconInlay

            C.CheckBoxIconInlay {
                width: parent.width
                height: parent.height
                icon: form.getElementIcon(modelData.uid)
                checked: root.getCheckState(modelData.uid)
                onClicked: root.setCheckState(modelData.uid, checked)
                onPressAndHold: App.showToast(form.getElementName(modelData.uid))
            }
        }

        Component {
            id: delegateIconOnly

            C.CheckBoxIconOnly {
                width: parent.width
                height: parent.height
                icon: form.getElementIcon(modelData.uid)
                checked: root.getCheckState(modelData.uid)
                onClicked: root.setCheckState(modelData.uid, checked)
                onPressAndHold: App.showToast(form.getElementName(modelData.uid))
            }
        }

        Component {
            id: delegateTextOnly

            C.CheckBoxTextOnly {
                width: parent.width
                height: parent.height
                font.pixelSize: root.fontSize
                font.bold: root.fontBold
                text: form.getElementName(modelData.uid)
                checked: root.getCheckState(modelData.uid)
                onClicked: root.setCheckState(modelData.uid, checked)
            }
        }

        Component {
            id: delegateTextBesideIcon

            C.CheckBoxTextBesideIcon {
                width: parent.width
                height: parent.height
                icon: form.getElementIcon(modelData.uid)
                font.pixelSize: root.fontSize
                font.bold: root.fontBold
                text: form.getElementName(modelData.uid)
                checked: root.getCheckState(modelData.uid)
                onClicked: root.setCheckState(modelData.uid, checked)
            }
        }

        sourceComponent: {
            switch (root.style.toLowerCase()) {
            case "iconinlay":
                return delegateIconInlay

            case "icononly":
                return delegateIconOnly

            case "textonly":
                return delegateTextOnly

            case "textbesideicon":
                return delegateTextBesideIcon

            default:
                return delegateTextOnly
            }
        }

        onClicked: {
            if (form.getElement(modelData.uid).hasChildren) {
                itemClicked(modelData.uid)
            } else if (App.config.fullRowCheck) {
                setCheckState(modelData.uid, !getCheckState(modelData.uid))
            }
        }
    }

    function getCheckState(elementUid) {
        return fieldBinding.getValue({})[elementUid] === true
    }

    function setCheckState(elementUid, checked) {
        let value = fieldBinding.getValue({})
        let wasChecked = value[elementUid] === true
        let changed
        if (checked) {
            value[elementUid] = true
            changed = !wasChecked
        } else {
            delete value[elementUid]
            changed = wasChecked
        }

        if (changed) {
            fieldBinding.setValue(value)
        }
    }
}
