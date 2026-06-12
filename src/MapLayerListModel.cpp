#include "MapLayerListModel.h"
#include "App.h"

namespace CyberTracker {

MapLayerListModel::MapLayerListModel(QObject* parent): VariantListModel(parent)
{
    m_addTypeHeaders = m_addBaseLayers = m_addOfflineLayers = m_addDataLayers = true;
    m_hasLookupLayers = false;
}

MapLayerListModel::~MapLayerListModel()
{
}

void MapLayerListModel::classBegin()
{
}

void MapLayerListModel::componentComplete()
{
    connect(App::instance()->settings(), &Settings::activeBasemapChanged, this, &MapLayerListModel::rebuild);
    connect(App::instance()->offlineMapManager(), &OfflineMapManager::layersChanged, this, &MapLayerListModel::rebuild);
    connect(App::instance()->settings(), &Settings::dataMapLayersChanged, this, &MapLayerListModel::rebuild);

    m_completed = true;

    rebuild();
}

void MapLayerListModel::enumActive(const QString& filterType, const std::function<void(const QVariantMap& layer)> callback) const
{
    for (auto i = 0; i < count(); i++)
    {
        auto layer = get(i).toMap();
        auto type = layer["type"].toString();
        if ((filterType.isEmpty() || (type == filterType)) && layer["checked"].toBool())
        {
            callback(layer);
        }
    }
}

int MapLayerListModel::indexOfLayer(const QString& layerId) const
{
    for (auto i = 0; i < count(); i++)
    {
        auto layer = get(i).toMap();
        if (layer.value("id") == layerId)
        {
            return i;
        }
    }

    return -1;
}

void MapLayerListModel::setBasemap(const QString& basemapId)
{
    auto app = App::instance();
    auto basemap = basemapId.isEmpty() ? "BasemapOpenStreetMap" : basemapId;

    m_blockRebuild = true;
    app->settings()->set_activeBasemap(basemap);
    m_blockRebuild = false;
}

void MapLayerListModel::setLayerState(const QVariantMap& layer, bool checked)
{
    auto layerId = layer["id"].toString();
    auto layerType = layer["type"].toString();

    auto layerIndex = indexOfLayer(layerId);
    if (layerIndex == -1)
    {
        qDebug() << "Error: unknown layerid" << layer;
        return;
    }

    m_blockRebuild = true;
    auto removeBlock = qScopeGuard([&] { m_blockRebuild = false; });

    if (layerType == OFFLINE_LAYER_TYPE)
    {
        App::instance()->offlineMapManager()->setLayerActive(layerId, checked);
    }
    else if (layerType == DATA_LAYER_TYPE)
    {
        auto layers = App::instance()->settings()->dataMapLayers();
        layers[layerId] = checked;
        App::instance()->settings()->set_dataMapLayers(layers);
    }

    m_blockRebuild = false;

    auto item = get(layerIndex).toMap();
    item["checked"] = checked;
    replace(layerIndex, item);
}

void MapLayerListModel::rebuild()
{
    if (m_blockRebuild)
    {
        return;
    }

    update_hasLookupLayers(false);

    clear();

    if (!m_completed)
    {
        return;
    }

    // Build the layers.
    auto layerList = QVariantList();

    auto appendHeader = [&](const QString& title, const QString& configIcon = QString(), const QString& configPage = QString())
    {
        if (m_addTypeHeaders)
        {
            layerList.append(QVariantMap {{ "type", "title" }, { "name", title }, { "configIcon", configIcon }, { "configPage", configPage }, { "divider", true } });
        }
    };

    auto appendLayers = [&](const QVariantList& layers)
    {
        for (auto layerIt = layers.constBegin(); layerIt != layers.constEnd(); layerIt++)
        {
            layerList.append(*layerIt);
        }
    };

    // Data layers.
    if (m_addDataLayers)
    {
        appendHeader(tr("Data layers"), "qrc:/icons/filter_outline.svg", "qrc:/SightingFilterPage.qml");
        appendLayers(buildDataLayers());
    }

    // Offline layers.
    if (m_addOfflineLayers)
    {
        auto layers = buildOfflineLayers();
        auto kioskMode = App::instance()->kioskMode();

        // Add config button, but not in kiosk mode.
        if (!layers.isEmpty() || !kioskMode)
        {
            appendHeader(tr("Offline layers"), kioskMode ? QString() : "qrc:/icons/settings_outline.svg", "qrc:/OfflineMapPage.qml");
            appendLayers(layers);
        }
    }

    // Base map.
    if (m_addBaseLayers)
    {
        appendHeader(tr("Base map"));
        appendLayers(buildBaseLayers());
    }

    // Populate hasLookupLayers.
    update_hasLookupLayers(App::instance()->offlineMapManager()->hasLookupLayers());

    appendList(layerList);

    emit changed();
}

QVariantList MapLayerListModel::buildBaseLayers() const
{
    auto results = QVariantList();

    auto activeBasemap = App::instance()->settings()->activeBasemap();
    if (activeBasemap.isEmpty())
    {
        // Default.
        activeBasemap = "BasemapNone";
    }

    auto addMap = [&](const QString& id, const QString& name)
    {
        results.append(QVariantMap {
            { "id", id },
            { "name", name },
            { "type", "basemap" },
            { "checked", activeBasemap == id },
        });
    };

    addMap("BasemapNone", tr("None"));
    addMap("BasemapOpenStreetMap", "OpenStreetMap");
    addMap("BasemapHumanitarian", "OpenStreetMap - " + tr("Humanitarian"));
    addMap("BasemapOpenTopoMap", "OpenTopoMap");
    addMap("BasemapNatGeo", "National Geographic");
    addMap("BasemapTopographic", tr("Topographic"));
    addMap("BasemapStreets", tr("Streets"));
    addMap("BasemapImagery", tr("Imagery"));
    addMap("BasemapDarkGrayCanvas", tr("Dark gray canvas"));
    addMap("BasemapLightGrayCanvas", tr("Light gray canvas"));
    addMap("BasemapNavigationVector", tr("Navigation (vector)"));
    addMap("BasemapOceans", tr("Oceans"));

    return results;
}

QVariantList MapLayerListModel::buildOfflineLayers()
{
    auto results = QVariantList();
    auto offlineMapManager = App::instance()->offlineMapManager();

    auto layerIds = offlineMapManager->getLayers();
    for (auto layerIt = layerIds.constBegin(); layerIt != layerIds.constEnd(); layerIt++)
    {
        auto layerId = *layerIt;
        auto layer = offlineMapManager->getLayer(layerId);
        auto package = offlineMapManager->getPackage(layer.packageId);
        auto filePath = offlineMapManager->layersPath() + "/" + package.folder + "/" + layer.filename;
        auto fileInfo = QFileInfo(filePath);
        auto format = fileInfo.suffix().toLower();
        auto vector = Utils::isMapLayerVector(fileInfo.suffix());
        auto timestamp = Utils::decodeTimestamp(package.lastUpdate);

        auto entry = QVariantMap
        {
            { "id", layerId },
            { "filePath", filePath },
            { "name", layer.name },
            { "format", format },
            { "vector", vector },
            { "type", OFFLINE_LAYER_TYPE },
            { "timestamp", App::instance()->formatDateTime(timestamp) },
            { "lookup", layer.lookup },
            { "symbol", layer.symbol },
            { "checked", layer.active },
            { "opacity", layer.opacity },
        };

        results.append(entry);
    }

    // Vector layers always go on top of raster layers.
    // stable_partition so the user defined order does not change.
    std::stable_partition(results.begin(), results.end(), [](const QVariant& item)
    {
        return !item.toMap().value("vector").toBool();
    });

    return results;
}

QVariantList MapLayerListModel::buildDataLayers() const
{
    auto result = QVariantList();

    auto layerState = App::instance()->settings()->dataMapLayers();

    result.append(QVariantMap {
        { "id", SIGHTING_LAYER_ID },
        { "name", tr("Sightings") },
        { "type", DATA_LAYER_TYPE },
        { "checked", layerState.value(SIGHTING_LAYER_ID, true).toBool() },
    });

    result.append(QVariantMap {
        { "id", LOCATION_LAYER_ID },
        { "name", tr("Tracks") },
        { "type", DATA_LAYER_TYPE },
        { "checked", layerState.value(LOCATION_LAYER_ID, true).toBool() },
    });

    return result;
}

} // namespace CyberTracker
