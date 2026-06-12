#include "ClassicConnector.h"
#include "ClassicProvider.h"
#include "ClassicRenderer.h"
#include "ClassicSchema.h"
#include "ClassicView.h"
#include "App.h"
#include "Esri/EsriConnector.h"
#include "lib/Main/ctSession.h"

#if defined(Q_OS_WINDOWS)   // Hack to fix Curl not finding _WIN32
#define _WIN32
#endif
#include <curl/curl.h>

namespace CyberTracker {

ClassicConnector::ClassicConnector(QObject* parent) : Connector(parent)
{
    update_name(CONNECTOR_NAME);
}

ClassicConnector::~ClassicConnector()
{
}

QString getSubtitle(const QString& serverFileName)
{
    if (serverFileName.startsWith("KEY:SMART"))
    {
        return "SMART";
    }
    else if (serverFileName.startsWith("C:\\Users\\justi\\AppData\\Local\\Temp\\")) // Fixup server filename
    {
        return "Sample";
    }
    else
    {
        auto filePath = serverFileName;
        filePath = QFileInfo(filePath.replace("%", "/").replace("\"", "/")).baseName();
        if (filePath.startsWith("$CT"))
        {
            filePath = "";
        }

        return filePath;
    }
}

QSet<QString> loadSentHistory(const QString& path)
{
    auto result = QSet<QString>();

    auto file = QFile(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return result;
    }

    auto bytes = file.readAll();
    file.close();

    auto count = bytes.size() / (qsizetype)sizeof(GUID);
    auto guids = reinterpret_cast<const GUID*>(bytes.constData());
    for (qsizetype i = 0; i < count; i++)
    {
        result.insert(GUIDToString(const_cast<GUID*>(&guids[i])));
    }

    return result;
}

void bootstrapDatabase(Project* project, const QString& ctsPath, const QString& datPath, const QSet<QString>& sentHistory)
{
    if (!QFile::exists(datPath))
    {
        return;
    }

    auto app = App::instance();
    auto database = app->database();
    auto projectUid = project->uid();

    auto form = app->createFormPtr(projectUid, "");

    auto datPathUtf8 = datPath.toUtf8();
    auto chunkDb = std::make_unique<CfxDatabase>(nullptr, datPathUtf8.data());

    auto resourcePathUtf8 = ctsPath.toUtf8();
    auto resource = std::unique_ptr<CfxResource>(new CfxResource(nullptr, resourcePathUtf8.data()));

    DBID id;
    int index = 1;
    while (chunkDb->Enum(index++, &id))
    {
        void* buffer = nullptr;
        if (chunkDb->Read(id, &buffer) && buffer && MEM_SIZE(buffer) > 0)
        {
            auto bytes = QByteArray((const char*)buffer, MEM_SIZE(buffer));
            MEM_FREE(buffer);

            CctSighting oldSighting(nullptr);
            oldSighting.LoadBlob(bytes);

            auto sightingUid = GUIDToString(oldSighting.GetUniqueId());
            if (database->testData(projectUid, sightingUid))
            {
                continue;
            }

            auto sighting = std::unique_ptr<Sighting>(ClassicView::makeSighting(form.get(), &oldSighting, resource.get()));

            form->saveSighting(sighting.get(), true);

            // Mark completed: draft is incomplete.
            form->setSightingFlag(sightingUid, Database::DataFlag::DB_COMPLETED_FLAG, true);

            // A save (including edits) invalidates any previous upload — the row needs
            // to be re-sent on the next transfer.
            if (sentHistory.contains(sightingUid))
            {
                form->setSightingFlag(sightingUid, Database::DataFlag::DB_UPLOADED_FLAG, true);
            }
        }
    }

    // Release the memory map before we delete the file.
    chunkDb.reset();

    QFile::remove(datPath);
}

void bootstrapState(const QString& projectUid, const QString& staPath)
{
    if (!QFile::exists(staPath))
    {
        return;
    }

    auto staPathUtf8 = staPath.toUtf8();
    auto db = std::make_unique<CfxDatabase>(nullptr, staPathUtf8.data());
    if (!db->Verify())
    {
        QFile::remove(staPath);
        return;
    }

    DBID id;
    if (!db->Enum(0, &id))
    {
        db.reset();
        QFile::remove(staPath);
        return;
    }

    void* buffer = nullptr;
    if (db->Read(id, &buffer) && buffer && MEM_SIZE(buffer) > 0)
    {
        auto blob = QByteArray((const char*)buffer, MEM_SIZE(buffer));
        MEM_FREE(buffer);

        auto database = App::instance()->database();
        auto data = QVariantMap();
        auto attachments = QStringList();
        auto streamers = QStringList();
        database->loadFormState(projectUid, "", &data, &attachments, &streamers);
        auto providerData = data.value("provider").toMap();
        providerData["classicState"] = blob;
        data["provider"] = providerData;
        database->saveFormState(projectUid, "", data, attachments, streamers);
    }

    db.reset();
    QFile::remove(staPath);
}

void ClassicConnector::migrateProject(Project* project) const
{
    // Prevent re-entrancy.
    static bool s_mutex = false;
    if (s_mutex)
    {
        return;
    }
    s_mutex = true;
    QScopeGuard cleanup([&] { s_mutex = false; });

    // Migrate outgoing data to the new queue.
    auto outgoingPath = Utils::classicRoot() + "/Outgoing";
    Utils::enumFiles(outgoingPath, [&](const QFileInfo& fileInfo)
    {
        if (!fileInfo.fileName().endsWith(".OUT", Qt::CaseInsensitive))
        {
            return;
        }

        FXSENDDATA sendData = {};
        auto file = QFile(fileInfo.filePath());
        qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to open OUT file");
        qFatalIf(file.read((char*)&sendData, sizeof(FXSENDDATA)) != sizeof(FXSENDDATA), "Bad OUT file");
        file.close();

        auto queueItem = Database::DbQueueItem();
        queueItem.projectUid = project->uid();
        queueItem.url = sendData.Url;
        queueItem.summary = tr("Migrated data");

        switch (sendData.Protocol)
        {
        case FXSENDDATA_PROTOCOL_FTP:
            queueItem.type = UPLOAD_FTP;
            break;

        case FXSENDDATA_PROTOCOL_AZURE:
            queueItem.type = UPLOAD_AZURE;
            break;

        case FXSENDDATA_PROTOCOL_HTTP:
        case FXSENDDATA_PROTOCOL_HTTPS:
        case FXSENDDATA_PROTOCOL_HTTPZ:
            queueItem.type = UPLOAD_HTTP;
            break;

        default:
            qDebug() << "Failed to migrate payload: Unknown";
            return;
        };

        queueItem.payload = Utils::variantMapToJson(QVariantMap
        {
            { "username", QString(sendData.UserName) },
            { "password", QString(sendData.Password) },
            { "deflate", sendData.Protocol == FXSENDDATA_PROTOCOL_HTTPZ },
        });

        auto dataExt = queueItem.type == UPLOAD_HTTP ? ".JSON" : ".CTX";
        auto dataFileInfo = QFileInfo(outgoingPath + "/" + fileInfo.baseName() + dataExt);
        if (!dataFileInfo.exists())
        {
            qDebug() << "Failed to migrate payload: " << dataFileInfo.fileName();
            return;
        }

        auto copySuccess = QFile::copy(dataFileInfo.filePath(), App::instance()->getMediaFilePath(dataFileInfo.fileName()));
        if (!copySuccess)
        {
            qDebug() << "Failed to copy payload: " << dataFileInfo.fileName();
            return;
        }

        queueItem.attachments = QStringList { dataFileInfo.fileName() };

        App::instance()->uploadQueue()->addItem(queueItem);

        QFile::remove(fileInfo.filePath());
        QFile::remove(dataFileInfo.filePath());
    });

    // Migrate Classic data and state.
    auto root = Utils::classicRoot();
    auto appName = project->connectorParams()["app"].toString();

    // Migrate the session state.
    auto staPath = root + "/" + appName + ".STA";
    bootstrapState(project->uid(), staPath);

    // Migrate the data: one time only so we can later rewrite the DAT and WAY for importers.
    if (!project->hasMigrated())
    {
        auto ctsPath = root + "/" + appName + ".CTS";
        auto datPath = root + "/" + appName + ".DAT";

        // Capture sent history so we can tag data as having been sent.
        auto sentHistory = loadSentHistory(root + "/" + "SentHistory.SNT");
        sentHistory += loadSentHistory(root + "/" + "SentHistory.BAK");
        bootstrapDatabase(project, ctsPath, datPath, sentHistory);

        // Install classic ECW layers.
        App::instance()->offlineMapManager()->installClassicLayers();

        project->set_hasMigrated(true);
    }
}

bool ClassicConnector::canExportData(Project* project) const
{
    auto ctsMetadata = QVariantMap();
    getCTSMetadata(Utils::classicRoot() + "/" + project->connectorParams()["app"].toString() + ".CTS", &ctsMetadata);

    return ctsMetadata["protocol"] == "NONE";
}

bool ClassicConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool ClassicConnector::canShareAuth(Project* /*project*/) const
{
    return false;
}

QVariantMap ClassicConnector::getShareData(Project* project, bool /*auth*/) const
{
    auto webUpdateUrl = project->connectorParams()["webUpdateUrl"].toString();
    if (webUpdateUrl.isEmpty())
    {
        return QVariantMap();
    }

    return QVariantMap
    {
        { "connector", CONNECTOR_NAME },
        { "webUpdateUrl", webUpdateUrl },
        { "launch", true }
    };
}

bool ClassicConnector::isESRI(Project* project) const
{
    auto ctsMetadata = QVariantMap();
    getCTSMetadata(getFormFilePath(project), &ctsMetadata);

    return ctsMetadata["protocol"] == "ESRI";
}

static std::unique_ptr<EsriConnector> makeEsriConnector(QNetworkAccessManager* networkAccessManager)
{
    auto esriConnector = std::make_unique<EsriConnector>();
    esriConnector->setNetworkAccessManager(networkAccessManager);
    return esriConnector;
}

bool ClassicConnector::canLogin(Project* project) const
{
    return isESRI(project);
}

bool ClassicConnector::loggedIn(Project* project) const
{
    if (!isESRI(project))
    {
        return true;
    }

    return makeEsriConnector(m_networkAccessManager)->loggedIn(project);
}

bool ClassicConnector::login(Project* project, const QString& server, const QString& username, const QString& password) const
{
    if (!isESRI(project))
    {
        return false;
    }

    return makeEsriConnector(m_networkAccessManager)->login(project, server, username, password);
}

ApiResult ClassicConnector::refreshAccessToken(Project* project, QString* oldRefreshTokenOut, QString* newAccessTokenOut, QString* newRefreshTokenOut)
{
    if (!isESRI(project))
    {
        return NotSupported();
    }

    return makeEsriConnector(m_networkAccessManager)->refreshAccessToken(project, oldRefreshTokenOut, newAccessTokenOut, newRefreshTokenOut);
}

void ClassicConnector::bootstrapEsriLogin(Project* project) const
{
    auto ctsMetadata = QVariantMap();
    if (!getCTSMetadata(getFormFilePath(project), &ctsMetadata))
    {
        return;
    }

    if (ctsMetadata["protocol"] != "ESRI" || loggedIn(project))
    {
        return;
    }

    auto username = ctsMetadata["username"].toString();
    auto password = ctsMetadata["password"].toString();
    if (username.isEmpty() || password.isEmpty())
    {
        return;
    }

    // Exchange the resource credentials for OAuth tokens.
    login(project, QString(), username, password);
}

QString ClassicConnector::getFormFilePath(Project* project) const
{
    return Utils::classicRoot() + "/" + project->connectorParams()["app"].toString() + ".CTS";
}

ApiResult ClassicConnector::bootstrap(const QVariantMap& params) const
{
    auto path = Utils::classicRoot();

    if (!Utils::ensurePath(path, true))
    {
        return Failure(tr("Cannot create path"));
    }

    Utils::ensurePath(path + "/Media", true);
    Utils::ensurePath(path + "/Outgoing", true);
    Utils::ensurePath(path + "/Backup", true);
    Utils::ensurePath(path + "/Temp", true);
    Utils::ensurePath(path + "/Update", true);

    // Create version.txt.
    {
        QFile outFile(path + "/version.txt");
        outFile.remove();
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        ts << "Storage=" << path << Qt::endl;
        ts << "OsVersion=" << 0 << Qt::endl;
        ts << "ResourceVersion=" << ClassicProvider::version() << Qt::endl;

        auto screenSize = qApp->screens()[0]->size();
        ts << "Width=" + QString::number(screenSize.width()) << Qt::endl;
        ts << "Height=" + QString::number(screenSize.height()) << Qt::endl;
        ts << "Dpi=" + QString::number((int)(qApp->screens()[0]->logicalDotsPerInch())) << Qt::endl;

        outFile.close();
        Utils::mediaScan(outFile.fileName());
    }

    // Extract the world map if not already there.
    if (!QFile::exists(path + "/World.ecw"))
    {
        Utils::extractResource(":/Classic/World.ecw", path);
    }

    // Build list of existing projects.
    auto projects = m_projectManager->buildListPtr();

    // Perform web install if specified.
    auto webUpdateUrl = params.value("webUpdateUrl").toString();
    if (!webUpdateUrl.isEmpty())
    {
        qDebug() << "WebInstall: looking for " << webUpdateUrl;

        // Check for a duplicate project.
        for (auto project: projects)
        {
            if (project->provider() != ClassicProvider::PROVIDER_NAME)
            {
                continue;
            }

            if (project->connectorParams().value("webUpdateUrl").toString().compare(webUpdateUrl, Qt::CaseInsensitive) != 0)
            {
                continue;
            }

            return AlreadyConnected();
        }

        // Check that the url is good.
        if (!App::instance()->pingUrl(webUpdateUrl))
        {
            return Failure(tr("Server not found"));
        }

        auto projectUid = "CLASSIC2_" + QUuid::createUuid().toString(QUuid::Id128);
        m_projectManager->init(projectUid, ClassicProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, QVariantMap());

        auto project = m_projectManager->loadPtr(projectUid);
        project->set_version(0);
        project->set_icon("qrc:/Classic/logo.svg");
        project->set_name("Pending update");
        project->set_subtitle("");
        project->set_updateOnLaunch(true);
        project->set_requireBackgroundLocation(true);

        QVariantMap connectorParams;
        connectorParams["webUpdateUrl"] = webUpdateUrl;
        connectorParams["app"] = projectUid;
        project->set_connectorParams(connectorParams);

        // Commit project file.
        auto projectFilePath = m_projectManager->getFilePath(projectUid, "Project.qml");
        project->saveToQmlFile(projectFilePath);

        // Update the project.
        return bootstrapUpdate(projectUid);
    }

    // Extract sample.
    if (params.contains("sampleId"))
    {
        auto sampleId = params["sampleId"].toString();
        QFile::remove(path + ".CTS");
        QFile::remove(path + ".WAY");
        QFile::remove(path + ".DAT");
        QFile::remove(path + ".STA");
        Utils::extractResource(":/Classic/" + sampleId + ".CTS", path);
    }

    // Build a list of CTS files.
    auto dir = QDir(path);
    auto filter = QStringList({"*.CTS", "*.cts"});
    auto ctsFiles = QVariantList();

    auto fileInfoList = QFileInfoList(dir.entryInfoList(filter, QDir::Files));
    for (auto fileInfo: fileInfoList)
    {
        auto ctsFile = fileInfo.filePath();
        auto metadata = QVariantMap();
        if (getCTSMetadata(ctsFile, &metadata))
        {
            metadata["path"] = ctsFile;
            ctsFiles.append(metadata);
        }
    }

    // Set up projects for each newly discovered package.
    for (auto ctsFile: ctsFiles)
    {
        auto ctsMetadata = ctsFile.toMap();
        auto projectUid = "CLASSIC2_" + QUuid::createUuid().toString(QUuid::Id128);
        bool replaceExisting = false;
        auto webUpdateUrl = QString();

        // Look for a duplicate to replace.
        for (auto project: projects)
        {
            if (project->provider() != ClassicProvider::PROVIDER_NAME)
            {
                continue;
            }

            if (project->connectorParams().value("app") != ctsMetadata["app"])
            {
                continue;
            }

            replaceExisting = true;
            projectUid = project->uid();
            webUpdateUrl = project->connectorParams().value("webUpdateUrl").toString();

            // Override the update url if existing one is empty.
            if (webUpdateUrl.isEmpty())
            {
                webUpdateUrl = ctsMetadata["webUpdateUrl"].toString();
            }

            break;
        }

        // Create Project.qml files for the project.
        QVariantMap connectorParams;
        connectorParams["serverFileName"] = ctsMetadata["serverFileName"];
        connectorParams["app"] = ctsMetadata["app"];
        connectorParams["webUpdateUrl"] = webUpdateUrl;

        if (!replaceExisting)
        {
            m_projectManager->init(projectUid, ClassicProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, connectorParams);
        }

        auto project = m_projectManager->loadPtr(projectUid);
        project->set_version(1);
        project->set_icon("qrc:/Classic/logo.svg");
        project->set_name(ctsMetadata["name"].toString());
        project->set_subtitle(getSubtitle(ctsMetadata["serverFileName"].toString()));
        project->set_updateOnLaunch(!connectorParams["webUpdateUrl"].toString().isEmpty());
        project->set_connectorParams(connectorParams);
        project->set_requireCamera(true);
        project->set_requireMicrophone(true);
        project->set_requireLocation(true);
        project->set_requireBackgroundLocation(true);
        project->set_telemetry(ctsMetadata["telemetry"].toMap());

        // Commit project file.
        auto projectFilePath = m_projectManager->getFilePath(projectUid, "Project.qml");
        project->saveToQmlFile(projectFilePath);

        // Ensure we have a valid project in the projects table.
        // Normally this would be handled in the ProjectManager::update, but if the
        // classic app is from a web install, then the update may fail
        // if it is already up to date.
        m_database->addProject(projectUid);

        // Message has been seen and acted upon.
        App::instance()->settings()->set_classicUpgradeMessage(false);
    }

    emit m_projectManager->projectsChanged();

    App::instance()->settings()->set_classicDesktopConnected(true);

    return Success();
}

ApiResult ClassicConnector::hasUpdate(Project* project)
{
    auto ctsMetadata = QVariantMap();
    getCTSMetadata(Utils::classicRoot() + "/" + project->getConnectorParam("app").toString() + ".CTS", &ctsMetadata);
    auto currentVersion = ctsMetadata["stampId"].toString();

    auto webUpdateUrl = project->getConnectorParam("webUpdateUrl").toString();
    auto request = Utils::httpCreateRequest(webUpdateUrl);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->get(request));

