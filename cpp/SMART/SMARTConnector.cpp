#include "SMARTConnector.h"
#include "SMARTProvider.h"
#include "App.h"

#include "jlcompress.h"
#include "quazip.h"
#include "quazipfile.h"

SMARTConnector::SMARTConnector(QObject* parent) : Connector(parent)
{
    update_name(SMART_CONNECTOR);
    update_icon("qrc:/SMART/logo.svg");

    m_smart7Required = "SMART 7+ " + tr("required");
}

bool SMARTConnector::canExportData(Project* project) const
{
    return project->connectorParams().value("mode").toString() != "Collect";
}

bool SMARTConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool SMARTConnector::canShareAuth(Project* /*project*/) const
{
    return false;
}

QVariantMap SMARTConnector::getShareData(Project* project, bool /*auth*/) const
{
    if (!project->connectorParams().contains("status_url"))
    {
        return QVariantMap();
    }

    auto connectorParams = project->connectorParams();
    return QVariantMap
    {
        { "connector", SMART_CONNECTOR },
        { "server", connectorParams["server"] },
        { "auth", connectorParams["auth"].toBool() },
        { "launch", true }
    };
}

ApiResult SMARTConnector::bootstrapOffline(const QString& zipFilePath)
{
    auto smart6Detected = false;

    auto zipFiles = QStringList();
    if (zipFilePath.isEmpty())
    {
        // Build a list of candidates.
        zipFiles = Utils::searchStandardFolders("*.zip");
    }
    else
    {
        // Candidate provided.
        zipFiles.append(zipFilePath);
    }

    // Build list of existing projects.
    auto projects = m_projectManager->buildList();

    // Set up projects for each newly discovered package.
    for (auto zipFileIt = zipFiles.constBegin(); zipFileIt != zipFiles.constEnd(); zipFileIt++)
    {
        auto zipFile = *zipFileIt;

        // Check for route.
        if (App::instance()->offlineMapManager()->canInstallPackage(zipFile))
        {
            if (App::instance()->offlineMapManager()->installPackage(zipFile).success)
            {
                QFile::remove(zipFile);
                continue;
            }
        }

        // Check for project.
        auto zipMetadata = QVariantMap();
        auto smartVersion = 0;
        if (!getZipMetadata(zipFile, &zipMetadata, &smartVersion))
        {
            continue;
        }

        if (smartVersion < 700)
        {
            smart6Detected = true;
            continue;
        }

        // Look for a duplicate to replace.
        for (auto projectIt = projects.constBegin(); projectIt != projects.constEnd(); projectIt++)
        {
            auto project = *projectIt;

            if (project->provider() != SMART_PROVIDER)
            {
                continue;
            }

            if (project->title() != zipMetadata["projectName"])
            {
                continue;
            }

            if (m_database->hasSightings(project->uid(), "*", Sighting::DB_SIGHTING_FLAG | Sighting::DB_LOCATION_FLAG))
            {
                return Failure("\"" + project->title() + "\" " + tr("has unexported data"));
            }

            m_projectManager->remove(project->uid());

            break;
        }

        // Create Project.qml files for the project.
        auto createdTime = zipMetadata["creation_date"].toString();
        auto projectUid = "SMART_" + QUuid::createUuid().toString(QUuid::Id128);

        QVariantMap connectorParams;
        connectorParams["mode"] = "Offline";
        connectorParams["created"] = createdTime;
        m_projectManager->init(projectUid, SMART_PROVIDER, QVariantMap(), SMART_CONNECTOR, connectorParams);

        // Copy the zip to the project folder and try to install it.
        QFile::copy(zipFile, m_projectManager->getFilePath(projectUid, "SMART.zip"));

        auto updateResult = bootstrapUpdate(projectUid);
        if (!updateResult.success)
        {
            return updateResult;
        }

        // Update successful: remove the zip: we own it now.
        // Only remove if retrieved via a scan so as to avoid trying to install the same packages.
        if (zipFilePath.isEmpty())
        {
            QFile::remove(zipFile);
            Utils::mediaScan(zipFile);
        }
    }

    return Success(smart6Detected ? m_smart7Required : "");
}

