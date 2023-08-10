import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C
import ".."

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Connect to %1").arg("Rilla Forms")
    }

    Pane {
        id: pane
        anchors.fill: parent

        Material.background: C.Style.colorContent
        topPadding: flickable.contentHeight < parent.height ? (parent.height - flickable.contentHeight) / 2 : App.scaleByFontSize(8)
        leftPadding: parent.width * 0.1
        rightPadding: parent.width * 0.1

        contentItem: Flickable {
            id: flickable

            flickableDirection: Flickable.VerticalFlick
            contentHeight: column.height
            contentWidth: column.width

            ColumnLayout {
                id: column
                width: pane.width * 0.8
                spacing: App.scaleByFontSize(8)

                C.SquareIcon {
                    Layout.alignment: Qt.AlignHCenter
                    size: C.Style.iconSize64
                    source: "qrc:/Google/ggforms.svg"
                }

                ItemDelegate {
                    Layout.fillWidth: true
                    contentItem: Label {
                        font.pixelSize: App.settings.font18
                        text: qsTr("%1 from the %2 add-on for %3").arg("**" + qsTr("Scan QR code") + "**").arg("Rilla Forms").arg("Google Forms™")
                        textFormat: Label.MarkdownText
                        horizontalAlignment: Label.AlignHCenter
                        verticalAlignment: Label.AlignVCenter
                        wrapMode: Label.WordWrap
                        elide: Label.ElideRight
                    }
                    onClicked: {
                        appPageStack.push("qrc:/ConnectQRCodePage.qml")
                    }
                }

                Item { height: 1 }

                C.HorizontalDivider {

                    Layout.fillWidth: true

                }

                ToolButton {
                    Layout.alignment: Qt.AlignHCenter
                    font.pixelSize: App.settings.font14
                    icon.source: "qrc:/icons/help.svg"
                    icon.width: C.Style.toolButtonSize
                    icon.height: C.Style.toolButtonSize
                    text: qsTr("Getting started")
                    font.capitalization: Font.AllUppercase
                    visible: App.config.helpUrl !== ""
                    onClicked: {
                        Qt.openUrlExternally("https://rillaforms.com")
                    }
                }
            }
        }
    }
}
