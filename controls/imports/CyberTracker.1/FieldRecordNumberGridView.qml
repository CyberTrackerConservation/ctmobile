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

    signal itemClicked(string recordUid, string fieldUid)

    Text {
        id: textHeightCheck
        text: "Wg"
        font.pixelSize: root.fontSize
        font.bold: root.fontBold
        visible: false
    }

    clip: false

    cellWidth: {
        return width / root.columns
    }

    cellHeight: {
        switch (root.style.toLowerCase()) {
        case "icononly":
            return Math.min((cellWidth + root.padding) / 2, root.itemHeight)

        case "textonly":
            return Math.max(textHeightCheck.height + 8, root.itemHeight)

        case "textbesideicon":
            return Math.max(textHeightCheck.height + 8, root.itemHeight)

        default:
            return Math.max(textHeightCheck.height + 8, root.itemHeight)
        }
    }

    C.FieldBinding {
        id: fieldBinding
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

        C.FieldBinding {
            id: fieldBindingDelegate
            recordUid: modelData.recordUid
            fieldUid: modelData.fieldUid
        }

        Component {
            id: delegateIconOnly

            RowLayout {
                spacing: root.padding
                clip: true

                SquareIcon {
                    Layout.alignment: Qt.AlignLeft
                    source: fieldBindingDelegate.fieldIcon
                    size: parent.height
                }

                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideLeft
                    font.pixelSize: root.fontSize
                    font.bold: root.fontBold
                    color: Material.foreground
                    clip: true
                    text: {
                        let result = fieldBindingDelegate.displayValue
                        return result === "" ? "-" : result
                    }
                }
            }
        }

        Component {
            id: delegateTextOnly
            RowLayout {
                spacing: root.padding
                clip: true

                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    elide: Text.ElideRight
                    text: fieldBindingDelegate.fieldName
                    font.pixelSize: root.fontSize
                    verticalAlignment: Text.AlignVCenter
                    color: Material.foreground
                }
                Text {
                    Layout.alignment: Qt.AlignRight
                    Layout.fillHeight: true
                    font.pixelSize: root.fontSize
                    font.bold: root.fontBold
                    color: Material.foreground
                    verticalAlignment: Text.AlignVCenter
                    text: {
                        let result = fieldBindingDelegate.displayValue
                        return result === "" ? "-" : result
                    }
                }
            }
        }

        Component {
            id: delegateTextBesideIcon
            RowLayout {
                spacing: root.padding
                clip: true

                SquareIcon {
                    source: fieldBindingDelegate.fieldIcon
                    size: parent.height
                }
                Text {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    elide: Text.ElideRight
                    text: fieldBindingDelegate.fieldName
                    font.pixelSize: root.fontSize
                    verticalAlignment: Text.AlignVCenter
                    color: Material.foreground
                }
                Text {
                    Layout.alignment: Qt.AlignRight
                    Layout.fillHeight: true
                    font.pixelSize: root.fontSize
                    font.bold: root.fontBold
                    color: Material.foreground
                    verticalAlignment: Text.AlignVCenter
                    text: {
                        let result = fieldBindingDelegate.displayValue
                        return result === "" ? "-" : result
                    }
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

            default:
                return delegateTextOnly
            }
        }

        onClicked: {
            root.itemClicked(modelData.recordUid, modelData.fieldUid)
        }
    }

    Component.onCompleted: {
        root.model = rebuild()
    }

    function rebuild() {
        let result = []
        let recordFieldUid = form.getRecordFieldUid(recordUid)
        let recordField = form.getField(recordFieldUid)

        for (let i = 0; i < recordField.fields.length; i++) {
            let field = recordField.fields[i]

            if (!form.getFieldVisible(recordUid, field.uid)) {
                continue
            }

            if (form.getFieldType(field.uid) !== "NumberField") {
                App.showError(qsTr("Group field not a number"))
            }

            result.push({ recordUid: recordUid, fieldUid: field.uid })
        }

        return result
    }
}
