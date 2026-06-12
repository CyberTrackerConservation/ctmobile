#include "SMARTConnector.h"
#include "SMARTProvider.h"
#include "SMARTSchema.h"
#include "App.h"

namespace CyberTracker {

namespace {
constexpr bool COMPRESS_UPLOAD = false;
}

SMARTConnector::SMARTConnector(QObject* parent) : Connector(parent)
{
    update_name(CONNECTOR_NAME);
    update_icon("qrc:/SMART/logo.svg");

    m_smart7Required = "SMART 7+ " + tr("required");
}

SMARTConnector::~SMARTConnector()
{
}

void SMARTConnector::migrateProject(Project* project) const
{
    auto apiKey = project->getConnectorParam("api_key").toString();

    auto removeApiKeyFromMap = [&](QVariantMap& map)
    {
        if (apiKey.isEmpty() && !map.value("API_KEY").toString().isEmpty())
        {
            apiKey = map["API_KEY"].toString();
        }

        map.remove("API_KEY");
    };

    // project.json.
    auto filePath = m_projectManager->getFilePath(project->uid(), "project.json");
    if (QFile::exists(filePath))
    {
        auto data = Utils::variantMapFromJsonFile(filePath);
        removeApiKeyFromMap(data);
        Utils::variantMapToJsonFile(filePath, data);
    }

    // ct_profile.json.
    filePath = m_projectManager->getFilePath(project->uid(), "ct_profile.json");
    if (QFile::exists(filePath))
    {
        auto data = Utils::variantMapFromJsonFile(filePath);
        auto dataServer = data.value("DATA_SERVER").toMap();
        if (!dataServer.isEmpty())
        {
            removeApiKeyFromMap(dataServer);
            data["DATA_SERVER"] = dataServer;
        }

        auto positionUpdates = data.value("POSITION_UPDATES").toMap();
        if (!positionUpdates.isEmpty())
        {
            removeApiKeyFromMap(positionUpdates);
            data["POSITION_UPDATES"] = positionUpdates;
        }

        auto alerts = data.value("ALERTS").toList();
        auto newAlerts = QVariantList();
        for (auto alertIt = alerts.constBegin(); alertIt != alerts.constEnd(); alertIt++)
        {
            auto newAlert = alertIt->toMap();
            removeApiKeyFromMap(newAlert);
            newAlerts.append(newAlert);
        }

        if (!alerts.isEmpty())
        {
            data["ALERTS"] = newAlerts;
        }

        Utils::variantMapToJsonFile(filePath, data);
    }

    if (!apiKey.isEmpty())
    {
        if (project->accessToken().isEmpty())
        {
            project->set_accessToken(apiKey);
        }

        m_projectManager->modify(project->uid(), [&](Project* project)
        {
            project->setConnectorParam("api_key", QVariant());
        });
    }
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
        { "connector", CONNECTOR_NAME },
        { "server", connectorParams["server"] },
        { "auth", connectorParams["auth"].toBool() },
        { "launch", true }
    };
}

ApiResult SMARTConnector::bootstrapOffline(const QString& zipFilePath) const
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
    auto projects = m_projectManager->buildListPtr();

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

        // Check versions.
        auto zipMetadata = QVariantMap();
        auto smartVersion = 0;
        auto minMobileBuild = 0;
        if (!getZipMetadata(zipFile, &zipMetadata, &smartVersion, &minMobileBuild))
        {
            continue;
        }

        auto buildNumber = App::instance()->buildString().split('.').constLast().toInt();
        if (buildNumber < minMobileBuild)
        {
            return Failure(tr("Requires build") + " " + QString::number(minMobileBuild));
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

            if (project->provider() != SMARTProvider::PROVIDER_NAME)
            {
                continue;
            }

            if (project->title() != zipMetadata["projectName"])
            {
                continue;
            }

            if (m_database->hasData(project->uid(), "*", Database::DataFlag::DB_SIGHTING_FLAG))
            {
                return Failure("\"" + project->title() + "\" " + tr("has unexported data"));
            }

            m_projectManager->remove(project->uid());

            break;
        }

        // Create Project.qml files for the project.
        auto createdTime = zipMetadata["creation_date"].toString();
        auto projectUid = "SMART_" + QUuid::createUuid().toString(QUuid::Id128);

        // Store the access token.
        if (zipMetadata.contains("API_KEY"))
        {
            App::instance()->vault()->setAccessToken(projectUid, zipMetadata["API_KEY"].toString());
        }

        QVariantMap connectorParams;
        connectorParams["mode"] = "Offline";
        connectorParams["created"] = createdTime;
        m_projectManager->init(projectUid, SMARTProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, connectorParams);

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

