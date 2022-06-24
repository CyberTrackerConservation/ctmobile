#include "EarthRangerConnector.h"
#include "EarthRangerProvider.h"
#include "App.h"

EarthRangerConnector::EarthRangerConnector(QObject *parent) : Connector(parent)
{
    update_name(EARTH_RANGER_CONNECTOR);
    update_icon(App::instance()->settings()->darkTheme() ? "qrc:/EarthRanger/logoDark.svg" : "qrc:/EarthRanger/logo.svg");
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
        { "connector", EARTH_RANGER_CONNECTOR },
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
    }

    return result;
}

bool EarthRangerConnector::loggedIn(Project* project) const
{
    return !project->accessToken().isEmpty();
}

bool EarthRangerConnector::login(Project* project, const QString& server, const QString& username, const QString& password)
{
    auto serverUrl = QUrl(server);
    serverUrl.setPath("/oauth2/token");

    auto query = QUrlQuery();
    query.addQueryItem("grant_type", "password");
    query.addQueryItem("username", username);
    query.addQueryItem("password", password);
    query.addQueryItem("client_id", EARTH_RANGER_MOBILE_CLIENT_ID);

    auto response = Utils::httpPostQuery(App::instance()->networkAccessManager(), serverUrl.toString(), query);
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

void EarthRangerConnector::logout(Project* project)
{
    project->set_accessToken("");
    project->set_refreshToken("");
}

ApiResult EarthRangerConnector::bootstrap(const QVariantMap& params)
{
    auto serverUrl = params["server"].toUrl();
    serverUrl.setPath("");
    auto server = Utils::trimUrl(serverUrl.toString());
    auto username = params["username"].toString();
    auto password = params["password"].toString();
    auto accessToken = params.value("accessToken").toString();
    auto refreshToken = params.value("refreshToken").toString();

    auto projectUid = QString(EARTH_RANGER_CONNECTOR) + "_" + QUrl(server).host().replace('.', '_');
    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    if (!m_projectManager->init(projectUid, EARTH_RANGER_PROVIDER, QVariantMap(), EARTH_RANGER_CONNECTOR, QVariantMap {{"server", server }}))
    {
        return Failure(tr("Failed to create project"));
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
                auto response = Utils::httpGetWithToken(App::instance()->networkAccessManager(), server + "/api/v1.0/user/me", accessToken);
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

    // Run an update.
    return bootstrapUpdate(projectUid);
}

ApiResult EarthRangerConnector::hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project)
{
    auto connectorParams = project->connectorParams();
    auto server = connectorParams["server"].toString();
    auto accessToken = project->accessToken();

    auto response = Utils::httpGetWithToken(networkAccessManager, server + "/api/v1.0/activity/events/schema?format=json", accessToken);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    if (response.etag == connectorParams["etag"])
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

bool EarthRangerConnector::canUpdate(Project* project)
{
    return loggedIn(project);
}

ApiResult EarthRangerConnector::update(Project* project)
{
    // Skip update if not logged in.
    auto projectUid = project->uid();
    if (!loggedIn(project))
    {
        return Failure(tr("Logged out"));
    }

    // Skip update if there is any unsent data (represented by unfinished tasks).
    QStringList uids, parentUids;
    m_database->getTasks(projectUid, Task::State::Active | Task::State::Paused, &uids, &parentUids);
    if (uids.count() > 0)
    {
        return Failure(tr("Unsent data"));
    }

    auto networkAccessManager = App::instance()->networkAccessManager();
    auto connectorParams = project->connectorParams();
    auto server = connectorParams["server"].toString();
    auto accessToken = project->accessToken();

    // Check if an update is required.
    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    auto response = Utils::httpGetWithToken(networkAccessManager, server + "/api/v1.0/activity/events/schema?format=json", accessToken);
    if (response.success)
    {
        if (response.etag == connectorParams["etag"])
        {
            return AlreadyUpToDate();
        }

        connectorParams["etag"] = response.etag;

        auto jsonObj = QJsonDocument::fromJson(response.data).object().value("data").toObject();
        Utils::writeJsonToFile(updateFolder + "/schema.json", QJsonDocument(jsonObj).toJson());
    }

    // Update the project.
    App::instance()->initProgress(projectUid, "EarthRangerConnect", { 10, 100, 100 });

    // eventtypes.json
    auto eventTypes = QStringList();
    {
        response = Utils::httpGetWithToken(networkAccessManager, server + "/api/v1.0/activity/events/eventtypes?format=json", accessToken);
        if (!response.success)
        {
            return Failure(response.errorString);
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

            response = Utils::httpGetWithToken(networkAccessManager, server + "/api/v1.0/activity/events/schema/eventtype/" + eventType + "?definition=flat&format=json", accessToken);
            if (!response.success)
            {
                return Failure(response.errorString);
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
            response = Utils::httpGetWithToken(networkAccessManager, iconIt->second, accessToken);
            if (!response.success)
            {
                return Failure(response.errorString);
            }

            QFile file(updateFolder + "/" + iconIt->first + ".svg");
            file.open(QIODevice::WriteOnly);
            file.write(response.data);
            file.close();

            App::instance()->sendProgress((index++ * 100) / icons.count(), 2);
        }
    }

    // Finalize.
    {
        Project updateProject;
        updateProject.set_uid(projectUid);
        updateProject.set_provider(EARTH_RANGER_PROVIDER);
        updateProject.set_providerParams(project->providerParams());
        updateProject.set_connector(project->connector());
        updateProject.set_connectorParams(connectorParams);
        updateProject.set_version(1);
        updateProject.set_title("EarthRanger");
        updateProject.set_subtitle(QUrl(server).host());
        updateProject.set_icon("qrc:/EarthRanger/logo.svg");
        updateProject.set_iconDark("qrc:/EarthRanger/logoDark.svg");

        auto androidPermissions = QStringList();
        androidPermissions << "CAMERA";
        androidPermissions << "RECORD_AUDIO";
        androidPermissions << "ACCESS_FINE_LOCATION";
        androidPermissions << "ACCESS_COARSE_LOCATION";
        updateProject.set_androidPermissions(androidPermissions);

        auto colors = QVariantMap();
        colors["primary"] = "#607d8b";
        colors["accent"] = "#f4511e";
        updateProject.set_colors(colors);

        updateProject.saveToQmlFile(updateFolder + "/Project.qml");
    }

    // Reset the project database state.
    m_projectManager->reset(projectUid);

    return Success();
}
