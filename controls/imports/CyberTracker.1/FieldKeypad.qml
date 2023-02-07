import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

Item {
    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property string listElementUid

    property color textColor: Material.foreground
    property color displayColor: colorContent || Style.colorContent
    property color digitColor: colorContent || Style.colorContent
    property color otherColor: Qt.lighter(colorContent || Style.colorContent, 0.80)
    property int wspace: Style.lineWidth1
    property int keyW: width / 4
    property int keyH: (height - displayH) / 4
    property int displayW: width - wspace * 2
    property int displayH: height / 5
    property int digitFontPixelSize: App.settings.font18 * 1.5
    property int displayFontPixelSize: App.settings.font20 * 1.5

    C.FieldBinding {
        id: fieldBinding
        Component.onCompleted: updateDisplay()
    }

    Rectangle {
        anchors.fill: parent
        color: Style.colorGroove
    }

    GridLayout {
        x: wspace
        y: wspace
        width: parent.width - wspace * 2
        height: parent.height - wspace * 2
        columns: 4
        rows: 4
        rowSpacing: wspace
        columnSpacing: wspace
        Layout.margins: wspace

        Rectangle {
            Layout.columnSpan: 4
            color: displayColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: displayH
            Text {
                id: displayText
                anchors.fill: parent; color: textColor; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                font.pixelSize: displayFontPixelSize
                fontSizeMode: Text.Fit
            }
        }

        // Row 1
        Rectangle {
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: keyH
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "7"; onClicked: buttonClick("7")
            }
        }

        Rectangle {
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "8"; onClicked: buttonClick("8")
            }
        }

        Rectangle {
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "9"; onClicked: buttonClick("9")
            }
        }

        Rectangle {
            color: otherColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "C"; onClicked: buttonClick("CLEAR")
            }
        }

        // Row 2
        Rectangle {
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: keyH
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "4"; onClicked: buttonClick("4")
            }
        }

        Rectangle {
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "5"; onClicked: buttonClick("5")
            }
        }

        Rectangle {
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "6"; onClicked: buttonClick("6")
            }
        }

        Rectangle {
            color: otherColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                icon.source: "qrc:/icons/backspace.svg"; onClicked: buttonClick("BACKSPACE")
            }
        }

        // Row 3
        Rectangle {
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: keyH
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "1"; onClicked: buttonClick("1")
            }
        }

        Rectangle {
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "2"; onClicked: buttonClick("2")
            }
        }

        Rectangle {
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "3"; onClicked: buttonClick("3")
            }
        }

        Rectangle {
            color: otherColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize; enabled: fieldBinding.field.decimals > 0
                text: enabled ? "." : ""; onClicked: buttonClick(".")
            }
        }

        // Row 4
        Rectangle {
            Layout.columnSpan: 3
            color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: keyH
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "0"; onClicked: buttonClick("0")
            }
        }

        Rectangle {
            color: otherColor; Layout.fillWidth: true; Layout.fillHeight: true
            ToolButton {
                anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                text: "\u00b1"; onClicked: buttonClick("+-")
            }
        }
    }

    function getValue() {
        let value
        if (listElementUid === "") {
            value = fieldBinding.getValue()
        } else {
            value = fieldBinding.getValue({})[listElementUid]
        }
        return value === undefined ? "" : value.toString()
    }

    function setValue(value) {
        if (listElementUid === "") {
            fieldBinding.setValue(value)
        } else {
            let v = fieldBinding.getValue({})
            if (value === undefined) {
                delete v[listElementUid]
            } else {
                v[listElementUid] = value
            }

            fieldBinding.setValue(v)
        }
    }

    function buttonClick(t) {
        let newText = getValue()
        switch (t) {
        case "CLEAR":
            newText = ""
            break

        case "BACKSPACE":
            newText = newText.slice(0, -1)
            break

        case "+-":
            if (newText.charAt(0) === '-') {
                newText = newText.substr(1)
            } else {
                newText = "-" + newText
            }
            break

        case ".":
            if (!newText.includes(".") && fieldBinding.field.decimals > 0) {
                newText += t
            }
            break

        default: // number 0..9
            if (fieldBinding.field.decimals > 0) {
                let parts = newText.split('.', 2)
                if (parts.length === 2 && parts[1].length === fieldBinding.field.decimals) {
                    t = '';
                }
            }

            newText += t

            // Remove leading 0
            if (newText.length > 1 && newText[0] === '0' && newText[1] !== '.') {
                newText = newText.substring(1)
            }
        }

        setValue(newText !== "" ? newText : undefined)

        updateDisplay()
    }

    function updateDisplay() {
        displayText.text = getValue()
    }
}