    Utils::httpWait(reply.get());

    auto response = Utils::HttpResponse::fromReply(webUpdateUrl, reply.get());
    if (!response.success)
    {
        return Failure(response);
    }

    auto defList = QString(response.data).split("\r\n");
    if (defList.length() < 4)
    {
        return Failure("DEF file corrupt");
    }

    auto deployedVersion = defList[1];
    if (deployedVersion.isEmpty())
    {
        return Failure("No deployed versions");
    }

    if (deployedVersion.compare(currentVersion, Qt::CaseInsensitive) == 0)
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

size_t ftpReadCallback(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto file = static_cast<QFile*>(userdata);
    if (!file->isOpen() || file->atEnd())
    {
        return 0; // No more data to read
    }

    qint64 bytesToRead = size * nmemb;
    QByteArray data = file->read(bytesToRead);
    size_t bytesRead = data.size();
    std::memcpy(ptr, data.constData(), bytesRead);
    return bytesRead;
};

ApiResult ClassicConnector::uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* state, const CommitState& commitState)
{
    // Nothing to do, this is just a webhook.
    if (queueItem.type == UPLOAD_WEBHOOK_ONLY)
    {
        return Success();
    }

    // All other types.
    auto result = ApiResult::Error("");
    auto payload = Utils::variantMapFromJson(queueItem.payload);
    auto username = payload["username"].toString();
    auto password = payload["password"].toString();
    auto deflate = payload["deflate"].toBool();

    if (queueItem.type == UPLOAD_AZURE)
    {
        auto filename = queueItem.attachments.constFirst();
        auto filePath = App::instance()->getMediaFilePath(filename);
        auto file = QFile(filePath);
        qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to read CTX");

        auto multipart = QHttpMultiPart(QHttpMultiPart::FormDataType);
        auto part = QHttpPart();
        part.setHeader(QNetworkRequest::ContentTypeHeader, "application/ctx");
        part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"" + filename + "\"");
        part.setBodyDevice(&file);

        multipart.append(part);

        // Build url.
        auto urlFinal = queueItem.url;
        if (urlFinal.back() != '/')
        {
            urlFinal += "/";
        }

        urlFinal += "api/queue/";
        urlFinal += username;
        urlFinal += "/upload";

        // Send.
        auto response = Utils::httpPostMultiPart(m_networkAccessManager, urlFinal, &multipart);

        return ResultFromResponse(response);
    }
    else if (queueItem.type == UPLOAD_FTP)
    {
        auto filename = queueItem.attachments.constFirst();
        auto filePath = App::instance()->getMediaFilePath(filename);
        auto file = QFile(filePath);
        qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to read CTX");

        auto finalUrl = queueItem.url;
        if (finalUrl[finalUrl.length() - 1] != '/')
        {
            finalUrl += "/";
        }

        auto serverUrl = QUrl(finalUrl + filename);
        serverUrl.setUserName(username);
        serverUrl.setPassword(password);

        if (finalUrl.startsWith("ftps:"))
        {
            serverUrl.setPort(22);
        }
        else
        {
            serverUrl.setPort(21);
        }

        auto curl = curl_easy_init();
        qFatalIf(!curl, "Failed to initialize curl");

        auto onExit = qScopeGuard([&] { curl_easy_cleanup(curl); });

        curl_easy_setopt(curl, CURLOPT_URL, serverUrl.toString().toStdString().c_str());
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);                    // Enable upload mode
        curl_easy_setopt(curl, CURLOPT_READDATA, &file);               // File to upload
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, ftpReadCallback); // Callback to read file data
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file.size()); // Set file size
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);                   // Optional: Enable verbose output for debugging

        auto curlResult = curl_easy_perform(curl);

        return curlResult == CURLE_OK ? ApiResult::Success() : ApiResult::Error(curl_easy_strerror(curlResult));
    }
    else if (queueItem.type == UPLOAD_HTTP)
    {
        for (auto attachmentIt = queueItem.attachments.constBegin(); attachmentIt != queueItem.attachments.constEnd(); attachmentIt++)
        {
            auto filename = *attachmentIt;
            if (state->contains(filename))
            {
                // Already sent.
                continue;
            }

            auto filePath = App::instance()->getMediaFilePath(filename);
            auto file = QFile(filePath);
            qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to read attachment");

            auto request = Utils::httpCreateRequest(queueItem.url);
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

            auto reply = std::unique_ptr<QNetworkReply>();

            if (deflate)
            {
                request.setRawHeader("Content-Encoding", "deflate");
                auto payloadZip = qCompress(file.readAll());
                payloadZip.remove(0, 4);    // Remove the first 4 bytes which are just a header.
                reply.reset(m_networkAccessManager->post(request, payloadZip));
            }
            else
            {
                reply.reset(m_networkAccessManager->post(request, &file));
            }

            Utils::httpWait(reply.get());

            auto response = Utils::HttpResponse::fromReply(queueItem.url, reply.get());
            if (!response.success)
            {
                return ResultFromResponse(response);
            }

            // Track the file as uploaded.
            state->insert(filename, true);
            commitState();
        }

        return ApiResult::Success();
    }
    else if (queueItem.type == UPLOAD_ESRI)
    {
        // ESRI uses the OAuth access token maintained by EsriConnector, not the queue payload credentials.
        auto accessToken = project->accessToken();
        if (accessToken.isEmpty())
        {
            return Connector::Unauthorized();
        }

        auto mediaFiles = QStringList();

        for (auto attachmentIt = queueItem.attachments.constBegin(); attachmentIt != queueItem.attachments.constEnd(); attachmentIt++)
        {
            auto filename = *attachmentIt;
            if (state->contains(filename))
            {
                // Already sent.
                continue;
            }

            // Pick up the layer id.
            auto layerId = QString();
            if (filename.endsWith("-0"))        // Sightings.
            {
                layerId = "0";
            }
            else if (filename.endsWith("-1"))   // Waypoints.
            {
                layerId = "1";
            }
            else                                // Attachments.
            {
                mediaFiles.append(filename);
                continue;
            }

            auto filePath = App::instance()->getMediaFilePath(filename);
            auto file = QFile(filePath);
            qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to read attachment");

            auto url = QString("%1/FeatureServer/%2/addFeatures").arg(queueItem.url, layerId);

            auto query = QUrlQuery();
            query.addQueryItem("f", "json");
            query.addQueryItem("token", accessToken);
            query.addQueryItem("features", file.readAll());
            auto payload = query.toString(QUrl::FullyEncoded).toUtf8();

            auto response = Utils::esriDecodeResponse(Utils::httpPostData(m_networkAccessManager, url, payload));
            if (!response.success)
            {
                return ResultFromResponse(response);
            }

            auto results = QJsonDocument::fromJson(response.data).object().toVariantMap();
            if (!results.contains("addResults"))
            {
                return ApiResult::Error("No addResults");
            }

            // Pick up the object id for attachments.
            if (layerId == "0")
            {
                auto addResults = results["addResults"].toList();
                if (addResults.count() != 1)
                {
                    return ApiResult::Error("Bad result list");
                }

                auto addResult = addResults.constFirst().toMap();
                if (!addResult.value("success").toBool())
                {
                    return ApiResult::Error("Failed addResult");
                }

                auto objectId = addResult.value("objectId", -1).toInt();
                if (objectId == -1)
                {
                    return ApiResult::Error("Bad objectId");
                }

                state->insert("objectId", objectId);
            }

            state->insert(filename, true);
            commitState();
        }

        // Attachments.
        auto objectId = state->value("objectId", -1).toInt();
        if (!mediaFiles.isEmpty() && objectId == -1)
        {
            return ApiResult::Error("No objectId for attachments");
        }

        if (mediaFiles.isEmpty())
        {
            return ApiResult::Success();
        }

        for (auto mediaIt = mediaFiles.constBegin(); mediaIt != mediaFiles.constEnd(); mediaIt++)
        {
            auto filename = *mediaIt;

            if (state->contains(filename))
            {
                // Already sent.
                continue;
            }

            auto filePath = App::instance()->getMediaFilePath(filename);
            auto file = QFile(filePath);
            qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to read attachment");

            auto multipart = QHttpMultiPart(QHttpMultiPart::FormDataType);
            auto part1 = QHttpPart();
            part1.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"f\"");
            part1.setBody("json");
            multipart.append(part1);

            auto part2 = QHttpPart();
            part2.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"token\"");
            part2.setBody(accessToken.toUtf8());
            multipart.append(part2);

            auto part3 = QHttpPart();
            part3.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"attachment\"");
            part3.setBody(filename.toUtf8());
            multipart.append(part3);

            auto part4 = QHttpPart();
            part4.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; token=\"" + accessToken + "\"; name=\"attachment\"; filename=\"" + filename + "\"");
            part4.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, Utils::detectMimeType(filePath));
            part4.setBodyDevice(&file);
            multipart.append(part4);

            auto url = QString("%1/FeatureServer/0/%2/addAttachment").arg(queueItem.url).arg(objectId);

            auto response = Utils::esriDecodeResponse(Utils::httpPostMultiPart(m_networkAccessManager, url, &multipart));
            if (!response.success)
            {
                return ResultFromResponse(response);
            }

            state->insert(filename, true);
            commitState();
        }

        return ApiResult::Success();
    }

    return NotImplemented();
}

