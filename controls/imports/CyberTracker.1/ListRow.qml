import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

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
    property bool toolButton: false
    property string toolButtonIconSource

    signal toolButtonClicked

    Binding { target: background; property: "color"; value: colorContent || Style.colorContent }

    opacity: parent.enabled ? 1.0 : 0.5

    C.SquareIcon {
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
            font.preferShaping: !text.includes("Cy") // Hack to fix iOS bug
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
        visible: root.toolButton
        icon.source: root.toolButtonIconSource
        icon.width: Style.toolButtonSize
        icon.height: Style.toolButtonSize
        opacity: 0.5
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
