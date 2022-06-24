import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

Popup {
    id: popup

    closePolicy: Popup.NoAutoClose
    bottomMargin: 80
    implicitWidth: parent.width * 0.80

    background: Rectangle {
        color: App.settings.darkTheme ? "Darkgrey" : "#323232"
        radius: 0
        opacity: App.settings.darkTheme ? 0.9 : 0.75
    }

    Timer {
        id: toastTimer
        interval: 3000
        repeat: false
        onTriggered: popup.close()
    }

    Label {
        id: toastLabel
        width: parent.width
        horizontalAlignment: Label.AlignHCenter
        font.pixelSize: App.settings.font14
        color: "white"
        wrapMode: Label.WordWrap
    }

    onAboutToShow: toastTimer.start()

    function start(parentWindow, toastText) {
        toastLabel.text = toastText

        popup.x = (parentWindow.width - implicitWidth) / 2
        popup.y = (parentWindow.height - height)

        if (!toastTimer.running) {
            open()
        } else {
            toastTimer.restart()
        }
    }
}
