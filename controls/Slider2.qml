import QtQuick
import QtQuick.Controls.Material

Slider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitHandleHeight + topPadding + bottomPadding)
    spacing: App.scaleByFontSize(3)
    padding: App.scaleByFontSize(6)
    stepSize: 1

    handle: Rectangle {
        x: control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.vertical ? control.visualPosition * (control.availableHeight - height) : (control.availableHeight - height) / 2)

        implicitWidth: App.scaleByFontSize(16)
        implicitHeight: App.scaleByFontSize(16)

        radius: width / 2

        border.width: App.scaleByFontSize(control.pressed ? width / 2 : 2)
        border.color: Material.accent
        color: App.settings.darkTheme ? "white" : Material.foreground

        Behavior on border.width { SmoothedAnimation {} }
    }

    background: Rectangle {
        x: (control.width  - width) / 2
        y: (control.height - height) / 2

        implicitWidth: App.scaleByFontSize(control.horizontal ? 200 : 2)
        implicitHeight: App.scaleByFontSize(control.horizontal ? 2 : 200)

        width: control.horizontal ? control.availableWidth : implicitWidth
        height: control.horizontal ? implicitHeight : control.availableHeight

        radius: width

        color: Material.accent
    }
}
