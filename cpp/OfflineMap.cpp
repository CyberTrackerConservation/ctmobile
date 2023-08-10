#include "OfflineMap.h"
#include "App.h"
#include <jlcompress.h>
#include "quazip.h"

namespace {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Layer

class Layer
{
public:
    QString filename;
    QString name;
    bool active;
    bool deleted;
    double opacity;

    Layer(const QVariantMap& source)
    {
        load(source);
    }

    void load(const QVariantMap& source)
    {
        filename = source["filename"].toString();
        name = source["name"].toString();
        active = source.value("active", true).toBool();
        deleted = source.value("deleted", false).toBool();
        opacity = source.value("opacity", 1.0).toDouble();
    };

    QVariantMap save() const
    {
        return QVariantMap
        {
            { "filename", filename },
            { "name", name },
            { "active", active },
            { "deleted", deleted },
            { "opacity", opacity }
        };
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Package

class Package
{
public:
    QString etag;
    QString url;
    QString timestamp;
    QList<Layer*> layers;

    Package(const QVariantMap& source)
    {
        load(source);
    }

    Package(const QString layerPath)
    {
        timestamp = App::instance()->timeManager()->currentDateTimeISO();
        discoverLayers(layerPath);
    }

    ~Package()
    {
        clear();
    }

    void clear()
    {
        qDeleteAll(layers);
    }

    void discoverLayers(const QString& layerPath)
    {
        clear();
        auto layersFilePath = layerPath + "/layers.json";
        if (QFile::exists(layersFilePath))
        {
            // Discover layers via a json file.
            auto layersV = Utils::variantListFromJsonFile(layersFilePath);
            for (auto layerIt = layersV.constBegin(); layerIt != layersV.constEnd(); layerIt++)
            {
                layers.append(new Layer(layerIt->toMap()));
            }
        }
        else
        {
            // Discover layers by searching.
            auto files = Utils::buildMapLayerList(layerPath);
            for (auto fileIt = files.constBegin(); fileIt != files.constEnd(); fileIt++)
            {
                auto fileInfo = QFileInfo(*fileIt);
                layers.append(new Layer(QVariantMap {{ "filename", fileInfo.fileName() }, { "name", fileInfo.baseName()}}));
            }
        }
    }

    void load(const QVariantMap& source)
    {
        clear();

        etag = source["etag"].toString();
        url = source["url"].toString();
        timestamp = source["timestamp"].toString();

        auto layersV = source["layers"].toList();
        for (auto layerIt = layersV.constBegin(); layerIt != layersV.constEnd(); layerIt++)
        {
            layers.append(new Layer(layerIt->toMap()));
        }
    }

    QVariantMap save() const
    {
        auto layersV = QVariantList();
        for (auto layerIt = layers.constBegin(); layerIt != layers.constEnd(); layerIt++)
        {
            layersV.append((*layerIt)->save());
        }

        return QVariantMap
        {
            { "etag", etag },
            { "url", url },
            { "timestamp", timestamp },
            { "layers", layersV }
        };
    }

    Layer* getLayer(const QString& filename) const
    {
        for (auto layerIt = layers.constBegin(); layerIt != layers.constEnd(); layerIt++)
        {
            if ((*layerIt)->filename == filename)
            {
                return *layerIt;
            }
        }

        return nullptr;
    }

    bool hasLayers() const
    {
        for (auto layerIt = layers.constBegin(); layerIt != layers.constEnd(); layerIt++)
        {
            if (!(*layerIt)->deleted)
            {
                return true;
            }
        }

        return false;
    }

    QStringList getLayerFilenames() const
    {
        auto result = QStringList();
        for (auto layerIt = layers.constBegin(); layerIt != layers.constEnd(); layerIt++)
        {
            if (!(*layerIt)->deleted)
            {
                result.append((*layerIt)->filename);
            }
        }

        return result;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Packages

struct Packages
{
    QMap<QString, Package*> packages;

    Packages()
    {
    }

    ~Packages()
    {
        clear();
    }

    void clear()
    {
        qDeleteAll(packages);
    }

    void load()
    {
        auto source = App::instance()->settings()->offlineMapPackages();
        for (auto packageIt = source.constKeyValueBegin(); packageIt != source.constKeyValueEnd(); packageIt++)
        {
            packages.insert(packageIt->first, new Package(packageIt->second.toMap()));
        }
    }

    void save()
    {
        auto allLayerIds = QStringList();

        // Create "offlineMapPackages" setting.
        auto packagesV = QVariantMap();
        for (auto packageIt = packages.constKeyValueBegin(); packageIt != packages.constKeyValueEnd(); packageIt++)
        {
            packagesV.insert(packageIt->first, packageIt->second->save());

            auto layerFilenames = packageIt->second->getLayerFilenames();
            for (auto layerIt = layerFilenames.constBegin(); layerIt != layerFilenames.constEnd(); layerIt++)
            {
                allLayerIds.append(packageIt->first + "/" + *layerIt);
            }
        }

        App::instance()->settings()->set_offlineMapPackages(packagesV);

        // Create "offlineMapLayers" setting.
        auto oldLayerIds = App::instance()->settings()->offlineMapLayers();
        auto newLayerIds = QStringList();

        for (auto layerIt = oldLayerIds.constBegin(); layerIt != oldLayerIds.constEnd(); layerIt++)
        {
            auto layerId = *layerIt;
            if (allLayerIds.contains(layerId))
            {
                newLayerIds.append(layerId);
                allLayerIds.removeOne(layerId);
            }
        }

        newLayerIds.append(allLayerIds);
        App::instance()->settings()->set_offlineMapLayers(newLayerIds);
    }

    QString findByUrl(const QString& url)
    {
        for (auto packageIt = packages.constKeyValueBegin(); packageIt != packages.constKeyValueEnd(); packageIt++)
        {
            if (packageIt->second->url == url)
            {
                return packageIt->first;
            }
        }

        return QString();
    }

    void insert(const QString& packageUid, const Package& package)
    {
        packages.insert(packageUid, new Package(package.save()));
        save();
    }

    Package* getPackage(const QString& packageUid)
    {
        return packages.value(packageUid, nullptr);
    }

    Layer* getLayer(const QString& layerId)
    {
        auto fileInfo = QFileInfo(layerId);
        auto packageId = fileInfo.path();
        auto filename = fileInfo.fileName();

        for (auto packageIt = packages.constKeyValueBegin(); packageIt != packages.constKeyValueEnd(); packageIt++)
        {
            if (packageId != packageIt->first)
            {
                continue;
            }

            auto layer = packageIt->second->getLayer(filename);
            if (layer)
            {
                return layer;
            }
        }

        return nullptr;
    }
};

Packages* s_packages = nullptr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineMapManager

OfflineMapManager::OfflineMapManager(QObject *parent): QObject{parent}
{
    qFatalIf(true, "Singleton");
}

OfflineMapManager::OfflineMapManager(const QString& offlineMapsPath, QObject *parent): QObject{parent}
{
    s_packages = new Packages();
    s_packages->load();

    m_packagesPath = offlineMapsPath;
    qFatalIf(!Utils::ensurePath(m_packagesPath), "Failed to create packages folder");

    installLegacyGotos();

    garbageCollect();
}

OfflineMapManager::~OfflineMapManager()
{
    delete s_packages;
    s_packages = nullptr;
}

void OfflineMapManager::installLegacyGotos()
{
    auto gotoFolder = App::instance()->gotoPath();

    Utils::enumFiles(gotoFolder, [&](const QFileInfo& fileInfo)
    {
        if (fileInfo.suffix().toLower() == "json")
        {
            auto geoJsonFilePath = gotoFolder + "/" + fileInfo.completeBaseName() + ".geojson";
            QFile::remove(geoJsonFilePath);
            QFile::rename(fileInfo.filePath(), geoJsonFilePath);
            installPackage(geoJsonFilePath);
        }
    });
}

void OfflineMapManager::garbageCollect()
{
    auto packages = new Packages();

    Utils::enumFolders(m_packagesPath, [&](const QFileInfo& fileInfo)
    {
        auto packageUid = fileInfo.fileName();
        auto package = s_packages->getPackage(packageUid);
        if (package)
        {
            if (!package->hasLayers())
            {
                // All layers deleted so cleanup.
                QDir(m_packagesPath + "/" + packageUid).removeRecursively();
            }
            else
            {
                // Layers valid, so reuse.
                packages->insert(packageUid, *package);
            }
        }
        else
        {
            // Discovered a new package.
            packages->insert(packageUid, Package(m_packagesPath + "/" + packageUid));
        }
    });

    delete s_packages;
    s_packages = packages;
    s_packages->save();
}

bool OfflineMapManager::canInstallPackage(const QString& filePath) const
{
    // Look for regular map file.
    auto suffix = Utils::detectFileSuffix(Utils::resolveContentUri(filePath));
    if (Utils::isMapLayerSuffix(suffix))
    {
        return true;
    }

    // Look for files inside a zip.
    if (suffix != "zip")
    {
        return false;
    }

    QuaZip zip(filePath);
    if (!zip.open(QuaZip::mdUnzip))
    {
        return false;
    }

    auto filenameList = zip.getFileNameList();

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

ApiResult OfflineMapManager::installPackage(const QString& filePath, QString* packageUidOut)
{
    // Stage files.
    auto stagingFolder = QDir::cleanPath(App::instance()->tempPath() + "/OfflineMap");
    QDir(stagingFolder).removeRecursively();
    qFatalIf(!Utils::ensurePath(stagingFolder), "Failed to create OfflineMap temp folder");

    // Cleanup on exit.
    auto cleanupStagingOnExit = qScopeGuard([&] { QDir(stagingFolder).removeRecursively(); });

    auto suffix = Utils::detectFileSuffix(Utils::resolveContentUri(filePath));

    // Extract ZIP or copy map to the staging folder.
    if (suffix == "zip")
    {
        JlCompress::extractDir(filePath, stagingFolder);
    }
    else
    {
        QFile::copy(filePath, stagingFolder + "/" + QFileInfo(filePath).fileName());
    }

    // Tweak SMART route file to look like a regular offline map.
    auto projectJsonFilePath = stagingFolder + "/project.json";
    if (QFile::exists(projectJsonFilePath))
    {
        auto projectMap = Utils::variantMapFromJsonFile(projectJsonFilePath);
        if (!projectMap.contains("decoder") || !projectMap.contains("projectName") || !projectMap.contains("definition"))
        {
            qDebug() << "Missing fields from project.json";
            return ApiResult::Error(tr("No a valid route file"));
        }

        auto layerJsonFilePath = stagingFolder + "/" + projectMap["definition"].toString();
        if (!QFile::exists(layerJsonFilePath))
        {
            qDebug() << "Missing definition";
            return ApiResult::Error(tr("No a valid route file"));
        }

        auto layerMap = Utils::variantMapFromJsonFile(layerJsonFilePath);
        layerMap["name"] = layerMap.value("name", projectMap["projectName"].toString());

        auto layerJsonFileInfo = QFileInfo(layerJsonFilePath);
        auto layerGeoJsonFilePath = layerJsonFileInfo.path() + "/" + layerJsonFileInfo.baseName() + ".geojson";
        Utils::writeJsonToFile(layerGeoJsonFilePath, QJsonDocument(QJsonObject::fromVariantMap(layerMap)).toJson());

        // Remove the project.json file.
        QFile::remove(projectJsonFilePath);
        QFile::remove(layerJsonFilePath);
    }

    // Look for maps.
    auto offlineMaps = Utils::buildMapLayerList(stagingFolder);
    if (offlineMaps.isEmpty())
    {
        return ApiResult::Error(tr("No maps found"));
    }

    auto packageUid = Utils::computeFolderHash(stagingFolder);
    auto targetFolder = m_packagesPath + "/" + packageUid;

    // Overwrite if already exists.
    if (QDir(targetFolder).exists())
    {
        Utils::copyPath(stagingFolder, targetFolder);
    }
    // Install the staging folder.
    else
    {
        QDir(targetFolder).removeRecursively();
        if (!QDir().rename(stagingFolder, targetFolder))
        {
            if (!Utils::copyPath(stagingFolder, targetFolder))
            {
                return ApiResult::Error(tr("Install failed"));
            }
        }
    }

    // Success.
    s_packages->insert(packageUid, Package(targetFolder));
    s_packages->save();

    emit layersChanged();

    if (packageUidOut)
    {
        *packageUidOut = packageUid;
    }

    return ApiResult::Success();
}

ApiResult OfflineMapManager::hasUpdate(QNetworkAccessManager* networkAccessManager, const QString& offlineMapUrl, QString* foundPackageUidOut, QString* responseEtagOut) const
{
    // Get the file head.
    auto response = Utils::httpGetHead(networkAccessManager, offlineMapUrl);
    if (!response.success)
    {
        return ApiResult::Error(response.errorString);
    }

    // Lookup existing package.
    auto packageUid = s_packages->findByUrl(offlineMapUrl);
    auto packageETag = packageUid.isEmpty() ? QString() : s_packages->getPackage(packageUid)->etag;

    // Up to date.
    if (!packageUid.isEmpty() && packageETag == response.etag)
    {
        return ApiResult::Expected(tr("Up to date"));
    }

    // Update available.
    if (foundPackageUidOut)
    {
        *foundPackageUidOut = packageUid;
    }

    if (responseEtagOut)
    {
        *responseEtagOut = response.etag;
    }

    return ApiResult::Success();
}

ApiResult OfflineMapManager::installOrUpdatePackage(const QString& offlineMapUrl)
{
    auto networkAccessManager = App::instance()->networkAccessManager();

    auto foundPackageUid = QString();
    auto updateETag = QString();
    auto result = hasUpdate(networkAccessManager, offlineMapUrl, &foundPackageUid, &updateETag);
    if (!result.success)
    {
        return result;
    }

    auto filePath = App::instance()->downloadFile(offlineMapUrl);
    if (!QFile::exists(filePath))
    {
        return ApiResult::Error(tr("Map download failed"));
    }

    auto suffix = QFileInfo(filePath).suffix();
    if (!Utils::isMapLayerSuffix(suffix))
    {
        suffix = "zip";
    }

    auto realFilePath = App::instance()->tempPath() + "/Map." + suffix;
    QFile::remove(realFilePath);
    QFile::rename(filePath, realFilePath);
    if (!canInstallPackage(realFilePath))
    {
        return ApiResult::Error(tr("Invalid map format"));
    }

    // Remove the old package.
    if (!foundPackageUid.isEmpty())
    {
        auto folderPath = m_packagesPath + "/" + foundPackageUid;
        if (QDir().exists(folderPath))
        {
            QDir(folderPath).removeRecursively();
        }
    }

    // Install new package.
    auto packageUid = QString();
    auto installResult = installPackage(realFilePath, &packageUid);
    if (!installResult.success)
    {
        return installResult;
    }

    // Success.
    Package package(m_packagesPath + "/" + packageUid);
    package.url = offlineMapUrl;
    package.etag = updateETag;
    s_packages->insert(packageUid, package);
    s_packages->save();

    emit layersChanged();

    return ApiResult::Success();
}

ApiResult OfflineMapManager::installProjectMaps(const QString& path, const QVariantList& mapList)
{
    // Stage files.
    auto stagingFolder = QDir::cleanPath(App::instance()->tempPath() + "/OfflineMap");
    QDir(stagingFolder).removeRecursively();
    qFatalIf(!Utils::ensurePath(stagingFolder), "Failed to create OfflineMap temp folder");

    auto cleanupFiles = QStringList();
    auto cleanupFolders = QStringList();

    // Cleanup on exit.
    auto cleanupStagingOnExit = qScopeGuard([&] { QDir(stagingFolder).removeRecursively(); });

    // Move the maps to the staging folder.
    for (auto mapIt = mapList.constBegin(); mapIt != mapList.constEnd(); mapIt++)
    {
        auto map = mapIt->toMap();

        // Single file maps.
        auto file = map.value("file").toString();
        if (!file.isEmpty() && !file.contains("/"))
        {
            auto name = map.value("name").toString();
            auto sourceFilePath = path + "/" + file;
            auto targetFilePath = QString("%1/%2.%3").arg(stagingFolder, name, QFileInfo(file).suffix());
            QFile::copy(sourceFilePath, targetFilePath);

            auto title = map.value("title", name).toString();
            auto layers = QVariantList { QVariantMap {{ "filename", file }, { "name", title }, { "active", true }, { "opacity", 1.0 }}};
            Utils::writeJsonToFile(QString("%1/layers.json").arg(stagingFolder), Utils::variantListToJson(layers));

            cleanupFiles.append(sourceFilePath);

            continue;
        }

        // Maps in a folder.
        if (!file.isEmpty() && file.contains("/"))
        {
            auto sourceFolder = QFileInfo(path + "/" + file).path();
            if (QDir(sourceFolder).exists() && !cleanupFolders.contains(sourceFolder))
            {
                Utils::copyPath(sourceFolder, stagingFolder);
                cleanupFolders.append(sourceFolder);
            }
            continue;
        }

        // WMS maps.
        auto type = map.value("type").toString();
        if (type == "wms")
        {
            auto layer = map.value("layers").toString();
            auto service = map.value("service").toString();
            auto layerJson = QJsonObject {{ "layer", layer }, { "service", service }};
            Utils::writeJsonToFile(QString("%1/WMS %2.wms").arg(stagingFolder, layer), QJsonDocument(layerJson).toJson());
            continue;
        }
    }

    // Look for maps.
    auto offlineMaps = Utils::buildMapLayerList(stagingFolder);
    if (offlineMaps.isEmpty())
    {
        return ApiResult::Error(tr("No maps found"));
    }

    auto packageUid = Utils::computeFolderHash(stagingFolder);
    auto targetFolder = m_packagesPath + "/" + packageUid;

    // Overwrite if already exists.
    if (QDir(targetFolder).exists())
    {
        Utils::copyPath(stagingFolder, targetFolder);
    }
    // Install the staging folder.
    else
    {
        QDir(targetFolder).removeRecursively();
        if (!QDir().rename(stagingFolder, targetFolder))
        {
            if (!Utils::copyPath(stagingFolder, targetFolder))
            {
                return ApiResult::Error(tr("Install failed"));
            }
        }
    }

    // Success.
    s_packages->insert(packageUid, Package(m_packagesPath + "/" + packageUid));
    s_packages->save();
    emit layersChanged();

    // Cleanup files and folders. They have been installed now.
    for (auto fileIt = cleanupFiles.constBegin(); fileIt != cleanupFiles.constEnd(); fileIt++)
    {
        QFile::remove(*fileIt);
    }

    for (auto folderIt = cleanupFolders.constBegin(); folderIt != cleanupFolders.constEnd(); folderIt++)
    {
        QDir(*folderIt).removeRecursively();
    }

    return ApiResult::Success();
}

QString OfflineMapManager::createPackage(const QString& layerId) const
{
    auto packageUid = QFileInfo(layerId).path();

    auto filePath = App::instance()->sendFilePath() + "/OfflineMap.zip";
    qDebug() << "Create offline map package for layer: " << layerId;
    qFatalIf(QFile::exists(filePath) && !QFile::remove(filePath), "Failed to delete old package");

    auto packagePath = QDir::cleanPath(App::instance()->tempPath() + "/Package");
    QDir(packagePath).removeRecursively();
    qFatalIf(!Utils::ensurePath(packagePath), "Failed to create temp folder");

    auto cleanupOnExit = qScopeGuard([&] { QDir(packagePath).removeRecursively(); });

    if (!Utils::copyPath(m_packagesPath + "/" + packageUid, packagePath))
    {
        qDebug() << "Failed to copy maps to package path";
        return QString();
    }

    // Zip the file.
    if (!JlCompress::compressDir(filePath, packagePath, true))
    {
        qDebug() << "ZIP failed to finalize the package";
        return QString();
    }

    // Return the package.
    Utils::mediaScan(filePath);

    return filePath;
}

void OfflineMapManager::moveLayer(const QString& layerId, int delta)
{
    auto offlineLayers = App::instance()->settings()->offlineMapLayers();
    auto index = offlineLayers.indexOf(layerId);

    if (index == -1)
    {
        qDebug() << "Error bad index";
        return;
    }

    offlineLayers.move(index, index + delta);
    App::instance()->settings()->set_offlineMapLayers(offlineLayers);

    emit layersChanged();
}

QStringList OfflineMapManager::getLayers() const
{
    return App::instance()->settings()->offlineMapLayers();
}

bool OfflineMapManager::getLayerActive(const QString& layerId) const
{
    return s_packages->getLayer(layerId)->active;
}

void OfflineMapManager::setLayerActive(const QString& layerId, bool value)
{
    s_packages->getLayer(layerId)->active = value;
    s_packages->save();

    emit layersChanged();
}

void OfflineMapManager::deleteLayer(const QString& layerId)
{
    s_packages->getLayer(layerId)->deleted = true;
    s_packages->save();

    emit layersChanged();

    garbageCollect();
}

QString OfflineMapManager::getLayerName(const QString& layerId) const
{
    return s_packages->getLayer(layerId)->name;
}

double OfflineMapManager::getLayerOpacity(const QString& layerId) const
{
    return s_packages->getLayer(layerId)->opacity;
}

QString OfflineMapManager::getLayerFilePath(const QString& layerId) const
{
    return m_packagesPath + "/" + layerId;
}

QString OfflineMapManager::getLayerTimestamp(const QString& layerId) const
{
    auto packageUid = layerId.split("/").constFirst();

    return s_packages->getPackage(packageUid)->timestamp;
}
