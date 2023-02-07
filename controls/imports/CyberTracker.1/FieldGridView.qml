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

    QtObject {
        id: internal
        property int textHeight: textHeightCheck.height + 8
    }

    Text {
        id: textHeightCheck
        text: "Wg"
        font.pixelSize: root.fontSize
        font.bold: root.fontBold
        visible: false
    }

    Component.onCompleted: {
        let index = fieldBinding.getControlState(listElementUid + "_CurrentIndex", -1)
        if (index !== -1) {
            root.positionViewAtIndex(index, GridView.Contain)
        }
    }

    clip: true
    cellWidth: width / root.columns
    cellHeight: {
        switch (root.style.toLowerCase()) {
        case "icononly":
            return Math.min(cellWidth, root.itemHeight)

        case "textonly":
            return Math.max(internal.textHeight, root.itemHeight)

        case "textbesideicon":
            return Math.max(internal.textHeight, root.itemHeight)

        case "textundericon":
            return Math.min(root.itemHeight, cellWidth) + internal.textHeight + root.padding

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
        highlighted: modelData.uid === fieldBinding.value

        Component {
            id: delegateIconOnly
            SquareIcon {
                source: form.getElementIcon(modelData.uid)
                size: parent.height
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        d.clicked()
                    }

                    onPressAndHold: {
                        showToast(form.getElementName(modelData.uid))
                    }
                }
            }
        }

        Component {
            id: delegateTextOnly
            Text {
                width: parent.width
                height: parent.height
                elide: Text.ElideRight
                text: form.getElementName(modelData.uid)
                font.pixelSize: root.fontSize
                font.bold: root.fontBold
                verticalAlignment: Text.AlignVCenter
                color: Material.foreground
            }
        }

        Component {
            id: delegateTextBesideIcon
            RowLayout {
                spacing: root.padding
                SquareIcon {
                    source: form.getElementIcon(modelData.uid)
                    size: parent.height
                }
                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    elide: Text.ElideRight
                    text: form.getElementName(modelData.uid)
                    font.pixelSize: root.fontSize
                    font.bold: root.fontBold
                    verticalAlignment: Text.AlignVCenter
                    color: Material.foreground
                }
            }
        }

        Component {
            id: delegateTextUnderIcon
            ColumnLayout {
                spacing: root.padding
                SquareIcon {
                    Layout.alignment: Qt.AlignHCenter
                    source: form.getElementIcon(modelData.uid)
                    size: parent.height - internal.textHeight
                }
                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    elide: Text.ElideRight
                    text: form.getElementName(modelData.uid)
                    font.pixelSize: root.fontSize
                    font.bold: root.fontBold
                    horizontalAlignment: Text.AlignHCenter
                    color: Material.foreground
                }
            }
        }

        sourceComponent: {
            switch (root.style.toLowerCase()) {
            case "icononly":
                return delegateIconOnly

            case "textonly":
                return delegateTextOnly

            case "textbesideicon":
                return delegateTextBesideIcon

            case "textundericon":
                return delegateTextUnderIcon

            default:
                return delegateTextOnly
            }
        }

        onClicked: {
            fieldBinding.setControlState(listElementUid + "_CurrentIndex", model.index)
            itemClicked(modelData.uid)
        }
    }
}
