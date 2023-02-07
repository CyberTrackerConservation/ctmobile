import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

ItemDelegate {
    id: root

    Binding {
        target: background
        property: "color"
        value: root.highlighted ? (Style.darkTheme ? Material.background : Material.foreground) : (colorContent || Style.colorContent)
    }

    Binding {
        target: background
        property: "opacity"
        value: root.highlighted ? 0.5 : 1.0
    }
}
