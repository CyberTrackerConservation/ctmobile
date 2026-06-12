#include "OfflineMap.h"
#include "App.h"

namespace CyberTracker {

namespace {

static constexpr char PACKAGE_JSON[] = "package.json";
static constexpr char LAYERS_JSON[] = "layers.json";

static constexpr char LEGACY_LAYER_JSON[] = "__layer.json";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StagingFolder

class StagingFolder
{
public:
    QString path;

    StagingFolder()
    {
        path = QDir::cleanPath(App::instance()->tempPath() + "/OfflineMap");
        QDir(path).removeRecursively();
        qFatalIf(!Utils::ensurePath(path), "Failed to create staging folder");
    }

    ~StagingFolder()
    {
        QDir(path).removeRecursively();
    }

    QString makePath(const QString& filename)
    {
        return path + "/" + filename;
    }
};

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineMapManager

OfflineMapManager::OfflineMapManager(QObject *parent): QObject{parent}
{
    qFatalIf(true, "Singleton");
}

OfflineMapManager::OfflineMapManager(const QString& offlineMapsPath, QObject *parent): QObject(parent)
{
    m_updatesAvailable = false;
    m_hasUpdateableMaps = false;
    m_layersPath = offlineMapsPath;
    qFatalIf(!Utils::ensurePath(m_layersPath), "Failed to create offline map layers folder");

    migrateLegacyFolders();
    recomputeHasUpdateableMaps();

    connect(this, &OfflineMapManager::layersChanged, this, &OfflineMapManager::recomputeHasUpdateableMaps);
}

OfflineMapManager::~OfflineMapManager()
{
}

QStringList OfflineMapManager::findLayersByPackageId(const QString& packageId) const
{
    auto result = QStringList();
    App::instance()->database()->enumMapLayersByPackage(packageId, [&](const Database::DbMapLayer& layer)
    {
        result.append(layer.id);
    });
    return result;
}

bool OfflineMapManager::findPackageByUrl(const QString& offlineMapUrl, Database::DbMapPackage* packageOut) const
{
    *packageOut = Database::DbMapPackage();

    auto found = false;
    App::instance()->database()->enumMapPackages([&](const Database::DbMapPackage& package)
    {
        if (!found && package.updateUrl == offlineMapUrl)
        {
            *packageOut = package;
            found = true;
        }
    });
    return found;
}

void OfflineMapManager::recomputeHasUpdateableMaps()
{
    auto any = false;
    App::instance()->database()->enumMapPackages([&](const Database::DbMapPackage& package)
    {
        if (!any && !package.updateUrl.isEmpty())
        {
            any = true;
        }
    });
    update_hasUpdateableMaps(any);
}

void OfflineMapManager::removePackage(const Database::DbMapPackage& package)
{
    if (!package.folder.isEmpty())
    {
        QDir(m_layersPath + "/" + package.folder).removeRecursively();
    }

    auto database = App::instance()->database();
    database->deleteMapLayersByPackage(package.id);
    database->deleteMapPackage(package.id);
}

void OfflineMapManager::migrateLegacyFolders()
{
    Utils::enumFolders(m_layersPath, [&](const QFileInfo& fileInfo)
    {
        auto folderPath = m_layersPath + "/" + fileInfo.fileName();
        auto layerJsonPath = folderPath + "/" + LEGACY_LAYER_JSON;
        if (!QFile::exists(layerJsonPath))
        {
            return;
        }

        auto legacyLayer = Utils::variantMapFromJsonFile(layerJsonPath);
        Utils::variantListToJsonFile(folderPath + "/" + LAYERS_JSON, QVariantList { legacyLayer });
        QFile::remove(layerJsonPath);

        // Do not migrate legacy legal atlas layers.
        if (legacyLayer["legalAtlas"].toBool())
        {
            QDir(folderPath).removeRecursively();
            return;
        }

        normalizePackage(folderPath);
        installStagedPackage(folderPath);
        QDir(folderPath).removeRecursively();
    });

    // Discard the legacy Settings keys now that the on-disk state has been migrated.
    auto settings = App::instance()->settings();
    settings->setSetting("offlineMapLayers", QVariant());
    settings->setSetting("offlineMapPackages", QVariant());
}

void OfflineMapManager::normalizePackage(const QString& stagingPath) const
{
    auto packageFilePath = stagingPath + "/" + PACKAGE_JSON;
    auto layersFilePath = stagingPath + "/" + LAYERS_JSON;

    // Convert SMART route file to a standard layers.json.
    auto projectJsonFilePath = stagingPath + "/project.json";
    if (QFile::exists(projectJsonFilePath))
    {
        auto projectMap = Utils::variantMapFromJsonFile(projectJsonFilePath);
        if (projectMap.contains("decoder") && projectMap.contains("projectName") && projectMap.contains("definition"))
        {
            auto layerJsonFilePath = stagingPath + "/" + projectMap["definition"].toString();
            if (QFile::exists(layerJsonFilePath))
            {
                auto layerMap = Utils::variantMapFromJsonFile(layerJsonFilePath);
                layerMap["name"] = layerMap.value("name", projectMap["projectName"].toString());

                auto layerGeoJsonFilePath = stagingPath + "/" + QFileInfo(layerJsonFilePath).baseName() + ".geojson";
                Utils::writeJsonToFile(layerGeoJsonFilePath, QJsonDocument(QJsonObject::fromVariantMap(layerMap)).toJson());

                auto layer = QVariantMap();
                layer["filename"] = QFileInfo(layerGeoJsonFilePath).fileName();
                layer["name"] = layerMap["name"].toString();

                Utils::variantListToJsonFile(layersFilePath, QVariantList { layer });

                QFile::remove(layerJsonFilePath);
            }
        }

        QFile::remove(projectJsonFilePath);
    }

    // Read or build the layer list.
    auto layers = QVariantList();
    if (QFile::exists(layersFilePath))
    {
        layers = Utils::variantListFromJsonFile(layersFilePath);
    }
    else
    {
        const auto files = Utils::buildMapLayerList(stagingPath);
        for (const auto& file : files)
        {
            auto fileInfo = QFileInfo(file);
            auto layer = QVariantMap();
            layer["filename"] = fileInfo.fileName();
            layer["name"] = fileInfo.baseName();

            // Classic world layer should be inactive by default.
            if (fileInfo.fileName() == "World.ecw")
            {
                layer["active"] = false;
            }

            layers.append(layer);
        }
    }

    // Read or synthesise package.json.
    auto package = QVariantMap();
    if (QFile::exists(packageFilePath))
    {
        package = Utils::variantMapFromJsonFile(packageFilePath);
    }

    if (package.value("id").toString().isEmpty())
    {
        package["id"] = Utils::createUuid();
    }

    Utils::variantMapToJsonFile(packageFilePath, package);
    Utils::variantListToJsonFile(layersFilePath, layers);
}

ApiResult OfflineMapManager::installStagedPackage(const QString& stagingPath, const QString& projectUid, const QString& updateUrl, const QString& eTag)
{
    auto database = App::instance()->database();

    auto package = Utils::variantMapFromJsonFile(stagingPath + "/" + PACKAGE_JSON);
    auto packageId = package["id"].toString();
    qFatalIf(packageId.isEmpty(), "normalizePackage must produce a package id");

    const auto layers = Utils::variantListFromJsonFile(stagingPath + "/" + LAYERS_JSON);
    if (layers.isEmpty())
    {
        return ApiResult::Error(tr("No maps found"));
    }

    // Replace any existing package with the same id (handles re-install of stably identified packages).
    auto existing = Database::DbMapPackage();
    if (database->loadMapPackage(packageId, &existing))
    {
        removePackage(existing);
    }

    // Allocate the on-disk folder.
    auto folder = Utils::createUuid();
    auto folderPath = m_layersPath + "/" + folder;
    qFatalIf(!Utils::ensurePath(folderPath), "Failed to create package folder");

    // Move every staging file into the package folder.
    Utils::enumFiles(stagingPath, [&](const QFileInfo& fileInfo)
    {
        QFile::rename(fileInfo.filePath(), folderPath + "/" + fileInfo.fileName());
    });

    // Persist package + layers.
    auto dbPackage = Database::DbMapPackage();
    dbPackage.id = packageId;
    dbPackage.folder = folder;
    dbPackage.projectUid = projectUid;
    dbPackage.name = package.value("name").toString();
    dbPackage.version = package.value("version").toString();
    dbPackage.updateUrl = updateUrl;
    dbPackage.eTag = eTag;
    dbPackage.lastUpdate = App::instance()->timeManager()->currentDateTimeISO();

    auto sequence = database->getMaxMapLayerSequence() + 1;

    auto addedNames = QStringList();

    database->transaction([&]() {
        database->saveMapPackage(dbPackage);

        for (const auto& layerV : layers)
        {
            auto layerMap = layerV.toMap();

            auto dbLayer = Database::DbMapLayer();
            dbLayer.id = Utils::createUuid();
            dbLayer.packageId = packageId;
            dbLayer.filename = layerMap.value("filename").toString();
            dbLayer.name = layerMap.value("name").toString();
            dbLayer.category = layerMap.value("category").toString();
            dbLayer.active = layerMap.value("active", true).toBool();
            dbLayer.opacity = layerMap.value("opacity", 1.0).toDouble();
            dbLayer.symbol = layerMap.value("symbol").toMap();
            dbLayer.lookup = layerMap.value("lookup", false).toBool();
            dbLayer.sequence = sequence++;

            database->saveMapLayer(dbLayer);
            addedNames.append(dbLayer.name);
        }

        return true;
    });

    for (const auto& name : std::as_const(addedNames))
    {
        emit layerAdded(name);
    }

    emit layersChanged();

    return ApiResult::Success();
}

void OfflineMapManager::installClassicLayers()
{
    auto classicRoot = Utils::classicRoot();
    if (!QDir(classicRoot).exists())
    {
        return;
    }

    // Install them individually so they can be deleted individually.
    const auto ecwFiles = QDir(classicRoot).entryList({"*.ecw"}, QDir::Files);
    for (const auto& fileName : ecwFiles)
    {
        // Phase 1: Stage.
        auto staging = StagingFolder();
        QFile::copy(classicRoot + "/" + fileName, staging.makePath(fileName));

        // Phase 2: Normalize.
        normalizePackage(staging.path);

        // Phase 3: Install.
        installStagedPackage(staging.path);
    }
}

QString OfflineMapManager::createPackage(const QString& layerId) const
{
    auto database = App::instance()->database();

    auto layer = Database::DbMapLayer();
    if (!database->loadMapLayer(layerId, &layer))
    {
        qDebug() << "createPackage: layer not found";
        return QString();
    }

    auto package = Database::DbMapPackage();
    if (!database->loadMapPackage(layer.packageId, &package))
    {
        qDebug() << "createPackage: package not found";
        return QString();
    }

    // Stage the layer's files (main + same-baseName companions) plus a fresh package.json + layers.json.
    auto staging = StagingFolder();
    auto baseName = QFileInfo(layer.filename).baseName();

    Utils::enumFiles(m_layersPath + "/" + package.folder, [&](const QFileInfo& fileInfo)
    {
        if (fileInfo.baseName() == baseName)
        {
            QFile::copy(fileInfo.filePath(), staging.makePath(fileInfo.fileName()));
        }
    });

    auto packageJson = QVariantMap();
    packageJson["id"] = package.id;
    if (!package.name.isEmpty()) packageJson["name"] = package.name;
    if (!package.version.isEmpty()) packageJson["version"] = package.version;
    Utils::variantMapToJsonFile(staging.makePath(PACKAGE_JSON), packageJson);

    auto layerJson = QVariantMap();
    if (!layer.filename.isEmpty()) layerJson["filename"] = layer.filename;
    if (!layer.name.isEmpty()) layerJson["name"] = layer.name;
    if (!layer.category.isEmpty()) layerJson["category"] = layer.category;
    if (layer.opacity != 1.0) layerJson["opacity"] = layer.opacity;
    if (layer.lookup) layerJson["lookup"] = true;
    if (!layer.symbol.isEmpty()) layerJson["symbol"] = layer.symbol;
    Utils::variantListToJsonFile(staging.makePath(LAYERS_JSON), QVariantList { layerJson });

    // Create a package zip file.
    auto filePath = App::instance()->sendFilePath() + "/" + App::instance()->timeManager()->makeUniqueFilename("OfflineMap.zip");
    qDebug() << "Create offline map package for layer: " << layerId;
    qFatalIf(QFile::exists(filePath) && !QFile::remove(filePath), "Failed to delete old package");

    if (!Utils::zipCompressDir(filePath, staging.path))
    {
        qDebug() << "ZIP failed to finalize the package";
        return QString();
    }

    Utils::mediaScan(filePath);
    return filePath;
}

QString OfflineMapManager::getShareUrl(const QString& layerId) const
{
    auto database = App::instance()->database();

    auto layer = Database::DbMapLayer();
    if (!database->loadMapLayer(layerId, &layer))
    {
        return QString();
    }

    auto package = Database::DbMapPackage();
    if (!database->loadMapPackage(layer.packageId, &package))
    {
        return QString();
    }

    if (package.updateUrl.isEmpty())
    {
        return QString();
    }

    auto data = QVariantMap {{ "webUpdateUrl", package.updateUrl }};
    return QString("%1?x=%2")
        .arg(App::instance()->config()["appLinkUrls"].toStringList().constFirst(),
             App::instance()->appLink()->encode(data));
}

bool OfflineMapManager::canInstallPackage(const QString& filePath) const
{
    // Look for regular map file.
    auto suffix = QFileInfo(filePath).suffix();
    if (Utils::isMapLayerSuffix(suffix))
    {
        return true;
    }

    // Look for files inside a zip.
    if (!Utils::isZipFile(filePath))
    {
        return false;
    }

    auto filenameList = Utils::zipGetFilenames(filePath);

    // SMART route file.
    if (filenameList.contains("project.json"))
    {
        return filenameList.length() == 2;
    }

    // Scan for map layers.
    for (auto it = filenameList.constBegin(); it != filenameList.constEnd(); it++)
    {
        auto fileInfo = QFileInfo(*it);
        if (fileInfo.filePath().contains("/") || fileInfo.filePath().contains("\""))
        {
            // Skip subfolders.
            continue;
        }

        if (Utils::isMapLayerSuffix(fileInfo.suffix()))
        {
            return true;
        }
    }

    return false;
}

ApiResult OfflineMapManager::installPackage(const QString& filePath, const QString& projectUid, const QString& updateUrl, const QString& eTag)
{
    // Phase 1: Stage.
    auto staging = StagingFolder();

    if (Utils::isZipFile(filePath))
    {
        Utils::zipExtractToDir(filePath, staging.path);
    }
    else
    {
        QFile::copy(filePath, staging.makePath(QFileInfo(filePath).fileName()));
    }

    // Phase 2: Normalize.
    normalizePackage(staging.path);

    // Phase 3: Replace any existing URL-installed package before re-installing.
    if (!updateUrl.isEmpty())
    {
        auto existing = Database::DbMapPackage();
        if (findPackageByUrl(updateUrl, &existing))
        {
            removePackage(existing);
        }
    }

    // Phase 4: Install.
    return installStagedPackage(staging.path, projectUid, updateUrl, eTag);
}

ApiResult OfflineMapManager::hasUpdate(const QString& offlineMapUrl, QString* responseEtagOut) const
{
    // Get the file head.
    auto response = Utils::httpGetHead(App::instance()->networkAccessManager(), offlineMapUrl);
    if (!response.success)
    {
        return ApiResult::Error(response.errorString);
    }

    // Lookup existing package by URL.
    auto existing = Database::DbMapPackage();
    findPackageByUrl(offlineMapUrl, &existing);

    // Up to date.
    if (!existing.eTag.isEmpty() && existing.eTag == response.etag)
    {
        return ApiResult::Expected(tr("Up to date"));
    }

    // Update available.
    if (responseEtagOut)
    {
        *responseEtagOut = response.etag;
    }

    return ApiResult::Success();
}

void OfflineMapManager::checkForUpdates()
{
    if (m_pendingChecks > 0 || m_updatesAvailable)
    {
        return;
    }

    auto database = App::instance()->database();
    auto networkAccessManager = App::instance()->networkAccessManager();

    auto urls = QList<QPair<QString, QString>>();    // {packageId, updateUrl}
    database->enumMapPackages([&](const Database::DbMapPackage& package)
    {
        if (!package.updateUrl.isEmpty())
        {
            urls.append({ package.id, package.updateUrl });
        }
    });

    for (const auto& entry : std::as_const(urls))
    {
        auto packageId = entry.first;
        auto request = Utils::httpCreateRequest(entry.second);
        auto reply = networkAccessManager->head(request);
        m_pendingChecks++;

        connect(reply, &QNetworkReply::finished, this, [this, reply, packageId]()
        {
            auto serverETag = QString::fromUtf8(reply->rawHeader("ETag"));
            reply->deleteLater();

            if (!serverETag.isEmpty())
            {
                auto current = Database::DbMapPackage();
                if (App::instance()->database()->loadMapPackage(packageId, &current) && current.eTag != serverETag)
                {
                    update_updatesAvailable(true);
                }
            }

            m_pendingChecks--;
        });
    }
}

void OfflineMapManager::updateAll()
{
    auto database = App::instance()->database();

    auto urls = QList<QPair<QString, QString>>();    // {updateUrl, projectUid}
    database->enumMapPackages([&](const Database::DbMapPackage& package)
    {
        if (!package.updateUrl.isEmpty())
        {
            urls.append({ package.updateUrl, package.projectUid });
        }
    });

    auto updatedCount = 0;
    auto failedCount = 0;
    for (const auto& entry : std::as_const(urls))
    {
        auto result = installOrUpdatePackage(entry.first, entry.second);
        if (result.success)
        {
            updatedCount++;
        }
        else if (!result.expected)
        {
            failedCount++;
        }
    }

    if (failedCount == 0)
    {
        update_updatesAvailable(false);
    }

    if (failedCount > 0)
    {
        emit App::instance()->showError(tr("Failed to update"));
    }
    else if (updatedCount > 0)
    {
        emit App::instance()->showToast(tr("Maps updated"));
    }
    else
    {
        emit App::instance()->showToast(tr("Up to date"));
    }
}

ApiResult OfflineMapManager::installOrUpdatePackage(const QString& offlineMapUrl, const QString& projectUid)
{
    auto updateETag = QString();
    auto result = hasUpdate(offlineMapUrl, &updateETag);
    if (!result.success)
    {
        return result;
    }

    auto filePath = App::instance()->downloadFile(offlineMapUrl);
    if (!QFile::exists(filePath))
    {
        return ApiResult::Error(tr("Map download failed"));
    }
    auto cleanup = qScopeGuard([&] { QFile::remove(filePath); });

    if (!canInstallPackage(filePath))
    {
        return ApiResult::Error(tr("Invalid map format"));
    }

    return installPackage(filePath, projectUid, offlineMapUrl, updateETag);
}

ApiResult OfflineMapManager::installProjectMaps(const QString& path, const QVariantList& mapList, const QString& projectUid)
{
    // Replace any existing project-bundled packages for this project.
    if (!projectUid.isEmpty())
    {
        deletePackagesByProjectUid(projectUid);
    }

    // Phase 1: Stage all maps into a single staging folder.
    auto staging = StagingFolder();

    auto layers = QVariantList();
    auto cleanupFiles = QStringList();
    auto cleanupFolders = QStringList();

    for (const auto& mapV : mapList)
    {
        auto map = mapV.toMap();

        // Single file maps.
        auto file = map.value("file").toString();
        if (!file.isEmpty() && !file.contains("/"))
        {
            // Copy the file and any companion files (e.g. .tfw for .tif).
            auto baseName = QFileInfo(file).baseName();
            Utils::enumFiles(path, [&](const QFileInfo& fileInfo)
            {
                if (fileInfo.baseName() == baseName)
                {
                    QFile::copy(fileInfo.filePath(), staging.makePath(fileInfo.fileName()));
                }
            });

            auto layer = QVariantMap();
            layer["filename"] = file;
            layer["name"] = map.value("title", map.value("name").toString()).toString();
            layers.append(layer);

            cleanupFiles.append(path + "/" + file);

            continue;
        }

        // Maps in a folder.
        if (!file.isEmpty() && file.contains("/"))
        {
            auto sourceFolder = QFileInfo(path + "/" + file).path();
            if (QDir(sourceFolder).exists() && !cleanupFolders.contains(sourceFolder))
            {
                // Copy all map files from the folder to staging.
                const auto mapFiles = Utils::buildMapLayerList(sourceFolder);
                for (const auto& mapFile : mapFiles)
                {
                    auto fileInfo = QFileInfo(mapFile);
                    QFile::copy(mapFile, staging.makePath(fileInfo.fileName()));

                    auto layer = QVariantMap();
                    layer["filename"] = fileInfo.fileName();
                    layer["name"] = fileInfo.baseName();
                    layers.append(layer);
                }

                cleanupFolders.append(sourceFolder);
            }

            continue;
        }

        // WMS maps.
        auto type = map.value("type").toString();
        if (type == "wms")
        {
            auto name = map.value("layers").toString();
            auto service = map.value("service").toString();
            auto layerJson = QVariantMap {{ "layer", name }, { "service", service }};

            auto filename = "layer.wms";
            Utils::variantMapToJsonFile(staging.makePath(filename), layerJson);

            auto layer = QVariantMap();
            layer["filename"] = filename;
            layer["name"] = name;
            layers.append(layer);

            continue;
        }
    }

    // Phase 2: Write layers.json. normalizePackage will synthesise a UUID-id package.json.
    Utils::variantListToJsonFile(staging.makePath(LAYERS_JSON), layers);
    normalizePackage(staging.path);

    // Phase 3: Install.
    auto result = installStagedPackage(staging.path, projectUid);

    // Cleanup source files and folders.
    for (const auto& file : std::as_const(cleanupFiles))
    {
        QFile::remove(file);
    }

    for (const auto& folder: std::as_const(cleanupFolders))
    {
        QDir(folder).removeRecursively();
    }

    return result;
}

void OfflineMapManager::moveLayer(const QString& layerId, int delta)
{
    auto database = App::instance()->database();

    auto layerIds = QStringList();
    database->enumMapLayers([&](const Database::DbMapLayer& layer)
    {
        layerIds.append(layer.id);
    });

    auto index = layerIds.indexOf(layerId);
    if (index == -1)
    {
        qDebug() << "Error bad index";
        return;
    }

    auto newIndex = qBound(0, index + delta, layerIds.size() - 1);
    if (newIndex == index)
    {
        return;
    }
    layerIds.move(index, newIndex);

    database->transaction([&]() {
        auto sequence = 1;
        for (const auto& id : std::as_const(layerIds))
        {
            auto layer = Database::DbMapLayer();
            if (!database->loadMapLayer(id, &layer))
            {
                continue;
            }
            if (layer.sequence != sequence)
            {
                layer.sequence = sequence;
                database->saveMapLayer(layer);
            }
            sequence++;
        }
        return true;
    });

    emit layersChanged();
}

QStringList OfflineMapManager::getLayers() const
{
    auto result = QStringList();
    App::instance()->database()->enumMapLayers([&](const Database::DbMapLayer& layer)
    {
        result.append(layer.id);
    });
    return result;
}

Database::DbMapLayer OfflineMapManager::getLayer(const QString& layerId) const
{
    auto result = Database::DbMapLayer();
    App::instance()->database()->loadMapLayer(layerId, &result);
    return result;
}

Database::DbMapPackage OfflineMapManager::getPackage(const QString& packageId) const
{
    auto result = Database::DbMapPackage();
    App::instance()->database()->loadMapPackage(packageId, &result);
    return result;
}

void OfflineMapManager::setLayerActive(const QString& layerId, bool value)
{
    auto database = App::instance()->database();

    auto layer = Database::DbMapLayer();
    if (!database->loadMapLayer(layerId, &layer))
    {
        return;
    }
    layer.active = value;
    database->saveMapLayer(layer);

    emit layersChanged();
}

QString OfflineMapManager::getLayerPackageId(const QString& layerId) const
{
    return getLayer(layerId).packageId;
}

int OfflineMapManager::getPackageLayerCount(const QString& packageId) const
{
    return findLayersByPackageId(packageId).size();
}

void OfflineMapManager::deleteLayer(const QString& layerId)
{
    auto database = App::instance()->database();

    auto layer = Database::DbMapLayer();
    if (!database->loadMapLayer(layerId, &layer))
    {
        return;
    }

    auto package = Database::DbMapPackage();
    auto packageFound = database->loadMapPackage(layer.packageId, &package);

    auto layerName = layer.name;

    // Remove the layer's main file plus any companion files matching its baseName.
    if (packageFound && !package.folder.isEmpty() && !layer.filename.isEmpty())
    {
        auto packageFolder = m_layersPath + "/" + package.folder;
        auto baseName = QFileInfo(layer.filename).baseName();
        Utils::enumFiles(packageFolder, [&](const QFileInfo& fileInfo)
        {
            if (fileInfo.baseName() == baseName)
            {
                QFile::remove(fileInfo.filePath());
            }
        });
    }

    // If this layer was installed from the Classic root (.ecw), remove the
    // source file so installClassicLayers() doesn't reinstall it.
    if (layer.filename.endsWith(".ecw", Qt::CaseInsensitive))
    {
        QFile::remove(Utils::classicRoot() + "/" + layer.filename);
    }

    database->deleteMapLayer(layerId);

    // If the package now has no layers, drop it.
    if (packageFound && findLayersByPackageId(layer.packageId).isEmpty())
    {
        QDir(m_layersPath + "/" + package.folder).removeRecursively();
        database->deleteMapPackage(layer.packageId);
    }

    emit layerDeleted(layerName);
    emit layersChanged();
}

void OfflineMapManager::deleteLayersByPackageId(const QString& packageId)
{
    auto database = App::instance()->database();

    auto package = Database::DbMapPackage();
    auto packageFound = database->loadMapPackage(packageId, &package);

    auto layerNames = QStringList();
    database->enumMapLayersByPackage(packageId, [&](const Database::DbMapLayer& layer)
    {
        layerNames.append(layer.name);
    });

    database->transaction([&]() {
        database->deleteMapLayersByPackage(packageId);
        if (packageFound)
        {
            database->deleteMapPackage(packageId);
        }
        return true;
    });

    if (packageFound && !package.folder.isEmpty())
    {
        QDir(m_layersPath + "/" + package.folder).removeRecursively();
    }

    for (const auto& name : std::as_const(layerNames))
    {
        emit layerDeleted(name);
    }
    emit layersChanged();
}

void OfflineMapManager::deletePackagesByProjectUid(const QString& projectUid)
{
    auto database = App::instance()->database();

    auto packageIds = QStringList();
    database->enumMapPackagesByProject(projectUid, [&](const Database::DbMapPackage& package)
    {
        packageIds.append(package.id);
    });

    for (const auto& packageId : std::as_const(packageIds))
    {
        deleteLayersByPackageId(packageId);
    }
}

bool OfflineMapManager::hasLookupLayers() const
{
    auto found = false;
    App::instance()->database()->enumMapLayers([&](const Database::DbMapLayer& layer)
    {
        if (!found && layer.active && layer.lookup)
        {
            found = true;
        }
    });
    return found;
}

} // namespace CyberTracker
