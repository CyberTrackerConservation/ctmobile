#include "MapController.h"
#include "App.h"
#include "UtilsMap.h"
#include "MapQuickView.h"
#include "Map.h"
#include "WebTiledLayer.h"
#include "Envelope.h"
#include "EnvelopeBuilder.h"
#include "ArcGISTiledLayer.h"
#include "ArcGISVectorTiledLayer.h"
#include "PictureMarkerSymbol.h"
#include "LocationDisplay.h"
#include "LinearUnit.h"
#include "DefaultLocationDataSource.h"
#include "SimpleFillSymbol.h"
#include "SimpleMarkerSymbol.h"
#include "SimpleLineSymbol.h"
#include "RasterLayer.h"
#include "WmsLayer.h"
#include "ShapefileFeatureTable.h"
#include "FeatureLayer.h"
#include "KmlDataset.h"
#include "KmlLayer.h"
#include "GeometryEngine.h"
#include "Viewpoint.h"
#include "Graphic.h"
#include "MultipointBuilder.h"
#include "PolylineBuilder.h"
#include "PolygonBuilder.h"
#include "Point.h"
#include "PointBuilder.h"
#include "GraphicsOverlay.h"
#include "SpatialReference.h"
#include "GraphicsOverlayListModel.h"
#include "SymbolTypes.h"
#include "MapViewTypes.h"
#include "GraphicListModel.h"
#include "Basemap.h"
#include "Raster.h"
#include "PointCollection.h"
#include "LayerListModel.h"
#include "MapTypes.h"
#include "Fields.h"
#include "IdentifyGraphicsOverlayResult.h"
#include "AttributeListModel.h"
#include "SimpleRenderer.h"
#include "HeatmapRenderer.h"
#include "TaskWatcher.h"
#include "Polyline.h"
#include "Polygon.h"
#include "Part.h"
#include "PartCollection.h"
#include "Multipoint.h"
#include "Feature.h"
#include "Field.h"
#include "AbstractLocationDataSource.h"
#include "../include/Location.h"

using namespace Esri::ArcGISRuntime;

