#include "EarthRangerConnector.h"
#include "EarthRangerProvider.h"
#include "EarthRangerSchema.h"
#include "App.h"

namespace CyberTracker {

EarthRangerConnector::EarthRangerConnector(QObject *parent) : Connector(parent)
{
    update_name(CONNECTOR_NAME);
    update_icon(App::instance()->settings()->darkTheme() ? "qrc:/EarthRanger/logoDark.svg" : "qrc:/EarthRanger/logo.svg");
}

EarthRangerConnector::~EarthRangerConnector()
{
}

bool EarthRangerConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool EarthRangerConnector::canShareAuth(Project* project) const
{
    return loggedIn(project);
}

QVariantMap EarthRangerConnector::getShareData(Project* project, bool auth) const
{
    auto result = QVariantMap
    {
        { "connector", CONNECTOR_NAME },
        { "server", project->connectorParams()["server"].toString() },
        { "launch", true }
    };

    if (auth || !loggedIn(project))
    {
        result.insert("auth", true);
    }
    else
    {
        result.insert("accessToken", project->accessToken());
        result.insert("refreshToken", project->refreshToken());
    }

    return result;
}

bool EarthRangerConnector::canLogin(Project* /*project*/) const
{
    return true;
}

bool EarthRangerConnector::loggedIn(Project* project) const
{
    return !project->refreshToken().isEmpty();
}

bool EarthRangerConnector::login(Project* project, const QString& server, const QString& username, const QString& password) const
{
    auto serverUrl = QUrl(server);
    serverUrl.setPath("/oauth2/token");

    auto query = QUrlQuery();
    query.addQueryItem("grant_type", "password");
    query.addQueryItem("username", username);
    query.addQueryItem("password", password);
    query.addQueryItem("client_id", MOBILE_CLIENT_ID);

    auto response = Utils::httpPostQuery(m_networkAccessManager, serverUrl.toString(), query);
    if (response.success)
    {
        auto jsonObj = QJsonDocument::fromJson(response.data).object();
        auto accessToken = jsonObj.value("access_token").toString();
        auto refreshToken = jsonObj.value("refresh_token").toString();
        if (!accessToken.isEmpty() && !refreshToken.isEmpty())
        {
            project->set_username(username);
            project->set_accessToken(accessToken);
            project->set_refreshToken(refreshToken);
            return true;
        }
    }

    qDebug() << "Login failed: " << response.data;

    return false;
}

void EarthRangerConnector::logout(Project* project) const
{
    project->set_accessToken("");
    project->set_refreshToken("");
}

QString EarthRangerConnector::getFormFilePath(Project* project) const
{
    return m_projectManager->getFilePath(project->uid(), "eventtypes.json");
}

ApiResult EarthRangerConnector::bootstrap(const QVariantMap& params) const
{
    auto serverUrl = params["server"].toUrl();
    serverUrl.setPath("");
    auto server = Utils::trimUrl(serverUrl.toString());
    auto username = params["username"].toString();
    auto password = params["password"].toString();
    auto accessToken = params.value("accessToken").toString();
    auto refreshToken = params.value("refreshToken").toString();

    auto projectUid = QString(CONNECTOR_NAME) + "_" + QUrl(server).host().replace('.', '_');
    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    if (!m_projectManager->init(projectUid, EarthRangerProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, QVariantMap {{ "server", server }}))
    {
        return Failure(QString(tr("Failed to create %1")).arg(App::instance()->alias_project()));
    }

    m_projectManager->modify(projectUid, [&](Project* project)
    {
        // Login if no token.
        if (accessToken.isEmpty() && !username.isEmpty() && !password.isEmpty())
        {
            login(project, server, username, password);
        }
        else
        {
            // Pick up the username if not provided.
            if (username.isEmpty() && !accessToken.isEmpty())
            {
                auto response = Utils::httpGetWithToken(m_networkAccessManager, server + "/api/v1.0/user/me", accessToken);
                if (response.success)
                {
                    auto jsonObj = QJsonDocument::fromJson(response.data).object().value("data").toObject();
                    username = jsonObj.value("username").toString();
                }
            }

            // Update the project.
            project->set_username(username);
            project->set_accessToken(accessToken);
            project->set_refreshToken(refreshToken);
        }
    });

    // Activate the form filler.
    App::instance()->settings()->set_formFiller(true);

    // Run an update.
    return bootstrapUpdate(projectUid);
}

ApiResult EarthRangerConnector::refreshAccessToken(Project* project, QString* oldRefreshTokenOut, QString* newAccessTokenOut, QString* newRefreshTokenOut)
{
    auto oldRefreshToken = project->refreshToken();
    if (oldRefreshToken.isEmpty())
    {
        return Unauthorized();
    }

    auto query = QUrlQuery();
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", oldRefreshToken);
    query.addQueryItem("client_id", MOBILE_CLIENT_ID);
    auto payload = query.toString(QUrl::FullyEncoded).toUtf8();

    auto serverUrl = Utils::trimUrl(project->getConnectorParam("server").toString()) + "/oauth2/token";

    auto request = Utils::httpCreateRequest(serverUrl);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");

    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->post(request, payload));

