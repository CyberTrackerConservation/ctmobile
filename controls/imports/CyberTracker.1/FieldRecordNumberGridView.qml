import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

GridViewV {
    id: root

    property alias recordUid: fieldListModel.recordUid
    property string fieldUid
    property bool highlightInvalid: false

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

    model: C.FieldListProxyModel {
        id: fieldListModel
        recordUid: root.recordUid
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
            recordUid: model.recordUid
            fieldUid: model.fieldUid
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
                    color: getTextColor(fieldBindingDelegate)
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
                    font.bold: root.fontBold
                    verticalAlignment: Text.AlignVCenter
                    color: getTextColor(fieldBindingDelegate)
                }
                Text {
                    Layout.alignment: Qt.AlignRight
                    Layout.fillHeight: true
                    font.pixelSize: root.fontSize
                    font.bold: root.fontBold
                    color: getTextColor(fieldBindingDelegate)
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
                    font.bold: root.fontBold
                    verticalAlignment: Text.AlignVCenter
                    color: getTextColor(fieldBindingDelegate)
                }
                Text {
                    Layout.alignment: Qt.AlignRight
                    Layout.fillHeight: true
                    font.pixelSize: root.fontSize
                    font.bold: root.fontBold
                    color: getTextColor(fieldBindingDelegate)
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
            root.itemClicked(model.recordUid, model.fieldUid)
        }
    }

    function getTextColor(fieldBinding) {
        return (highlightInvalid && !fieldBinding.isValid) ? Style.colorInvalid : Material.foreground
    }

    function validate() {
        for (let i = 0; i < model.count; i++) {
            let fieldValue = model.get(i)
            if (fieldValue.valid) {
                continue
            }

            highlightInvalid = true
            console.log("Binding value invalid: " + fieldValue.fieldUid)
            root.positionViewAtIndex(i, GridView.Center)

            let message = qsTr("Value required")
            if (fieldValue.constraintMessage !== "") {
                message = fieldValue.constraintMessage
            } else if (fieldValue.requiredMessage !== "") {
                message = fieldValue.requiredMessage
            }
            showError(message)

            return false
        }

        highlightInvalid = false
        return true
    }
}
