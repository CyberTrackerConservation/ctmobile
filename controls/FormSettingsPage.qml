import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ContentPage {
    id: page

    header: PageHeader {
        text: qsTr("Settings")
    }

    FormSettingsListView {
        anchors.fill: parent
    }
}
