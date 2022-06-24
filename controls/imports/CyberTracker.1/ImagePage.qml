import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtMultimedia 5.12
import QtQuick.Controls.Material 2.12
import CyberTracker 1.0 as C

ContentPage {
    id: page

    property alias text: pageHeader.text
    property alias source: image.source

    header: C.PageHeader {
        id: pageHeader
        text: page.title
    }

    Image {
        id: image
        cache: false
        autoTransform: true
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
    }
}
