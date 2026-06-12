import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    id: page

    property var model: []

    header: PageHeader {
        text: page.title
    }

    DetailsPane {
        anchors.fill: parent
        model: page.model
    }
}