    Utils::httpWait(reply.get());

    auto response = Utils::HttpResponse::fromReply(serverUrl, reply.get());
    if (!response.success)
    {
        *oldRefreshTokenOut = oldRefreshToken;
        return Failure(response);
    }

    auto jsonObj = QJsonDocument::fromJson(response.data).object();
    auto accessToken = jsonObj["access_token"].toString();
    auto refreshToken = jsonObj["refresh_token"].toString();

    if (accessToken.isEmpty() || refreshToken.isEmpty())
    {
        *oldRefreshTokenOut = oldRefreshToken;
        return Unauthorized();
    }

    *oldRefreshTokenOut = oldRefreshToken;
    *newAccessTokenOut = accessToken;
    *newRefreshTokenOut = refreshToken;

    return Success();
}

ApiResult EarthRangerConnector::hasUpdate(Project* project)
{
    auto server = project->getConnectorParam("server").toString();
    auto token = project->accessToken();
    auto url = server + "/api/v1.0/activity/events/schema?format=json";
    auto currentVersion = project->getConnectorParam("etag").toString();

    auto request = Utils::httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Authorization", QString("Bearer " + token).toUtf8());

    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->get(request));

    Utils::httpWait(reply.get());

    auto response = Utils::HttpResponse::fromReply(url, reply.get());
    if (!response.success)
    {
        return Failure(response);
    }

    auto deployedVersion = response.etag;
    if (deployedVersion.isEmpty())
    {
        return Failure("No deployed versions");
    }

    if (deployedVersion == currentVersion)
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

