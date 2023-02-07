import QtQml 2.12
import QtQuick 2.12
import Esri.ArcGISRuntime 100.15 as Esri

Esri.Basemap {
    id: basemap

    property var layerMap: {
        "BasemapNone": basemapNone,
        "BasemapOpenStreetMap": basemapOpenStreetMap,
        "BasemapOpenTopoMap": basemapOpenTopoMap,
        "BasemapHumanitarian": basemapHumanitarian,
        "BasemapNatGeo": basemapNatGeo,
        "BasemapTopographic": basemapTopographic,
        "BasemapStreets": basemapStreets,
        "BasemapStreetsVector": basemapStreetsVector,
        "BasemapStreetsNightVector": basemapStreetsNightVector,
        "BasemapImagery": basemapImagery,
        "BasemapDarkGrayCanvas": basemapDarkGrayCanvas,
        "BasemapDarkGrayCanvasVector": basemapDarkGrayCanvasVector,
        "BasemapLightGrayCanvas": basemapLightGrayCanvas,
        "BasemapLightGrayCanvasVector": basemapLightGrayCanvasVector,
        "BasemapNavigationVector": basemapNavigationVector,
        "BasemapOceans": basemapOceans
    }

    function setLayerId(layerId) {
        basemap.baseLayers.clear()
        basemap.baseLayers.append(layerMap[layerId].createObject(null))
    }

    Component {
        id: basemapNone
        Esri.ImageTiledLayer {
            Esri.SpatialReference { id: webMercator; wkid: 3857}
            fullExtent: Esri.Envelope { xMin: -20037508.34; yMin: -20037508.34; xMax: +20037508.34; yMax: +20037508.34; spatialReference: webMercator }
            noDataTileBehavior: Esri.Enums.NoDataTileBehaviorBlank
            tileInfo: createTileInfo()
            tileCallback: function(level, row, column) {
                return ""
            }
            function createTileInfo() {
                return Esri.ArcGISRuntimeEnvironment.createObject("TileInfo", { origin: createOrigin(), dpi: 96, tileWidth: 256, tileHeight: 256, spatialReference: webMercator, levelsOfDetail: createLods() })
            }
            function createLods() {
                let lods = []
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 0,  resolution: 156543.03390624999884,    scale: 591657527.50934994221}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 1,  resolution: 78271.516953124999418,    scale: 295828763.75467497110}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 2,  resolution: 39135.758476562499709,    scale: 147914381.87733748555}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 3,  resolution: 19567.879238281249854,    scale: 73957190.938668742776}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 4,  resolution: 9783.9396191406249272,    scale: 36978595.469334371388}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 5,  resolution: 4891.9698095703124636,    scale: 18489297.734667185694}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 6,  resolution: 2445.9849047851562318,    scale: 9244648.8673335928470}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 7,  resolution: 1222.9924523925781159,    scale: 4622324.4336667964235}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 8,  resolution: 611.49622619628905795,    scale: 2311162.2168333982117}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 9,  resolution: 305.74811309814452898,    scale: 1155581.1084166991059}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 10, resolution: 152.87405654907226449,    scale: 577790.55420834955294}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 11, resolution: 76.437028274536132244,    scale: 288895.27710417477647}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 12, resolution: 38.218514137268066122,    scale: 144447.63855208738823}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 13, resolution: 19.109257068634033061,    scale: 72223.819276043694117}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 14, resolution: 9.5546285343170165305,    scale: 36111.909638021847059}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 15, resolution: 4.7773142671585082653,    scale: 18055.954819010923529}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 16, resolution: 2.3886571335792541326,    scale: 9027.9774095054617646}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 17, resolution: 1.1943285667896270663,    scale: 4513.9887047527308823}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 18, resolution: 0.59716428339481353316,   scale: 2256.9943523763654412}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 19, resolution: 0.29858214169740676658,   scale: 1128.4971761881827206}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 20, resolution: 0.14929107084870338329,   scale: 564.24858809409136029}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 21, resolution: 0.074645535424351691645,  scale: 282.12429404704568014}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 22, resolution: 0.037322767712175845822,  scale: 141.06214702352284007}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 23, resolution: 0.018661383856087922911,  scale: 70.531073511761420036}))
                lods.push(Esri.ArcGISRuntimeEnvironment.createObject("LevelOfDetail", {level: 24, resolution: 0.0093306919280439614556, scale: 35.265536755880710018}))
                return lods
            }
            function createOrigin() {
                return Esri.ArcGISRuntimeEnvironment.createObject("Point", { x: -20037508.34, y: +20037508.34, spatialReference: webMercator })
            }
        }
    }

    Component {
        id: basemapOpenStreetMap
        Esri.WebTiledLayer {
            templateUrl: "http://{subDomain}.tile.openstreetmap.org/{level}/{col}/{row}.png"
            subDomains: ["a", "b", "c"]
        }
    }

    Component {
        id: basemapOpenTopoMap
        Esri.WebTiledLayer {
            templateUrl: "http://{subDomain}.tile.opentopomap.org/{level}/{col}/{row}.png"
            subDomains: ["a", "b", "c"]
        }
    }

    Component {
        id: basemapHumanitarian
        Esri.WebTiledLayer {
            templateUrl: "http://{subDomain}.tile.openstreetmap.fr/hot/{level}/{col}/{row}.png"
            subDomains: ["a"]
        }
    }

    Component {
        id: basemapNatGeo
        Esri.ArcGISTiledLayer {
            url: "http://services.arcgisonline.com/ArcGIS/rest/services/NatGeo_World_Map/MapServer"
        }
    }

    Component {
        id: basemapTopographic
        Esri.ArcGISTiledLayer {
            url: "http://server.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer"
        }
    }

    Component {
        id: basemapStreets
        Esri.ArcGISTiledLayer {
            url: "http://server.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer"
        }
    }

    Component {
        id: basemapImagery
        Esri.ArcGISTiledLayer {
            url: "http://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer"
        }
    }

    Component {
        id: basemapLightGrayCanvas
        Esri.ArcGISTiledLayer {
            url: "http://services.arcgisonline.com/ArcGIS/rest/services/Canvas/World_Light_Gray_Base/MapServer"
        }
    }

    Component {
        id: basemapDarkGrayCanvas
        Esri.ArcGISTiledLayer {
            url: "http://services.arcgisonline.com/ArcGIS/rest/services/Canvas/World_Dark_Gray_Base/MapServer"
        }
    }

    Component {
        id: basemapOceans
        Esri.ArcGISTiledLayer {
            url: "http://services.arcgisonline.com/arcgis/rest/services/Ocean/World_Ocean_Base/MapServer"
        }
    }

    Component {
        id: basemapStreetsVector
        Esri.ArcGISVectorTiledLayer {
            url: "http://www.arcgis.com/sharing/rest/content/items/de26a3cf4cc9451298ea173c4b324736/resources/styles/root.json"
        }
    }

    Component {
        id: basemapStreetsNightVector
        Esri.ArcGISVectorTiledLayer {
            url: "http://www.arcgis.com/sharing/rest/content/items/86f556a2d1fd468181855a35e344567f/resources/styles/root.json"
        }
    }

    Component {
        id: basemapLightGrayCanvasVector
        Esri.ArcGISVectorTiledLayer {
            url: "http://www.arcgis.com/sharing/rest/content/items/291da5eab3a0412593b66d384379f89f/resources/styles/root.json"
        }
    }

    Component {
        id: basemapDarkGrayCanvasVector
        Esri.ArcGISVectorTiledLayer {
            url: "http://www.arcgis.com/sharing/rest/content/items/5e9b3685f4c24d8781073dd928ebda50/resources/styles/root.json"
        }
    }

    Component {
        id: basemapNavigationVector
        Esri.ArcGISVectorTiledLayer {
            url: "http://www.arcgis.com/sharing/rest/content/items/63c47b7177f946b49902c24129b87252/resources/styles/root.json"
        }
    }
}