bool ClassicConnector::canUpdate(Project* project) const
{
    return !project->connectorParams().value("webUpdateUrl").toString().isEmpty();
}

ApiResult ClassicConnector::update(Project* project) const
{
    auto updateFolder = m_projectManager->getUpdateFolder(project->uid());
    auto webUpdateUrl = project->connectorParams().value("webUpdateUrl").toString();
    project->saveToQmlFile(updateFolder + "/Project.qml");

    // Update only works for web update.
    if (webUpdateUrl.isEmpty())
    {
        return Failure(tr("Not supported"));
    }

    // Flush if there is any unsent data.
    auto hasData = App::instance()->database()->hasData(project->uid(), {}, Database::DataFlag::DB_SIGHTING_FLAG, Database::DataFlag::DB_UPLOADED_FLAG);
    if (hasData)
    {
        // FlushData will destroy host.
        if (!CctTransferManager::FlushData(project->uid()))
        {
            return Failure(tr("Unsent data"));
        }
    }

    auto defGet = Utils::httpGet(m_networkAccessManager, webUpdateUrl);
    if (!defGet.success)
    {
        return Failure(tr("Not found"));
    }

    auto defList = QString(defGet.data).split("\r\n");
    if (defList.length() < 4)
    {
        return Failure(tr("DEF file corrupt"));
    }

    // Check the version number of CT
//    auto clientVersion = QString("3.") + QString::number(RESOURCE_VERSION);
//    if (defList[0] != clientVersion)
//    {
//        error(4, tr("Wrong version"));
//        return false;
//    }

    auto ctsMetadata = QVariantMap();
    getCTSMetadata(Utils::classicRoot() + "/" + project->connectorParams()["app"].toString() + ".CTS", &ctsMetadata);

    // Check the DEF file to see if it matches the stampid
    if (ctsMetadata["stampId"].toString().compare(defList[1], Qt::CaseSensitivity::CaseInsensitive) == 0)
    {
        return AlreadyUpToDate();
    }

    // Stamp is different, so try and download the ZIP file
    auto zipFilePath = App::instance()->downloadFile(defList[2]);
    if (zipFilePath.isEmpty())
    {
        return Failure(tr("Download failed"));
    }
    auto cleanupZipFile = qScopeGuard([&] { QFile::remove(zipFilePath); });

    // Unpack teh new package.
    auto packagePath = Utils::zipUnpack(App::instance()->tempPath(), "Package", zipFilePath);
    if (packagePath.isEmpty())
    {
        return Failure(tr("Bad package"));
    }
    auto cleanupPackagePath = qScopeGuard([&] { QDir(packagePath).removeRecursively(); });

    getCTSMetadata(packagePath + "/Application.CTS", &ctsMetadata);
    project->set_version(1);
    project->set_name(ctsMetadata["name"].toString());
    project->set_subtitle(getSubtitle(ctsMetadata["serverFileName"].toString()));
    project->set_requireBackgroundLocation(true);
    project->set_telemetry(ctsMetadata["telemetry"].toMap());

    // Success.
    auto tag = defList.length() > 4 ? defList[4] : "";

    CctUpdateManager::Install(
        packagePath.toStdString().c_str(),
        Utils::classicRoot().toStdString().c_str(),
        project->connectorParams()["app"].toString().toStdString().c_str(),
        tag.toStdString().c_str(),
        webUpdateUrl.toStdString().c_str());

    project->saveToQmlFile(updateFolder + "/Project.qml");

    return Success();
}

