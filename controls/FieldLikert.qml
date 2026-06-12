import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

Item {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    signal itemClicked(string elementUid)

    implicitHeight: likertButton.implicitHeight

    C.FieldBinding {
        id: fieldBinding
    }

    RoundButton {
        id: likertButton
        text: "2"
        font.pixelSize: App.settings.font14
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
                font.pixelSize: App.settings.font14
                onClicked: {
                    itemClicked(modelData.uid)
                }
            }
        }
    }
}
