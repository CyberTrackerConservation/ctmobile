#pragma once
#include "pch.h"

namespace CyberTracker {

class MapLayerListModel : public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES (QQmlParserStatus)

    QML_WRITABLE_AUTO_PROPERTY(bool, addTypeHeaders)
    QML_WRITABLE_AUTO_PROPERTY(bool, addBaseLayers)
    QML_WRITABLE_AUTO_PROPERTY(bool, addOfflineLayers)
    QML_WRITABLE_AUTO_PROPERTY(bool, addDataLayers)

    QML_READONLY_AUTO_PROPERTY(bool, hasLookupLayers)

public:
    static constexpr char DATA_LAYER_TYPE[] = "data";
    static constexpr char OFFLINE_LAYER_TYPE[] = "offline";
    static constexpr char SIGHTING_LAYER_ID[] = "sightings";
    static constexpr char LOCATION_LAYER_ID[] = "tracks";

public:
    MapLayerListModel(QObject* parent = nullptr);
    ~MapLayerListModel();

    void classBegin() override;
    void componentComplete() override;

    Q_INVOKABLE int indexOfLayer(const QString& layerId) const;

    Q_INVOKABLE void setBasemap(const QString& basemapId);
    Q_INVOKABLE void setLayerState(const QVariantMap& layer, bool checked);

    void enumActive(const QString& filterType, const std::function<void(const QVariantMap& layer)> callback) const;

signals:
    void changed();

private slots:
    void rebuild();

private:
    bool m_completed = false;
    bool m_blockRebuild = false;

    QVariantList buildBaseLayers() const;
    QVariantList buildOfflineLayers();
    QVariantList buildDataLayers() const;
};

} // namespace CyberTracker