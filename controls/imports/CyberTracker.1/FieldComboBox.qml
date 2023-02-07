import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

ComboBox {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property string defaultDisplayText: ""

    C.FieldBinding {
        id: fieldBinding
    }

    model: C.ElementListModel {
        elementUid: fieldBinding.field.listElementUid
    }

    delegate: ItemDelegate {
        width: root.width
        font.pixelSize: App.settings.font14
        text: form.getElementName(modelData.uid)
    }

    onActivated: {
        onAccepted()
    }

    onAccepted: {
        fieldBinding.setControlState("CurrentIndex", currentIndex)
        if (currentIndex === -1) {
            fieldBinding.resetValue()
            return
        }

        let element = form.getElement(model.get(currentIndex).uid)
        fieldBinding.setValue(element.uid);
        displayText = fieldBinding.valueElementName
    }

    Component.onCompleted: {
        let elementUid = fieldBinding.value
        if (elementUid !== undefined) {
            currentIndex = fieldBinding.getControlState("CurrentIndex", -1)
            displayText = fieldBinding.valueElementName
        } else {
            currentIndex = -1
            displayText = defaultDisplayText
        }
    }
}
