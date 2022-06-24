import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12

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
    property int insets: -16

    Component.onCompleted: {
        topInset = insets
        leftInset = insets
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
            if (typeof(formPageStack) !== "undefined") {
                formPageStack.setProjectColors(popup)
            }

            appWindow.popupCount++
        }

        function onAboutToHide() {
            appWindow.popupCount--
        }
    }
}
