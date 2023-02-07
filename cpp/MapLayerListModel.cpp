#include "MapLayerListModel.h"
#include "App.h"

MapLayerListModel::MapLayerListModel(QObject* parent): VariantListModel(parent)
{
    m_addTypeHeaders = m_addBaseLayers = m_addOfflineLayers = m_addProjectLayers = m_addFormLayers = true;

    connect(App::instance()->settings(), &Settings::activeBasemapChanged, this, &MapLayerListModel::rebuild);
    connect(App::instance()->offlineMapManager(), &OfflineMapManager::layersChanged, this, &MapLayerListModel::rebuild);
    connect(App::instance()->projectManager(), &ProjectManager::projectSettingChanged, this, [&](const QString& /*projectUid*/, const QString& name, const QVariant& /*value*/)
    {
        if (name == "activeMapLayers")
        {
            rebuild();
        }
    });

    // Rebuild the data layers.
    connect(App::instance(), &App::sightingSaved, this, &MapLayerListModel::rebuild);
    connect(App::instance(), &App::sightingRemoved, this, &MapLayerListModel::rebuild);
}

MapLayerListModel::~MapLayerListModel()
{
}

void MapLayerListModel::classBegin()
{
}

void MapLayerListModel::componentComplete()
{
    m_completed = true;
    rebuild();
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

QVariantMap MapLayerListModel::findExtent(const QString& layerId) const
{
    auto index = indexOfLayer(layerId);
    if (index == -1)
    {
        return QVariantMap();
    }

    auto layer = get(index).toMap();

    return layer["extent"].toMap();
}

QVariantList MapLayerListModel::findPath(int pathId) const
{
    qFatalIf(pathId < 0 || pathId >= m_paths.count(), "Bad path index");
    return m_paths[pathId].toList();
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

    if (layerType == "offline")
    {
        App::instance()->offlineMapManager()->setLayerActive(layerId, checked);
    }
    else if (layerType == "form")
    {
        auto form = Form::parentForm(this);
        if (!form)
        {
            return;
        }

        auto project = form->project();
        auto layers = project->activeMapLayers();
        layers[layerId] = checked;
        project->set_activeMapLayers(layers);
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

    clear();

    if (!m_completed)
    {
        return;
    }

    // Pick up the default
    auto form = Form::parentForm(this);
    auto project = form ? form->project() : nullptr;

    // Build the layers.
    auto layerList = QVariantList();

    auto appendHeader = [&](const QString& title, bool config = false)
    {
        if (m_addTypeHeaders)
        {
            layerList.append(QVariantMap {{ "type", "title" }, { "name", title }, { "config", config }, { "divider", true } });
        }
    };

    auto appendLayers = [&](const QVariantList& layers)
    {
        for (auto layerIt = layers.constBegin(); layerIt != layers.constEnd(); layerIt++)
        {
            layerList.append(*layerIt);
        }
    };

    // Form layers.
    if (m_addFormLayers && project)
    {
        appendHeader(tr("Data layers"));
        appendLayers(buildFormLayers(project));
    }

    // Offline layers.
    if (m_addOfflineLayers)
    {
        auto layers = buildOfflineLayers();
        auto kioskMode = App::instance()->kioskMode();

        // Add config button, but not in kiosk mode.
        if (!layers.isEmpty() || !kioskMode)
        {
            appendHeader(tr("Offline layers"), !kioskMode);
            appendLayers(layers);
        }
    }

    // Base map.
    if (m_addBaseLayers)
    {
        appendHeader(tr("Base map"));
        appendLayers(buildBaseLayers());
    }

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
        results.append(QVariantMap
        {
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
    addMap("BasemapStreetsVector", tr("Streets (vector)"));
    addMap("BasemapStreetsNightVector", tr("Streets night (vector)"));
    addMap("BasemapImagery", tr("Imagery"));
    addMap("BasemapDarkGrayCanvas", tr("Dark gray canvas"));
    addMap("BasemapDarkGrayCanvasVector", tr("Dark gray canvas (vector)"));
    addMap("BasemapLightGrayCanvas", tr("Light gray canvas"));
    addMap("BasemapLightGrayCanvasVector", tr("Light gray canvas (vector)"));
    addMap("BasemapNavigationVector", tr("Navigation (vector)"));
    addMap("BasemapOceans", tr("Oceans"));

    return results;
}

QVariantList MapLayerListModel::buildOfflineLayers()
{
    auto results = QVariantList();
    auto offlineMapManager = App::instance()->offlineMapManager();

    m_paths.clear();

    auto layerIds = offlineMapManager->getLayers();
    for (auto layerIt = layerIds.constBegin(); layerIt != layerIds.constEnd(); layerIt++)
    {
        auto layerId = *layerIt;
        auto filePath = offlineMapManager->getLayerFilePath(layerId);
        auto fileInfo = QFileInfo(layerId);
        auto format = fileInfo.suffix().toLower();
        auto vector = Utils::isMapLayerVector(fileInfo.suffix());
        auto timestamp = Utils::decodeTimestamp(offlineMapManager->getLayerTimestamp(layerId));

        auto layer = QVariantMap
        {
            { "id", layerId },
            { "filePath", filePath },
            { "name", offlineMapManager->getLayerName(layerId) },
            { "format", format },
            { "vector", vector },
            { "type", "offline" },
            { "timestamp", App::instance()->formatDateTime(timestamp) }
        };

        if (format == "geojson")
        {
            auto symbolScale = App::instance()->scaleByFontSize(1);
            auto outlineColor = App::instance()->settings()->darkTheme() ? Qt::white : Qt::black;
            layer.insert(Utils::esriLayerFromGeoJson(filePath, symbolScale, outlineColor, &m_paths));
        }
        else if (format == "wms")
        {
            auto wmsData = Utils::variantMapFromJsonFile(filePath);
            layer.insert(wmsData);
        }

        layer["checked"] = offlineMapManager->getLayerActive(layerId);
        layer["opacity"] = offlineMapManager->getLayerOpacity(layerId);

        results.append(layer);
    }

    return results;
}

QVariantList MapLayerListModel::buildFormLayers(Project* project) const
{
    auto form = Form::parentForm(this);
    qFatalIf(!form, "Project must have come from a form");

    auto results = QVariantList();
    auto layers = form->provider()->buildMapDataLayers();

    for (auto it = layers.begin(); it != layers.end(); it++)
    {
        auto layer = it->toMap();
        layer["type"] = "form";
        layer["checked"] = project->activeMapLayers().value(layer["id"].toString(), true).toBool();

        results.append(layer);
    }

    return results;
}
