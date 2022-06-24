import QtQuick 2.12
import QtQuick.Controls 2.12
import CyberTracker 1.0 as C

C.ContentPage {
    id: page

    property var model: []

    header: C.PageHeader {
        text: page.title
    }

    C.DetailsPane {
        anchors.fill: parent
        model: page.model
    }
}