ApiResult SMARTConnector::bootstrapConnect(const QVariantMap& params) const
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

    m_projectManager->init(projectUid, SMARTProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, connectorParams);

    QFile::rename(zipFilePath, m_projectManager->getFilePath(projectUid, "SMART.zip"));

    return bootstrapUpdate(projectUid);
}

QString SMARTConnector::getFormFilePath(Project* project) const
{
    return m_projectManager->getFilePath(project->uid(), "project.json");
}

ApiResult SMARTConnector::bootstrap(const QVariantMap& params) const
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

ApiResult SMARTConnector::hasUpdate(Project* project)
{
    auto apiKey = project->accessToken();
    auto url = project->getConnectorParam("status_url").toString() + (apiKey.isEmpty() ? "" : "?api_key=" + apiKey);
    auto currentVersion = project->getConnectorParam("packageVersion").toString();

    auto request = Utils::httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->get(request));

    Utils::httpWait(reply.get());

    auto response = Utils::HttpResponse::fromReply(url, reply.get());
    if (!response.success)
    {
        return Failure(response);
    }

    auto deployedVersion = QJsonDocument::fromJson(response.data).object().value("version").toString();
    if (deployedVersion == currentVersion)
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

ApiResult SMARTConnector::hasAnnouncements(Project* project, QVariantList* announcementsOut)
{
    auto announcement_url = project->getConnectorParam("announcement_url").toString();
    if (announcement_url.isEmpty())
    {
        return NotSupported();
    }

    auto apiKey = project->accessToken();
    auto url = announcement_url + (apiKey.isEmpty() ? "" : "?api_key=" + apiKey);

    auto request = Utils::httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->get(request));

    Utils::httpWait(reply.get());

    auto response = Utils::HttpResponse::fromReply(url, reply.get());
    if (!response.success)
    {
        return Failure(response);
    }

    *announcementsOut = QJsonDocument::fromJson(response.data).array().toVariantList();
    return Success();
}

ApiResult SMARTConnector::uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* /*state*/, const CommitState& /*commitState*/)
{
    auto dataServer = Utils::variantMapFromJson(queueItem.payload);
    auto url = dataServer["URL"].toString();
    auto username = dataServer["USERNAME"].toString();
    auto password = dataServer["PASSWORD"].toString();
    auto deflate = dataServer["PROTOCOL"].toString() == "GEOJSON_COMPRESSED" && COMPRESS_UPLOAD;
    auto usePUT = queueItem.type == "PINGv7" || queueItem.type == "ALERTv7";
    auto isPing = queueItem.type.startsWith("PING");;
    auto apiKey = App::instance()->vault()->getAccessToken(project->uid());

    auto request = Utils::httpCreateRequest(url + (apiKey.isEmpty() ? "" : "?api_key=" + apiKey));
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/json; charset=utf-8");

    if (!username.isEmpty() && !password.isEmpty())
    {
        auto concatenated = username + ":" + password;
        auto data = concatenated.toLocal8Bit().toBase64();
        auto headerData = QString("Basic " + data).toLocal8Bit();
        request.setRawHeader("Authorization", headerData);
    }

    auto file = QFile(App::instance()->getMediaFilePath(queueItem.attachments.first()));
    qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to open payload");
    auto payloadZip = QByteArray();
    auto reply = std::unique_ptr<QNetworkReply>();

    if (usePUT)
    {
        reply.reset(m_networkAccessManager->put(request, &file));
    }
    else
    {
        if (deflate)
        {
            request.setRawHeader("Content-Encoding", "deflate");
            payloadZip = qCompress(file.readAll());
            payloadZip.remove(0, 4);    // Remove the first 4 bytes which are just a header.
            reply.reset(m_networkAccessManager->post(request, payloadZip));
        }
        else
        {
            reply.reset(m_networkAccessManager->post(request, &file));
        }
    }

    Utils::httpWait(reply.get());

    auto result = ResultFromResponse(Utils::HttpResponse::fromReply(queueItem.url, reply.get()));

    // Pings always succeed.
    if (isPing && !result.success)
    {
        result = ApiResult::Success();
    }

    return result;
}

