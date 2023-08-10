import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("About")
    }

    Flickable {
        anchors.fill: parent
        ScrollBar.vertical: ScrollBar { interactive: false }
        contentWidth: parent.width
        contentHeight: column.height

        ColumnLayout {
            id: column
            width: parent.width
            spacing: 0

            ItemDelegate {
                Layout.fillWidth: true
                onClicked: Qt.openUrlExternally(App.config.webPageUrl)
                contentItem: ColumnLayout {
                    Layout.fillWidth: true
                    C.SquareIcon {
                        Layout.alignment: Qt.AlignHCenter
                        source: App.config.logo
                        size: App.scaleByFontSize(80)
                    }

                    Label {
                        Layout.fillWidth: true
                        text: App.config.title
                        font.pixelSize: App.settings.font14
                        font.preferShaping: !text.includes("Cy") // Hack to fix iOS bug
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "<html><style type=\"text/css\"></style><a href=\"" + App.config.webPageUrl + "\">" + App.config.webPageTitle + "</a></html>"
                        font.pixelSize: App.settings.font12
                        font.preferShaping: !text.includes("Cy") // Hack to fix iOS bug
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        Layout.fillWidth: true
                        text: App.buildString
                        font.pixelSize: App.settings.font12
                        horizontalAlignment: Text.AlignHCenter
                    }

                    ColumnLayout {
                        width: parent.width

                        Item {
                            implicitHeight: 8
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Powered by") + " CyberTracker, ArcGIS & Qt"
                            font.pixelSize: App.settings.font12
                            font.bold: true
                            font.preferShaping: !text.includes("Cy") // Hack to fix iOS bug
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Label.WordWrap
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            C.SquareIcon {
                                source: "qrc:/app/appicon.svg"
                                size: C.Style.minRowHeight
                            }
                            C.SquareIcon {
                                source: "qrc:/app/esrilogo.svg"
                                size: C.Style.minRowHeight
                            }
                            C.SquareIcon {
                                source: "qrc:/app/qtlogo.svg"
                                size: C.Style.minRowHeight
                            }
                        }
                    }
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                onClicked: Qt.openUrlExternally(App.config.privacyStatementUrl)
                contentItem: Label {
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignHCenter
                    verticalAlignment: Label.AlignVCenter
                    text: "<html><style type=\"text/css\"></style><a href=\"" + App.config.privacyStatementUrl + "\">" + qsTr("Privacy statement") + "</a></html>"
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                onClicked: Qt.openUrlExternally(App.config.licenseUrl)
                visible: App.config.licenseUrl !== ""
                contentItem: Label {
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignHCenter
                    verticalAlignment: Label.AlignVCenter
                    text: "<html><style type=\"text/css\"></style><a href=\"" + App.config.licenseUrlUrl + "\">" + qsTr("Software license") + "</a></html>"
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                onClicked: Qt.openUrlExternally(App.config.supportForumUrl)
                contentItem: Label {
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignHCenter
                    verticalAlignment: Label.AlignVCenter
                    text: "<html><style type=\"text/css\"></style><a href=\"" + App.config.supportForumUrl + "\">" + qsTr("Support forum") + "</a></html>"
                }

                C.HorizontalDivider {}
            }

            ItemDelegate {
                Layout.fillWidth: true
                onClicked: Qt.openUrlExternally(App.config.wikiUrl)
                contentItem: Label {
                    font.pixelSize: App.settings.font14
                    horizontalAlignment: Label.AlignHCenter
                    verticalAlignment: Label.AlignVCenter
                    text: "<html><style type=\"text/css\"></style><a href=\"" + App.config.wikiUrl + "\">" + qsTr("More resources") + "</a></html>"
                }

                C.HorizontalDivider {}
            }
        }
    }
}
