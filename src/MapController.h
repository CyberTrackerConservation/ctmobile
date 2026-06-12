#pragma once
#include "pch.h"
#include "MapLayerListModel.h"
#include "SightingListModel.h"
#include "MapQuickView.h"

namespace Esri {
namespace ArcGISRuntime {
class Map;
class SpatialReference;
class Symbol;
class GraphicsOverlay;
class Layer;
enum class LocationDisplayAutoPanMode;
} // namespace ArcGISRuntime
} // namespace Esri

namespace CyberTracker {

class MapController : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES (QQmlParserStatus)

public:
    enum class AutoPanMode {
        AutoPanModeNone = -1,
        AutoPanModeOff = 0,
        AutoPanModeRecenter = 1,
        AutoPanModeNavigation = 2,
        AutoPanModeCompassNavigation = 3
    };
    Q_ENUM(AutoPanMode)

    enum class Mode {
        ModeDefault = 0,
        ModeFlag = 1,
        ModeField = 2,
    };
    Q_ENUM(Mode)

    Q_PROPERTY(Esri::ArcGISRuntime::MapQuickView* mapView READ mapView WRITE setMapView NOTIFY mapViewChanged)
    Q_PROPERTY(MapLayerListModel* layers READ layers CONSTANT)
    Q_PROPERTY(double rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(AutoPanMode autoPanMode READ autoPanMode WRITE setAutoPanMode NOTIFY autoPanModeChanged)
    Q_PROPERTY(QVariantMap centerPoint READ centerPoint NOTIFY centerPointChanged)

    QML_WRITABLE_AUTO_PROPERTY(Mode, mode)
    QML_WRITABLE_AUTO_PROPERTY(QVariantMap, flagLocation)
    QML_WRITABLE_AUTO_PROPERTY(bool, hideDataLayers)
    QML_WRITABLE_AUTO_PROPERTY(QVariantMap, initialExtent)
    QML_WRITABLE_AUTO_PROPERTY(AutoPanMode, initialPanMode)

    QML_WRITABLE_AUTO_PROPERTY(QString, recordUid)
    QML_WRITABLE_AUTO_PROPERTY(QString, fieldUid)

    QML_WRITABLE_AUTO_PROPERTY(QVariantMap, fieldValue)

    QML_READONLY_AUTO_PROPERTY(SightingFilter*, filter)

public:
    explicit MapController(QObject *parent = nullptr);
    ~MapController() override;

    void classBegin() override;
    void componentComplete() override;

    double rotation() const;
    void setRotation(double value);
    AutoPanMode autoPanMode() const;
    void setAutoPanMode(AutoPanMode value);
    QVariantMap centerPoint();

    Q_INVOKABLE void locationStart();
    Q_INVOKABLE void rebuild();
    Q_INVOKABLE void centerOnFlagLocation();
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();

signals:
    void mapViewChanged();
    void requestLocationPermission();
    void rotationChanged();
    void autoPanModeChanged();
    void centerPointChanged();
    void inspectClicked(const QVariantList& results);
    void pointClicked(double x, double y);
    void longPress(double x, double y);

private slots:
    void loaded();

private:
    bool m_completed = false;
    bool m_loaded = false;
    bool m_longPress = false;
    bool m_formTrackVisible = false;
    double m_minScale = 0;
    bool m_locationStarted = false;
    bool m_heatmapActive = false;
    QVariantList m_paths;
    QVariantList findPath(int pathId) const;

    QObject* m_layerCleanup = nullptr;
    QMap<QString, QVariantMap> m_layerExtents;

    Esri::ArcGISRuntime::Map* m_map = nullptr;
    Esri::ArcGISRuntime::MapQuickView* m_mapView = nullptr;
    Esri::ArcGISRuntime::MapQuickView* mapView() const;
    void setMapView(Esri::ArcGISRuntime::MapQuickView* mapView);

    Esri::ArcGISRuntime::GraphicsOverlay* m_gotoLinesOverlay = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_gotoPointsOverlay = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_trackOverlay = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_heatmapOverlay = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_formTrackOverlay = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_sightingOverlay = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_sightingTrackOverlay = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_gotoTargetOverlay1 = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_gotoTargetOverlay2 = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_flagOverlay = nullptr;
    Esri::ArcGISRuntime::GraphicsOverlay* m_fieldOverlay = nullptr;
    QList<Esri::ArcGISRuntime::GraphicsOverlay*> m_formLayerOverlays;

    MapLayerListModel* m_layers = nullptr;
    MapLayerListModel* layers() const;

    QMap<QString, Esri::ArcGISRuntime::Layer*> m_layerLookup;

    void rebuildBasemap();
    void rebuildOfflineLayers();
    void rebuildSightingLayer(bool showSightings, bool showTracks);
    void rebuildTrackLayer();
    void rebuildDataLayers();
    void rebuildGotoTarget();
    void rebuildFlagOverlay();
    void rebuildFieldOverlay();
    void rebuildFormLayers();

    void resetHeatmap();
    void addToHeatmap(double x, double y, const Esri::ArcGISRuntime::SpatialReference& spatialReference);
};

} // namespace CyberTracker