bool SMARTConnector::canLogin(Project* /*project*/) const
{
    return false;
}

bool SMARTConnector::canUpdate(Project* project) const
{
    return project->connectorParams().contains("status_url");
}

bool SMARTConnector::getZipMetadata(const QString& zipFilePath, QVariantMap* outMetadata, int* smartVersionOut, int* minMobileBuildOut) const
{
    // Load project.json.
    auto projectJson = Utils::zipGetVariantMapFromFile(zipFilePath, "project.json");
    if (projectJson.isEmpty())
    {
        return false;
    }

    // Must have these fields to be considered a valid SMART zip.
    if (!projectJson.contains("creation_date"))
    {
        qDebug() << "Not a valid SMART zip: " << zipFilePath;
        return false;
    }

    if (!projectJson.contains("metadata"))
    {
        qDebug() << "Not a valid SMART zip: " << zipFilePath;
        return false;
    }

    if (!projectJson.contains("projectName") && !projectJson.contains("projectNames"))
    {
        qDebug() << "Not a valid SMART zip: " << zipFilePath;
        return false;
    }

    // Load ct_profile.json.
    auto profileJson = Utils::zipGetVariantMapFromFile(zipFilePath, "ct_profile.json");
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

    // Return min mobile version.
    if (minMobileBuildOut)
    {
        *minMobileBuildOut = outMetadata->value("smart_mobile_min_build", 1).toInt();
    }

    return true;
}

ApiResult SMARTConnector::updateOffline(Project* project) const
{
    auto projectUid = project->uid();

    // Skip updates if there are any unsent sightings.
    if (m_database->hasData(projectUid, "*", Database::DataFlag::DB_SIGHTING_FLAG))
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
    auto extractedFiles = Utils::zipExtractToDir(zipPath, updateFolder);
    if (extractedFiles.isEmpty())
    {
        return Failure(tr("Cannot unpack project"));
    }

    // Set the Project version.
    project->set_version(1);

    // Commit the project qml file.
    if (zipMetadata.contains("projectNames"))
    {
        project->set_names(zipMetadata["projectNames"].toMap());
    }
    else
    {
        project->set_name(zipMetadata["projectName"].toString());
    }

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

    fixThemeColor("THEME_COLOR_1", "primary");
    fixThemeColor("THEME_COLOR_2", "accent");
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
            // Install regular layers.
            auto offlineMaps = Utils::buildMapLayerList(mapFileInfo.filePath());
            for (auto it = offlineMaps.constBegin(); it != offlineMaps.constEnd(); it++)
            {
                auto fileInfo = QFileInfo(*it);
                mapList.append(QVariantMap {{ "name", fileInfo.baseName() }, { "file", mapSource + "/" + fileInfo.fileName() }});
            }

            // Install package layers.
            Utils::enumFiles(mapFileInfo.filePath(), [&](const QFileInfo& fileInfo)
            {
                if (fileInfo.suffix().toLower() == "zip")
                {
                    App::instance()->offlineMapManager()->installPackage(fileInfo.filePath(), project->uid());
                }
            });
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
        App::instance()->offlineMapManager()->installProjectMaps(updateFolder, mapList, project->uid());
    }

    // Kiosk mode.
    project->set_kioskMode(Utils::isAndroid() && App::instance()->config().value("androidKiosk").toBool() && profile.value("KIOSK_MODE").toBool());
    project->set_kioskModePin(QString::number(profile.value("EXIT_PIN").toInt()));

    // Hack, because SIMPLE_CAMERA should only be enabled on 810 and above.
    if (smartVersion >= 810)
    {
        project->set_defaultSimpleCamera(profile.value("SIMPLE_CAMERA").toBool());
    }

    // Disable archive.
    project->set_disableArchive(!profile.value("ARCHIVE_MODE", true).toBool());

    // Permissions.
    project->set_requireCamera(true);
    project->set_requireMicrophone(true);
    project->set_requireLocation(true);

    // Background GPS for patrols and surveys.
    if (QFile::exists(updateFolder + "/patrol_metadata.json") || QFile::exists(updateFolder + "/survey_metadata.json"))
    {
        project->set_requireBackgroundLocation(true);
    }

    // ConnectorParams.
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

    if (zipMetadata.contains("announcement_url"))
    {
        connectorParams["announcement_url"] = Utils::removeTrailingSlash(zipMetadata["announcement_url"].toString());
    }

    if (zipMetadata.contains("version"))
    {
        connectorParams["packageVersion"] = zipMetadata["version"].toString();
    }

    if (zipMetadata.contains("API_KEY"))
    {
        App::instance()->vault()->setAccessToken(projectUid, zipMetadata["API_KEY"].toString());
    }

    // If a status_url is specified then update on launch.
    project->set_updateOnLaunch(connectorParams.contains("status_url"));

    // Commit project file.
    project->set_connectorParams(connectorParams);
    project->saveToQmlFile(updateFolder + "/Project.qml");

    return Success();
}