ApiResult SMARTConnector::bootstrapConnect(const QVariantMap& params)
{
    // Ensure we do not bootstrap an existing project.
    auto server = params["server"].toString();
    auto packageUuid = server.split('/').constLast();
    auto projectUid = "SMART_" + packageUuid;

    // Strip off additional parameters.
    if (projectUid.contains('?'))
    {
        projectUid = projectUid.left(projectUid.indexOf('?'));
    }

    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    // Download the zip.
    auto auth = params.contains("username") && params.contains("password");
    auto zipFilePath = QString();
    if (auth)
    {
        zipFilePath = App::instance()->downloadFile(server, params["username"].toString(), params["password"].toString());
    }
    else
    {
        zipFilePath = App::instance()->downloadFile(server);
    }

    if (zipFilePath.isEmpty())
    {
        return Failure(tr("Download failed"));
    }

    // Unpack.
    auto zipMetadata = QVariantMap();
    auto smartVersion = 0;
    auto success = getZipMetadata(zipFilePath, &zipMetadata, &smartVersion);
    if (!success)
    {
        QFile::remove(zipFilePath);
        return Failure(tr("Download bad"));
    }

    if (smartVersion < 700)
    {
        QFile::remove(zipFilePath);
        return Failure(m_smart7Required);
    }

    QVariantMap connectorParams = params;
    connectorParams["mode"] = zipMetadata.value("metadata").toString().contains("smartcollect", Qt::CaseInsensitive) ? "Collect" : "Connect";
    connectorParams["server"] = server;
    connectorParams["auth"] = auth;

    if (zipMetadata.contains("status_url"))
    {
        connectorParams["status_url"] = Utils::removeTrailingSlash(zipMetadata["status_url"].toString());
    }

    if (zipMetadata.contains("navigation_url"))
    {
        connectorParams["navigation_url"] = Utils::removeTrailingSlash(zipMetadata["navigation_url"].toString());
    }

    if (zipMetadata.contains("download_url"))
    {
        connectorParams["download_url"] = Utils::removeTrailingSlash(zipMetadata["download_url"].toString());
    }

    if (zipMetadata.contains("API_KEY"))
    {
        connectorParams["api_key"] = zipMetadata["API_KEY"];
    }

    m_projectManager->init(projectUid, SMART_PROVIDER, QVariantMap(), SMART_CONNECTOR, connectorParams);

    QFile::rename(zipFilePath, m_projectManager->getFilePath(projectUid, "SMART.zip"));

    return bootstrapUpdate(projectUid);
}

ApiResult SMARTConnector::bootstrap(const QVariantMap& params)
{
    if (params.isEmpty() || params.contains("filePath"))
    {
        return bootstrapOffline(params.value("filePath").toString());
    }
    else
    {
        return bootstrapConnect(params);
    }
}

