#include "KoBoConnector.h"
#include "../ODK/ODKProvider.h"
#include "App.h"
#include "XlsForm.h"

KoBoConnector::KoBoConnector(QObject *parent) : Connector(parent)
{
    update_name(KOBO_CONNECTOR);
    update_icon("qrc:/KoBo/logo.svg");
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
        { "connector", KOBO_CONNECTOR },
        { "formId", connectorParams["formId"] },
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

    return result;
}

bool KoBoConnector::canLogin(Project* /*project*/) const
{
    return true;
}

bool KoBoConnector::loggedIn(Project* project) const
{
    return !project->accessToken().isEmpty();
}

bool KoBoConnector::login(Project* project, const QString& server, const QString& username, const QString& password)
{
    auto serverUrl = QUrl(server + "/token/?format=json");

    auto response = Utils::httpGet(App::instance()->networkAccessManager(), serverUrl.toString(), username, password);
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

void KoBoConnector::logout(Project* project)
{
    project->set_accessToken("");
}

ApiResult KoBoConnector::bootstrap(const QVariantMap& params)
{
    auto formId = params["formId"].toString();
    auto projectUid = "KoBo_" + formId;

    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    auto serverUrl = params["server"].toUrl();
    auto username = params["username"].toString();
    auto password = params["password"].toString();
    auto token = params["token"].toString();

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
        { "exportId", formId }
    };
    
    if (!m_projectManager->init(projectUid, ODK_PROVIDER, QVariantMap(), KOBO_CONNECTOR, connectorParams))
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
                auto response = Utils::httpGetWithToken(App::instance()->networkAccessManager(), server + "/me/", token, connectorParams["tokenType"].toString());
                if (response.success)
                {
                    username = QJsonDocument::fromJson(response.data).object().value("username").toString();
                }
            }

            project->set_username(username);
            project->set_accessToken(token);
        }
    });

    return bootstrapUpdate(projectUid);
}

QString KoBoConnector::getDeployedVersion(const QVariantMap& formMap)
{
    auto deployedVersions = formMap.value("deployed_versions").toMap().value("results").toList();
    if (deployedVersions.isEmpty())
    {
        qDebug() << "Error: deployed versions empty";
        return "";
    }

    return deployedVersions.constFirst().toMap().value("uid").toString();
}

ApiResult KoBoConnector::hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project)
{
    auto connectorParams = project->connectorParams();
    auto server = connectorParams["server"].toString();
    auto token = project->accessToken();
    auto tokenType = connectorParams["tokenType"].toString();
    auto formId = connectorParams["formId"].toString();

    auto response = Utils::httpGetWithToken(networkAccessManager, server + "/api/v2/assets/" + formId + "/", token, tokenType);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto formMap = QJsonDocument::fromJson(response.data).object().toVariantMap();

    auto deployedVersion = getDeployedVersion(formMap);
    if (deployedVersion.isEmpty())
    {
        return Failure(tr("No deployed versions"));
    }

    if (deployedVersion == connectorParams["version"])
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

bool KoBoConnector::canUpdate(Project* project) const
{
    return loggedIn(project);
}

ApiResult KoBoConnector::update(Project* project)
{
    auto projectUid = project->uid();

    // Skip updates if there are any unsent sightings.
//    auto sightingUids = QStringList();
//    m_database->getSightings(projectUid, "", Sighting::DB_SIGHTING_FLAG, &sightingUids);
//    if (sightingUids.count() > 0)
//    {
//        return Failure(tr("Unsent data"));
//    }

    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);

    // Check if an update is required.
    auto connectorParams = project->connectorParams();
    auto accountName = connectorParams["account"].toString();
    auto server = connectorParams["server"].toString();
    auto token = project->accessToken();
    auto tokenType = connectorParams["tokenType"].toString();
    auto formId = connectorParams["formId"].toString();
    auto submissionUrl = QUrl();

    auto response = Utils::httpGetWithToken(App::instance()->networkAccessManager(), server + "/api/v2/assets/" + formId + "/", token, tokenType);
    if (!response.success)
    {
        if (response.status == 401)
        {
            logout(project);
        }

        return Failure(response.errorString);
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

    submissionUrl = QUrl(deploymentUrl);
    submissionUrl.setPath("/submission" + deploymentUrl.path());

    // Download the form.
    response = Utils::httpGetWithToken(App::instance()->networkAccessManager(), server + "/api/v2/assets/" + formId + ".xls", token, tokenType);
    if (!response.success)
    {
        if (response.status == 401)
        {
            logout(project);
        }

        return Failure(response.errorString);
    }

    Utils::writeDataToFile(updateFolder + "/form.xlsx", response.data);
    if (!QFile::exists(updateFolder + "/form.xlsx"))
    {
        qDebug() << "Failed to read XLSX";
        return Failure(tr("Form download failed"));
    }

    auto formSettings = QVariantMap();
    if (!XlsFormParser::parseSettings(updateFolder + "/form.xlsx", &formSettings))
    {
        qDebug() << "Failed to read form settings sheet";
    }

    // Download the media.
    response = Utils::httpGetWithToken(App::instance()->networkAccessManager(), server + "/api/v2/assets/" + formId + "/files/?file_type=form_media", token, tokenType);
    if (!response.success)
    {
        if (response.status == 401)
        {
            logout(project);
        }

        return Failure(response.errorString);
    }

    auto mediaArr = QJsonDocument::fromJson(response.data).object().value("results").toArray();
    for (auto mediaIt = mediaArr.constBegin(); mediaIt != mediaArr.constEnd(); mediaIt++)
    {
        auto mediaObj = (*mediaIt).toObject();
        auto mediaFilename = mediaObj.value("metadata").toObject().value("filename").toString();
        auto mediaUrl = mediaObj.value("content").toString();
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
    connectorParams["version"] = deployedVersion;
    connectorParams["submissionUrl"] = submissionUrl.toString();

    Project updateProject;
    updateProject.set_uid(projectUid);
    updateProject.set_provider(ODK_PROVIDER);

    updateProject.set_connector(KOBO_CONNECTOR);
    updateProject.set_connectorParams(connectorParams);
    updateProject.set_version(1);
    updateProject.set_title(formMap["name"].toString());
    updateProject.set_defaultWizardMode(true);

    auto icon = QString("qrc:/KoBo/logo.svg");
    auto colors = QVariantMap
    {
        { "primary", "#414452" },
        { "accent", "#2696EF" },
    };

    auto subtitle = QUrl(server).host();
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
    else if (server.startsWith("kobo.humanitarianresponse.info", Qt::CaseInsensitive))
    {
        subtitle = tr("Humanitarian server");
    }

    updateProject.set_icon(icon);
    updateProject.set_subtitle(subtitle);
    updateProject.set_colors(colors);

    XlsFormParser::configureProject(&updateProject, formSettings);

    updateProject.saveToQmlFile(updateFolder + "/Project.qml");

    // Reset the project database state.
    m_projectManager->reset(projectUid, true);

    return Success();
}

void KoBoConnector::reset(Project* project)
{
    auto path = m_projectManager->getFilePath(project->uid());

    QFile::remove(path + "/Elements.qml");
    QFile::remove(path + "/Fields.qml");
    QFile::remove(path + "/Settings.json");
}
