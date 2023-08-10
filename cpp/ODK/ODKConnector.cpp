#include "ODKConnector.h"
#include "ODKProvider.h"
#include "App.h"
#include "XlsForm.h"

ODKConnector::ODKConnector(QObject *parent) : Connector(parent)
{
    update_name(ODK_CONNECTOR);
    update_icon("qrc:/ODK/logo.svg");
}

bool ODKConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool ODKConnector::canShareAuth(Project* project) const
{
    return loggedIn(project);
}

QVariantMap ODKConnector::getShareData(Project* project, bool auth) const
{
    auto connectorParams = project->connectorParams();

    auto result = QVariantMap
    {
        { "connector", ODK_CONNECTOR },
        { "server", connectorParams["server"] },
        { "projectId", connectorParams["projectId"] },
        { "formId", connectorParams["formId"] },
        { "launch", true }
    };

    if (auth || !loggedIn(project))
    {
        result.insert("auth", true);
    }
    else
    {
        result.insert("token", project->accessToken());
    }

    return result;
}

bool ODKConnector::canLogin(Project* /*project*/) const
{
    return true;
}

bool ODKConnector::loggedIn(Project* project) const
{
    return !project->accessToken().isEmpty();
}

bool ODKConnector::login(Project* project, const QString& server, const QString& username, const QString& password)
{
    auto serverUrl = QUrl(server);
    serverUrl.setPath("/v1/sessions");

    auto response = Utils::httpPostJson(App::instance()->networkAccessManager(), serverUrl.toString(), QVariantMap {{ "email", username}, {"password", password }});

    if (response.success)
    {
        auto token = QJsonDocument::fromJson(response.data).object().value("token").toString();
        if (!token.isEmpty())
        {
            project->set_accessToken(token);
            return true;
        }
    }

    qDebug() << "Login failed: " << response.errorString;

    return false;
}

void ODKConnector::logout(Project* project)
{
    project->set_accessToken("");
}

ApiResult ODKConnector::bootstrap(const QVariantMap& params)
{
    auto projectId = params["projectId"].toString();
    auto formId = params["formId"].toString();
    auto projectUid = "ODK_" + projectId + "_" + formId;

    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    auto serverUrl = params["server"].toUrl();
    serverUrl.setPath("");
    auto server = serverUrl.toString();
    auto username = params["username"].toString();
    auto password = params["password"].toString();
    auto token = params["token"].toString();

    auto connectorParams = QVariantMap
    {
        { "server", server },
        { "tokenType", "Bearer" },
        { "projectId", projectId },
        { "formId", formId },
        { "exportId", formId },
        { "submissionUrl", server + "/v1/projects/" + projectId + "/submission" }
    };

    if (!m_projectManager->init(projectUid, ODK_PROVIDER, QVariantMap(), ODK_CONNECTOR, connectorParams))
    {
        return Failure(QString(tr("Failed to create %1")).arg(App::instance()->alias_project()));
    }

    m_projectManager->modify(projectUid, [&](Project* project)
    {
        if (token.isEmpty() && !username.isEmpty() && !password.isEmpty())
        {
            login(project, server, username, password);
        }
        else
        {
            // Pick up the username if not provided.
            if (username.isEmpty() && !token.isEmpty())
            {
                auto response = Utils::httpGetWithToken(App::instance()->networkAccessManager(), server + "/v1/users/current", token);
                if (response.success)
                {
                    username = QJsonDocument::fromJson(response.data).object().value("email").toString();
                }
            }

            project->set_username(username);
            project->set_accessToken(token);
        }
    });

    return bootstrapUpdate(projectUid);
}

