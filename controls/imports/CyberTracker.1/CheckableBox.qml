import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

ToolButton {
    property int size: App.settings.font16 * 2
    property bool radioIcon: false

    icon.source: {
        if (radioIcon) {
            return checked ? "qrc:/icons/radio_button_checked.svg" : "qrc:/icons/radio_button_unchecked.svg"
        } else {
            return checked ? "qrc:/icons/checkbox_fill.svg" : "qrc:/icons/checkbox_blank_outline.svg"
        }
    }

    icon.color: checked ? Material.accent : Material.foreground
    icon.width: size
    icon.height: size
    opacity: checked ? 1.0 : 0.75
    flat: true
    checkable: true
    display: text === "" ? Button.IconOnly : Button.TextBesideIcon
    antialiasing: false
}
