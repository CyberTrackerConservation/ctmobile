import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

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

    C.FieldBinding {
        id: fieldBinding
    }

    TextField {
        id: textField
        width: parent.width
        font.pixelSize: App.settings.font17
        placeholderText: root.placeholderText
        maximumLength: root.maximumLength
        inputMask: root.inputMask
        inputMethodHints: Qt.ImhNoPredictiveText | (root.numbers ? Qt.ImhDigitsOnly : Qt.ImhNone)
        text: root.text
        visible: !root.multiLine

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

    TextArea {
        id: textArea
        width: parent.width
        anchors.fill: parent
        font: root.font
        placeholderText: root.placeholderText
        inputMethodHints: Qt.ImhNoPredictiveText | (root.numbers ? Qt.ImhDigitsOnly : Qt.ImhNone)
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