ApiResult EarthRangerConnector::uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* state, const CommitState& commitState)
{
    auto sightingUrl = state->value("sightingUrl").toString();
    auto eventId = state->value("eventId").toString();

    // Upload sighting.
    {
        auto request = Utils::httpCreateRequest();
        request.setRawHeader("Connection", "keep-alive");
        request.setRawHeader("Accept", "application/json, text/plain, */*");
        request.setRawHeader("Content-Type", "application/json; charset=utf-8");
        request.setRawHeader("Authorization", QString("Bearer " + project->accessToken()).toUtf8());

        auto reply = std::unique_ptr<QNetworkReply>();

        // POST for a new sighting, PATCH for update of an existing sighting.
        if (sightingUrl.isEmpty())
        {
            request.setUrl(queueItem.url + "/api/v1.0/activity/events");
            reply.reset(m_networkAccessManager->post(request, queueItem.payload));
        }
        else
        {
            request.setUrl(sightingUrl);
            reply.reset(m_networkAccessManager->sendCustomRequest(request, "PATCH", queueItem.payload));
        }

        Utils::httpWait(reply.get());

        auto response = Utils::HttpResponse::fromReply(request.url().toString(), reply.get());
        if (!response.success)
        {
            return ResultFromResponse(response);
        }

        auto jsonData = QJsonDocument::fromJson(response.data).object().value("data").toObject().toVariantMap();
        eventId = jsonData.value("id").toString();
        state->insert("eventId", eventId);
        sightingUrl = jsonData.value("url").toString();
        state->insert("sightingUrl", sightingUrl);
        state->insert("unpatchable", response.status == 201 && sightingUrl.isEmpty());
        commitState();
    }

    // Upload files.
    for (auto attachmentIt = queueItem.attachments.constBegin(); attachmentIt != queueItem.attachments.constEnd(); attachmentIt++)
    {
        auto filename = *attachmentIt;

        // Skip bad filenames.
        if (filename.trimmed().isEmpty())
        {
            continue;
        }

        // Skip already sent files.
        if (state->contains(filename))
        {
            continue;
        }

        // Skip missing files - this likely happens when the user modifies an existing sighting's picture field which
        // had not yet been sent. As a result, the media file may be missing. Easiest thing to do is just skip it.
        auto filePath = App::instance()->getMediaFilePath(filename);
        if (!QFile::exists(filePath))
        {
            qDebug() << "Warning: missing attachment: " << filename;
            continue;
        }

        auto multipart = QHttpMultiPart(QHttpMultiPart::FormDataType);
        multipart.setBoundary(Utils::makeMultiPartBoundary());
        auto part = QHttpPart();
        part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"filecontent.file\"; filename=\"" + filename + "\"");

        auto file = QFile(filePath);
        qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to open attachment");
        part.setBodyDevice(&file);
        multipart.append(part);

        auto request = Utils::httpCreateRequest(sightingUrl + "/files/");
        request.setRawHeader("Connection", "keep-alive");
        request.setRawHeader("Accept", "application/json, text/plain, */*");
        request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + multipart.boundary());
        request.setRawHeader("Authorization", QString("Bearer " + project->accessToken()).toUtf8());

        if (sightingUrl.isEmpty() && !eventId.isEmpty())
        {
            sightingUrl = queueItem.url + "/api/v1.0/activity/event/" + eventId;
        }

        request.setUrl(sightingUrl + "/files/");

        auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->post(request, &multipart));

        Utils::httpWait(reply.get());

        auto response = Utils::HttpResponse::fromReply(request.url().toString(), reply.get());
        if (!response.success)
        {
            return ResultFromResponse(response);
        }

        state->insert(filename, "sent");
        commitState();
    }

    // Upload note.
    if (state->value("note").toString() != state->value("sentNote").toString())
    {
        auto request = Utils::httpCreateRequest();
        request.setRawHeader("Connection", "keep-alive");
        request.setRawHeader("Accept", "application/json, text/plain, */*");
        request.setRawHeader("Content-Type", "application/json; charset=utf-8");
        request.setRawHeader("Authorization", QString("Bearer " + project->accessToken()).toUtf8());

        if (sightingUrl.isEmpty() && !eventId.isEmpty())
        {
            sightingUrl = queueItem.url + "/api/v1.0/activity/event/" + eventId;
        }

        auto reply = std::unique_ptr<QNetworkReply>();

        auto payload = QVariantMap {{ "text", state->value("note", "[removed]").toString() }};
        auto noteId = state->value("noteId").toString();
        if (noteId.isEmpty())
        {
            request.setUrl(sightingUrl + "/notes/");
            reply.reset(m_networkAccessManager->post(request, Utils::variantMapToJson(payload)));
        }
        else
        {
            payload.insert("id", noteId);
            request.setUrl(sightingUrl + "/note/" + noteId + "/");
            reply.reset(m_networkAccessManager->sendCustomRequest(request, "PATCH", Utils::variantMapToJson(payload)));
        }

        Utils::httpWait(reply.get());

        auto response = Utils::HttpResponse::fromReply(request.url().toString(), reply.get());
        if (!response.success)
        {
            return ResultFromResponse(response);
        }

        auto jsonData = QJsonDocument::fromJson(response.data).object().value("data").toObject().toVariantMap();
        noteId = jsonData.value("id").toString();
        state->insert("noteId", noteId);
        state->insert("sentNote", state->value("note").toString());
        commitState();
    }

    return Success();
}

bool EarthRangerConnector::canUpdate(Project* project) const
{
    return loggedIn(project);
}