ApiResult ClassicConnector::buildSchema(Project* project, const QString& path) const
{
    auto formFilePath = Utils::classicRoot() + "/" + project->connectorParams()["app"].toString() + ".CTS";
    auto schema = Schema();
    ClassicSchema::parse(project, formFilePath, &schema);
    schema.set_rendererName(ClassicRenderer::RENDERER_NAME);
    schema.setProjectInfo(project->title(), project->subtitle(), project->icon());
    schema.saveToQmlFile(path + "/Form.qml");

    project->set_trackFieldUid(ClassicSchema::TRACK_FIELD_UID);
    project->set_trackFileFormat("kmz");

    return Success();
}

void ClassicConnector::reset(Project* project) const
{
    auto projectPath = m_projectManager->getFilePath(project->uid());
    QFile::remove(projectPath + "/Form.qml");

    for (const auto& file : QDir(projectPath).entryList({"Element_*.png"}, QDir::Files))
    {
        QFile::remove(projectPath + "/" + file);
    }
}

void ClassicConnector::remove(Project* project) const
{
    auto name = project->connectorParams().value("app").toString();

    auto rootFilePath = Utils::classicRoot() + "/" + name;
    QFile::remove(rootFilePath + ".CTS");
    QFile::remove(rootFilePath + ".TXT");
    QFile::remove(rootFilePath + ".WAY");
    QFile::remove(rootFilePath + ".DAT");
    QFile::remove(rootFilePath + ".STA");
    QFile::remove(rootFilePath + ".DEF");
    QFile::remove(rootFilePath + ".TAG");
    QFile::remove(rootFilePath + ".HSI");
    QFile::remove(rootFilePath + ".HST");
}

