#include "CTOConnector.h"
#include "../ODK/ODKProvider.h"
#include "App.h"
#include "XlsForm.h"

CTOConnector::CTOConnector(QObject *parent) : Connector(parent)
{
    update_name(CTO_CONNECTOR);
    update_icon("qrc:/CTO/logo.svg");
}

bool CTOConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool CTOConnector::canShareAuth(Project* project) const
{
    return loggedIn(project);
}

QVariantMap CTOConnector::getShareData(Project* project, bool /*auth*/) const
{
    auto result = QVariantMap
    {
        { "connector", CTO_CONNECTOR },
        { "formId", project->connectorParams()["formId"] },
        { "server", project->connectorParams()["server"] },
        { "token", project->accessToken() },
        { "launch", true }
    };

    return result;
}

bool CTOConnector::canLogin(Project* /*project*/) const
{
    return false;
}

bool CTOConnector::loggedIn(Project* project) const
{
    return !project->accessToken().isEmpty();
}

bool CTOConnector::login(Project* /*project*/, const QString& /*server*/, const QString& /*username*/, const QString& /*password*/)
{
    return false;
}

void CTOConnector::logout(Project* project)
{
    project->set_accessToken("");
    project->set_refreshToken("");
}

ApiResult CTOConnector::bootstrap(const QVariantMap& params)
{
    auto server = params["server"].toString();
    server = Utils::removeTrailingSlash(server);

    auto formId = params["formId"].toString();
    auto projectUid = "CTO_" + formId;
    auto token = params.value("token").toString();

    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    auto connectorParams = QVariantMap
    {
        { "server", server },
        { "formId", formId },
        { "exportId", formId },
        { "tokenType", "Bearer" },
        { "submissionUrl", server + "/api/applications/" + formId + "/data" }
    };

    if (!m_projectManager->init(projectUid, ODK_PROVIDER, QVariantMap(), CTO_CONNECTOR, connectorParams))
    {
        return Failure(tr("Failed to create project"));
    }

    m_projectManager->modify(projectUid, [&](Project* project)
    {
        project->set_accessToken(token);
    });

    return bootstrapUpdate(projectUid);
}

bool CTOConnector::refreshAccessToken(QNetworkAccessManager* /*networkAccessManager*/, Project* /*project*/)
{
    return false;
}

ApiResult CTOConnector::hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project)
{
    // Check the form version.
    auto connectorParams = project->connectorParams();
    auto server = connectorParams.value("server").toString();
    auto formId = connectorParams.value("formId").toString();
    auto token = project->accessToken();

    auto response = Utils::httpGetWithToken(networkAccessManager, server + "/api/applications/" + formId + "/version", token);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto formMap = QJsonDocument::fromJson(response.data).object().toVariantMap();

    // Check if up to date.
    auto versionName = formMap.value("version").toString();
    if (versionName == connectorParams["version"])
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

bool CTOConnector::canUpdate(Project* project) const
{
    return loggedIn(project);
}

ApiResult CTOConnector::update(Project* project)
{
    // Skip update if not logged in.
    auto projectUid = project->uid();
    if (!loggedIn(project))
    {
        return Failure(tr("Logged out"));
    }

    if (hasUpdate(App::instance()->networkAccessManager(), project) == AlreadyUpToDate())
    {
        return AlreadyUpToDate();
    }

    // Skip updates if there are any unsent sightings.
//    auto sightingUids = QStringList();
//    m_database->getSightings(project->uid(), "", Sighting::DB_SIGHTING_FLAG, &sightingUids);
//    if (sightingUids.count() > 0)
//    {
//        return Failure(tr("Unsent data"));
//    }

    // Fetch the survey metadata.
    auto connectorParams = project->connectorParams();
    auto server = connectorParams["server"].toString();
    auto formId = connectorParams["formId"].toString();
    auto tokenType = connectorParams["tokenType"].toString();
    auto token = project->accessToken();

    // Download the form zip.
    auto formUrl = server + "/api/applications/" + formId + "/package";
    auto zipFilePath = App::instance()->downloadFile(formUrl, "", "", token, tokenType);
    if (zipFilePath.isEmpty())
    {
        return Failure(tr("Form download failed"));
    }

    // Unpack the project.
    auto packagePath = Utils::unpackZip(App::instance()->tempPath(), "Package", zipFilePath);
    if (packagePath.isEmpty())
    {
        return Failure(tr("Bad package"));
    }

    // Get the form version and add it to the connectorParams.
    auto formSettings = QVariantMap();
    if (!XlsFormParser::parseSettings(packagePath + "/project/form.xlsx", &formSettings))
    {
        return Failure(tr("Failed to read form settings sheet"));
    }
    connectorParams["version"] = formSettings.value("version").toString();

    // Copy the project subfolder to the update folder.
    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    Utils::copyPath(packagePath + "/project", updateFolder);

    // Load the project.
    auto projectJson = Utils::variantMapFromJsonFile(packagePath + "/project.json");

    // Overwrite with current project values.
    Project updateProject;
    updateProject.load(projectJson, false);
    updateProject.set_version(1);
    updateProject.set_uid(projectUid);
    updateProject.set_provider(ODK_PROVIDER);
    updateProject.set_providerParams(project->providerParams());
    updateProject.set_connector(CTO_CONNECTOR);
    updateProject.set_connectorParams(connectorParams);
    updateProject.set_defaultWizardMode(true);

    if (updateProject.icon().isEmpty())
    {
        updateProject.set_icon("qrc:/app/appicon.svg");
    }

    QStringList androidPermissions = QStringList();
    androidPermissions << "CAMERA";
    androidPermissions << "RECORD_AUDIO";
    androidPermissions << "ACCESS_FINE_LOCATION";
    androidPermissions << "ACCESS_COARSE_LOCATION";
    updateProject.set_androidPermissions(androidPermissions);

    XlsFormParser::configureProject(&updateProject, formSettings);

    updateProject.saveToQmlFile(updateFolder + "/Project.qml");

    // Cleanup.
    QDir(packagePath).removeRecursively();

    // Reset the project.
    m_projectManager->reset(projectUid, true);

    return Success();
}

void CTOConnector::reset(Project* project)
{
    auto path = m_projectManager->getFilePath(project->uid());

    QFile::remove(path + "/Elements.qml");
    QFile::remove(path + "/Fields.qml");
    QFile::remove(path + "/Settings.json");
}
