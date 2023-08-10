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

    property color colorGroove: getColorGroove(Material.foreground)
    property color colorContent: getColorContent(Material.background)
    property color colorToolbar: getColorToolbar(Material.background)
    property color colorHighlight: getColorHighlight(Material.foreground)

    property color colorCalculated: darkTheme ? "lightsteelblue" : "steelblue"
    property color colorInvalid: darkTheme ? "red" : "crimson"

    property int toolButtonSize: (24 * App.settings.fontSize) / 100
    property int minRowHeight: Math.max(40, (40 * App.settings.fontSize) / 100)
    property int iconSize24: Math.min(48, (24 * App.settings.fontSize) / 100)
    property int iconSize48: Math.max(40, (40 * App.settings.fontSize) / 100)
    property int iconSize64: Math.max(64, (64 * App.settings.fontSize) / 100)

    property int lineWidth1: (1 * App.settings.fontSize) / 100
    property int lineWidth2: (2 * App.settings.fontSize) / 100

    readonly property string backIconSource: "qrc:/icons/arrow_back.svg"
    readonly property string nextIconSource: "qrc:/icons/arrow_right.svg"
    readonly property string saveIconSource: "qrc:/icons/save.svg"
    readonly property string mapIconSource: "qrc:/icons/map_outline.svg"
    readonly property string formIconSource: "qrc:/icons/file_outline.svg"
    readonly property string homeIconSource: "qrc:/icons/home_import_outline.svg"
    readonly property string swapIconSource: "qrc:/icons/arrange_send_backward.svg"
    readonly property string okIconSource: "qrc:/icons/ok.svg"
    readonly property string cancelIconSource: "qrc:/icons/cancel.svg"

    function getColorGroove(foregroundColor) {
        return Qt.rgba(foregroundColor.r, foregroundColor.g, foregroundColor.b, 0.25)
    }

    function getColorContent(backgroundColor) {
        return Qt.lighter(backgroundColor, 0.95)
    }

    function getColorToolbar(backgroundColor) {
        return Qt.lighter(backgroundColor, Style.darkTheme ? 0.5 : 1.0)
    }

    function getColorHighlight(highlightColor) {
        return Qt.rgba(highlightColor.r, highlightColor.g, highlightColor.b, 0.65)
    }
}