namespace CyberTracker {

//=====================================================================================================================
// MapController.

MapController::MapController(QObject *parent): QObject(parent), m_map(new Map(SpatialReference::webMercator(), this)), m_layers(new MapLayerListModel(this))
{
    m_mode = MapController::Mode::ModeDefault;
    m_initialPanMode = MapController::AutoPanMode::AutoPanModeNone;
    m_hideDataLayers = false;
    m_completed = m_loaded = false;

    m_filter = new SightingFilter(this);
    m_filter->load(App::instance()->settings()->sightingFilter());

    m_gotoLinesOverlay = new GraphicsOverlay(this);
    m_gotoPointsOverlay = new GraphicsOverlay(this);
    m_gotoPointsOverlay->setOverlayId("gotoPoints");

    m_heatmapOverlay = new GraphicsOverlay(this);
    m_trackOverlay = new GraphicsOverlay(this);
    m_formTrackOverlay = new GraphicsOverlay(this);

    m_sightingOverlay = new GraphicsOverlay(this);
    m_sightingOverlay->setOverlayId("sighting");
    m_sightingTrackOverlay = new GraphicsOverlay(this);
    m_sightingTrackOverlay->setOverlayId("sighting");

    m_gotoTargetOverlay1 = new GraphicsOverlay(this);
    m_gotoTargetOverlay2 = new GraphicsOverlay(this);
    m_flagOverlay = new GraphicsOverlay(this);
    m_flagOverlay->setOverlayId("flagOverlay");

    m_fieldOverlay = new GraphicsOverlay(this);
    m_fieldOverlay->setOverlayId("fieldValue");

    connect(m_map, &Map::loadStatusChanged, this, [&](LoadStatus loadStatus)
    {
        if (loadStatus == LoadStatus::Loaded)
        {
            loaded();
        }
    });
}

MapController::~MapController()
{
}

void MapController::classBegin()
{
    m_layers->classBegin();
}

void MapController::componentComplete()
{
    m_layers->componentComplete();

    connect(m_layers, &MapLayerListModel::changed, this, &MapController::rebuild);

    connect(App::instance(), &App::zoomToMapLayer, this, [&](const QVariantMap& layer)
    {
        m_mapView->locationDisplay()->setAutoPanMode(LocationDisplayAutoPanMode::Off);

        auto layerId = layer["id"].toString();

        auto mapLayer = m_layerLookup.value(layerId, nullptr);
        if (mapLayer)
        {
            m_mapView->setViewpointGeometryAsync(mapLayer->fullExtent(), 64);
            return;
        }

        if (layer.contains("extent") || m_layerExtents.contains(layerId))
        {
            auto extent = layer["extent"].toMap();
            if (extent.isEmpty())
            {
                extent = m_layerExtents[layerId];
            }

            auto xmin = extent["xmin"].toDouble() - 0.01;
            auto ymin = extent["ymin"].toDouble() - 0.01;
            auto xmax = extent["xmax"].toDouble() + 0.01;
            auto ymax = extent["ymax"].toDouble() + 0.01;
            auto envelope = Envelope(xmin, ymin, xmax, ymax, SpatialReference::wgs84());
            m_mapView->setViewpointGeometryAsync(envelope, 64);
            return;
        }

        if (layerId == MapLayerListModel::SIGHTING_LAYER_ID)
        {
            if (!m_sightingOverlay->extent().isEmpty())
            {
                m_mapView->setViewpointGeometryAsync(m_sightingOverlay->extent(), 64);
            }

            return;
        }

        if (layerId == MapLayerListModel::LOCATION_LAYER_ID)
        {
            auto envelopeBuilder = EnvelopeBuilder(m_trackOverlay->extent());
            envelopeBuilder.unionWithEnvelope(m_formTrackOverlay->extent());
            envelopeBuilder.unionWithEnvelope(m_sightingTrackOverlay->extent());
            if (!envelopeBuilder.isEmpty())
            {
                m_mapView->setViewpointGeometryAsync(envelopeBuilder.toEnvelope(), 64);
            }

            return;
        }
    });

    connect(App::instance(), &App::lastLocationChanged, this, [&]()
    {
        if (m_mode == Mode::ModeDefault)
        {
            rebuildGotoTarget();
        }
    });

    connect(App::instance()->settings(), &Settings::sightingFilterChanged, this, [&]()
    {
        m_filter->load(App::instance()->settings()->sightingFilter());
        emit filterChanged();

        rebuildDataLayers();
    });

    auto form = Form::parentForm(this);
    if (form)
    {
        connect(form, &Form::mapLayersChanged, this, &MapController::rebuildFormLayers);
    }

    connect(App::instance(), &App::sightingDeleted, this, &MapController::rebuildDataLayers);
    connect(App::instance(), &App::sightingSaved, this, &MapController::rebuildDataLayers);
    connect(App::instance(), &App::sightingModified, this, &MapController::rebuildDataLayers);
    connect(App::instance(), &App::sightingFlagsChanged, this, &MapController::rebuildDataLayers);
    connect(App::instance(), &App::sightingsChanged, this, &MapController::rebuildDataLayers);

    connect(App::instance()->settings(), &Settings::heatmapActiveChanged, this, [&]()
    {
        App::instance()->settings()->heatmapActive() ? rebuild() : resetHeatmap();
    });

    connect(this, &MapController::flagLocationChanged, this, [&]()
    {
        rebuildFlagOverlay();
    });

    if (form && form->connected())
    {
        connect(form, &Form::fieldValueChanged, this, [&](const QString& recordUid, const QString& fieldUid, const QVariant& /*oldValue*/, const QVariant& /*newValue*/)
        {
            if (recordUid == m_recordUid && fieldUid == m_fieldUid)
            {
                rebuildFieldOverlay();
            }
        });

        connect(form, &Form::locationSaved, this, [&](CyberTracker::Location* location, const QString& /*locationUid*/)
        {
            if (m_hideDataLayers || !m_formTrackVisible)
            {
                return;
            }

            auto wgs84 = SpatialReference::wgs84();
            auto x = location->x();
            auto y = location->y();

            addToHeatmap(x, y, wgs84);

            auto polyline = Esri::ArcGISRuntime::Polyline();
            auto graphics = m_formTrackOverlay->graphics();
            if (graphics->isEmpty())
            {
                auto polylineBuilder = PolylineBuilder(wgs84);
                polylineBuilder.addPoint(x, y);
                polyline = polylineBuilder.toPolyline();
            }
            else
            {
                auto polylineBuilder = PolylineBuilder(static_cast<Esri::ArcGISRuntime::Polyline>(graphics->first()->geometry()));
                polylineBuilder.addPoint(x, y);
                polyline = polylineBuilder.toPolyline();
            }

            m_formTrackOverlay->graphics()->clear();
            m_formTrackOverlay->graphics()->append(new Graphic(polyline));
        });
    }

    connect(App::instance()->settings(), &Settings::heatmapMaxIntensityChanged, this, [&]()
    {
        if (m_heatmapActive && App::instance()->settings()->heatmapActive())
        {
            auto renderer = Utils::createHeatmapRenderer(App::instance()->scaleByFontSize(10), 0, App::instance()->settings()->heatmapMaxIntensity());
            m_heatmapOverlay->setRenderer(renderer);
        }
    });

    rebuild();

    m_completed = true;
}

void MapController::loaded()
{
    auto viewpointCenter = App::instance()->settings()->mapViewpointCenter();
    auto targetScale = viewpointCenter.value("scale", 50000).toDouble();
    targetScale = std::max(m_minScale, targetScale);

    if (!m_initialExtent.isEmpty())
    {
        // Hack because otherwise this does not work.
        QTimer::singleShot(0, this, [&]()
        {
            auto geometry = Geometry::fromJson(QJsonDocument(QJsonObject::fromVariantMap(m_initialExtent)).toJson(), SpatialReference::wgs84());
            m_mapView->setViewpointGeometryAsync(geometry, 64);
        });
    }
    else if (m_mode == Mode::ModeFlag && !m_flagLocation.isEmpty())
    {
        auto point = Point(m_flagLocation["x"].toDouble(), m_flagLocation["y"].toDouble(), SpatialReference::wgs84());
        auto viewpoint = Viewpoint(point, targetScale);
        m_mapView->setViewpointAsync(viewpoint);
    }
    else if (m_mode == Mode::ModeFlag && App::instance()->lastLocation()->isValid())
    {
        auto lastLocation = App::instance()->lastLocation();
        auto point = Point(lastLocation->x(), lastLocation->y(), SpatialReference::wgs84());
        auto viewpoint = Viewpoint(point, targetScale);
        m_mapView->setViewpointAsync(viewpoint);
    }
    else if (targetScale != 50000)
    {
        auto point = Point(viewpointCenter["x"].toDouble(), viewpointCenter["y"].toDouble(), SpatialReference::wgs84());
        auto viewpoint = Viewpoint(point, targetScale);
        m_mapView->setViewpointAsync(viewpoint, 0.0);
    }

    // Prompt for permission as needed.
    if (App::instance()->promptForLocation())
    {
        emit requestLocationPermission();
    }
    else
    {
        locationStart();
    }

    m_loaded = true;
}

QVariantList MapController::findPath(int pathId) const
{
    qFatalIf(pathId < 0 || pathId >= m_paths.count(), "Bad path index");
    return m_paths[pathId].toList();
}

MapQuickView* MapController::mapView() const
{
    return m_mapView;
}

void MapController::setMapView(MapQuickView* mapView)
{
    if (!mapView)
    {
        if (m_mapView && m_locationStarted)
        {
            m_mapView->locationDisplay()->stop();
        }

        if (m_mapView)
        {
            disconnect(m_mapView, nullptr, this, nullptr);
        }

        m_mapView = nullptr;
        return;
    }

    if (mapView == m_mapView)
    {
        return;
    }

    m_mapView = mapView;
    m_mapView->setMap(m_map);
    m_mapView->setRotationByPinchingEnabled(true);
    m_mapView->setMagnifierEnabled(true);

    m_mapView->graphicsOverlays()->append(m_gotoLinesOverlay);
    m_mapView->graphicsOverlays()->append(m_gotoPointsOverlay);
    m_mapView->graphicsOverlays()->append(m_heatmapOverlay);
    m_mapView->graphicsOverlays()->append(m_trackOverlay);
    m_mapView->graphicsOverlays()->append(m_formTrackOverlay);
    m_mapView->graphicsOverlays()->append(m_sightingTrackOverlay);
    m_mapView->graphicsOverlays()->append(m_sightingOverlay);
    m_mapView->graphicsOverlays()->append(m_gotoTargetOverlay1);
    m_mapView->graphicsOverlays()->append(m_gotoTargetOverlay2);
    m_mapView->graphicsOverlays()->append(m_flagOverlay);
    m_mapView->graphicsOverlays()->append(m_fieldOverlay);

    connect(m_mapView, &MapQuickView::viewpointChanged, this, [&]()
    {
        emit centerPointChanged();

        if (!m_loaded || !m_completed || (m_mode != Mode::ModeDefault))
        {
            return;
        }

        auto center = m_mapView->screenToLocation(m_mapView->width() / 2, m_mapView->height() / 2);
        auto projectedCenter = GeometryEngine::project(center, SpatialReference::wgs84()).extent().center();
        App::instance()->settings()->set_mapViewpointCenter(QVariantMap {
            { "x", projectedCenter.x() },
            { "y", projectedCenter.y() },
            { "scale", m_mapView->mapScale() }
        });
    });

    connect(m_mapView, &MapQuickView::mouseClicked, this, [&](QMouseEvent& mouseEvent)
    {
        m_longPress = false;

        switch (m_mode)
        {
        case Mode::ModeDefault:
            if (!m_hideDataLayers)
            {
                m_mapView->identifyGraphicsOverlays(mouseEvent.position().x(), mouseEvent.position().y(), 22, false, 1000);
            }
            break;

        case Mode::ModeField:
            {
                auto point = m_mapView->screenToLocation(mouseEvent.position().x(), mouseEvent.position().y());
                auto projectedPoint = GeometryEngine::project(point, SpatialReference::wgs84());
                emit pointClicked(projectedPoint.extent().center().x(), projectedPoint.extent().center().y());
            }
            break;

        default:
            ;
        }
    });

    connect(m_mapView, &MapQuickView::mousePressedAndHeld, this, [&](QMouseEvent& mouseEvent)
    {
        m_longPress = true;
    });

    connect(m_mapView, &MapQuickView::mouseReleased, this, [&](QMouseEvent& mouseEvent)
    {
        if (m_longPress)
        {
            m_longPress = false;
            auto point = m_mapView->screenToLocation(mouseEvent.position().x(), mouseEvent.position().y());
            auto projectedPoint = GeometryEngine::project(point, SpatialReference::wgs84());
            emit longPress(projectedPoint.extent().center().x(), projectedPoint.extent().center().y());
        }
    });

    connect(m_mapView, &MapQuickView::identifyGraphicsOverlaysCompleted, this, [&](QUuid, QList<IdentifyGraphicsOverlayResult*> identifyResults)
    {
        auto sightingList = QVariantList();
        auto duplicateHash = QHash<QString, bool>();
        auto gotoPointList = QVariantList();

        for (auto overlayIt = identifyResults.constBegin(); overlayIt != identifyResults.constEnd(); overlayIt++)
        {
            // Delete rawIdentifyResult on leaving scope.
            auto result = std::unique_ptr<IdentifyGraphicsOverlayResult>(*overlayIt);
            if (!result)
            {
                continue;
            }

            for (auto j = 0; j < result->graphics().size(); j++)
            {
                auto graphic = result->graphics().at(j);
                auto attributes = graphic->attributes()->attributesMap();
                auto geometry = graphic->geometry();
                auto graphicsOverlay = graphic->graphicsOverlay();
                if (!graphicsOverlay)
                {
                    continue;
                }
                auto overlayId = graphicsOverlay->overlayId();
                auto path = QVariantList();

                if (overlayId == m_sightingOverlay->overlayId())
                {
                    path.append(QVariant(QVariantList { geometry.extent().center().x(), geometry.extent().center().y() }));
                    auto sighting = attributes;
                    auto sightingUid = sighting.value("sightingUid").toString();
                    if (!duplicateHash.contains(sightingUid))
                    {
                        sighting.insert("overlayId", overlayId);
                        sighting.insert("source", "Sighting");
                        sighting.insert("name", tr("Sighting"));
                        sighting.insert("path", path);
                        sighting.insert("pathIndex", 0);
                        sightingList.append(sighting);
                        duplicateHash.insert(sightingUid, true);
                    }
                }
                else if (overlayId == m_gotoPointsOverlay->overlayId())
                {
                    path = findPath(attributes.value("pathId", -1).toInt());
                    auto gotoPoint = attributes;
                    gotoPoint.insert("overlayId", overlayId);
                    gotoPoint.insert("path", path);
                    gotoPoint.insert("pathIndex", attributes.value("pathIndex", 0).toInt());
                    gotoPointList.append(gotoPoint);
                }
                else if (std::any_of(m_formLayerOverlays.cbegin(), m_formLayerOverlays.cend(),
                         [&](GraphicsOverlay* o) { return o->overlayId() == overlayId; }))
                {
                    auto x = attributes.value("x").toDouble();
                    auto y = attributes.value("y").toDouble();
                    if (x == 0 && y == 0) continue;
                    path.append(QVariant(QVariantList { x, y }));
                    auto gotoPoint = attributes;
                    gotoPoint.insert("overlayId", overlayId);
                    gotoPoint.insert("path", path);
                    gotoPoint.insert("pathIndex", 0);
                    if (!gotoPoint.contains("name"))
                    {
                        gotoPoint.insert("name", "");
                    }
                    gotoPointList.append(gotoPoint);
                }
            }
        }

        if (!sightingList.isEmpty())
        {
            std::sort(sightingList.begin(), sightingList.end(), [&](const QVariant& s1, const QVariant& s2) -> bool
            {
                return s1.toMap()["createTime"].toString() < s2.toMap()["createTime"].toString();
            });
        }

        auto inspectList = QVariantList();

        inspectList.append(sightingList);
        inspectList.append(gotoPointList);

        if (!inspectList.isEmpty())
        {
            emit inspectClicked(inspectList);
        }
    });

    connect(m_mapView, &MapQuickView::mapRotationChanged, this, &MapController::rotationChanged);

    emit mapViewChanged();
}

MapLayerListModel* MapController::layers() const
{
    return m_layers;
}

double MapController::rotation() const
{
    auto result = m_mapView ? m_mapView->mapRotation() : 0.0;
    return std::isnan(result) ? 0 : result;
}

void MapController::setRotation(double value)
{
    auto prevAutoPanMode = autoPanMode();
    m_mapView->setViewpointRotationAsync(value);

    if (prevAutoPanMode == AutoPanMode::AutoPanModeNavigation)
    {
        setAutoPanMode(AutoPanMode::AutoPanModeRecenter);
    }
    else
    {
        setAutoPanMode(prevAutoPanMode);
    }
}

MapController::AutoPanMode MapController::autoPanMode() const
{
    return (AutoPanMode)m_mapView->locationDisplay()->autoPanMode();
}

void MapController::setAutoPanMode(MapController::AutoPanMode value)
{
    m_mapView->locationDisplay()->setAutoPanMode((LocationDisplayAutoPanMode)value);
}

QVariantMap MapController::centerPoint()
{
    auto centerPoint = m_mapView->screenToLocation(m_mapView->width() / 2, m_mapView->height() / 2);
    auto projectedCenter = GeometryEngine::project(centerPoint, SpatialReference::wgs84());

    return QVariantMap {{ "x", projectedCenter.extent().center().x() }, { "y", projectedCenter.extent().center().y() }};
}

void MapController::locationStart()
{
    qputenv("QT_POSITIONING_BACKEND", App::instance()->positionInfoSourceName().toLatin1());
    auto cleanupEnv = qScopeGuard([&] { qunsetenv("QT_POSITIONING_BACKEND"); });

    auto locationSymbolSize = App::instance()->scaleByFontSize(40);
    auto locationDisplay = m_mapView->locationDisplay();
    locationDisplay->setAcquiringSymbol(Utils::createPictureMarkerSymbol(":/map/acquiringSymbol.svg", locationSymbolSize));
    locationDisplay->setCourseSymbol(Utils::createPictureMarkerSymbol(":/map/courseSymbol.svg", locationSymbolSize));
    locationDisplay->setDefaultSymbol(Utils::createPictureMarkerSymbol(":/map/defaultSymbol.svg", locationSymbolSize));
    locationDisplay->setHeadingSymbol(Utils::createPictureMarkerSymbol(":/map/headingSymbol.svg", locationSymbolSize));

    auto lineWidth = App::instance()->scaleByFontSize(4);
    auto accuracyOutline = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, QColor(0, 128, 255, 230), lineWidth);
    auto accuracySymbol = new SimpleFillSymbol(SimpleFillSymbolStyle::Solid, QColor(255, 255, 255, 51), accuracyOutline);
    locationDisplay->setAccuracySymbol(accuracySymbol);

