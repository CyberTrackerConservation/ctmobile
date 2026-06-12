import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Window
import CyberTracker as C

Item {
    id: root

    property alias mode: mapController.mode
    property alias flagLocation: mapController.flagLocation
    property alias hideDataLayers: mapController.hideDataLayers
    property alias centerPoint: mapController.centerPoint
    property alias initialExtent: mapController.initialExtent
    property alias initialPanMode: mapController.initialPanMode
    property alias autoPanMode: mapController.autoPanMode

    property alias recordUid: mapController.recordUid
    property alias fieldUid: mapController.fieldUid
    property alias fieldValue: mapController.fieldValue

    property var controller: mapController

    signal inspectClicked(var inspectList)
    signal pointClicked(var x, var y)
    signal longPress(var x, var y)

    C.MapView {
        id: mapView
        anchors.fill: parent
        focus: true

        Component.onDestruction: {
            mapController.mapView = null
        }
    }

    Slider2 {
        id: heatmapSlider
        anchors {
            left: parent.left
            right: heatmapEnable.left
            verticalCenter: heatmapEnable.verticalCenter
            leftMargin: App.scaleByFontSize(16)
            rightMargin: App.scaleByFontSize(16)
            
        }
        visible: !mapController.hideDataLayers && App.settings.heatmapActive
        from: 100
        to: 20000
        value: App.settings.heatmapMaxIntensity
        onMoved: {
            App.settings.heatmapMaxIntensity = value
        }
    }

    CircleButton {
        id: heatmapEnable
        checkable: true
        checked: App.settings.heatmapActive
        icon.source: "qrc:/icons/heatmap.svg"
        x: parent.width - implicitWidth - App.scaleByFontSize(4)
        y: toolbarInternal.height + App.scaleByFontSize(8)
        opacity: App.settings.darkTheme ? 1.0 : 0.75
        radius: 0
        visible: !mapController.hideDataLayers
        backgroundColor: App.settings.heatmapActive ? Material.accent : "transparent"

        onCheckedChanged: {
            App.settings.heatmapActive = checked
        }

        ToolBar {
            id: toolbarInternal
            visible: false
        }
    }

    CircleButton {
        x: parent.width - implicitWidth - App.scaleByFontSize(4)
        y: parent.height - implicitHeight * 4 - App.scaleByFontSize(32)
        opacity: App.settings.darkTheme ? 1.0 : 0.75
        radius: 0
        icon.source: "qrc:/icons/filter_outline.svg"
        visible: App.settings.archiveSupport && !mapController.hideDataLayers
        backgroundColor: controller.filter.active ? Material.accent : "transparent"

        onClicked: {
            pushPageImmediate(root, "qrc:/SightingFilterPage.qml", {})
        }
    }

    CircleButton {
        x: parent.width - implicitWidth - App.scaleByFontSize(4)
        y: parent.height - implicitHeight * 3 - App.scaleByFontSize(20)
        opacity: App.settings.darkTheme ? 1.0 : 0.75
        radius: 0
        icon.source: "qrc:/icons/plus.svg"
        onClicked: mapController.zoomIn()
    }

    CircleButton {
        x: parent.width - implicitWidth - App.scaleByFontSize(4)
        y: parent.height - implicitHeight * 2 - App.scaleByFontSize(20)
        opacity: App.settings.darkTheme ? 1.0 : 0.75
        radius: 0
        icon.source: "qrc:/icons/minus.svg"
        onClicked: mapController.zoomOut()
    }

    CircleButton {
        x: parent.width - implicitWidth - App.scaleByFontSize(4)
        y: parent.height - implicitHeight - App.scaleByFontSize(20)
        opacity: App.settings.darkTheme ? 1.0 : 0.75
        icon.source: "qrc:/icons/north_direction.svg"
        rotation: -mapController.rotation
        onClicked: mapController.rotation = 0
    }

    CircleButton {
        x: parent.width - implicitWidth - App.scaleByFontSize(4)
        y: parent.height - implicitHeight * 4 - App.scaleByFontSize(32)
        opacity: App.settings.darkTheme ? 1.0 : 0.75
        radius: 0
        icon.source: "qrc:/icons/layers_outline.svg"
        visible: mapController.hideDataLayers
        onClicked: {
            pushPageImmediate(page, "qrc:/MapLayersPage.qml")
        }
    }

    C.MapController {
        id: mapController
        mapView: mapView

        onRequestLocationPermission: {
            popupRequestLocation.open({ callback: () => { mapController.locationStart(); } } )
        }

        onInspectClicked: (inspectList) => {
            root.inspectClicked(inspectList)
        }

        onPointClicked: (x, y) => {
            root.pointClicked(x, y)
        }

        onLongPress: (x, y) => {
            root.longPress(x, y)
        }
    }

    function centerOnFlagLocation() {
        mapController.centerOnFlagLocation();
    }
}
