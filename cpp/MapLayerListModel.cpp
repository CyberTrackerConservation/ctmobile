#include "MapLayerListModel.h"
#include "App.h"

MapLayerListModel::MapLayerListModel(QObject* parent): VariantListModel(parent)
{
    connect(App::instance()->settings(), &Settings::activeBasemapChanged, this, &MapLayerListModel::rebuild);
    connect(App::instance()->settings(), &Settings::activeProjectLayersChanged, this, &MapLayerListModel::rebuild);
    connect(App::instance()->projectManager(), &ProjectManager::projectSettingChanged, this, [&](const QString& /*projectUid*/, const QString& name, const QVariant& /*value*/)
    {
        if (name == "activeProjectLayers")
        {
            rebuild();
        }
    });
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

int MapLayerListModel::layerIdToIndex(const QString& layerId) const
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

void MapLayerListModel::setChecked(const QString& layerId, bool checked)
{
    auto index = layerIdToIndex(layerId);
    if (index != -1)
    {
        auto layer = get(index).toMap();
        layer["checked"] = checked;
        replace(index, layer);
    }
}

void MapLayerListModel::setBasemap(const QString& basemapId)
{
    auto app = App::instance();
    auto basemap = basemapId.isEmpty() ? "BasemapOpenStreetMap" : basemapId;

    m_blockRebuild = true;
    app->settings()->set_activeBasemap(basemap);
    m_blockRebuild = false;
}

void MapLayerListModel::setProjectLayerState(const QString& layerId, bool checked)
{
    auto app = App::instance();
    auto layers = app->settings()->activeProjectLayers();
    layers[layerId] = checked;

    m_blockRebuild = true;
    app->settings()->set_activeProjectLayers(layers);
    m_blockRebuild = false;

    setChecked(layerId, checked);
}

void MapLayerListModel::setDataLayerState(const QString& layerId, bool checked)
{
    auto form = Form::parentForm(this);
    if (!form)
    {
        return;
    }

    auto project = form->project();

    auto layers = project->activeProjectLayers();
    layers[layerId] = checked;

    m_blockRebuild = true;
    project->set_activeProjectLayers(layers);
    m_blockRebuild = false;

    setChecked(layerId, checked);
}

void MapLayerListModel::removeGotoLayer(const QString& layerId)
{
    App::instance()->gotoManager()->removeLayer(layerId);

    rebuild();
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
    auto app = App::instance();
    auto form = Form::parentForm(this);
    auto project = form ? form->project() : nullptr;
    auto settings = app->settings();

    auto activeBasemap = settings->activeBasemap();
    auto activeProjectLayers = settings->activeProjectLayers();
    auto activeFormLayers = project ? project->activeProjectLayers() : QVariantMap();

    // Build the layers.
    auto layerList = QVariantList();
    layerList.append(buildFormLayers());
    layerList.append(buildGotoLayers());
    layerList.append(buildProjectLayers(app->projectManager(), project ? project->uid() : ""));
    layerList.append(buildBaseLayers());

    // Populate the "checked" and "divider" attributes.
    auto lastLayerType = QString();
    for (auto i = 0; i < layerList.count(); i++)
    {
        auto layer = layerList[i].toMap();
        auto layerId = layer.value("id").toString();
        auto layerType = layer.value("type").toString();
        auto layerChecked = false;

        if (layerType == "basemap")
        {
            layerChecked = layerId == activeBasemap;
        }
        else if (layerType == "project" || layerType == "goto")
        {
            layerChecked = activeProjectLayers.value(layerId, true).toBool();
        }
        else if (layerType == "form")
        {
            layerChecked = activeFormLayers.value(layerId, true).toBool();
        }

        // Set the checked property.
        layer["checked"] = layerChecked;
        layerList[i] = layer;

        // Add a divider.
        if (!lastLayerType.isEmpty() && lastLayerType != layerType)
        {
            auto lastLayer = layerList[i - 1].toMap();
            lastLayer["divider"] = true;
            layerList[i - 1] = lastLayer;
        }

        lastLayerType = layerType;
    }

    appendList(layerList);

    emit changed();
}

QVariantList MapLayerListModel::buildBaseLayers()
{
    auto layerList = QVariantList();

    auto addMap = [&](const QString& name, const QString& id) {
        QVariantMap layer;
        layer["name"] = name;
        layer["type"] = "basemap";
        layer["id"] = id;
        layerList.append(layer);
    };

    addMap(tr("None"), "BasemapNone");
    addMap("OpenStreetMap", "BasemapOpenStreetMap");
    addMap("OpenStreetMap - " + tr("Humanitarian"), "BasemapHumanitarian");
    addMap("OpenTopoMap", "BasemapOpenTopoMap");
    addMap("National Geographic", "BasemapNatGeo");
    addMap(tr("Topographic"), "BasemapTopographic");
    addMap(tr("Streets"), "BasemapStreets");
    //addMap(tr("Streets (vector)"), "BasemapStreetsVector");
    //addMap(tr("Streets night (vector)"), "BasemapStreetsNightVector");
    addMap(tr("Imagery"), "BasemapImagery");
    addMap(tr("Dark gray canvas"), "BasemapDarkGrayCanvas");
    //addMap(tr("Dark gray canvas (vector)"), "BasemapDarkGrayCanvasVector");
    addMap(tr("Light gray canvas"), "BasemapLightGrayCanvas");
    //addMap(tr("Light gray canvas (vector)"), "BasemapLightGrayCanvasVector");
    //addMap(tr("Navigation (vector)"), "BasemapNavigationVector");
    addMap(tr("Oceans"), "BasemapOceans");

    return layerList;
}

QVariantList MapLayerListModel::buildProjectLayers(ProjectManager* projectManager, const QString& filterProjectUid)
{
    auto layerList = QVariantList();

    // Get the offline layers from all projects.
    auto projects = projectManager->buildList();
    for (auto it = projects.constBegin(); it != projects.constEnd(); it++)
    {
        auto project = it->get();
        if (filterProjectUid != project->uid())
        {
            continue;
        }

        auto maps = project->maps();
        if (maps.isEmpty())
        {
            continue;
        }

        for (auto it = maps.constBegin(); it != maps.constEnd(); it++)
        {
            auto mapItem = it->toMap();
            auto layer = QVariantMap({{"type", "project"}});

            if (mapItem.contains("file"))
            {
                auto file = mapItem.value("file").toString();
                auto path = projectManager->getFilePath(project->uid(), file);
                if (!QFile::exists(path))
                {
                    qDebug() << "Map not found:" << file;
                    continue;
                }

                layer["id"] = path;
                layer["name"] = mapItem.value("name");
                layer["format"] = QFileInfo(path).suffix();
            }
            else if (mapItem.contains("service"))
            {
                layer["id"] = mapItem["layers"].toString() + "_" + mapItem["service"].toString();
                layer["url"] = mapItem["service"];
                layer["format"] = mapItem.value("type");
                layer["name"] = mapItem.value("layers");
            }
            else
            {
                qDebug() << "Unknown map layer type";
                continue;
            }

            layerList.append(layer);
        }
    }

    // Get offline layers from the file system.
    auto searchPaths = QStringList();
    searchPaths.append(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation));

#if defined(Q_OS_ANDROID)
    for (auto path: QStringList({ "/sdcard/Download", Utils::androidGetExternalFilesDir(), Utils::androidGetExternalFilesDir() + "/Download" }))
    {
        searchPaths << path;
    }
#elif defined(Q_OS_IOS)
    searchPaths.append(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation));
