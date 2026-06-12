import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

Item {
    id: root

    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid
    property bool forceFocus: false

    property string placeholderText: form.getElementName(fieldBinding.field.hintElementUid)
    property int maximumLength: fieldBinding.field.maxLength === 0 ? -1 : fieldBinding.field.maxLength
    property string inputMask: fieldBinding.field.inputMask
    property string text
    property bool multiLine: fieldBinding.field.multiLine
    property bool numbers: fieldBinding.field.numbers

    property alias font: textField.font

    implicitHeight: textField.implicitHeight

    C.FieldBinding {
        id: fieldBinding
    }

    TextField {
        id: textField
        width: parent.width
        font.pixelSize: App.settings.font17
        maximumLength: root.maximumLength
        inputMask: root.inputMask
        inputMethodHints: root.numbers ? Qt.ImhDigitsOnly : Qt.ImhNone
        text: root.text
        visible: !root.multiLine

        Material.containerStyle: Material.Filled

        onTextChanged: {
            if (visible && !internal.mutex) {
                valueChange(textField, text)
            }
        }

        onCursorPositionChanged: {
            if (visible && !internal.mutex) {
                fieldBinding.setControlState("cursorPosition", cursorPosition)
            }
        }

        onSelectedTextChanged: {
            if (visible && !internal.mutex) {
                fieldBinding.setControlState("selectionStart", selectionStart)
                fieldBinding.setControlState("selectionEnd", selectionEnd)
            }
        }
    }

    Text {
        anchors.fill: textField
        visible: textField.visible && root.placeholderText !== "" && textField.text === ""
        padding: 12
        clip: true
        font: root.font
        text: root.placeholderText.replace(/\n/g, ' ')
        color: Material.color(Material.Grey)
        elide: Text.ElideRight
    }

    TextArea {
        id: textArea
        width: parent.width
        anchors.fill: parent
        font: root.font
        inputMethodHints: root.numbers ? Qt.ImhDigitsOnly : Qt.ImhNone
        text: root.text
        visible: root.multiLine
        wrapMode: TextArea.WordWrap

        onTextChanged: {
            if (visible && !internal.mutex) {
                valueChange(textArea, text)
            }
        }

        onCursorPositionChanged: {
            if (visible && !internal.mutex) {
                fieldBinding.setControlState("cursorPosition", cursorPosition)
            }
        }

        onSelectedTextChanged: {
            if (visible && !internal.mutex) {
                fieldBinding.setControlState("selectionStart", selectionStart)
                fieldBinding.setControlState("selectionEnd", selectionEnd)
            }
        }
    }

    Text {
        anchors.fill: textArea
        visible: textArea.visible && root.placeholderText !== "" && textArea.text === ""
        padding: 12
        clip: true
        font: root.font
        text: root.placeholderText
        color: Material.color(Material.Grey)
        wrapMode: TextArea.WordWrap
        elide: Text.ElideRight
    }

    QtObject {
        id: internal
        property bool mutex: true
    }

    Component.onCompleted: {
        internal.mutex = true
        try {
            let value = fieldBinding.getValue(fieldBinding.field.defaultValue)
            root.text = value !== undefined ? value : ""

            let selectStart = fieldBinding.getControlState("selectionStart", 0)
            let selectEnd = fieldBinding.getControlState("selectionEnd", 0)

            if (root.multiLine) {
                textArea.select(selectStart, selectEnd)
                textArea.cursorPosition = fieldBinding.getControlState("cursorPosition", 0)
                if (root.forceFocus) {
                    textArea.forceActiveFocus()
                }
            } else {
                textField.select(selectStart, selectEnd)
                textField.cursorPosition = fieldBinding.getControlState("cursorPosition", 0)
                if (root.forceFocus) {
                    textField.forceActiveFocus()
                }
            }

        } finally {
            internal.mutex = false
        }
    }

    function valueChange(control, text) {
        internal.mutex = true
        try {
            fieldBinding.setValue(text)
            let value = fieldBinding.getValue(fieldBinding.field.defaultValue)
            control.text = value !== undefined ? value : ""

        } finally {
            internal.mutex = false
        }
    }
}
