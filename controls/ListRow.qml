import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import CyberTracker as C

RowLayout {
    id: root

    property string iconSource
    property double iconOpacity: 1.0
    property var iconColor
    property string text
    property string subText
    property int textFontSize: App.settings.font18
    property int subTextFontSize: App.settings.font12
    property bool checkable: false
    property alias checked: checkableBox.checked
    property bool wrapSubText: false
    property bool chevronRight: false
    property bool toolButtonVisible: false
    property bool toolButtonEnabled: true
    property string toolButtonIconSource

    signal toolButtonClicked

    Binding { target: background; property: "color"; value: colorContent || Style.colorContent }

    opacity: parent.enabled ? 1.0 : 0.5

    SquareIcon {
        size: Style.iconSize48
        recolor: root.iconColor !== undefined
        source: root.iconSource
        opacity: root.iconOpacity
    }

    ColumnLayout {
        id: titleColumn
        Layout.fillWidth: true

        Label {
            id: labelTitle
            text: root.text
            font.pixelSize: root.textFontSize
            Layout.fillWidth: true
            elide: Label.ElideRight
        }

        Label {
            text: root.subText
            font.pixelSize: root.subTextFontSize
            opacity: root.parent.enabled ? 0.5 : 1.0
            Layout.fillWidth: true
            wrapMode: root.wrapSubText ? Label.WordWrap : Label.NoWrap
            visible: text !== ""
            elide: Label.ElideRight
        }
    }

    ToolButton {
        visible: root.toolButtonVisible
        icon.source: root.toolButtonIconSource
        icon.width: Style.toolButtonSize
        icon.height: Style.toolButtonSize
        opacity: root.toolButtonEnabled ? 0.5 : 1.0
        enabled: root.toolButtonEnabled
        onClicked: root.toolButtonClicked()
    }

    CheckableBox {
        id: checkableBox
        visible: root.checkable
        Layout.alignment: Qt.AlignRight
        size: Style.minRowHeight
    }

    ChevronRight {
        id: chevronRight
        visible: root.chevronRight
    }
}