ApiResult EarthRangerConnector::update(Project* project) const
{
    // Skip update if not logged in.
    auto projectUid = project->uid();
    if (!loggedIn(project))
    {
        return Failure(tr("Logged out"));
    }

    // Update.
    auto connectorParams = project->connectorParams();
    auto server = connectorParams["server"].toString();
    auto accessToken = project->accessToken();

    // Check if an update is required.
    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    auto response = Utils::httpGetWithToken(m_networkAccessManager, server + "/api/v1.0/activity/events/schema?format=json", accessToken);
    if (response.success)
    {
        if (response.etag == connectorParams["etag"])
        {
            return AlreadyUpToDate();
        }

        project->setConnectorParam("etag", response.etag);

        auto jsonObj = QJsonDocument::fromJson(response.data).object().value("data").toObject();
        Utils::writeJsonToFile(updateFolder + "/schema.json", QJsonDocument(jsonObj).toJson());
    }

    // Download choice icons zip.
    auto iconsZipFilePath = App::instance()->downloadFile(server + "/api/v1.0/choices/icons/download", "", "", accessToken);
    if (!iconsZipFilePath.isEmpty())
    {
        Utils::zipExtractToDir(iconsZipFilePath, updateFolder);
        QFile::remove(iconsZipFilePath);
    }

    // Update the project.
    App::instance()->initProgress(projectUid, "EarthRangerConnect", { 10, 100, 100 });

    // eventtypes.json
    auto eventTypes = QStringList();
    {
        response = Utils::httpGetWithToken(m_networkAccessManager, server + "/api/v1.0/activity/events/eventtypes?format=json", accessToken);
        if (!response.success)
        {
            return Failure(response);
        }

        auto jsonArr = QJsonDocument::fromJson(response.data).object().value("data").toArray();
        Utils::writeJsonToFile(updateFolder + "/eventtypes.json", QJsonDocument(jsonArr).toJson());

        for (auto it = jsonArr.constBegin(); it != jsonArr.constEnd(); it++)
        {
            auto map = it->toObject().toVariantMap();
            eventTypes.append(map["value"].toString());
        }

        App::instance()->sendProgress(10, 0);
    }

    // event reports.
    QMap<QString, QString> icons;
    {
        auto index = 0;
        for (auto eventIt = eventTypes.constBegin(); eventIt != eventTypes.constEnd(); eventIt++)
        {
            auto eventType = *eventIt;

            response = Utils::httpGetWithToken(m_networkAccessManager, server + "/api/v1.0/activity/events/schema/eventtype/" + eventType + "?definition=flat&format=json", accessToken);
            if (!response.success)
            {
                return Failure(response);
            }

            auto jsonObj = QJsonDocument::fromJson(response.data).object().value("data").toObject();
            Utils::writeJsonToFile(updateFolder + "/" + eventType + ".json", QJsonDocument(jsonObj).toJson());

            auto schemaObject = jsonObj.value("schema").toObject();
            auto iconUrl = schemaObject.value("image_url").toString();
            auto iconId = schemaObject.value("icon_id").toString();
            if (!iconUrl.isEmpty() && !iconId.isEmpty())
            {
                icons.insert(iconId, iconUrl);
            }

            App::instance()->sendProgress((index++ * 100) / eventTypes.count(), 1);
        }
    }

    // icons.
    {
        auto index = 0;
        for (auto iconIt = icons.constKeyValueBegin(); iconIt != icons.constKeyValueEnd(); iconIt++)
        {
            response = Utils::httpGetWithToken(m_networkAccessManager, iconIt->second, accessToken);
            if (!response.success)
            {
                return Failure(response);
            }

            QFile file(updateFolder + "/" + iconIt->first + ".svg");
            file.open(QIODevice::WriteOnly);
            file.write(response.data);
            file.close();

            App::instance()->sendProgress((index++ * 100) / icons.count(), 2);
        }
    }

    // Reset the project.
    m_projectManager->reset(projectUid, true);

    // Prepare the project.
    return buildSchema(project, updateFolder);
}

ApiResult EarthRangerConnector::buildSchema(Project* project, const QString& path) const
{
    project->set_provider(EarthRangerProvider::PROVIDER_NAME);
    project->set_version(1);
    project->set_name("EarthRanger");
    project->set_subtitle(QUrl(project->getConnectorParam("server").toUrl()).host());
    project->set_icon("qrc:/EarthRanger/logo.svg");
    project->set_iconDark("qrc:/EarthRanger/logoDark.svg");
    project->set_requireCamera(true);
    project->set_requireMicrophone(true);
    project->set_requireLocation(true);
    project->set_wizardAutosave(false);

    auto colors = QVariantMap();
    colors["primary"] = "#5E4C40";
    colors["accent"] = "#6DC4BC";
    project->set_colors(colors);

    auto schema = Schema();
    if (!EarthRangerSchema::parse(path, &schema))
    {
        return Failure("Parse error");
    }

    // Commit the project and schema file.
    project->saveToQmlFile(path + "/Project.qml");
    schema.setProjectInfo(project->title(), project->subtitle(), project->icon());
    schema.saveToQmlFile(path + "/Form.qml");

    return Success();
}

void EarthRangerConnector::reset(Project* project) const
{
    QFile::remove(m_projectManager->getFilePath(project->uid(), "Form.qml"));
}

} // namespace CyberTracker