ApiResult SMARTConnector::updateConnect(Project* project) const
{
    auto projectUid = project->uid();
    auto connectorParams = project->connectorParams();
    auto apiKey = project->accessToken();
    auto statusUrl = connectorParams["status_url"].toString() + (apiKey.isEmpty() ? "" : "?api_key=" + apiKey);

    auto response = Utils::httpGet(m_networkAccessManager, statusUrl);
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
    if (m_database->hasData(projectUid, "*", Database::DataFlag::DB_SIGHTING_FLAG))
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

    project->set_version(project->version() + 1);
    project->set_connectorParams(connectorParams);
    project->saveToQmlFile(m_projectManager->getFilePath(projectUid, "Project.qml"));

    QFile::rename(zipFilePath, m_projectManager->getFilePath(projectUid, "SMART.zip"));

    return updateOffline(project);
}

ApiResult SMARTConnector::update(Project* project) const
{
    auto result = project->connectorParams().contains("status_url") ? updateConnect(project) : updateOffline(project);
    if (!result.success)
    {
        return result;
    }

    m_projectManager->reset(project->uid(), true);

    return buildSchema(project, m_projectManager->getUpdateFolder(project->uid()));
}

ApiResult SMARTConnector::buildSchema(Project* project, const QString& path) const
{
    auto languages = QVariantList();
    auto languageIndex = -1;
    auto samplingGeoJson = QVariantMap();

    if (!SMARTSchema().parse(project, path, &languages, &languageIndex, &samplingGeoJson))
    {
        return Failure(tr("Bad package"));
    }

    project->set_languages(languages);
    if (project->languageIndex() == -1)
    {
        project->set_languageIndex(languageIndex);
    }

    if (!samplingGeoJson.isEmpty())
    {
        samplingGeoJson["name"] = project->title() + " (" + tr("units") + ")";
        samplingGeoJson["project"] = project->uid();

        auto tempFolder = QDir::cleanPath(App::instance()->tempPath() + "/SurveyLayer");
        QDir(tempFolder).removeRecursively();
        qFatalIf(!Utils::ensurePath(tempFolder), "Failed to create survey layer temp folder");

        Utils::writeJsonToFile(tempFolder + "/survey.geojson", Utils::variantMapToJson(samplingGeoJson));

        auto mapList = QVariantList();
        mapList.append(QVariantMap {{ "name", "survey" }, { "file", "survey.geojson" } });

        App::instance()->offlineMapManager()->installProjectMaps(tempFolder, mapList, project->uid());

        QDir(tempFolder).removeRecursively();
    }

    return Success();
}

void SMARTConnector::reset(Project* project) const
{
    auto path = m_projectManager->getFilePath(project->uid());

    auto stateSpaces = QStringList { "", "Patrol", "Incident", "Collect"};

    for (auto it = stateSpaces.constBegin(); it != stateSpaces.constEnd(); it++)
    {
        auto stateSpace = *it;
        QFile::remove(path + "/Form" + stateSpace + ".qml");
    }
}

} // namespace CyberTracker