    auto pingOutline = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, QColor(0, 128, 255, 230), lineWidth);
    auto pingSymbol = new SimpleMarkerSymbol(SimpleMarkerSymbolStyle::Circle, QColor(255, 255, 255, 51), locationSymbolSize * 2);
    pingSymbol->setOutline(pingOutline);
    locationDisplay->setPingAnimationSymbol(pingSymbol);

    // Pan mode.
    if (m_initialPanMode != AutoPanMode::AutoPanModeNone)
    {
        locationDisplay->setAutoPanMode((LocationDisplayAutoPanMode)m_initialPanMode);
    }
    else if (m_mode == Mode::ModeFlag)
    {
        if (m_flagLocation.isEmpty())
        {
            locationDisplay->setAutoPanMode(LocationDisplayAutoPanMode::Recenter);
        }
        else
        {
            locationDisplay->setAutoPanMode(LocationDisplayAutoPanMode::Off);
        }
    }
    else if (!m_initialExtent.isEmpty())
    {
        locationDisplay->setAutoPanMode(LocationDisplayAutoPanMode::Off);
    }
    else
    {
        locationDisplay->setAutoPanMode((LocationDisplayAutoPanMode)App::instance()->settings()->mapPanMode());
    }

    // Create the data source for the location display.
    class LocationDataSource: public Esri::ArcGISRuntime::AbstractLocationDataSource
    {
    public:
        LocationDataSource(QObject* parent): AbstractLocationDataSource(parent)
        {}

        void onStart() override
        {
            m_positionInfoSource.setUpdateInterval(1000);
            m_positionInfoSource.startUpdates();
            connect(&m_positionInfoSource, &PositionInfoSource::positionUpdated, this, [&](const QGeoPositionInfo& update)
            {
                Esri::ArcGISRuntime::Location location(
                    update.timestamp(),
                    Esri::ArcGISRuntime::Point(update.coordinate().longitude(), update.coordinate().latitude(), Esri::ArcGISRuntime::SpatialReference::wgs84()),
                    update.attribute(QGeoPositionInfo::HorizontalAccuracy),
                    update.attribute(QGeoPositionInfo::VerticalAccuracy),
                    update.attribute(QGeoPositionInfo::GroundSpeed),
                    update.attribute(QGeoPositionInfo::Direction),
                    false);
                updateHeading(update.attribute(QGeoPositionInfo::Direction));
                updateLocation(location);
            });
            onStartCompleted();
        }

        void onStop() override
        {
            m_positionInfoSource.stopUpdates();
            onStopCompleted();
        }

    private:
        PositionInfoSource m_positionInfoSource;
    };

    if (App::instance()->settings()->simulateGPS())
    {
        locationDisplay->setDataSource(new LocationDataSource(this));
    }

    locationDisplay->start();
    m_locationStarted = true;

    connect(locationDisplay, &LocationDisplay::autoPanModeChanged, this, [&](LocationDisplayAutoPanMode panMode)
    {
        if (m_mode == Mode::ModeDefault)
        {
            auto prevMode = App::instance()->settings()->mapPanMode();
            if (prevMode != (int)panMode)
            {
                App::instance()->settings()->set_mapPanMode((int)panMode);
            }
        }

        emit autoPanModeChanged();
    });
}

