import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import QtGraphicalEffects 1.0

ContentPage {
    id: page

    header: PageHeader {
        text: qsTr("Settings")
    }

    FormSettingsListView {
        anchors.fill: parent
    }
}