void ClassicConnector::deleteState(Project* project) const
{
    project->set_locationTrackInterval(0);
    project->set_locationStreamingEnabled(false);
}

void ClassicConnector::deleteData(Project* project) const
{
    auto name = project->connectorParams().value("app").toString();

    // auto rootFilePath = Utils::classicRoot() + "/" + name;
    // QFile::remove(rootFilePath + ".WAY");
    // QFile::remove(rootFilePath + ".DAT");
}

bool ClassicConnector::getCTSMetadata(const QString& ctsFile, QVariantMap* metadataOut)
{
    auto metadata = QVariantMap();

    auto fileMap = FXFILEMAP();
    if (!FxMapFile(ctsFile.toStdString().c_str(), &fileMap))
    {
        return false;
    }

    auto result = false;
    auto header = static_cast<CTRESOURCEHEADER*>(fileMap.BasePointer);

    if (header->Magic == MAGIC_RESOURCE)
    {
        result = true;
        metadata["name"] = QString(header->Name);
        metadata["serverFileName"] = QString(header->ServerFileName);
        metadata["app"] = QFileInfo(ctsFile).baseName();
        metadata["webUpdateUrl"] = QString(header->WebUpdateUrl);

        auto protocolTable = QMap<int, QString>
        {
            { FXSENDDATA_PROTOCOL_UNKNOWN, "NONE"   },
            { FXSENDDATA_PROTOCOL_FTP,     "FTP"    },
            { FXSENDDATA_PROTOCOL_UNC,     "UNC"    },
            { FXSENDDATA_PROTOCOL_HTTP,    "HTTP"   },
            { FXSENDDATA_PROTOCOL_HTTPS,   "HTTPS"  },
            { FXSENDDATA_PROTOCOL_BACKUP,  "BACKUP" },
            { FXSENDDATA_PROTOCOL_HTTPZ,   "HTTPZ"  },
            { FXSENDDATA_PROTOCOL_ESRI,    "ESRI"   },
            { FXSENDDATA_PROTOCOL_AZURE,   "AZURE"  },
        };

        auto protocol = protocolTable.value(header->SendData.Protocol);
        metadata["protocol"] = protocol;
        metadata["username"] = QString::fromUtf8(header->SendData.UserName);
        metadata["password"] = QString::fromUtf8(header->SendData.Password);

        metadata["telemetry"] = QVariantMap
        {
            { "sendData", protocol },
        };

        metadata["clearDataOnSend"] = header->ClearDataOnSend != 0;

        auto appGuid = header->SequenceId;
        char appId[39] = { 0 };
        sprintf(appId, "%08lx%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                appGuid.Data1, appGuid.Data2, appGuid.Data3, appGuid.Data4[0],
                appGuid.Data4[1], appGuid.Data4[2], appGuid.Data4[3], appGuid.Data4[4],
                appGuid.Data4[5], appGuid.Data4[6], appGuid.Data4[7]);

        metadata["appId"] = QString(appId);

        auto stampGuid = header->StampId;
        char stampId[39] = { 0 };
        sprintf(stampId, "%08lx%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                stampGuid.Data1, stampGuid.Data2, stampGuid.Data3, stampGuid.Data4[0],
                stampGuid.Data4[1], stampGuid.Data4[2], stampGuid.Data4[3],
                stampGuid.Data4[4], stampGuid.Data4[5], stampGuid.Data4[6],
                stampGuid.Data4[7]);

        metadata["stampId"] = QString(stampId);

        auto movingMaps = QStringList();
        if (header->MovingMapsOffset != 0)
        {
            for (auto i = 0; i < (int)header->MovingMapsCount; i++)
            {
                auto movingMap = (FXMOVINGMAP *)((quint64)fileMap.BasePointer + (quint64)header->MovingMapsOffset + i * sizeof(FXMOVINGMAP));
                movingMaps.append(movingMap->FileName);
            }
        }
        metadata["movingMaps"] = movingMaps;
    }

    FxUnmapFile(&fileMap);

    *metadataOut = metadata;

    return result;
}

