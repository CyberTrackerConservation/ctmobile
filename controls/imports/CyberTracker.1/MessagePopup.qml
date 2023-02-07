import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0

PopupBase {
    id: popup

    property string title: ""
    property string message: ""

    insets: -12
    width: parent.width * 0.75

    signal clicked()

    contentItem: ColumnLayout {
        width: popup.width
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: popup.title
            font.pixelSize: App.settings.font18
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Label.AlignHCenter
            wrapMode: Label.Wrap
            text: popup.message
            font.pixelSize: App.settings.font12
        }

        RoundButton {
            Layout.alignment: Qt.AlignHCenter
            icon.source: "qrc:/icons/ok.svg"
            icon.width: Style.toolButtonSize
            icon.height: Style.toolButtonSize
            onClicked: {
                popup.close()
                popup.clicked()
            }
        }
    }
}