ApiResult ODKConnector::hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project)
{
    // Retrieve a new form.
    auto connectorParams = project->connectorParams();
    auto server = connectorParams.value("server").toString();
    auto token = project->accessToken();
    auto tokenType = connectorParams.value("tokenType").toString();
    auto projectId = connectorParams.value("projectId").toString();
    auto formId = connectorParams.value("formId").toString();

    // Read the form metadata.
    auto response = Utils::httpGetWithToken(networkAccessManager, server + "/v1/projects/" + projectId + "/forms/" + formId + "/versions", token, tokenType);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto formMap = QJsonDocument::fromJson(response.data).array().first().toObject().toVariantMap();

    // Check if up to date.
    auto versionName = formMap.value("version").toString();
    if (versionName == connectorParams["version"])
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

bool ODKConnector::canUpdate(Project* project) const
{
    return loggedIn(project);
}

ApiResult ODKConnector::update(Project* project)
{
    auto projectUid = project->uid();

    // Skip updates if there are any unsent sightings.
//    auto sightingUids = QStringList();
//    m_database->getSightings(projectUid, "", Sighting::DB_SIGHTING_FLAG, &sightingUids);
//    if (sightingUids.count() > 0)
//    {
//        return Failure(tr("Unsent data"));
//    }

    // Reset the project.
    m_projectManager->reset(projectUid, true);

    // Retrieve a new form.
    auto connectorParams = project->connectorParams();
    auto server = connectorParams.value("server").toString();
    auto token = project->accessToken();
    auto tokenType = connectorParams.value("tokenType").toString();
    auto projectId = connectorParams.value("projectId").toString();
    auto formId = connectorParams.value("formId").toString();

    // Read the form metadata.
    auto networkAccessManager = App::instance()->networkAccessManager();
    auto response = Utils::httpGetWithToken(networkAccessManager, server + "/v1/projects/" + projectId + "/forms/" + formId + "/versions", token, tokenType);
    if (!response.success)
    {
        if (response.status == 401)
        {
            logout(project);
        }

        return Failure(response.errorString);
    }

    auto formMap = QJsonDocument::fromJson(response.data).array().first().toObject().toVariantMap();

    // Check if up to date.
    auto versionName = formMap.value("version").toString();
    if (versionName == connectorParams["version"])
    {
        return AlreadyUpToDate();
    }

    connectorParams["version"] = versionName.isEmpty() ? "???" : versionName;

    // Download the form.
    response = Utils::httpGetWithToken(networkAccessManager, server + "/v1/projects/" + projectId + "/forms/" + formId + "/versions/" + versionName + ".xlsx", token, tokenType);
    if (!response.success)
    {
        if (response.status == 401)
        {
            logout(project);
        }

        return Failure(response.errorString);
    }

    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    Utils::writeDataToFile(updateFolder + "/form.xlsx", response.data);
    if (!QFile::exists(updateFolder + "/form.xlsx"))
    {
        if (response.status == 401)
        {
            logout(project);
        }

        return Failure(tr("Download failed"));
    }

    auto formSettings = QVariantMap();
    if (!XlsFormParser::parseSettings(updateFolder + "/form.xlsx", &formSettings))
    {
        qDebug() << "Failed to read form settings sheet";
    }

    // Download the media.
    response = Utils::httpGetWithToken(networkAccessManager, server + "/v1/projects/" + projectId + "/forms/" + formId + "/versions/" + versionName + "/attachments", token, tokenType);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto mediaArr = QJsonDocument::fromJson(response.data).array();
    for (auto mediaIt = mediaArr.constBegin(); mediaIt != mediaArr.constEnd(); mediaIt++)
    {
        auto mediaObj = (*mediaIt).toObject();
        if (!mediaObj.value("exists").toBool())
        {
            continue;
        }

        auto mediaFilename = mediaObj.value("name").toString();
        auto mediaUrl = server + "/v1/projects/" + projectId + "/forms/" + formId + "/versions/" + versionName + "/attachments/" + mediaFilename;
        response = Utils::httpGetWithToken(App::instance()->networkAccessManager(), mediaUrl, token, tokenType);
        if (!response.success)
        {
            if (response.status == 401)
            {
                logout(project);
            }

            return Failure(response.errorString);
        }

        Utils::writeDataToFile(updateFolder + "/" + mediaFilename, response.data);
    }

    // Update the project.
    Project updateProject;
    updateProject.set_uid(projectUid);
    updateProject.set_provider(ODK_PROVIDER);

    connectorParams["version"] = versionName;
    updateProject.set_connector(ODK_CONNECTOR);
    updateProject.set_connectorParams(connectorParams);
    updateProject.set_version(1);
    updateProject.set_title(formMap["name"].toString());
    updateProject.set_subtitle(QUrl(server).host());
    updateProject.set_icon("qrc:/ODK/logo.svg");
    updateProject.set_colors(QVariantMap {{ "primary", "#3E77B4" }, { "accent", "#904A22" }});
    updateProject.set_defaultWizardMode(true);
    XlsFormParser::configureProject(&updateProject, formSettings);

    updateProject.saveToQmlFile(updateFolder + "/Project.qml");

    // Reset the project.
    m_projectManager->reset(projectUid, true);

    return Success();
}

void ODKConnector::reset(Project* project)
{
    auto path = m_projectManager->getFilePath(project->uid());

    QFile::remove(path + "/Elements.qml");
    QFile::remove(path + "/Fields.qml");
    QFile::remove(path + "/Settings.json");
}
