import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

OptionsPopup {
    id: popup

    property string confirmText: qsTr("Yes")
    property string confirmIcon: "qrc:/icons/ok.svg"
    property bool confirmDelay: true
    property string cancelText: qsTr("Cancel")
    property string cancelIcon: "qrc:/icons/cancel.svg"

    signal confirmed
    signal cancel

    icon: "qrc:/icons/alert_circle_outline.svg"

    model: [
        { text: confirmText, icon: confirmIcon, delay: confirmDelay },
        { text: cancelText, icon: cancelIcon }
    ]

    onClicked: function (index) {
        switch (index) {
        case 0:
            popup.confirmed()
            break

        case 1:
            popup.cancel()
            break
        }
    }
}
