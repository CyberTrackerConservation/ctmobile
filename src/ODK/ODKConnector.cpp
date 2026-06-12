#include "ODKConnector.h"
#include "ODKProvider.h"
#include "App.h"
#include "XlsForm.h"

namespace CyberTracker {

ODKConnector::ODKConnector(QObject *parent) : Connector(parent)
{
    update_name(CONNECTOR_NAME);
    update_icon("qrc:/ODK/logo.svg");
}

ODKConnector::~ODKConnector()
{
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
        { "connector", CONNECTOR_NAME },
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

    if (project->defaultTrackFieldUid().isEmpty() && !project->trackFieldUid().isEmpty())
    {
        result.insert("trackFieldUid", project->trackFieldUid());
        result.insert("trackFileFormat", project->trackFileFormat());
        result.insert("pointInterval", project->locationPointInterval());
        result.insert("trackInterval", project->locationTrackInterval());
        result.insert("trackDistance", project->locationTrackDistance());
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

bool ODKConnector::login(Project* project, const QString& server, const QString& username, const QString& password) const
{
    auto serverUrl = QUrl(server);
    serverUrl.setPath("/v1/sessions");

    auto response = Utils::httpPostJson(m_networkAccessManager, serverUrl.toString(), QVariantMap {{ "email", username}, {"password", password }});

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

void ODKConnector::logout(Project* project) const
{
    project->set_accessToken("");
}

QString ODKConnector::getFormFilePath(Project* project) const
{
    return m_projectManager->getFilePath(project->uid(), "form.xlsx");
}

ApiResult ODKConnector::bootstrap(const QVariantMap& params) const
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
    auto trackFieldUid = params["trackFieldUid"].toString();

    auto connectorParams = QVariantMap
    {
        { "server", server },
        { "tokenType", "Bearer" },
        { "projectId", projectId },
        { "formId", formId },
        { "exportId", formId },
        { "submissionUrl", server + "/v1/projects/" + projectId + "/submission" }
    };

    if (!m_projectManager->init(projectUid, ODKProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, connectorParams))
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
                auto response = Utils::httpGetWithToken(m_networkAccessManager, server + "/v1/users/current", token);
                if (response.success)
                {
                    username = QJsonDocument::fromJson(response.data).object().value("email").toString();
                }
            }

            project->set_username(username);
            project->set_accessToken(token);
        }

        if (!trackFieldUid.isEmpty())
        {
            project->set_trackFieldUid(trackFieldUid);
            project->set_trackFileFormat(params.value("trackFileFormat").toString());
            project->set_locationPointInterval(params.value("pointInterval").toInt());
            project->set_locationTrackInterval(params.value("trackInterval").toInt());
            project->set_locationTrackDistance(params.value("trackDistance").toInt());
        }
    });

    return bootstrapUpdate(projectUid);
}

ApiResult ODKConnector::hasUpdate(Project* project)
{
    auto server = project->getConnectorParam("server").toString();
    auto token = project->accessToken();
    auto tokenType = project->getConnectorParam("tokenType").toString();
    auto formId = project->getConnectorParam("formId").toString();
    auto projectId = project->getConnectorParam("projectId").toString();
    auto url = server + "/v1/projects/" + projectId + "/forms/" + formId + "/versions";
    auto currentVersion = project->getConnectorParam("version").toString();

    auto request = Utils::httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Authorization", QString(tokenType + " " + token).toUtf8());

    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->get(request));

    Utils::httpWait(reply.get());

    auto response = Utils::HttpResponse::fromReply(url, reply.get());
    if (!response.success)
    {
        return Failure(response);
    }

    auto formMap = QJsonDocument::fromJson(response.data).array().first().toObject().toVariantMap();
    auto deployedVersion = formMap.value("version").toString();
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

ApiResult ODKConnector::uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* /*state*/, const CommitState& /*commitState*/)
{
    return uploadDataODK(project, queueItem.url, queueItem.payload, queueItem.attachments);
}

bool ODKConnector::canUpdate(Project* project) const
{
    return loggedIn(project);
}

ApiResult ODKConnector::update(Project* project) const
{
    auto projectUid = project->uid();

    // Retrieve a new form.
    auto connectorParams = project->connectorParams();
    auto server = connectorParams.value("server").toString();
    auto token = project->accessToken();
    auto tokenType = connectorParams.value("tokenType").toString();
    auto projectId = connectorParams.value("projectId").toString();
    auto formId = connectorParams.value("formId").toString();

    // Read the form metadata.
    auto response = Utils::httpGetWithToken(m_networkAccessManager, server + "/v1/projects/" + projectId + "/forms/" + formId + "/versions", token, tokenType);
    if (!response.success)
    {
        if (response.unauthorized())
        {
            logout(project);
        }

        return Failure(response);
    }

    auto formJson = QJsonDocument::fromJson(response.data).array().first().toObject();

    // Check if up to date.
    auto versionName = formJson.value("version").toString();
    if (versionName == connectorParams["version"])
    {
        return AlreadyUpToDate();
    }

    project->set_name(formJson["name"].toString());
    project->setConnectorParam("version", versionName.isEmpty() ? "???" : versionName);

    // Download the form.
    response = Utils::httpGetWithToken(m_networkAccessManager, server + "/v1/projects/" + projectId + "/forms/" + formId + "/versions/" + versionName + ".xlsx", token, tokenType);
    if (!response.success || response.data.length() == 0)
    {
        if (response.unauthorized())
        {
            logout(project);
        }

        return Failure(response);
    }

    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    Utils::writeDataToFile(updateFolder + "/form.xlsx", response.data);

    // Download the media.
    response = Utils::httpGetWithToken(m_networkAccessManager, server + "/v1/projects/" + projectId + "/forms/" + formId + "/versions/" + versionName + "/attachments", token, tokenType);
    if (!response.success)
    {
        return Failure(response);
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
        response = Utils::httpGetWithToken(m_networkAccessManager, mediaUrl, token, tokenType);
        if (!response.success)
        {
            if (response.unauthorized())
            {
                logout(project);
            }

            return Failure(response);
        }

        Utils::writeDataToFile(updateFolder + "/" + mediaFilename, response.data);
    }

    // Reset the project.
    m_projectManager->reset(projectUid, true);

    // Prepare the project.
    return buildSchema(project, updateFolder);
}

ApiResult ODKConnector::buildSchema(Project* project, const QString& path) const
{
    // Parse the form.
    auto schema = Schema();
    auto errorString = QString();
    if (!XlsFormParser::parse(path + "/form.xlsx", &schema, &errorString))
    {
        return Failure(tr("Bad form"));
    }

    // Modify project settings.
    XlsFormParser::configureProject(project, &schema);

    // Overwrite with current project values.
    project->set_version(1);
    project->set_provider(ODKProvider::PROVIDER_NAME);
    project->set_subtitle(QUrl(project->getConnectorParam("server").toString()).host());
    project->set_icon("qrc:/ODK/logo.svg");
    project->set_colors(QVariantMap {{ "primary", "#3E77B4" }, { "accent", "#904A22" }});
    project->set_defaultWizardMode(true);

    // Commit the project and schema file.
    project->saveToQmlFile(path + "/Project.qml");
    schema.setProjectInfo(project->title(), project->subtitle(), project->icon());
    schema.saveToQmlFile(path + "/Form.qml");

    return Success();
}

void ODKConnector::reset(Project* project) const
{
    QFile::remove(m_projectManager->getFilePath(project->uid(), "Form.qml"));
}

} // namespace CyberTracker