void MapController::rebuild()
{
    delete m_layerCleanup;
    m_layerCleanup = new QObject(this);

    m_layerLookup.clear();
    m_map->basemap()->baseLayers()->clear();
    m_gotoPointsOverlay->graphics()->clear();
    m_gotoLinesOverlay->graphics()->clear();

    resetHeatmap();

    rebuildBasemap();

    rebuildOfflineLayers();

    switch (m_mode)
    {
    case Mode::ModeDefault:
        rebuildDataLayers();
        rebuildFormLayers();
        rebuildGotoTarget();
        break;

    case Mode::ModeFlag:
        rebuildFlagOverlay();
        break;

    case Mode::ModeField:
        rebuildFieldOverlay();
        break;

    default:
        ;
    }
}

void MapController::centerOnFlagLocation()
{
    if (m_flagLocation.isEmpty())
    {
        return;
    }

    auto viewpointCenter = App::instance()->settings()->mapViewpointCenter();
    auto targetScale = viewpointCenter.value("scale", 50000).toDouble();
    targetScale = std::max(m_minScale, targetScale);

    auto point = Point(m_flagLocation["x"].toDouble(), m_flagLocation["y"].toDouble(), SpatialReference::wgs84());
    auto viewpoint = Viewpoint(point, targetScale);
    m_mapView->setViewpointAsync(viewpoint);
}

void MapController::zoomIn()
{
    m_mapView->setViewpointScaleAsync(m_mapView->mapScale() / 2);
}

void MapController::zoomOut()
{
    m_mapView->setViewpointScaleAsync(m_mapView->mapScale() * 2);
}