ApiResult SMARTConnector::hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project)
{
    auto connectorParams = project->connectorParams();
    auto apiKey = connectorParams.value("api_key").toString();
    auto statusUrl = connectorParams["status_url"].toString() + (apiKey.isEmpty() ? "" : "?api_key=" + apiKey);

    auto response = Utils::httpGet(networkAccessManager, statusUrl);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto packageVersion = QJsonDocument::fromJson(response.data).object().value("version").toString();
    if (packageVersion.isEmpty() || packageVersion == connectorParams["packageVersion"])
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

bool SMARTConnector::canLogin(Project* /*project*/) const
{
    return false;
}

bool SMARTConnector::canUpdate(Project* project) const
{
    return project->connectorParams().contains("status_url");
}

bool SMARTConnector::getZipMetadata(const QString& zipFilePath, QVariantMap* outMetadata, int* smartVersionOut)
{
    // Load project.json.
    auto projectJson = Utils::variantMapFromFileInZip(zipFilePath, "project.json");
    if (projectJson.isEmpty())
    {
        return false;
    }

    // Must have these fields to be considered a valid SMART zip.
    if (!projectJson.contains("creation_date") || !projectJson.contains("projectName") || !projectJson.contains("metadata"))
    {
        qDebug() << "Missing fields from project.json";
        return false;
    }

    // Load ct_profile.json.
    auto profileJson = Utils::variantMapFromFileInZip(zipFilePath, "ct_profile.json");
    if (profileJson.isEmpty())
    {
        qDebug() << "Missing ct_profile.json";
        return false;
    }

    // Return metadata.
    if (outMetadata)
    {
        *outMetadata = projectJson;
        outMetadata->insert("profile", profileJson);
    }

    // Return version.
    if (smartVersionOut)
    {
        *smartVersionOut = qFloor(outMetadata->value("smart_version", 6.00).toDouble() * 100);
    }

    return true;
}

ApiResult SMARTConnector::updateOffline(Project* project)
{
    auto projectUid = project->uid();

    // Skip updates if there are any unsent sightings.
    auto sightingUids = QStringList();
    m_database->getSightings(projectUid, "*", Sighting::DB_SIGHTING_FLAG, &sightingUids);
    if (sightingUids.count() > 0)
    {
        return Failure(tr("Unexported data"));
    }

    // Reset the project.
    m_projectManager->reset(projectUid, true);

    // Update.
    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    auto connectorParams = project->connectorParams();
    auto zipPath = m_projectManager->getFilePath(projectUid, "SMART.zip");

    // Get the metadata for the zip file.
    auto zipMetadata = QVariantMap();
    auto smartVersion = 0;
    if (!getZipMetadata(zipPath, &zipMetadata, &smartVersion))
    {
        return Failure(tr("Bad project file"));
    }

    if (smartVersion < 700)
    {
        return Failure(m_smart7Required);
    }

    // Unpack zip to project path.
    auto extractedFiles = JlCompress::extractDir(zipPath, updateFolder);
    if (extractedFiles.isEmpty())
    {
        return Failure(tr("Cannot unpack project"));
    }

    // Set the Project version.
    project->set_version(1);

    // Commit the project qml file.
    project->set_title(zipMetadata["projectName"].toString());
    project->set_subtitle(Utils::decodeTimestamp(zipMetadata["creation_date"].toString()).toString("yyyy/MM/dd hh:mm:ss"));

    // Get the profile.
    auto profile = zipMetadata["profile"].toMap();

    // Colors.
    auto colors = QVariantMap();

    auto fixThemeColor = [&](const QString& profileName, const QString& colorName, const QString& defaultValue = QString()) {
        if (!defaultValue.isEmpty())
        {
            colors[colorName] = defaultValue;
        }

        // SMART 7 the color is a string.
        if (profile.value(profileName).userType() == QMetaType::QString)
        {
            colors[colorName] = "#" + profile.value(profileName).toString();
            return;
        }

        // SMART 6 the color is an int.
        auto themeColor = profile.value(profileName, "-1").toString();
        if (themeColor != "-1")
        {
            colors[colorName] = "#" + QString::number(themeColor.toInt() & 0xFFFFFF, 16).rightJustified(6, '0');
        }
    };

    fixThemeColor("THEME_COLOR_1", "primary", "#334469");
    fixThemeColor("THEME_COLOR_2", "accent", "#6c9779");
    fixThemeColor("THEME_COLOR_3", "foreground");
    fixThemeColor("THEME_COLOR_4", "background");

    project->set_colors(colors);

    // Logo.
    if (!zipMetadata["logo"].toString().isEmpty())
    {
        project->set_icon(zipMetadata["logo"].toString());
    }
    else
    {
        project->set_icon("qrc:/SMART/logo.svg");
        project->set_iconDark("qrc:/SMART/logoDark.svg");
    }

    // Map.
    auto map = zipMetadata["map"].toMap();

    // Detect Smart 6.
    if (!zipMetadata.contains("smart_version"))
    {
        map = QVariantMap {{ "basemap", zipMetadata["map"].toString() }};
    }

    // Try SMART 7+.
    if (map.count() > 0)
    {
        auto mapList = QVariantList();
        auto mapSource = map["basemap"].toString();

        // Handle SMART 6 custom map files where the name is useless.
        if (QDir().exists(updateFolder + "/map"))
        {
            mapSource = "map";
        }

        // Process individual files or a folder.
        auto mapFileInfo = QFileInfo(updateFolder + "/" + mapSource);
        if (!mapFileInfo.exists())
        {
            qDebug() << "Map file not found: " << mapSource;
        }
        else if (mapFileInfo.isFile())
        {
            auto mapItem = QVariantMap();
            mapItem.insert("name", mapFileInfo.baseName());
            mapItem.insert("title", project->title());
            mapItem.insert("file", mapSource);
            mapList.append(mapItem);
        }
        else if (mapFileInfo.isDir())
        {
            auto offlineMaps = Utils::buildMapLayerList(mapFileInfo.filePath());
            for (auto it = offlineMaps.constBegin(); it != offlineMaps.constEnd(); it++)
            {
                auto fileInfo = QFileInfo(*it);
                mapList.append(QVariantMap {{ "name", fileInfo.baseName() }, { "file", mapSource + "/" + fileInfo.fileName() }});
            }
        }

        // Process layers.
        if (map.contains("layers"))
        {
            auto mapLayers = map["layers"].toList();
            for (auto it = mapLayers.constBegin(); it != mapLayers.constEnd(); it++)
            {
                mapList.insert(0, it->toMap());
            }
        }

        // Pass the maps to the offline map manager.
        App::instance()->offlineMapManager()->installProjectMaps(updateFolder, mapList);
    }

    // Kiosk mode.
    project->set_kioskMode(Utils::isAndroid() && App::instance()->config().value("androidKiosk").toBool() && profile.value("KIOSK_MODE").toBool());
    project->set_kioskModePin(QString::number(profile.value("EXIT_PIN").toInt()));

    // Permissions.
    project->set_androidPermissions(QStringList { "CAMERA", "RECORD_AUDIO", "ACCESS_FINE_LOCATION", "ACCESS_COARSE_LOCATION" });

    // Background GPS for patrols and surveys.
    if (QFile::exists(updateFolder + "/patrol_metadata.json") || QFile::exists(updateFolder + "/survey_metadata.json"))
    {
        project->set_androidBackgroundLocation(true);
        project->set_androidDisableBatterySaver(true);
    }

    // If an api key is specified then update on launch.
    project->set_updateOnLaunch(connectorParams.contains("status_url"));

    // Commit project file.
    project->set_connectorParams(connectorParams);
    project->saveToQmlFile(updateFolder + "/Project.qml");

    return Success();
}

ApiResult SMARTConnector::updateConnect(Project* project)
{
    auto projectUid = project->uid();
    auto connectorParams = project->connectorParams();
    auto apiKey = connectorParams.value("api_key").toString();
    auto statusUrl = connectorParams["status_url"].toString() + (apiKey.isEmpty() ? "" : "?api_key=" + apiKey);

    auto response = Utils::httpGet(App::instance()->networkAccessManager(), statusUrl);
    auto packageVersion = QJsonDocument::fromJson(response.data).object().value("version").toString();

    if (!response.success || packageVersion.isEmpty() || packageVersion == connectorParams["packageVersion"])
    {
        if (QFile::exists(m_projectManager->getFilePath(projectUid, "SMART.zip")))
        {
            // Up to date ZIP, which is not yet unpacked.
            return updateOffline(project);
        }
        else
        {
            return AlreadyUpToDate();
        }
    }

    // Skip updates if there are any unsent sightings.
    auto sightingUids = QStringList();
    m_database->getSightings(projectUid, "*", Sighting::DB_SIGHTING_FLAG, &sightingUids);
    if (sightingUids.count() > 0)
    {
        return Failure(tr("Unsent data"));
    }

    // A new package is available: download it.
    auto downloadUrl = connectorParams["download_url"].toString() + (apiKey.isEmpty() ? "" : "?api_key=" + apiKey);
    auto zipFilePath = App::instance()->downloadFile(downloadUrl);
    if (zipFilePath.isEmpty())
    {
        return Failure(QString(tr("Bad %1 file")).arg(App::instance()->alias_project()));
    }

    auto zipMetadata = QVariantMap();
    auto smartVersion = 0;
    auto success = getZipMetadata(zipFilePath, &zipMetadata, &smartVersion);
    if (!success)
    {
        QFile::remove(zipFilePath);
        return Failure(QString(tr("Cannot unpack %1")).arg(App::instance()->alias_project()));
    }

    if (smartVersion < 700)
    {
        QFile::remove(zipFilePath);
        return Failure(m_smart7Required);
    }

    connectorParams["packageVersion"] = packageVersion;
    project->set_version(project->version() + 1);
    project->set_connectorParams(connectorParams);
    project->saveToQmlFile(m_projectManager->getFilePath(projectUid, "Project.qml"));

    QFile::rename(zipFilePath, m_projectManager->getFilePath(projectUid, "SMART.zip"));

    return updateOffline(project);
}

ApiResult SMARTConnector::update(Project* project)
{
    return project->connectorParams().contains("status_url") ? updateConnect(project) : updateOffline(project);
}

void SMARTConnector::reset(Project* project)
{
    auto path = m_projectManager->getFilePath(project->uid());

    auto stateSpaces = QStringList { "", "Patrol", "Incident", "Collect"};

    for (auto it = stateSpaces.constBegin(); it != stateSpaces.constEnd(); it++)
    {
        auto stateSpace = *it;
        QFile::remove(path + "/Elements" + stateSpace + ".qml");
        QFile::remove(path + "/Fields" + stateSpace + ".qml");
        QFile::remove(path + "/Settings" + stateSpace + ".json");
        QFile::remove(path + "/ExtraData" + stateSpace + ".json");
    }
}

