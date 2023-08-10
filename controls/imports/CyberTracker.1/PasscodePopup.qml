import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12

PopupBase {
    id: popup

    insets: 0

    anchors.centerIn: null

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    width: parent.width * 0.8
    height: parent.height * 0.8

    signal success;

    property string passcode: ""
    property string userCode: ""

    property bool dark: Material.theme === Material.Dark
    property var backColor    : dark ? Material.foreground : Material.color(Material.Grey, Material.Shade400)
    property var textColor    : dark ? Material.foreground : Material.color(Material.Grey, Material.Shade900)
    property var displayColor : dark ? Material.background : Material.color(Material.Grey, Material.Shade200)
    property var digitColor   : dark ? Material.background : Material.color(Material.Grey, Material.Shade100)
    property var otherColor   : dark ? Material.color(Material.Grey, Material.ShadeA700) : Material.color(Material.Grey, Material.Shade300)

    property int wspace: 2
    property int keyW: width / 4
    property int keyH: (height - displayH) / 4
    property int displayW: width - wspace * 2
    property int displayH: height / 5
    property int digitFontPixelSize: keyH * 3 / 8
    property int displayFontPixelSize: displayH * 3 / 8

    function buttonClick(t) {
        var newText = userCode
        switch (t) {
        case "CLEAR":
            newText = ""
            break

        case "BACKSPACE":
            newText = newText.slice(0, -1)
            break

        default: newText += t
        }

        if (newText.length <= 8) {
            userCode = newText
        }
    }

    Item {
        anchors.fill: parent
        Rectangle {
            anchors.fill: parent
            color: backColor
        }

        GridLayout {
            x: wspace
            y: wspace
            width: parent.width - wspace * 2
            height: parent.height - wspace * 2
            columns: 4
            rows: 4
            rowSpacing: wspace
            columnSpacing: wspace
            Layout.margins: wspace

            Label {
                Layout.columnSpan: 4
                Layout.fillWidth: true
                horizontalAlignment: Label.AlignHCenter
                verticalAlignment: Label.AlignVCenter
                font.pixelSize: App.settings.font18
                fontSizeMode: Label.Fit
                text: qsTr("Enter passcode")
                color: popup.dark ? Material.background : Material.foreground
            }

            Rectangle {
                Layout.columnSpan: 4
                color: displayColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: displayH
                Text {
                    anchors.fill: parent; color: textColor; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    text: userCode
                    font.pixelSize: displayFontPixelSize
                    fontSizeMode: Text.Fit
                }
            }

            // Row 1
            Rectangle {
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: keyH
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "7"; onClicked: buttonClick("7")
                }
            }

            Rectangle {
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "8"; onClicked: buttonClick("8")
                }
            }

            Rectangle {
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "9"; onClicked: buttonClick("9")
                }
            }

            Rectangle {
                color: otherColor; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "C"; onClicked: buttonClick("CLEAR")
                }
            }

            // Row 2
            Rectangle {
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: keyH
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "4"; onClicked: buttonClick("4")
                }
            }

            Rectangle {
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "5"; onClicked: buttonClick("5")
                }
            }

            Rectangle {
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "6"; onClicked: buttonClick("6")
                }
            }

            Rectangle {
                color: otherColor; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    icon.source: "qrc:/icons/backspace.svg"; onClicked: buttonClick("BACKSPACE")
                }
            }

            // Row 3
            Rectangle {
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: keyH
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "1"; onClicked: buttonClick("1")
                }
            }

            Rectangle {
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "2"; onClicked: buttonClick("2")
                }
            }

            Rectangle {
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "3"; onClicked: buttonClick("3")
                }
            }

            Rectangle {
                color: otherColor; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    icon.source: "qrc:/icons/cancel.svg"
                    onClicked: popup.close()
                }
            }

            // Row 4
            Rectangle {
                Layout.columnSpan: 3
                color: digitColor; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredHeight: keyH
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    text: "0"; onClicked: buttonClick("0")
                }
            }

            Rectangle {
                color: Material.accent; Layout.fillWidth: true; Layout.fillHeight: true
                ToolButton {
                    anchors.fill: parent; flat: true; font.pixelSize: digitFontPixelSize
                    icon.source: "qrc:/icons/ok.svg"
                    onClicked: if (popup.passcode == userCode) {
                        popup.close()
                        popup.success()
                    }
                }
            }
        }
    }
}
