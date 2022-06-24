import QtQuick 2.12
import QtQuick.Controls 2.12

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
    property alias toolButton: listRow.toolButton

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
