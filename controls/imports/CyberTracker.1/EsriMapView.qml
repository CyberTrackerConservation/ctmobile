import QtQml 2.12
import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import Esri.ArcGISRuntime 100.14 as Esri
import QtQuick.Controls.Material 2.12
import QtLocation 5.12
import QtSensors 5.12
import QtPositioning 5.12
import CyberTracker 1.0 as C

Esri.MapView {
    id: mapView

    property bool flagMode: false
    property var hideDataLayers: false
    property var flagLocation: undefined
    property var centerPoint: undefined
    property bool forceLocationActive: false
    property var initialExtent: undefined
    property var initialPanMode: undefined

    rotationByPinchingEnabled: true
    magnifierEnabled: true

    signal inspectClicked(var inspectList)

    QtObject {
        id: internal
        property bool loadComplete: false
        property var projectLayersLookup: ({})
    }

    Component.onCompleted: {
        internal.loadComplete = true
        mapView.rebuild()
    }

    onViewpointChanged: {
        if (internal.loadComplete) {
            let point = Esri.GeometryEngine.project(mapView.screenToLocation(width / 2, height / 2), Esri.Factory.SpatialReference.createWgs84())
            if (!point) {
                return
            }

            centerPoint = point.json
            point.x // Weird hack to work around a race inside ESRI code somewhere

            if (!flagMode) {
                let viewpoint = Esri.ArcGISRuntimeEnvironment.createObject("ViewpointCenter", { center: point, targetScale: mapView.mapScale })
                App.settings.mapViewpointCenter = viewpoint.json
            }
        }
    }

    C.MapLayerListModel {
        id: layerListModel
        onChanged: {
            if (internal.loadComplete) {
                mapView.rebuild()
            }
        }
    }

    Connections {
        target: App

        function onZoomToMapLayer(layerId) {
            let layerIndex = internal.projectLayersLookup[layerId]
            if (layerIndex !== undefined) {
                let layerObj = basemapBuilder.baseLayers.get(layerIndex)

                if (layerObj.mbMaxScale) {
                    let viewpoint = Esri.ArcGISRuntimeEnvironment.createObject("ViewpointCenter", { center: layerObj.fullExtent.center, targetScale: layerObj.mbMaxScale })
                    mapView.setViewpoint(viewpoint);
                } else {
                    mapView.setViewpointGeometry(layerObj.fullExtent)
                }
            }
        }

        function onZoomToMapOverlay(layerId) {
            let layer = layerListModel.get(layerListModel.layerIdToIndex(layerId))
            if (layer && layer.extent && layer.extent.xmin !== layer.extent.xmax) {
                let envelope = Esri.ArcGISRuntimeEnvironment.createObject("Envelope", { "json": layer.extent } )
                mapView.setViewpointGeometryAndPadding(envelope, 64)
            }
        }
    }

    Connections {
        target: form

        function onLocationTrack(locationMap, locationUid) {
            if (!flagMode && locationOverlay.symbol !== undefined) {
                mapView.appendLinesToLayer(locationOverlay, form.buildTrackMapLayer("Tracks", locationOverlay.symbol, true))
            }
        }
    }

    CircleButton {
        x: parent.width - implicitWidth
        y: parent.height - implicitHeight * 3 - 20
        buttonOpacity: 0.75
        buttonIcon.source: "qrc:/icons/plus.svg"
        onClicked: mapView.setViewpointScale(mapView.mapScale / 2)
    }

    CircleButton {
        x: parent.width - implicitWidth
        y: parent.height - implicitHeight * 2 - 24
        buttonOpacity: 0.75
        buttonIcon.source: "qrc:/icons/minus.svg"
        onClicked: mapView.setViewpointScale(mapView.mapScale * 2)
    }

    CircleButton {
        x: parent.width - implicitWidth
        y: parent.height - implicitHeight - 20
        buttonOpacity: 0.75
        buttonIcon.source: "qrc:/icons/north_direction.svg"
        rotation: -mapView.mapRotation
        //visible: (mapView.mapRotation > 0.001) && (mapView.mapRotation < 359.998)
        onClicked: {
            let prevMode = mapView.locationDisplay.autoPanMode
            mapView.setViewpointRotation(0)
            mapView.locationDisplay.autoPanMode = prevMode
        }
    }

    Esri.Map {
        id: map

        EsriBasemapBuilder {
            id: basemapBuilder
        }

        onLoadStatusChanged: {
            if (loadStatus === Esri.Enums.LoadStatusLoaded) {
                // Viewpoint.
                let viewpointJson = App.settings.mapViewpointCenter
                let targetScale = viewpointJson.scale === undefined ? 50000 : viewpointJson.scale

                if (initialExtent !== undefined) {
                    let envelope = Esri.ArcGISRuntimeEnvironment.createObject("Envelope", { "json": initialExtent } )
                    mapView.setViewpointGeometryAndPadding(envelope, 64)

                } else if (flagMode && flagLocation !== undefined) {
                    let point = Esri.ArcGISRuntimeEnvironment.createObject("Point", { "json": flagLocation } )
                    let viewpoint = Esri.ArcGISRuntimeEnvironment.createObject("ViewpointCenter", { center: point, targetScale: targetScale})
                    mapView.setViewpoint(viewpoint);

                } else if (flagMode && App.lastLocation.x !== undefined) {
                    let lastLocation = App.lastLocation
                    lastLocation.spatialReference = { wkid: 4326 }
                    let point = Esri.ArcGISRuntimeEnvironment.createObject("Point", { "json": lastLocation } );
                    let viewpoint = Esri.ArcGISRuntimeEnvironment.createObject("ViewpointCenter", { center: point, targetScale: targetScale})
                    mapView.setViewpoint(viewpoint);

                } else if (viewpointJson.scale !== undefined) {
                    let viewpoint = Esri.ArcGISRuntimeEnvironment.createObject("ViewpointCenter", { "json": viewpointJson })
                    mapView.setViewpoint(viewpoint)
                }

                // Pan mode.
                if (initialPanMode !== undefined) {
                    mapView.locationDisplay.autoPanMode = initialPanMode

                } else if (flagMode) {
                    if (flagLocation === undefined) {
                        mapView.locationDisplay.autoPanMode = Esri.Enums.LocationDisplayAutoPanModeRecenter
                    } else {
                        mapView.locationDisplay.autoPanMode = Esri.Enums.LocationDisplayAutoPanModeOff
                    }

                } else if (initialExtent !== undefined) {
                    mapView.locationDisplay.autoPanMode = Esri.Enums.LocationDisplayAutoPanModeOff

                } else {
                    mapView.locationDisplay.autoPanMode = App.settings.mapPanMode
                }

                mapView.locationDisplay.start()
            }
        }
    }

    Esri.GraphicsOverlay {
        id: gotoLinesOverlay
    }

    Esri.GraphicsOverlay {
        id: gotoPointsOverlay
        overlayId: "gotoPoints"
    }

    Esri.GraphicsOverlay {
        id: locationOverlay
        property var symbol
    }

    Esri.GraphicsOverlay {
        id: sightingOverlay
        overlayId: "sighting"
    }

    Esri.GraphicsOverlay {
        id: gotoTargetOverlay

        Connections {
            target: App

            function onLastLocationChanged() {
                if (!flagMode) {
                    mapView.rebuildGotoTarget()
                }
            }
        }
    }

    Esri.GraphicsOverlay {
        id: flagOverlay
        overlayId: "flagOverlay"
    }

    onFlagLocationChanged: {
        rebuildFlagOverlay()
    }

    onMouseClicked: {
        if (flagMode) {
            return
        }

        let tolerance = 22
        let returnPopupsOnly = false
        let maximumResults = 1000
        mapView.identifyGraphicsOverlaysWithMaxResults(mouse.x, mouse.y, tolerance, returnPopupsOnly, maximumResults)
    }

    onIdentifyGraphicsOverlaysStatusChanged: {
        if (identifyGraphicsOverlaysStatus !== Esri.Enums.TaskStatusCompleted) {
            return
        }

        let sightingList = []
        let gotoPointList = []

        for (let i = 0; i < identifyGraphicsOverlaysResults.length; i++) {
            let result = identifyGraphicsOverlaysResults[i]

            for (let j = 0; j < result.graphics.length; j++) {
                let attributes = result.graphics[j].attributes.attributesJson
                let geometry = result.graphics[j].geometry.json
                let overlayId = result.graphics[j].graphicsOverlay.overlayId
                let path = []

                switch (overlayId) {
                case sightingOverlay.overlayId:
                    path.push([geometry.x, geometry.y])
                    sightingList.push( {
                        overlayId: overlayId,
                        createTime: attributes.createTime,
                        stateSpace: attributes.stateSpace,
                        sightingUid: attributes.sightingUid,
                        source: "Sighting",
                        name: qsTr("Sighting"),
                        path: path,
                        pathIndex: 0
                    } )
                    break

                case gotoPointsOverlay.overlayId:
                    path = App.gotoManager.findPath(attributes.pathId)
                    gotoPointList.push( {
                        overlayId: overlayId,
                        source: attributes.source,
                        name: attributes.name,
                        path: path,
                        pathIndex: attributes.pathIndex
                    } )
                    break
                }
            }
        }

        if (sightingList.length !== 0) {
            sightingList.sort(function(a, b) { return a.createTime - b.createTime })
        }

        let inspectList = []
        inspectList = inspectList.concat(sightingList)
        inspectList = inspectList.concat(gotoPointList)

        if (inspectList.length !== 0) {
            mapView.inspectClicked(inspectList)
        }
    }

    locationDisplay {
        positionSource: PositionSource {
            active: mapView.forceLocationActive || Qt.application.active
            name: App.positionInfoSourceName
        }

        compass: Compass {
            id: compass
            active: Qt.application.active
        }
        onAutoPanModeChanged: {
            if (!flagMode) {
                App.settings.mapPanMode = mapView.locationDisplay.autoPanMode
            }
        }
    }

    function appendPointsToLayer(overlay, layer) {
        let geometry, symbol, graphic

        for (let j = 0; j < layer.geometry.length; j++) {
            let point = layer.geometry[j]
            geometry = Esri.ArcGISRuntimeEnvironment.createObject("Point", { "json": point } )
            symbol = Esri.ArcGISRuntimeEnvironment.createObject(point.symbol.symbolType, { "json": point.symbol } )

            graphic = Esri.ArcGISRuntimeEnvironment.createObject("Graphic")
            graphic.geometry = geometry
            graphic.symbol = symbol

            graphic.attributes.attributesJson = {
                source: layer.source,
                name: point.name,
                pathIndex: point.pathIndex,
                pathId: point.pathId,
                createTime: point.createTime,
                stateSpace: point.stateSpace,
                sightingUid: point.sightingUid
            }

            overlay.graphics.append(graphic)
        }
    }

    function appendLinesToLayer(overlay, layer) {
        if (layer.geometry.paths.length === 0) return

        let unitOfMeasurement, geometry, graphic
        unitOfMeasurement = Esri.ArcGISRuntimeEnvironment.createObject("LinearUnit", { linearUnitId: Esri.Enums.LinearUnitIdKilometers } );
        geometry = Esri.ArcGISRuntimeEnvironment.createObject("Polyline", { "json": layer.geometry } )
        geometry = Esri.GeometryEngine.densifyGeodetic(geometry, 1, unitOfMeasurement, Esri.Enums.GeodeticCurveTypeGeodesic);
        graphic = Esri.ArcGISRuntimeEnvironment.createObject("Graphic")
        graphic.geometry = geometry
        graphic.symbol = Esri.ArcGISRuntimeEnvironment.createObject(layer.symbol.symbolType, { "json": layer.symbol } )
        overlay.graphics.append(graphic)
    }

    function rebuild() {
        rebuildBasemap()
        rebuildProjectLayers()
        if (!flagMode) {
            rebuildGotoLayers()
            rebuildFormLayers()
            rebuildGotoTarget()
        } else {
            rebuildFlagOverlay()
        }
    }

    function rebuildBasemap() {
        for (let i = 0; i < layerListModel.count; i++) {
            let layer = layerListModel.get(i)
            if (layer.type === "basemap" && layer.checked) {
                basemapBuilder.setLayerId(layer.id)
                break
            }
        }
    }

    function rebuildProjectLayers() {
        internal.projectLayersLookup = ({})

        for (let i = layerListModel.count - 1; i >= 0; i--) {
            let layer = layerListModel.get(i)
            if (layer.type !== "project" || !layer.checked) {
                continue
            }

            let layerObj

            switch (layer.format) {
            case "tpk":
                layerObj = Esri.ArcGISRuntimeEnvironment.createObject("ArcGISTiledLayer", {url: layer.id})
                break

            case "vtpk":
                layerObj = Esri.ArcGISRuntimeEnvironment.createObject("ArcGISVectorTiledLayer", {url: layer.id})
                break

            case "img":
            case "I12":
            case "dt0":
            case "dt1":
            case "dt2":
            case "tc2":
            case "geotiff":
            case "tif":
            case "tiff":
            case "hr1":
            case "jpg":
            case "jpeg":
            case "jp2":
            case "ntf":
            case "png":
            case "i21":
                const raster = Esri.ArcGISRuntimeEnvironment.createObject("Raster", { path: "file:///" + layer.id })
                layerObj = Esri.ArcGISRuntimeEnvironment.createObject("RasterLayer", { raster: raster })
                break

            case "mbtiles":
                const layerQml = MBTilesReader.getQml(layer.id)
                layerObj = Qt.createQmlObject(layerQml, mapView, layer.id)
                break

            case "wms":
                layerObj = Esri.ArcGISRuntimeEnvironment.createObject("WmsLayer", {url: layer.url, layerNames: [layer.name]})
                break

            case "shp":
                const featureTable = Esri.ArcGISRuntimeEnvironment.createObject("ShapefileFeatureTable", { path: "file:///" + layer.id })
                layerObj = Esri.ArcGISRuntimeEnvironment.createObject("FeatureLayer", { featureTable: featureTable })
                break

            case "kml":
                const kmlDataset = Esri.ArcGISRuntimeEnvironment.createObject("KmlDataset", { url: "file:///" + layer.id })
                layerObj = Esri.ArcGISRuntimeEnvironment.createObject("KmlLayer", { dataset: kmlDataset })
                break

            default:
                console.log("Unknown project layer format: " + layer.format)
                return
            }

            internal.projectLayersLookup[layer.id] = basemapBuilder.baseLayers.count

            layerObj.minScale = Number.MAX_VALUE
            layerObj.maxScale = Number.MIN_VALUE
            basemapBuilder.baseLayers.append(layerObj)
        }
    }

    function rebuildGotoLayers() {
        if (hideDataLayers) {
            return
        }

        gotoPointsOverlay.graphics.clear()
        gotoLinesOverlay.graphics.clear()

        for (let i = layerListModel.count - 1; i >= 0; i--) {
            let layer = layerListModel.get(i)
            if (layer.type !== "goto" || !layer.checked) {
                continue
            }

            if (layer.points !== undefined) {
                appendPointsToLayer(gotoPointsOverlay, layer.points)
            }

            if (layer.polylines !== undefined) {
                for (let j = 0; j < layer.polylines.length; j++) {
                    appendLinesToLayer(gotoLinesOverlay, layer.polylines[j])
                }
            }
        }
    }

    function rebuildFormLayers() {
        if (hideDataLayers) {
            return
        }

        sightingOverlay.graphics.clear()
        locationOverlay.graphics.clear()

        for (let i = layerListModel.count - 1; i >= 0; i--) {
            let layer = layerListModel.get(i)
            if (layer.type !== "form" || !layer.checked) {
                continue
            }

            switch (layer.geometryType) {
            case "Points":
                appendPointsToLayer(sightingOverlay, layer)
                break

            case "Polyline":
                locationOverlay.symbol = layer.symbol
                appendLinesToLayer(locationOverlay, layer)
                break
            }
        }
    }

    function rebuildGotoTarget() {
        if (hideDataLayers) {
            return
        }

        if (App.gotoManager.gotoPath.length === 0) {
            gotoTargetOverlay.graphics.clear()
            return
        }

        let location = App.gotoManager.gotoLocation

        let polyline = {
            geometry: {
                paths: [[[location.x, location.y],
                         [App.lastLocation.x, App.lastLocation.y]]],
                spatialReference: { wkid: 4326 }
            },
            geometryType: "Polyline",
            symbol: {
                color: [255, 0, 0, 200],
                style: "esriSLSDot",
                symbolType: "SimpleLineSymbol",
                type: "esriSLS",
                width: 1.75
            },
        }

        appendLinesToLayer(gotoTargetOverlay, polyline)

        if (gotoTargetOverlay.graphics.count === 2) {
            gotoTargetOverlay.graphics.remove(0, 1)
        }
    }

    function rebuildFlagOverlay() {
        flagOverlay.graphics.clear()
        if (flagLocation === undefined) {
            return
        }

        let geometry = Esri.ArcGISRuntimeEnvironment.createObject("Point", { "json": flagLocation } )

        let symbol = {
            symbolType: "PictureMarkerSymbol",
            angle: 0.0,
            type: "esriPMS",
            url: "qrc:/icons/map_marker2.png",
            width: 24,
            height: 24,
            yoffset: 12.0
        }

        symbol = Esri.ArcGISRuntimeEnvironment.createObject(symbol.symbolType, { "json": symbol } )

        let graphic = Esri.ArcGISRuntimeEnvironment.createObject("Graphic")
        graphic.geometry = geometry
        graphic.symbol = symbol

        flagOverlay.graphics.append(graphic)
    }

    function centerOnFlagLocation() {
        let point = Esri.ArcGISRuntimeEnvironment.createObject("Point", { "json": flagLocation } )
        let viewpoint = Esri.ArcGISRuntimeEnvironment.createObject("ViewpointCenter", { center: point })
        mapView.setViewpoint(viewpoint);
    }
}