void MapController::rebuildBasemap()
{
    m_minScale = 0;
    m_layers->enumActive("basemap", [&](const QVariantMap& layer)
    {
        auto id = layer["id"].toString();
        auto layerObj = (Layer*)nullptr;

        if (id == "BasemapOpenStreetMap")
        {
            layerObj = new WebTiledLayer("https://{subDomain}.tile.openstreetmap.org/{level}/{col}/{row}.png", QStringList { "a", "b", "c" });
            m_minScale = 1128.5;
        }
        else if (id == "BasemapOpenTopoMap")
        {
            layerObj = new WebTiledLayer("https://{subDomain}.tile.opentopomap.org/{level}/{col}/{row}.png", QStringList { "a", "b", "c" });
            m_minScale = 1128.5;
        }
        else if (id == "BasemapHumanitarian")
        {
            layerObj = new WebTiledLayer("https://{subDomain}.tile.openstreetmap.fr/hot/{level}/{col}/{row}.png", QStringList { "a" } );
            m_minScale = 1128.5;
        }
        else if (id == "BasemapNatGeo")
        {
            layerObj = new ArcGISTiledLayer(QUrl("http://services.arcgisonline.com/ArcGIS/rest/services/NatGeo_World_Map/MapServer"));
        }
        else if (id == "BasemapTopographic")
        {
            layerObj = new ArcGISTiledLayer(QUrl("https://server.arcgisonline.com/ArcGIS/rest/services/World_Topo_Map/MapServer"));
        }
        else if (id == "BasemapStreets")
        {
            layerObj = new ArcGISTiledLayer(QUrl("https://server.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer"));
        }
        else if (id == "BasemapImagery")
        {
            layerObj = new ArcGISTiledLayer(QUrl("https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer"));
        }
        else if (id == "BasemapDarkGrayCanvas")
        {
            layerObj = new ArcGISTiledLayer(QUrl("https://services.arcgisonline.com/ArcGIS/rest/services/Canvas/World_Dark_Gray_Base/MapServer"));
        }
        else if (id == "BasemapLightGrayCanvas")
        {
            layerObj = new ArcGISTiledLayer(QUrl("https://services.arcgisonline.com/ArcGIS/rest/services/Canvas/World_Light_Gray_Base/MapServer"));
        }
        else if (id == "BasemapNavigationVector")
        {
            layerObj = new ArcGISVectorTiledLayer(QUrl("https://basemaps.arcgis.com/arcgis/rest/services/World_Basemap_v2/VectorTileServer"));
        }
        else if (id == "BasemapOceans")
        {
            layerObj = new ArcGISTiledLayer(QUrl("https://services.arcgisonline.com/arcgis/rest/services/Ocean/World_Ocean_Base/MapServer"));
        }
        else if (id != "BasemapNone")
        {
            qDebug() << "Unknown base layer type: " << id;
        }

        if (layerObj)
        {
            layerObj->setParent(m_layerCleanup);
            m_map->basemap()->baseLayers()->append(layerObj);
            m_layerLookup.insert(id, layerObj);
        }
    });
}

void MapController::rebuildOfflineLayers()
{
    m_layerLookup.clear();
    m_paths.clear();
    m_layerExtents.clear();

    auto offlineMapFilenames = QStringList();

    auto addLayer = [&](const QVariantMap& layer)
    {
        auto opacity = layer["opacity"].toDouble();
        if (opacity == 0.0)
        {
            return;
        }

        auto layerObj = (Layer*)nullptr;
        auto format = layer["format"].toString();
        auto filePath = layer["filePath"].toString();

        if (!filePath.isEmpty())
        {
            offlineMapFilenames.append(QFileInfo(filePath).fileName());
        }

        if (format == "tpk")
        {
            layerObj = new ArcGISTiledLayer(QUrl(filePath));
        }
        else if (format == "vtpk")
        {
            layerObj = new ArcGISVectorTiledLayer(QUrl(filePath));
        }
        else if (QStringList { "img", "i12", "dt0", "dt1", "dt2", "tc2", "geotiff", "tif", "tiff", "hr1", "jpeg", "jp2", "ntf", "png", "i21" }.contains(format))
        {
            layerObj = new RasterLayer(new Raster(filePath));
        }
        else if (format == "mbtiles")
        {
            layerObj = Utils::createMBTilesLayer(filePath);
        }
        else if (format == "ecw")
        {
            layerObj = Utils::createECWLayer(filePath);
        }
        else if (format == "wms")
        {
            auto wmsData = Utils::variantMapFromJsonFile(filePath);
            layerObj = new WmsLayer(wmsData["service"].toUrl(), QStringList { wmsData["layer"].toString() });
        }
        else if (format == "shp")
        {
            auto featureTable = new ShapefileFeatureTable(filePath);
            auto shapeLayer = new FeatureLayer(featureTable);
            auto symbolScale = App::instance()->scaleByFontSize(1);

            auto symbol = Utils::esriMakeSymbol(layer.value("symbol").toMap(), symbolScale);
            if (!symbol.isEmpty())
            {
                auto esriSymbol = Symbol::fromJson(Utils::variantMapToJson(symbol));
                shapeLayer->setRenderer(new SimpleRenderer(esriSymbol, this));
            }

            layerObj = shapeLayer;
        }
        else if (format == "kml")
        {
            auto kmlDataset = new KmlDataset("file:///" + filePath);
            layerObj = new KmlLayer(kmlDataset);
        }
        else if (format == "geojson")
        {
            auto symbolScale = App::instance()->scaleByFontSize(1);
            auto outlineColor = App::instance()->settings()->darkTheme() ? Qt::white : Qt::black;
            auto geoJsonLayer = Utils::esriLayerFromGeoJson(filePath, symbolScale, outlineColor, &m_paths);
            m_layerExtents.insert(layer["id"].toString(), geoJsonLayer.value("extent").toMap());

            auto points = geoJsonLayer.value("points").toMap();
            if (!points.isEmpty())
            {
                Utils::appendPointsToOverlay(m_gotoPointsOverlay, points);
            }

            auto lines = geoJsonLayer.value("polylines").toList();
            if (!lines.isEmpty())
            {
                for (auto lineIt = lines.constBegin(); lineIt != lines.constEnd(); lineIt++)
                {
                    Utils::appendLinesToOverlay(m_gotoLinesOverlay, lineIt->toMap());
                }
            }
        }
        else
        {
            qDebug() << "Unknown offline layer format: " + format;
            return;
        }

        if (layerObj)
        {
            layerObj->setMinScale(std::numeric_limits<double>().max());
            layerObj->setMaxScale(std::numeric_limits<double>().min());
            layerObj->setOpacity(layer["opacity"].toFloat());

            layerObj->setParent(m_layerCleanup);
            m_map->basemap()->baseLayers()->append(layerObj);

            m_layerLookup.insert(layer["id"].toString(), layerObj);
        }
    };

    m_layers->enumActive("offline", addLayer);

    // Usage telemetry.
    if (auto form = Form::parentForm(this))
    {
        /// Number of offline map files in the package. Projects with no offline maps (the field is absent) are counted as zero.
        /// @category Project
        /// @discrete true
        /// @absentZero true
        /// @docs https://cybertrackerwiki.org/smart/user-manual/6#635-basemap-settings-tab
        form->setUsage("offlineMaps", offlineMapFilenames);
    }
}

