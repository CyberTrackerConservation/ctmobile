import QtQml 2.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12
import Esri.ArcGISRuntime 100.15 as Esri
import QtGraphicalEffects 1.0
import QtPositioning 5.12
import CyberTracker 1.0 as C

ContentPage {
    id: page
    property alias recordUid: fieldBinding.recordUid
    property alias fieldUid: fieldBinding.fieldUid

    C.FieldBinding {
        id: fieldBinding
    }

    // Footer.
    footer: RowLayout {
        spacing: 0

        C.FooterButton {
            icon.source: "qrc:/icons/layers_outline.svg"
            text: qsTr("Layers")
            onClicked: {
                form.pushPage("qrc:/MapLayersPage.qml", {})
            }
        }

        C.FooterButton {
            checkable: true
            checked: mapView.locationDisplay.autoPanMode === Esri.Enums.LocationDisplayAutoPanModeRecenter
            icon.source: checked ? "qrc:/icons/gps_fixed.svg" : "qrc:/icons/gps_not_fixed.svg"
            text: qsTr("Follow")
            onClicked: {
                mapView.locationDisplay.autoPanMode = (checked ? Esri.Enums.LocationDisplayAutoPanModeRecenter : Esri.Enums.LocationDisplayAutoPanModeOff)
            }
        }

        C.FooterButton {
            id: buttonReset
            enabled: !fieldBinding.isEmpty && page.state === "default"
            icon.source: "qrc:/icons/delete_outline.svg"
            text: qsTr("Reset")
            onClicked: popupConfirmReset.open()
        }

        C.FooterButton {
            id: buttonRemoveLast
            icon.source: "qrc:/icons/map_marker_remove_outline.svg"
            text: qsTr("Remove")
            enabled: !fieldBinding.isEmpty && (fieldBinding.value.length > 0)
            onClicked: removeLastPoint()
        }

        C.FooterButton {
            id: buttonAdd
            onClicked: {
                if (page.state === "default") {
                    popupMethod.open()
                } else {
                    page.state = "default"
                }
            }
        }
    }

    C.ConfirmPopup {
        id: popupConfirmReset
        text: qsTr("Clear current points?")
        confirmText: qsTr("Yes, clear them")
        onConfirmed: {
            fieldBinding.resetValue()
            page.state = "default"
            rebuild()
        }
    }

    C.OptionsPopup {
        id: popupMethod

        icon: "qrc:/icons/map_marker_plus_outline.svg"

        model: [
            { text: qsTr("Tap"), icon: "qrc:/icons/touch_outline.svg" },
            { text: qsTr("GPS"), icon: "qrc:/icons/map_marker_account_outline.svg" },
            { text: qsTr("Auto GPS"), icon: "qrc:/icons/map_marker_multiple_outline.svg" }
        ]

        onClicked: function (index) {
            switch (index) {
            case 0:
                page.state = "tap"
                break;

            case 1:
                page.state = "manual"
                break;

            case 2:
                page.state = "auto"
                break;
            }
        }
    }

    EsriMapView {
        id: mapView
        anchors.fill: parent
        hideDataLayers: true
        forceLocationActive: page.state === "auto"

        Esri.GraphicsOverlay {
            id: areaOverlay

            property var symbolJson: ({
                symbolType: "SimpleFillSymbol",
                color: [0, 0, 255, 128],
                style: "esriSFSSolid",
                type: "esriSFS",
                outline: {
                    symbolType: "SimpleLineSymbol",
                    color: App.settings.darkTheme ? [255, 255, 255, 200] : [0, 0, 0, 200],
                    style: "esriSLSSolid",
                    type: "esriSLS",
                    width: App.scaleByFontSize(1.75)
                }
            })

            property var symbol: Esri.ArcGISRuntimeEnvironment.createObject(symbolJson.symbolType, { json: symbolJson } )
        }

        Esri.GraphicsOverlay {
            id: lineOverlay

            property var symbolJson: ({
                symbolType: "SimpleLineSymbol",
                color: [0, 0, 255, 200],
                style: "esriSLSShortDot",
                type: "esriSLS",
                width: App.scaleByFontSize(1.75)
            })

            property var symbol: Esri.ArcGISRuntimeEnvironment.createObject(symbolJson.symbolType, { json: symbolJson } )
        }

        Esri.GraphicsOverlay {
            id: pointsOverlay

            property var symbolJson: ({
                symbolType: "SimpleMarkerSymbol",
                color: [0, 0, 255, 128],
                style: "esriSMSCircle",
                type: "esriSMS",
                size: App.scaleByFontSize(12),
                outline: {
                    symbolType: "SimpleLineSymbol",
                    color: App.settings.darkTheme ? [255, 255, 255, 200] : [0, 0, 0, 200],
                    style: "esriSLSSolid",
                    type: "esriSLS",
                    width: App.scaleByFontSize(1.75)
                },
                angle: 0.0,
                yoffset: 0.0
            })

            property var symbol: Esri.ArcGISRuntimeEnvironment.createObject(symbolJson.symbolType, { json: symbolJson } )
        }

        onMouseClicked: {
            if (page.state === "tap") {
                let point = Esri.GeometryEngine.project(mapView.screenToLocation(mouse.x, mouse.y), Esri.Factory.SpatialReference.createWgs84())
                if (point) {
                    addPoint(point.x, point.y, 0, 0)
                }
            }
        }

        Connections {
            target: App

            function onLastLocationChanged() {
                if (page.state === "auto") {
                    if (!App.lastLocationAccurate) {
                        showToast(App.locationAccuracyText)
                        return
                    }

                    addPoint(App.lastLocation.x, App.lastLocation.y, App.lastLocation.z, App.lastLocation.accuracy)
                }
            }
        }

        Item {
            id: buttonAddPoint
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 24
            width: manualButton.implicitWidth + 16
            height: manualButton.implicitHeight

            Rectangle {
                anchors.fill: manualButton
                color: colorContent || Style.colorContent
                opacity: 0.5
                border.color: Material.foreground
                border.width: 1
            }

            ToolButton {
                id: manualButton
                anchors.fill: parent
                font.pixelSize: App.settings.font13
                text: qsTr("Add point")
                icon.source: "qrc:/icons/map_marker_account_outline.svg"
                icon.width: Style.toolButtonSize
                icon.height: Style.toolButtonSize
                font.capitalization: Font.AllUppercase
                enabled: App.lastLocationRecent
                onClicked: {
                    if (!App.lastLocationAccurate) {
                        showToast(App.locationAccuracyText)
                        return
                    }

                    addPoint(App.lastLocation.x, App.lastLocation.y, App.lastLocation.z, App.lastLocation.accuracy)
                }
            }
        }
    }

    state: ""
    states: [
        State {
            name: "default"
            StateChangeScript {
                script: {
                    buttonAdd.icon.source = "qrc:/icons/map_marker_plus_outline.svg"
                    buttonAdd.text = qsTr("Start")
                    buttonAddPoint.visible = false
                }
            }
        },
        State {
            name: "tap"
            StateChangeScript {
                script: {
                    popupMethod.close()
                    mapView.locationDisplay.autoPanMode = Esri.Enums.LocationDisplayAutoPanModeOff
                    buttonAdd.icon.source = "qrc:/icons/pause.svg"
                    buttonAdd.text = qsTr("Pause")
                    buttonAddPoint.visible = false
                }
            }
        },
        State {
            name: "manual"
            StateChangeScript {
                script: {
                    popupMethod.close()
                    mapView.locationDisplay.autoPanMode = Esri.Enums.LocationDisplayAutoPanModeRecenter
                    buttonAdd.icon.source = "qrc:/icons/pause.svg"
                    buttonAdd.text = qsTr("Pause")
                    buttonAddPoint.visible = true
                }
            }
        },
        State {
            name: "auto"
            StateChangeScript {
                script: {
                    popupMethod.close()
                    mapView.locationDisplay.autoPanMode = Esri.Enums.LocationDisplayAutoPanModeRecenter
                    buttonAdd.icon.source = "qrc:/icons/pause.svg"
                    buttonAdd.text = qsTr("Pause")
                    buttonAddPoint.visible = false
                }
            }
        }
    ]

    function removeLastPoint() {
        let value = fieldBinding.getValue([])
        value.pop()
        fieldBinding.setValue(value)
        form.saveState()
        rebuild()
    }

    Esri.MultipointBuilder {
        id: pointBuilder
        spatialReference: Esri.SpatialReference { wkid: 4326 }
    }

    Esri.PolylineBuilder {
        id: polylineBuilder
        spatialReference: Esri.SpatialReference { wkid: 4326 }
    }

    Esri.PolygonBuilder {
        id: polygonBuilder
        spatialReference: Esri.SpatialReference { wkid: 4326 }
    }

    function addPoint(x, y, z, a) {
        let points = fieldBinding.getValue([])
        let point = [ x, y, z, a ]
        points.push(point)
        fieldBinding.setValue(points)
        form.saveState()

        if (fieldBinding.fieldType === "AreaField") {
            rebuild()
            return
        }

        // Point.
        pointBuilder.geometry = null
        pointBuilder.points.addPointXY(x, y)
        let graphic = Esri.ArcGISRuntimeEnvironment.createObject("Graphic")
        graphic.geometry = pointBuilder.geometry
        graphic.symbol = pointsOverlay.symbol
        pointsOverlay.graphics.append(graphic)

        // Line.
        if (points.length > 1) {
            polylineBuilder.geometry = null
            polylineBuilder.addPointXY(point[0], point[1])

            let lastPoint = points[points.length - 2]
            polylineBuilder.addPointXY(lastPoint[0], lastPoint[1])
            let graphic = Esri.ArcGISRuntimeEnvironment.createObject("Graphic")
            graphic.geometry = polylineBuilder.geometry
            graphic.symbol = lineOverlay.symbol
            lineOverlay.graphics.append(graphic)
        }
    }

    function rebuild() {
        let points = fieldBinding.getValue([])

        pointsOverlay.graphics.clear()
        lineOverlay.graphics.clear()
        areaOverlay.graphics.clear()

        if (points.length === 0) {
            return
        }

        pointBuilder.geometry = null
        polylineBuilder.geometry = null
        polygonBuilder.geometry = null

        for (let i = 0; i < points.length; i++) {
            let point = points[i]
            pointBuilder.points.addPointXY(point[0], point[1])
            polylineBuilder.addPointXY(point[0], point[1])
            polygonBuilder.addPointXY(point[0], point[1])
        }

        let graphic = Esri.ArcGISRuntimeEnvironment.createObject("Graphic")
        graphic.geometry = pointBuilder.geometry
        graphic.symbol = pointsOverlay.symbol
        pointsOverlay.graphics.append(graphic)

        if (fieldBinding.fieldType === "LineField") {
            let graphic = Esri.ArcGISRuntimeEnvironment.createObject("Graphic")
            graphic.geometry = polylineBuilder.geometry
            graphic.symbol = lineOverlay.symbol
            lineOverlay.graphics.append(graphic)

        } else if (fieldBinding.fieldType === "AreaField") {
            let graphic = Esri.ArcGISRuntimeEnvironment.createObject("Graphic")
            graphic.geometry = polygonBuilder.geometry
            graphic.symbol = areaOverlay.symbol
            areaOverlay.graphics.append(graphic)
        }
    }

    Component.onCompleted: {
        page.state = "default"

        rebuild()

        if (fieldBinding.isEmpty) {
            mapView.initialPanMode = Esri.Enums.LocationDisplayAutoPanModeRecenter

        } else {
            let extent = Utils.computeExtent(fieldBinding.getValue([]))
            if (extent.xmin !== extent.xmax) {
                mapView.initialExtent = extent
            }
        }
    }
}
