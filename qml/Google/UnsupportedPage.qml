import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    header: C.PageHeader {
        text: form.project.title
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width * 0.8

        C.SquareIcon {
            Layout.alignment: Qt.AlignHCenter
            source: "qrc:/icons/file_document_alert_outline.svg"
            size: C.Style.iconSize48
        }

        Item { height: App.scaleByFontSize(16) }

        Label {
            Layout.fillWidth: true
            text: qsTr("Forms containing *File upload* questions are not currently supported.")
            font.pixelSize: App.settings.font14
            wrapMode: Label.WordWrap
            textFormat: Label.MarkdownText
        }

        Item { height: App.scaleByFontSize(8) }

        Label {
            Layout.fillWidth: true
            text: qsTr("Remove the question and update the form to continue.")
            font.pixelSize: App.settings.font14
            wrapMode: Label.WordWrap
            textFormat: Label.MarkdownText
        }
    }
}
