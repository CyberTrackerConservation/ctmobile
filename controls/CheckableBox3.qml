import QtQuick
import QtQuick.Controls.Material

ToolButton {
    enum CheckState {
        Unchecked,
        Checked,
        Indeterminate
    }

    property int size: App.settings.font16 * 2
    property int checkState: CheckableBox3.Unchecked

    icon.source: {
        switch (checkState) {
        case CheckableBox3.Checked:
            return "qrc:/icons/checkbox_fill.svg"
        case CheckableBox3.Indeterminate:
            return "qrc:/icons/checkbox_unknown.svg"
        default:
            return "qrc:/icons/checkbox_blank_outline.svg"
        }
    }

    icon.color: checkState === CheckableBox3.Checked ? Material.accent : Material.foreground
    icon.width: size
    icon.height: size
    opacity: enabled ? (checkState === CheckableBox3.Checked ? 1.0 : 0.75) : 0.5
    flat: true
    checkable: false
    display: text === "" ? Button.IconOnly : Button.TextBesideIcon
    antialiasing: false

    onClicked: {
        switch (checkState) {
        case CheckableBox3.Indeterminate:
            checkState = CheckableBox3.Checked
            break
        case CheckableBox3.Checked:
            checkState = CheckableBox3.Unchecked
            break
        default:
            checkState = CheckableBox3.Indeterminate
            break
        }
    }
}
