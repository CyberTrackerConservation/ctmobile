import QtQuick

Rectangle {
    x: App.scaleByFontSize(6)
    y: parent.height - height
    width: parent.width - App.scaleByFontSize(12)
    height: Style.lineWidth1
    color: Style.colorGroove
}
