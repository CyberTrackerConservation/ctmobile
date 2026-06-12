import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtWebView
import CyberTracker as C

ContentPage {
    id: page
    
    property string elementUid: ""
    property var link

    QtObject {
        id: internal

        property string text: form.getElementName(page.elementUid)
        property var icon: form.getElementIcon(page.elementUid)
        property string iconLocation: {
            let result = form.getElementTag(page.elementUid, "iconLocation")
            if (result === undefined || result === "") {
                result = "BEFORE"
            }
            return result
        }

        Component.onCompleted: {
            loader.sourceComponent = link.toString() !== "" ? webViewComponent : simpleViewComponent;
        }
    }

    header: PageHeader {
        text: title
    }

    Loader {
        id: loader
        clip: true
        anchors.fill: parent
    }

    Component {
        id: webViewComponent

        WebView {
            id: webView
            anchors.fill: parent
            clip: true
            Component.onCompleted: {
                webView.url = page.link
            }
        }
    }

    Component {
        id: simpleViewComponent

        Flickable {
            anchors.fill: parent
            contentWidth: parent.width
            contentHeight: column.height
            clip: true

            ColumnLayout {
                id: column
                width: parent.width - App.scaleByFontSize(32)
                x: App.scaleByFontSize(16)
                spacing: App.scaleByFontSize(16)

                Item {
                    height: 1
                }

                Label {
                    Layout.fillWidth: true
                    text: internal.text
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                    textFormat: Label.MarkdownText
                    visible: internal.iconLocation === "AFTER"
                }

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: internal.icon
                    sourceSize.width: parent.width - 16
                    fillMode: Image.PreserveAspectFit
                }

                Label {
                    Layout.fillWidth: true
                    text: internal.text
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
                    textFormat: Label.MarkdownText
                    visible: internal.iconLocation === "BEFORE"
                }

                Item {
                    Layout.fillHeight: true
                }

                FooterSpacer {}
            }
        }
    }
}
