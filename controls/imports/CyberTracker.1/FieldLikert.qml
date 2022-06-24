import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Item {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    implicitHeight: likertButton.implicitHeight

    C.FieldBinding {
        id: fieldBinding
    }

    RoundButton {
        id: likertButton
        text: "2"
        visible: false
    }

    ButtonGroup {
        buttons: likertRow.children
    }

    Row {
        id: likertRow
        anchors.fill: parent
        spacing: likertListModel.count > 1 ? ((parent.width - (likertButton.implicitWidth * likertListModel.count)) / (likertListModel.count - 1)) : 0

        Repeater {
            model: C.ElementListModel {
                id: likertListModel
                elementUid: fieldBinding.field.listElementUid
            }

            delegate: RoundButton {
                checkable: true
                checked: fieldBinding.value === modelData.uid
                text: form.getElementName(modelData.uid)
                onClicked: fieldBinding.setValue(modelData.uid)
            }
        }
    }
}
