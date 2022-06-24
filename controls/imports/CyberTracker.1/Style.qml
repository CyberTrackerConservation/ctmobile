pragma Singleton;
import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls.Material 2.12

QtObject {
    Material.primary: colorPrimary
    Material.accent: colorAccent
    Material.theme: darkTheme ? Material.Dark : Material.Light

    property bool darkTheme: App.settings.darkTheme
    property color colorPrimary: App.config.colorPrimary
    property color colorAccent: App.config.colorAccent
    property color colorForeground: Material.foreground

    property color colorGroove: darkTheme ? "#808080" : "#CECECE"
    property color colorContent: Qt.lighter(Material.background, 0.98)
    property color colorToolbar: Qt.lighter(Material.background, darkTheme ? 0.5 : 1.5)

    property color colorCalculated: darkTheme ? "lightsteelblue" : "steelblue"
    property color colorInvalid: darkTheme ? "red" : "crimson"
    property var colorHighlighted: darkTheme ? "lightsteelblue" : "steelblue"

    property int toolButtonSize: 30
    property int minRowHeight: 24 + (16 * App.settings.fontSize) / 100

    readonly property string backIconSource: "qrc:/icons/arrow_back.svg"
}