ApiResult ClassicConnector::copyAppFiles(Project* project, const QString& targetPath)
{
    auto rootPath = Utils::classicRoot();
    if (!Utils::ensurePath(rootPath, true))
    {
        return Failure(tr("Cannot create path"));
    }

    auto app = project->connectorParams().value("app").toString();

    auto metadata = QVariantMap();
    if (!getCTSMetadata(rootPath + "/" + app + ".CTS", &metadata))
    {
        return Failure(QString(tr("Failed to read %1")).arg(App::instance()->alias_project()));
    }

    auto copyFile = [&](const QString& suffix)
    {
        auto fileInfo = QFileInfo(rootPath + "/" + app + suffix);
        if (fileInfo.exists())
        {
            QFile::copy(fileInfo.filePath(), targetPath + "/app" + suffix.toLower());
        }
    };

    auto suffixes = QStringList { ".CTS", ".TXT", ".DEF", ".HSI", ".HST" };
    for (auto it = suffixes.constBegin(); it != suffixes.constEnd(); it++)
    {
        copyFile(*it);
    }

    auto movingMaps = metadata.value("movingMaps").toStringList();
    for (auto it = movingMaps.constBegin(); it != movingMaps.constEnd(); it++)
    {
        QFile::copy(rootPath + "/" + *it, targetPath + "/" + *it);
    }

    return Success();
}

ApiResult ClassicConnector::installAppFiles(Project* project, const QString& sourcePath)
{
    auto rootPath = Utils::classicRoot();
    if (!Utils::ensurePath(rootPath, true))
    {
        return Failure(tr("Cannot create path"));
    }

    auto app = project->connectorParams().value("app").toString();

    auto fileInfoList = QDir(sourcePath).entryInfoList(QDir::Files);
    for (auto it = fileInfoList.constBegin(); it != fileInfoList.constEnd(); it++)
    {
        auto fileInfo = *it;
        if (fileInfo.fileName().startsWith("app."))
        {
            QFile::copy(fileInfo.filePath(), rootPath + "/" + app + "." + fileInfo.suffix().toUpper());
            continue;
        }

        QFile::copy(fileInfo.filePath(), rootPath + "/" + fileInfo.fileName());
    }

    return Success();
}

} // namespace CyberTracker
