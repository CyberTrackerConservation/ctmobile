#include "KoBoConnector.h"
#include "../ODK/ODKProvider.h"
#include "App.h"
#include "XlsForm.h"

namespace CyberTracker {

KoBoConnector::KoBoConnector(QObject *parent) : Connector(parent)
{
    update_name(CONNECTOR_NAME);
    update_icon("qrc:/KoBo/logo.svg");
}

KoBoConnector::~KoBoConnector()
{
}

void KoBoConnector::migrateProject(Project* project) const
{
    // Around build 530, we shipped a change which lost tokens.
    // Try and recover for named account types.
    if (!project->accessToken().isEmpty())
    {
        return;
    }

    auto accountName = project->getConnectorParam("account").toString();
    if (accountName.isEmpty())
    {
        return;
    }

    auto accountConfig = App::instance()->config()["accounts"].toMap();
    auto account = accountConfig[accountName].toMap();
    if (account.isEmpty())
    {
        return;
    }

    login(project, account["server"].toString(), account["username"].toString(), account["password"].toString());
}

bool KoBoConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool KoBoConnector::canShareAuth(Project* project) const
{
    return loggedIn(project) && project->connectorParams()["account"].toString().isEmpty(); // No 'account' means auth.
}

QVariantMap KoBoConnector::getShareData(Project* project, bool auth) const
{
    auto connectorParams = project->connectorParams();

    auto result = QVariantMap
    {
        { "connector", CONNECTOR_NAME },
        { "formId", connectorParams["formId"] },
        { "formhubUuid", connectorParams["formhubUuid"] },
        { "launch", true },
    };

    auto accountName = connectorParams["account"].toString();
    if (!accountName.isEmpty())
    {
        result.insert("account", accountName);
    }
    else
    {
        result.insert("server", connectorParams["server"]);

        if (auth || !loggedIn(project))
        {
            result.insert("auth", true);
        }
        else
        {
            result.insert("token", project->accessToken());
        }
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

bool KoBoConnector::canLogin(Project* project) const
{
    // If an account is specified, then login is managed.
    return project->getConnectorParam("account").toString().isEmpty();
}

bool KoBoConnector::loggedIn(Project* project) const
{
    return !project->accessToken().isEmpty();
}

bool KoBoConnector::login(Project* project, const QString& server, const QString& username, const QString& password) const
{
    auto serverUrl = QUrl(server + "/token/?format=json");

    auto response = Utils::httpGet(m_networkAccessManager, serverUrl.toString(), username, password);
    if (response.success)
    {
        auto token = QJsonDocument::fromJson(response.data).object().value("token").toString();
        if (!token.isEmpty())
        {
            project->set_username(username);
            project->set_accessToken(token);
            return true;
        }
    }

    qDebug() << "Login failed: " << response.data;

    return false;
}

void KoBoConnector::logout(Project* project) const
{
    project->set_accessToken("");
}

QString KoBoConnector::getFormFilePath(Project* project) const
{
    return m_projectManager->getFilePath(project->uid(), "form.xlsx");
}

ApiResult KoBoConnector::bootstrap(const QVariantMap& params) const
{
    auto formId = params["formId"].toString();
    auto formhubUuid = params["formhubUuid"].toString();
    auto projectUid = "KoBo_" + formId;

    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    auto serverUrl = params["server"].toUrl();
    auto username = params["username"].toString();
    auto password = params["password"].toString();
    auto token = params["token"].toString();
    auto trackFieldUid = params["trackFieldUid"].toString();

    auto accountName = params["account"].toString();
    if (!accountName.isEmpty())
    {
        auto account = App::instance()->config()["accounts"].toMap()[accountName].toMap();
        serverUrl = account["server"].toString();
        username = account["username"].toString();
        password = account["password"].toString();
    }

    serverUrl.setPath("");
    auto server = serverUrl.toString();

    auto connectorParams = QVariantMap
    {
        { "account", accountName },
        { "server", serverUrl.toString() },
        { "tokenType", "Token" },
        { "formId", formId },
        { "formhubUuid", formhubUuid },
        { "exportId", formId }
    };
    
    if (!m_projectManager->init(projectUid, ODKProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, connectorParams))
    {
        return Failure(QString(tr("Failed to create %1")).arg(App::instance()->alias_project()));
    }

    m_projectManager->modify(projectUid, [&](Project* project)
    {
        if (token.isEmpty() && !username.isEmpty() && !password.isEmpty())
        {
            login(project, serverUrl.toString(), username, password);
        }
        else
        {
            // Pick up the username if not provided.
            if (username.isEmpty() && !token.isEmpty())
            {
                auto response = Utils::httpGetWithToken(m_networkAccessManager, server + "/me/", token, connectorParams["tokenType"].toString());
                if (response.success)
                {
                    username = QJsonDocument::fromJson(response.data).object().value("username").toString();
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

QString KoBoConnector::getDeployedVersion(const QVariantMap& formMap) const
{
    auto deployedVersions = formMap.value("deployed_versions").toMap().value("results").toList();
    if (deployedVersions.isEmpty())
    {
        qDebug() << "Error: deployed versions empty";
        return "";
    }

    return deployedVersions.constFirst().toMap().value("uid").toString();
}

ApiResult KoBoConnector::hasUpdate(Project* project)
{
    auto server = project->getConnectorParam("server").toString();
    auto token = project->accessToken();
    auto tokenType = project->getConnectorParam("tokenType").toString();
    auto formId = project->getConnectorParam("formId").toString();
    auto url = server + "/api/v2/assets/" + formId + "/";
    auto currentVersion = project->getConnectorParam("version");

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

    auto formMap = QJsonDocument::fromJson(response.data).object().toVariantMap();
    auto deployedVersion = getDeployedVersion(formMap);
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

ApiResult KoBoConnector::uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* /*state*/, const CommitState& /*commitState*/)
{
    return uploadDataODK(project, queueItem.url, queueItem.payload, queueItem.attachments);
}

bool KoBoConnector::canUpdate(Project* project) const
{
    return loggedIn(project);
}

ApiResult KoBoConnector::update(Project* project) const
{
    auto projectUid = project->uid();

    // Check if an update is required.
    auto connectorParams = project->connectorParams();
    auto server = connectorParams["server"].toString();
    auto token = project->accessToken();
    auto tokenType = connectorParams["tokenType"].toString();
    auto formId = connectorParams["formId"].toString();
    auto submissionUrl = QUrl();

    auto response = Utils::httpGetWithToken(m_networkAccessManager, server + "/api/v2/assets/" + formId + "/", token, tokenType);
    if (!response.success)
    {
        if (response.unauthorized())
        {
            logout(project);
        }

        return Failure(response);
    }

    auto formMap = QJsonDocument::fromJson(response.data).object().toVariantMap();
    auto deployedVersion = getDeployedVersion(formMap);
    if (deployedVersion == connectorParams["version"])
    {
        return AlreadyUpToDate();
    }

    // Get submission url.
    auto deploymentLinks = formMap.value("deployment__links").toMap();
    if (deploymentLinks.isEmpty())
    {
        return Failure(tr("No deployment links"));
    }

    auto deploymentUrl = QUrl(deploymentLinks["url"].toString());
    if (deploymentUrl.isEmpty())
    {
        return Failure(tr("No deployment url"));
    }

    App::instance()->initProgress(projectUid, "KoBoConnect", { 10, 100 });

    submissionUrl = QUrl(deploymentUrl);
    submissionUrl.setPath("/submission" + deploymentUrl.path());

    // Download the form.
    response = Utils::httpGetWithToken(m_networkAccessManager, server + "/api/v2/assets/" + formId + ".xls", token, tokenType);
    if (!response.success)
    {
        if (response.unauthorized())
        {
            logout(project);
        }

        return Failure(response);
    }

    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    Utils::writeDataToFile(updateFolder + "/form.xlsx", response.data);
    if (!QFile::exists(updateFolder + "/form.xlsx"))
    {
        qDebug() << "Failed to read XLSX";
        return Failure(tr("Form download failed"));
    }

    App::instance()->sendProgress(10, 0);

    // Download the media.
    auto index = 0;
    auto mediaUrl = server + "/api/v2/assets/" + formId + "/files/?file_type=form_media";
    while (!mediaUrl.isEmpty())
    {
        response = Utils::httpGetWithToken(m_networkAccessManager, mediaUrl, token, tokenType);
        if (!response.success)
        {
            if (response.unauthorized())
            {
                logout(project);
            }

            return Failure(response);
        }

        auto assets = QJsonDocument::fromJson(response.data).object().toVariantMap();
        auto mediaCount = assets.value("count").toInt();
        auto mediaArr = assets.value("results").toList();

        for (auto mediaIt = mediaArr.constBegin(); mediaIt != mediaArr.constEnd(); mediaIt++)
        {
            auto mediaObj = (*mediaIt).toMap();
            auto mediaFilename = mediaObj.value("metadata").toMap().value("filename").toString();
            auto mediaUrl = mediaObj.value("content").toString();
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

            App::instance()->sendProgress((index++ * 100) / mediaCount, 1);
        }

        mediaUrl = assets.value("next").toString();
    }

    // Finalize connection parameters.
    project->setConnectorParam("version", deployedVersion);
    project->setConnectorParam("submissionUrl", submissionUrl.toString());
    project->set_name(formMap["name"].toString());

    // Reset the project database state.
    m_projectManager->reset(projectUid, true);

    // Prepare the project.
    return buildSchema(project, updateFolder);
}

ApiResult KoBoConnector::buildSchema(Project* project, const QString& path) const
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
    project->set_defaultWizardMode(true);

    auto icon = QString("qrc:/KoBo/logo.svg");
    auto colors = QVariantMap
    {
        { "primary", "#414452" },
        { "accent", "#2696EF" },
    };

    auto subtitle = QUrl(project->getConnectorParam("server").toString()).host();
    auto accountName = project->getConnectorParam("account").toString();
    if (!accountName.isEmpty())
    {
        auto account = App::instance()->config()["accounts"].toMap()[accountName].toMap();
        subtitle = account.value("subtitle").toString();
        icon = account.value("icon").toString();
        colors["primary"] = account.value("colorPrimary");
        colors["accent"] = account.value("colorAccent");
    }
    else if (subtitle.startsWith("kf.kobotoolbox.org", Qt::CaseInsensitive))
    {
        subtitle = tr("Global server");
    }
    else if (subtitle.startsWith("eu.kobotoolbox.org", Qt::CaseInsensitive))
    {
        subtitle = tr("European Union server");
    }

    if (project->icon().isEmpty())
    {
        project->set_icon(icon);
    }

    if (project->subtitle().isEmpty())
    {
        project->set_subtitle(subtitle);
    }

    project->set_colors(colors);

    // Commit the project and schema file.
    project->saveToQmlFile(path + "/Project.qml");
    schema.setProjectInfo(project->title(), subtitle, icon);
    schema.saveToQmlFile(path + "/Form.qml");

    return Success();
}

void KoBoConnector::reset(Project* project) const
{
    QFile::remove(m_projectManager->getFilePath(project->uid(), "Form.qml"));
}

} // namespace CyberTracker