void MapController::rebuildFormLayers()
{
    // Remove previous form layer overlays.
    for (auto overlay : m_formLayerOverlays)
    {
        if (m_mapView)
        {
            m_mapView->graphicsOverlays()->removeOne(overlay);
        }
        delete overlay;
    }
    m_formLayerOverlays.clear();

    auto form = Form::parentForm(this);
    if (!form)
    {
        return;
    }

    auto wgs84 = SpatialReference::wgs84();
    auto scaleFactor = App::instance()->scaleByFontSize(1);

    for (const auto& layerVar : form->mapLayers())
    {
        auto layer = layerVar.toMap();
        auto layerId = layer["id"].toString();
        auto color = QColor(layer["color"].toString());
        auto pointSize = layer.value("pointSize", 0).toDouble() * scaleFactor;
        auto lineWidth = layer.value("lineWidth", 0).toDouble() * scaleFactor;

        auto overlay = new GraphicsOverlay(this);
        overlay->setOverlayId(layerId);

        // Render points.
        auto points = layer["points"].toList();
        if (!points.isEmpty() && pointSize > 0)
        {
            auto symbol = new SimpleMarkerSymbol(SimpleMarkerSymbolStyle::Circle, color, pointSize, this);
            auto outlineColorStr = layer.value("outlineColor").toString();
            if (!outlineColorStr.isEmpty())
            {
                symbol->setOutline(new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, QColor(outlineColorStr), scaleFactor, this));
            }
            for (const auto& ptVar : points)
            {
                auto pt = ptVar.toMap();
                auto point = Point(pt["x"].toDouble(), pt["y"].toDouble(), wgs84);
                auto graphic = new Graphic(point, symbol);
                auto attrs = QVariantMap {
                    { "x", pt["x"] },
                    { "y", pt["y"] },
                    { "layerId", layerId },
                };
                auto title = pt.value("title").toString();
                if (!title.isEmpty())
                {
                    attrs.insert("title", title);
                    attrs.insert("name", title);
                }
                auto layerName = layer.value("name").toString();
                if (!layerName.isEmpty())
                {
                    attrs.insert("layerName", layerName);
                    attrs.insert("source", layerName);
                }
                auto properties = pt.value("properties").toList();
                if (!properties.isEmpty())
                {
                    attrs.insert("properties", QString::fromUtf8(Utils::variantListToJson(properties, QJsonDocument::Compact)));
                }
                graphic->attributes()->setAttributesMap(attrs);
                overlay->graphics()->append(graphic);
            }
        }

        // Render lines.
        auto lines = layer["lines"].toList();
        if (!lines.isEmpty() && lineWidth > 0)
        {
            auto lineSymbol = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, color, lineWidth, this);
            for (const auto& lineVar : lines)
            {
                auto coords = lineVar.toList();
                if (coords.size() < 2) continue;

                auto builder = PolylineBuilder(wgs84);
                for (const auto& coordVar : coords)
                {
                    auto c = coordVar.toMap();
                    builder.addPoint(Point(c["x"].toDouble(), c["y"].toDouble(), wgs84));
                }
                overlay->graphics()->append(new Graphic(builder.toPolyline(), lineSymbol));
            }
        }

        if (m_mapView)
        {
            // Insert before the data overlays (after gotoPoints).
            auto insertIndex = m_mapView->graphicsOverlays()->indexOf(m_heatmapOverlay);
            if (insertIndex >= 0)
            {
                m_mapView->graphicsOverlays()->insert(insertIndex, overlay);
            }
            else
            {
                m_mapView->graphicsOverlays()->append(overlay);
            }
        }

        m_formLayerOverlays.append(overlay);
    }
}

void MapController::rebuildSightingLayer(bool showSightings, bool showTracks)
{
    m_sightingOverlay->graphics()->clear();
    m_sightingTrackOverlay->graphics()->clear();

    auto app = App::instance();
    auto database = app->database();
    auto languageCode = app->settings()->languageCode();
    auto form = Form::parentForm(this);

    auto symbolSize = app->scaleByFontSize(48);
    auto lineWidth = app->scaleByFontSize(2.5);

    auto wgs84 = SpatialReference::wgs84();

    m_filter->matchInit();

    database->enumData(m_filter->projectUid(), m_filter->stateSpace(), Database::DataFlag::DB_SIGHTING_FLAG,
        [&](const QString& projectUid, const QString& sightingUid, const Database::DbData& dbSighting)
        {
            if (!m_filter->matchProject(projectUid))
            {
                return;
            }

            auto sighting = std::unique_ptr<Sighting>(new Sighting(dbSighting.data, dbSighting.stateSpace, Schema::acquire(dbSighting.schemaHash), languageCode));
            auto onExit = qScopeGuard([&] { Schema::release(dbSighting.schemaHash); });

            if (!m_filter->matchSighting(sighting.get()))
            {
                return;
            }

            auto geoshapes = sighting->geoshapes();
            if (geoshapes.isEmpty())
            {
                return;
            }

            auto renderer = app->createRendererPtr(sighting->schema()->rendererName());
            auto renderSymbol = renderer->getSightingMapSymbol(sighting.get());

            auto sameForm = form && form->connected() && form->project()->uid() == projectUid;
            auto attributes = QVariantMap {
                { "projectUid", projectUid },
                { "stateSpace", dbSighting.stateSpace },
                { "sightingUid", sightingUid },
                { "createTime", sighting->createTime() },
                { "timestamp", dbSighting.timestamp }, // updateTime
                { "readonly", dbSighting.flags.testFlag(Database::DataFlag::DB_READONLY_FLAG) },
                { "canEdit", sameForm && renderer->getSightingCanEdit(sighting.get(), dbSighting.flags) },
                { "canDelete", sameForm && renderer->getSightingCanDelete(sighting.get(), dbSighting.flags) }
            };

            for (auto geoshapeIt = geoshapes.constBegin(); geoshapeIt != geoshapes.constEnd(); geoshapeIt++)
            {
                auto graphic = (Graphic*)nullptr;
                auto geoshape = geoshapeIt->toMap();
                auto type = geoshape["type"].toString();

                if (type == "track")
                {
                    if (!showTracks)
                    {
                        continue;
                    }

                    auto lineBuilder = PolylineBuilder(wgs84);

                    auto points = geoshape["value"].toList();
                    for (auto pointIt = points.constBegin(); pointIt != points.constEnd(); pointIt++)
                    {
                        auto point = pointIt->toList();
                        auto x = point[0].toDouble();
                        auto y = point[1].toDouble();
                        lineBuilder.addPoint(x, y);
                        addToHeatmap(x, y, wgs84);
                    }

                    graphic = new Graphic(lineBuilder.toPolyline());
                    graphic->attributes()->setAttributesMap(attributes);
                    m_sightingTrackOverlay->graphics()->append(graphic); // Track needs to be under sighting data.
                    continue;
                }

                if (!showSightings)
                {
                    continue;
                }

                if (type == "point")
                {
                    auto location = geoshape["value"].toMap();
                    auto x = location["x"].toDouble();
                    auto y = location["y"].toDouble();

                    auto symbol = (PictureMarkerSymbol*)nullptr;

                    if (!renderSymbol.filename.isEmpty())
                    {
                        auto filePath = app->getArchiveFilePath(sighting->getIconHash(renderSymbol.filename));
                        if (QFile::exists(filePath))
                        {
                            symbol = new PictureMarkerSymbol(QImage(app->renderMapCallout(filePath, symbolSize)));
                        }
                    }

                    if (!symbol)
                    {
                        symbol = new PictureMarkerSymbol(QImage(app->renderMapMarker(renderSymbol.url, renderSymbol.color, symbolSize)));
                    }

                    symbol->setWidth(symbolSize);
                    symbol->setHeight(symbolSize);
                    symbol->setOffsetY(symbolSize / 2);

                    graphic = new Graphic(Point(x, y, wgs84), symbol);

                    addToHeatmap(x, y, wgs84);
                }
                else if (type == "line")
                {
                    auto lineBuilder = PolylineBuilder(wgs84);

                    auto points = geoshape["value"].toList();
                    for (auto pointIt = points.constBegin(); pointIt != points.constEnd(); pointIt++)
                    {
                        auto point = pointIt->toList();
                        auto x = point[0].toDouble();
                        auto y = point[1].toDouble();
                        lineBuilder.addPoint(x, y);
                        addToHeatmap(x, y, wgs84);
                    }

                    auto lineSymbol = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, renderSymbol.color, lineWidth);
                    graphic = new Graphic(lineBuilder.toPolyline(), lineSymbol);
                }
                else if (type == "area")
                {
                    auto areaBuilder = PolygonBuilder(wgs84);

                    auto points = geoshape["value"].toList();
                    for (auto pointIt = points.constBegin(); pointIt != points.constEnd(); pointIt++)
                    {
                        auto point = pointIt->toList();
                        auto x = point[0].toDouble();
                        auto y = point[1].toDouble();
                        areaBuilder.addPoint(x, y);
                        addToHeatmap(x, y, wgs84);
                    }

                    auto areaOutlineSymbol = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, renderSymbol.color, lineWidth);
                    auto areaSymbolColor = renderSymbol.color;
                    areaSymbolColor.setAlpha(128);

                    auto areaSymbol = new SimpleFillSymbol(SimpleFillSymbolStyle::Solid, areaSymbolColor, areaOutlineSymbol);
                    graphic = new Graphic(areaBuilder.toPolygon(), areaSymbol);
                }
                else
                {
                    qDebug() << "Unknown geometry";
                    continue;
                }

                graphic->attributes()->setAttributesMap(attributes);
                m_sightingOverlay->graphics()->append(graphic);
            }
        });
}

