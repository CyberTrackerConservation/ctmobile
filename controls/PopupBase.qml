import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Popup {
    id: popup

    // QtBug: "anchors.centerIn: parent" only works the first time.
    x: ~~(Overlay.overlay.width / 2 - width / 2)
    y: ~~(Overlay.overlay.height / 2 - height / 2)

    parent: Overlay.overlay
    modal: true

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    focus: true

    property bool closeOnBack: true
    property int insets: -App.scaleByFontSize(8)

    Component.onCompleted: {
        leftInset = insets
        topInset = insets
        rightInset = insets
        bottomInset = insets
    }

    Connections {
        target: App

        function onBackPressed() {
            if (closeOnBack && popup.opened) {
                popup.close()
            }
        }
    }

    Connections {
        target: popup

        function onOpened() {
            setProjectColors(popup)
            appWindow.popupCount++
        }

        function onAboutToHide() {
            appWindow.popupCount--
        }
    }
}
