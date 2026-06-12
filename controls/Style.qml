pragma Singleton;
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Window

QtObject {
    Material.primary: colorPrimary
    Material.accent: colorAccent
    Material.theme: darkTheme ? Material.Dark : Material.Light

    property bool darkTheme: App.settings.darkTheme

    readonly property var themes: [
        { name: "forest",   colorPrimary: "#2A4F3F", colorPrimaryDark: "#5A8A6F", colorAccent: "#C75B4A", colorAccentDark: "#D8806E" },
        { name: "ocean",    colorPrimary: "#1E4D5C", colorPrimaryDark: "#5A8DA0", colorAccent: "#E8826E", colorAccentDark: "#F2A695" },
        { name: "sky",      colorPrimary: "#3B6A8F", colorPrimaryDark: "#7A9BB8", colorAccent: "#E89B5C", colorAccentDark: "#F2B583" },
        { name: "savannah", colorPrimary: "#7C6043", colorPrimaryDark: "#A88A6B", colorAccent: "#C9A032", colorAccentDark: "#DDBA5C" },
        { name: "desert",   colorPrimary: "#A86B4E", colorPrimaryDark: "#C9907B", colorAccent: "#5A7B8F", colorAccentDark: "#8AA3B3" },
        { name: "meadow",   colorPrimary: "#6B8A4C", colorPrimaryDark: "#94B074", colorAccent: "#D89A2C", colorAccentDark: "#E8B95E" },
        { name: "mountain", colorPrimary: "#4A5564", colorPrimaryDark: "#7A8794", colorAccent: "#C8723D", colorAccentDark: "#DD9162" },
        { name: "classic",  colorPrimary: "#607D8B", colorPrimaryDark: "#90A4AE", colorAccent: "#F4511E", colorAccentDark: "#FF8A65" },
        { name: "smart",    colorPrimary: "#334469", colorPrimaryDark: "#8AA1D1", colorAccent: "#6C9779", colorAccentDark: "#8FBE9A" },
        { name: "rilla",    colorPrimary: "#58A867", colorPrimaryDark: "#7DC78A", colorAccent: "#7248B9", colorAccentDark: "#9C7BD1" }
    ]

    function themeTitle(name) {
        switch (name) {
            case "forest":   return qsTr("Forest")
            case "ocean":    return qsTr("Ocean")
            case "sky":      return qsTr("Sky")
            case "savannah": return qsTr("Savannah")
            case "desert":   return qsTr("Desert")
            case "meadow":   return qsTr("Meadow")
            case "mountain": return qsTr("Mountain")
            case "classic":  return qsTr("Classic")
            case "smart":    return qsTr("SMART")
            case "rilla":    return qsTr("Rilla")
            default:         return name
        }
    }

    function findTheme(name) {
        for (var i = 0; i < themes.length; ++i) {
            if (themes[i].name === name) return themes[i]
        }
        return null
    }

    readonly property var activeTheme: findTheme(App.settings.theme || App.config.defaultTheme || "")

    property color colorPrimary: activeTheme
        ? (darkTheme ? activeTheme.colorPrimaryDark : activeTheme.colorPrimary)
        : (darkTheme ? (App.config.colorPrimaryDark || App.config.colorPrimary) : App.config.colorPrimary)
    property color colorAccent: activeTheme
        ? (darkTheme ? activeTheme.colorAccentDark : activeTheme.colorAccent)
        : (darkTheme ? (App.config.colorAccentDark || App.config.colorAccent) : App.config.colorAccent)
    property color colorForeground: Material.foreground

    property color colorGroove: getColorGroove(Material.foreground)
    property color colorContent: getColorContent(Material.background)
    property color colorToolbar: getColorToolbar(Material.background)
    property color colorHighlight: getColorHighlight(Material.foreground)

    property color colorCalculated: darkTheme ? "lightsteelblue" : "steelblue"
    property color colorInvalid: darkTheme ? "red" : "crimson"

    property int toolButtonSize: (24 * App.settings.fontSize) / 100
    property int minRowHeight: Math.max(40, (40 * App.settings.fontSize) / 100)
    property int iconSize24: Math.min(40, (24 * App.settings.fontSize) / 100)
    property int iconSize32: Math.min(40, (32 * App.settings.fontSize) / 100)
    property int iconSize40: Math.max(40, (32 * App.settings.fontSize) / 100)
    property int iconSize48: Math.max(40, (40 * App.settings.fontSize) / 100)
    property int iconSize64: Math.max(64, (64 * App.settings.fontSize) / 100)

    property int lineWidth1: (1 * App.settings.fontSize) / 100
    property int lineWidth2: (2 * App.settings.fontSize) / 100

    property double switchScale: Math.max(1.0, App.settings.fontSize / 125)

    readonly property string backIconSource: "qrc:/icons/arrow_back.svg"
    readonly property string nextIconSource: "qrc:/icons/arrow_right.svg"
    readonly property string saveIconSource: "qrc:/icons/save.svg"
    readonly property string mapIconSource: "qrc:/icons/map_outline.svg"
    readonly property string formIconSource: "qrc:/icons/file_outline.svg"
    readonly property string homeIconSource: "qrc:/icons/home_import_outline.svg"
    readonly property string swapIconSource: "qrc:/icons/arrange_send_backward.svg"
    readonly property string okIconSource: "qrc:/icons/ok.svg"
    readonly property string cancelIconSource: "qrc:/icons/cancel.svg"
    readonly property string checkIconSource: "qrc:/icons/check.svg"

    readonly property color frontColor1: !darkTheme ? "#10141c" : "#fdfdfd" //small areas
    readonly property color frontColor2: !darkTheme ? "#27282a" : "#d9d9d9" //medium areas
    readonly property color frontColor3: !darkTheme ? "#176B87" : "#9e9e9e" //large areas

    readonly property color midColor1: "#F4BF96"
    readonly property color midColor2: "#F4BF96"
    readonly property color midColor3: !darkTheme ? "#b0e5f7" : "#a8a8a8"

    readonly property color backColor1: !darkTheme ? "#FFF0F5" : "#232D3F" //large areas
    readonly property color backColor2: !darkTheme ? "#FFD1DA" : "#27282a" //medium areas
    readonly property color backColor3: !darkTheme ? "#FBA1B7" : "#10141c" //small areas

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
