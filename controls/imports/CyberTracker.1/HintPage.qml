import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtWebView 1.1
import CyberTracker 1.0 as C

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
        anchors.fill: parent
    }

    Component {
        id: webViewComponent

        C.WebView {
            id: webView
            anchors.fill: parent
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

            ColumnLayout {
                id: column
                width: parent.width - 32
                x: 16
                spacing: 16

                Item {
                    height: 1
                }

                Label {
                    Layout.fillWidth: true
                    text: internal.text
                    font.pixelSize: App.settings.font14
                    wrapMode: Label.WordWrap
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
                    visible: internal.iconLocation === "BEFORE"
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }
}
