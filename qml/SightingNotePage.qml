import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import CyberTracker as C

C.ContentPage {
    id: page

    property string projectUid
    property string sightingUid
    property alias note: sightingNote.text

    header: C.PageHeader {
        text: qsTr("Sighting note")
        menuIcon: "qrc:/icons/check.svg"
        menuVisible: true
        onMenuClicked: {
            Qt.inputMethod.hide()
            App.setSightingNote(projectUid, sightingUid, sightingNote.text)
            popPageImmediate(page)
        }
    }

    TextArea {
        id: sightingNote

        anchors.fill: parent
        anchors.margins: 10
        focus: true
        font.pixelSize: App.settings.font17
        wrapMode: Text.Wrap
        placeholderTextColor: Material.color(Material.Grey)
        inputMethodHints: Qt.ImhNoPredictiveText
    }
}

