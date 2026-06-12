import QtQuick
import QtQuick.Controls.Material

ItemDelegate {
    id: root

    property alias iconSource: listRow.iconSource
    property alias iconOpacity: listRow.iconOpacity
    property alias iconColor: listRow.iconColor
    property alias subText: listRow.subText
    property alias wrapSubText: listRow.wrapSubText
    property alias topTextFontSize: listRow.textFontSize
    property alias subTextFontSize: listRow.subTextFontSize
    property bool divider: true
    property alias chevronRight: listRow.chevronRight
    property alias toolButtonVisible: listRow.toolButtonVisible
    property alias toolButtonEnabled: listRow.toolButtonEnabled
    property alias toolButtonIconSource: listRow.toolButtonIconSource

    signal toolButtonClicked

    contentItem: ListRow {
        id: listRow
        text: root.text
        checkable: root.checkable
        checked: root.checked
        onCheckedChanged: root.checked = checked
        onToolButtonClicked: root.toolButtonClicked()
    }

    HorizontalDivider {
        visible: root.divider
    }
}
