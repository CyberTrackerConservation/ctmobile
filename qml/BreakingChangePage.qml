import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: qsTr("Breaking change")
    }

    Flickable {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: column.height

        ColumnLayout {
            id: column
            width: parent.width * 0.8
            x: parent.width * 0.1

            Item {
                height: 16
            }

            Label {
                Layout.fillWidth: true
                text: "Install CyberTracker Desktop 3.520 or later and re-install apps. The install location is: https://cybertrackerwiki.org/classic/download/."
                font.pixelSize: App.settings.font14
                wrapMode: Label.WordWrap
            }

            Item {
                height: 16
            }

            Label {
                Layout.fillWidth: true
                text: "This is a one-time change due to a policy update on the Google Play Store."
                font.pixelSize: App.settings.font14
                wrapMode: Label.WordWrap
            }

            Item {
                height: 16
            }

            Label {
                Layout.fillWidth: true
                text: "Outstanding data will not be lost, it will be downloaded as usual."
                font.pixelSize: App.settings.font14
                wrapMode: Label.WordWrap
            }
        }
    }
}
