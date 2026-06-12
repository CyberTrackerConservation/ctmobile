import QtQuick
import QtQuick.Controls.Material

ItemDelegate {
    id: root

    Binding {
        target: background
        property: "color"
        value: root.highlighted ? (colorHighlight || Style.colorHighlight) : (colorContent || Style.colorContent)
    }

    Binding {
        target: background
        property: "opacity"
        value: root.highlighted ? 0.75 : 1.0
    }
}
