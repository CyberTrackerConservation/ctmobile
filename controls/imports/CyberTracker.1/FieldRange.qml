import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

GridViewV {
    id: root

    property string recordUid
    property string fieldUid

    signal itemClicked(var value)

    C.FieldBinding {
        id: fieldBinding
        recordUid: root.recordUid
        fieldUid: root.fieldUid
    }

    Text {
        id: textHeightCheck
        text: "8"
        font.pixelSize: root.fontSize
        font.bold: root.fontBold
        visible: false
    }

    Component.onCompleted: {
        if (fieldBinding.value === undefined) {
            return
        }

        let i = (fieldBinding.value - fieldBinding.field.minValue) / fieldBinding.field.step
        root.positionViewAtIndex(i, GridView.Visible)
    }

    clip: true
    cellWidth: width / root.columns
    cellHeight: textHeightCheck.implicitHeight + root.padding * 2

    model: Math.ceil((fieldBinding.field.maxValue - fieldBinding.field.minValue + 1) / fieldBinding.field.step)

    delegate: GridFrameDelegate {
        id: d

        width: root.cellWidth
        height: root.cellHeight
        sideBorder: root.sideBorder
        borderWidth: root.borderWidth
        lines: root.lines
        columns: root.columns
        itemPadding: root.padding

        property int value: model.index * fieldBinding.field.step + fieldBinding.field.minValue
        highlighted: d.value === fieldBinding.value

        Component {
            id: textOnlyComponent
            Label {
                width: parent.width
                height: parent.height
                text: d.value
                color: Material.foreground
                font.pixelSize: root.fontSize
                font.bold: root.fontBold || d.highlighted
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        sourceComponent: textOnlyComponent

        onClicked: {
            root.itemClicked(d.value)
        }
    }
}