void MapController::rebuildTrackLayer()
{
    m_trackOverlay->graphics()->clear();
    m_formTrackVisible = false;
    m_formTrackOverlay->graphics()->clear();

    if (m_hideDataLayers)
    {
        return;
    }

    auto form = Form::parentForm(this);
    auto formProjectUid = form ? form->projectUid() : QString();
    auto wgs84 = SpatialReference::wgs84();

    m_filter->matchInit();

    Utils::enumFiles(App::instance()->tracksPath(), [&](const QFileInfo& fileInfo)
    {
        auto trackProjectUid = fileInfo.baseName();
        if (!m_filter->matchProject(trackProjectUid))
        {
            return;
        }

        auto trackFile = TrackFile(fileInfo.filePath());
        auto polylineBuilder = PolylineBuilder(wgs84);
        auto part = (Part*)nullptr;
        auto lastCounter = 0;

        TrackFile::enumLocations(fileInfo.filePath(), [&](qint64 t, double x, double y, int counter)
        {
            if (!m_filter->matchTimestamp(t))
            {
                return;
            }

            if (counter < lastCounter || !part)
            {
                part = new Part(wgs84);
                polylineBuilder.parts()->addPart(part);
            }

            lastCounter = counter;

            part->addPoint(x, y);
            addToHeatmap(x, y, wgs84);
        });

        // Put the current track in a separate overlay so it can be refreshed without redoing all the tracks.
        if (trackProjectUid == formProjectUid)
        {
            m_formTrackVisible = true; // Active track passed the filter.
            m_formTrackOverlay->graphics()->append(new Graphic(polylineBuilder.toPolyline()));
        }
        else
        {
            m_trackOverlay->graphics()->append(new Graphic(polylineBuilder.toPolyline()));
        }
    });
}

void MapController::rebuildDataLayers()
{
    m_sightingOverlay->graphics()->clear();
    m_sightingTrackOverlay->graphics()->clear();
    m_trackOverlay->graphics()->clear();
    m_formTrackOverlay->graphics()->clear();
    m_heatmapOverlay->graphics()->clear();

    if (m_hideDataLayers)
    {
        return;
    }

    auto showSightings = false;
    auto showTracks = false;

    m_layers->enumActive("data", [&](const QVariantMap& layer)
    {
        auto layerId = layer["id"].toString();

        if (layerId == MapLayerListModel::SIGHTING_LAYER_ID)
        {
            showSightings = true;
            return;
        }

        if (layerId == MapLayerListModel::LOCATION_LAYER_ID)
        {
            showTracks = true;
            return;
        }
    });

    // Create the renderers.
    auto app = App::instance();
    auto lineWidth = app->scaleByFontSize(2.5);
    auto arrowSize = app->scaleByFontSize(10);
    auto trackArrowSymbol = Utils::createTrackArrowSymbol(lineWidth, arrowSize, QColor(0, 0, 255, 200));

    auto renderer = new SimpleRenderer();
    renderer->setSymbol(trackArrowSymbol);
    m_sightingTrackOverlay->setRenderer(renderer);

    renderer = new SimpleRenderer();
    renderer->setSymbol(trackArrowSymbol);
    m_trackOverlay->setRenderer(renderer);

    renderer = new SimpleRenderer();
    renderer->setSymbol(trackArrowSymbol);
    m_formTrackOverlay->setRenderer(renderer);

    m_heatmapActive = App::instance()->settings()->heatmapActive();
    if (m_heatmapActive)
    {
        auto renderer = Utils::createHeatmapRenderer(App::instance()->scaleByFontSize(10), 0, App::instance()->settings()->heatmapMaxIntensity());
        m_heatmapOverlay->setRenderer(renderer);
    }

    // Figure out which things to add.
    if (showSightings || showTracks)
    {
        rebuildSightingLayer(showSightings, showTracks);
    }

    if (showTracks)
    {
        rebuildTrackLayer();
    }
}

