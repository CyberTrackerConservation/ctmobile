import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

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
