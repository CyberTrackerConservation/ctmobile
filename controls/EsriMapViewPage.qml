import QtQuick
import QtQuick.Controls.Material
import CyberTracker as C

ContentPage {
    id: page

    property alias footerText: pageFooter.text

    property alias mode: mapView.mode
    property alias flagLocation: mapView.flagLocation
    property alias hideDataLayers: mapView.hideDataLayers
    property alias centerPoint: mapView.centerPoint
    property alias initialExtent: mapView.initialExtent
    property alias initialPanMode: mapView.initialPanMode
    property alias autoPanMode: mapView.autoPanMode
    property alias recordUid: mapView.recordUid
    property alias fieldUid: mapView.fieldUid
    property alias fieldValue: mapView.fieldValue

    header: PageHeader {
        id: pageHeader
        text: page.title
    }

    footer: Label {
        id: pageFooter
        height: pageHeader.height
        font.pixelSize: App.settings.font16
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        visible: text !== ""
    }

    EsriMapView {
        id: mapView
        anchors.fill: parent
    }
}