void MapController::rebuildGotoTarget()
{
    auto gotoManager = App::instance()->gotoManager();
    if (gotoManager->gotoPath().isEmpty())
    {
        m_gotoTargetOverlay1->graphics()->clear();
        m_gotoTargetOverlay2->graphics()->clear();
        return;
    }

    if (auto form = Form::parentForm(this))
    {
        /// The user has used the goto target navigation feature.
        /// @category Actions
        /// @docs https://cybertrackerwiki.org/smart/user-manual/#734-goto-overlay
        form->setUsage("goto", true);
    }

    auto nextOverlay = m_gotoTargetOverlay1->isVisible() ? m_gotoTargetOverlay2 : m_gotoTargetOverlay1;
    auto lastOverlay = m_gotoTargetOverlay1->isVisible() ? m_gotoTargetOverlay1 : m_gotoTargetOverlay2;

    nextOverlay->graphics()->clear();

    auto app = App::instance();
    auto location = gotoManager->gotoLocation();
    auto locationX = location["x"].toDouble();
    auto locationY = location["y"].toDouble();

    if (!nextOverlay->renderer())
    {
        auto lineWidth = app->scaleByFontSize(2.5);
        auto arrowSize = app->scaleByFontSize(10);
        auto gotoArrowSymbol = Utils::createTrackArrowSymbol(lineWidth, arrowSize, QColor(255, 0, 0, 200));
        auto renderer = new SimpleRenderer();
        renderer->setSymbol(gotoArrowSymbol);
        nextOverlay->setRenderer(renderer);
    }

    if (!gotoManager->gotoIconFilePath().isEmpty())
    {
        auto symbolSize = app->scaleByFontSize(48);
        auto symbol = new PictureMarkerSymbol(QImage(app->renderMapCallout(gotoManager->gotoIconFilePath(), symbolSize, false)));
        symbol->setWidth(symbolSize);
        symbol->setHeight(symbolSize);
        symbol->setOffsetY(symbolSize / 2);

        auto graphic = new Graphic(Point(locationX, locationY, SpatialReference::wgs84()), symbol);
        nextOverlay->graphics()->append(graphic);
    }

    auto polylineBuilder = PolylineBuilder(SpatialReference::wgs84());
    auto lastLocation = app->lastLocation();
    polylineBuilder.addPoint(Point(lastLocation->x(), lastLocation->y()));
    polylineBuilder.addPoint(Point(locationX, locationY));

    auto geometry1 = polylineBuilder.toPolyline();
    auto geometry2 = GeometryEngine::densifyGeodetic(geometry1, 1, LinearUnit(LinearUnitId::Kilometers), GeodeticCurveType::Geodesic);

    auto graphic = new Graphic(geometry2);
    nextOverlay->graphics()->append(graphic);

    nextOverlay->setVisible(true);
    lastOverlay->setVisible(false);
}

void MapController::rebuildFlagOverlay()
{
    m_flagOverlay->graphics()->clear();
    if (m_flagLocation.isEmpty())
    {
        return;
    }

    auto size = App::instance()->scaleByFontSize(32);

    Utils::appendPointToOverlay(
        m_flagOverlay,
        m_flagLocation["x"].toDouble(),
        m_flagLocation["y"].toDouble(),
        size,
        App::instance()->renderMapMarker(":/icons/mark-circle.svg", QColor("red"), size));
}

void MapController::rebuildFieldOverlay()
{
    m_fieldOverlay->graphics()->clear();

    auto location = false;
    auto line = false;
    auto area = false;
    auto track = false;
    auto points = QVariantList();

    if (!m_fieldValue.isEmpty())
    {
        auto type = m_fieldValue.value("fieldType").toString();
        location = type == "LocationField";
        line = type == "LineField";
        area = type == "AreaField";
        track = type == "TrackFile";
        points = m_fieldValue.value("fieldValue").toList();
    }
    else
    {
        auto form = Form::parentForm(this);
        if (!form || m_recordUid.isEmpty() || m_fieldUid.isEmpty())
        {
            return;
        }

        auto field = form->getField(m_fieldUid);
        line = qobject_cast<LineField*>(field) != nullptr;
        area = qobject_cast<AreaField*>(field) != nullptr;

        points = form->getFieldValue(m_recordUid, m_fieldUid).toList();
    }

    if (points.isEmpty())
    {
        return;
    }

    auto multipointBuilder = MultipointBuilder(SpatialReference::wgs84());
    auto pointBuilder = PointBuilder(SpatialReference::wgs84());
    auto lineBuilder = PolylineBuilder(SpatialReference::wgs84());
    auto areaBuilder = PolygonBuilder(SpatialReference::wgs84());

    for (auto pointIt = points.constBegin(); pointIt != points.constEnd(); pointIt++)
    {
        auto point = pointIt->toList();
        auto x = point[0].toDouble();
        auto y = point[1].toDouble();
        
        if (location)
        {
            pointBuilder.setXY(x, y);
            continue;
        }
        else if (line || track)
        {
            lineBuilder.addPoint(x, y);
        }
        else if (area)
        {
            areaBuilder.addPoint(x, y);
        }

        multipointBuilder.points()->addPoint(x, y);
    }

    auto app = App::instance();
    auto fillColor = app->settings()->darkTheme() ? QColor(255, 255, 255, 128) : QColor(0, 0, 0, 128);
    auto lineWidth = app->scaleByFontSize(2.5);

    if (location)
    {
        auto symbolSize = app->scaleByFontSize(48);
        auto symbol = new PictureMarkerSymbol(QImage(app->renderMapMarker(":/icons/mark-circle.svg", Qt::darkRed, symbolSize)));
        symbol->setWidth(symbolSize);
        symbol->setHeight(symbolSize);
        symbol->setOffsetY(symbolSize / 2);
        m_fieldOverlay->graphics()->append(new Graphic(pointBuilder.toPoint(), symbol));
        return;
    }
    else if (line)
    {
        auto lineSymbol = new SimpleLineSymbol(SimpleLineSymbolStyle::ShortDot, QColor(0, 0, 255, 200), lineWidth);
        m_fieldOverlay->graphics()->append(new Graphic(lineBuilder.toPolyline(), lineSymbol));
    }
    else if (area)
    {
        auto areaOutlineSymbol = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, fillColor, lineWidth);
        auto areaSymbol = new SimpleFillSymbol(SimpleFillSymbolStyle::Solid, fillColor, areaOutlineSymbol);
        m_fieldOverlay->graphics()->append(new Graphic(areaBuilder.toPolygon(), areaSymbol));
    }
    else if (track)
    {
        auto arrowSize = app->scaleByFontSize(10);
        auto trackArrowSymbol = Utils::createTrackArrowSymbol(lineWidth, arrowSize, QColor(0, 0, 255, 200));
        m_fieldOverlay->graphics()->append(new Graphic(lineBuilder.toPolyline(), trackArrowSymbol));

        // Do not display individual points.
        return;
    }

    auto pointOutlineSymbol = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, fillColor, lineWidth);
    auto pointSymbol = new SimpleMarkerSymbol(SimpleMarkerSymbolStyle::Circle, QColor(0, 0, 255, 128), App::instance()->scaleByFontSize(12));
    pointSymbol->setOutline(pointOutlineSymbol);
    m_fieldOverlay->graphics()->append(new Graphic(multipointBuilder.toMultipoint(), pointSymbol));
}

void MapController::resetHeatmap()
{
    m_heatmapOverlay->graphics()->clear();
    m_heatmapActive = false;
}

void MapController::addToHeatmap(double x, double y, const SpatialReference& spatialReference)
{
    if (m_heatmapActive)
    {
        m_heatmapOverlay->graphics()->append(new Graphic(Point(x, y, spatialReference)));
    }
}

} // namespace CyberTracker
