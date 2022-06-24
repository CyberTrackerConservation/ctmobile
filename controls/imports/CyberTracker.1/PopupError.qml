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
        color: Material.color(Material.Red, App.settings.darkTheme ? Material.Shade500 : Material.Shade800)
        radius: 0
        opacity: 0.75
    }

    Timer {
        id: errorTimer
        interval: 3000
        repeat: false
        onTriggered: popup.close()
    }

    Label {
        id: errorLabel
        width: parent.width
        horizontalAlignment: Label.AlignHCenter
        font.pixelSize: App.settings.font14
        color: "white"
        wrapMode: Label.WordWrap
    }

    onAboutToShow: errorTimer.start()

    function start(parentWindow, errorText) {
        errorLabel.text = errorText

        popup.x = (parentWindow.width - implicitWidth) / 2
        popup.y = (parentWindow.height - height)

        if (!errorTimer.running) {
            open()
        } else {
            errorTimer.restart()
        }
    }
}