#endif

    auto dirToBeScanned = QDir();
    auto filterExtensions = QStringList({"*.tpk", "*.vtpk", "*.mbtiles", "*.shp", "*.kml", "*.tif", "*.tiff", "*.geotiff"});

    for (auto it = searchPaths.constBegin(); it != searchPaths.constEnd(); it++)
    {
        dirToBeScanned.setPath(*it);
        auto availableFilteredFiles = QFileInfoList(dirToBeScanned.entryInfoList(filterExtensions, QDir::Files));
        for (auto it = availableFilteredFiles.constBegin(); it != availableFilteredFiles.constEnd(); it++)
        {
            auto fileInfo = *it;
            auto layerId = fileInfo.filePath();

            auto layer = QVariantMap();

            auto name = fileInfo.baseName();
            auto tpkIndex = name.indexOf("_tpk_");
            if (tpkIndex > 1)
            {
                name = name.left(tpkIndex);
            }

            layer["name"] = name;
            layer["type"] = "project";
            layer["format"] = fileInfo.suffix();
            layer["id"] = layerId;

            layerList.append(layer);
        }
    }

    return layerList;
}

QVariantList MapLayerListModel::buildGotoLayers()
{
    auto layerList = QVariantList();
    auto app = App::instance();

    // Get the offline layers from all projects.
    auto gotoLayers = app->gotoManager()->getLayers();
    for (auto it = gotoLayers.constBegin(); it != gotoLayers.constEnd(); it++)
    {
        auto gotoLayerMap = it->toMap();
        auto layer = QVariantMap();
        layer["name"] = gotoLayerMap.value("name");
        layer["type"] = "goto";
        layer["points"] = gotoLayerMap.value("points");
        layer["polylines"] = gotoLayerMap.value("polylines");
        layer["extent"] = gotoLayerMap.value("extent");

        auto id = gotoLayerMap.value("id").toString();
        layer["id"] = id;

        layerList.append(layer);
    }

    return layerList;
}

QVariantList MapLayerListModel::buildFormLayers()
{
    auto form = Form::parentForm(this);
    if (!form)
    {
        return QVariantList();
    }

    return form->provider()->buildMapDataLayers();
}
