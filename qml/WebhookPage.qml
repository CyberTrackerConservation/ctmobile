import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import CyberTracker as C

C.ContentPage {
    id: page

    Component.onCompleted: {
        textField.text = App.settings.webhookUrl
    }

    header: C.PageHeader {
        text: qsTr("Webhook")
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        menuEnabled: App.settings.webhookUrl !== textField.text.trim()
        onMenuClicked: {
            Qt.inputMethod.hide()
            App.settings.webhookUrl = textField.text.trim()
            popPageImmediate(page)
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: App.scaleByFontSize(16)
        }

        Label {
            Layout.fillWidth: true
            font.pixelSize: App.settings.font16
            wrapMode: Label.WordWrap
            text: qsTr("When data is saved, it will also be uploaded to a webhook.") + " " +
                  qsTr("For example %1 can send data to %2.").arg("SMART").arg("EarthRanger") + "\n\n" +
                  qsTr("The URL must be provided by a trusted source.") + "\n\n" +
                  qsTr("Only newly saved data will be sent.")
        }

        Item {
            height: App.scaleByFontSize(8)
        }

        C.HorizontalDivider {
            Layout.fillWidth: true
        }

        Item {
            height: App.scaleByFontSize(8)
        }

        TextArea {
            id: textField
            Layout.fillWidth: true
            font.pixelSize: App.settings.font12
            placeholderText: qsTr("Webhook URL")
            rightPadding: C.Style.toolButtonSize + App.scaleByFontSize(8)
            wrapMode: TextArea.Wrap
            onTextChanged: {
                if (text.includes("\n") || text.includes("\r")) {
                    text = text.replace(/[\r\n]+/g, "") // Remove all newlines
                }
            }
            ToolButton {
                anchors {
                    verticalCenter: textField.verticalCenter
                    right: textField.right
                }

                icon.source: "qrc:/icons/cancel.svg"
                enabled: textField.text !== ""
                opacity: 0.5
                onClicked: {
                    textField.text = ""
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
