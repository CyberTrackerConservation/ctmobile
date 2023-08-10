#include "GoogleConnector.h"
#include "GoogleProvider.h"
#include "GoogleFormParser.h"
#include "App.h"
#include <jlcompress.h>

GoogleConnector::GoogleConnector(QObject *parent) : Connector(parent)
{
    update_name(GOOGLE_CONNECTOR);
    update_icon("qrc:/Google/logo.svg");
}

bool GoogleConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool GoogleConnector::canShareAuth(Project* /*project*/) const
{
    return false;
}

QVariantMap GoogleConnector::getShareData(Project* project, bool /*auth*/) const
{
    return QVariantMap
    {
        { "connector", GOOGLE_CONNECTOR },
        { "formId", project->getConnectorParam("formId") },
        { "launch", true },
    };
}

ApiResult GoogleConnector::bootstrap(const QVariantMap& params)
{
    auto formId = params["formId"].toString();
    if (formId.isEmpty())
    {
        return Failure("Form not specifed");
    }

    auto projectUid = "GF_" + formId;

    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    auto connectorParams = QVariantMap {{ "formId", formId }};
    if (!m_projectManager->init(projectUid, GOOGLE_PROVIDER, QVariantMap(), GOOGLE_CONNECTOR, connectorParams))
    {
        return Failure(QString(tr("Failed to create %1")).arg(App::instance()->alias_project()));
    }

    return bootstrapUpdate(projectUid);
}

ApiResult GoogleConnector::hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project)
{
    auto connectorParams = project->connectorParams();
    
    auto formId = connectorParams["formId"].toString();
    auto version = QString();
    auto response = Utils::googleFetchFormVersion(networkAccessManager, formId, &version);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    if (version == connectorParams["version"])
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

bool GoogleConnector::canUpdate(Project* /*project*/) const
{
    return true;
}

ApiResult GoogleConnector::update(Project* project)
{
    // Skip updates if there are any unsent sightings.
    auto sightingUids = QStringList();
    m_database->getSightings(project->uid(), "", Sighting::DB_SIGHTING_FLAG, &sightingUids);
    if (sightingUids.count() > 0)
    {
        return Failure(tr("Unsent data"));
    }

    auto projectUid = project->uid();
    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);

    // Check if an update is required.
    auto connectorParams = project->connectorParams();
    auto formId = connectorParams["formId"].toString();
    auto formMap = QVariantMap();

    auto response = Utils::googleFetchForm(App::instance()->networkAccessManager(), formId, &formMap);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto version = formMap["version"].toString();
    if (version == connectorParams["version"])
    {
        return AlreadyUpToDate();
    }

    // Write the form.
    auto formFilePath = updateFolder + "/form.json";
    Utils::writeJsonToFile(formFilePath, QJsonDocument::fromVariant(formMap).toJson());

    // Parse the form and download everything.
    auto parserError = QString();
    GoogleFormParser::parse(formFilePath, updateFolder, &parserError);

    // Load settings.
    auto settingsMap = QVariantMap();
    settingsMap = Utils::variantMapFromJsonFile(updateFolder + "/Settings.json");
    auto driveFolderId = settingsMap["driveFolderId"].toString();

    // Update the project.
    connectorParams["formId"] = formId;
    connectorParams["version"] = version;
    connectorParams["publishedUrl"] = formMap["publishedUrl"];
    connectorParams["driveFolderId"] = driveFolderId;
    connectorParams["email"] = settingsMap["email"];

    Project updateProject;
    updateProject.set_uid(projectUid);
    updateProject.set_provider(GOOGLE_PROVIDER);

    updateProject.set_connector(GOOGLE_CONNECTOR);
    updateProject.set_connectorParams(connectorParams);
    updateProject.set_version(1);
    updateProject.set_title(settingsMap["title"].toString());
    updateProject.set_subtitle(settingsMap["subtitle"].toString());
    updateProject.set_icon(settingsMap["icon"].toString());
    updateProject.set_iconDark(settingsMap["iconDark"].toString());
    updateProject.set_defaultWizardMode(settingsMap["wizardMode"].toBool());
    updateProject.set_immersive(settingsMap["immersive"].toBool());
    updateProject.set_defaultSubmitInterval(settingsMap["submitInterval"].toInt());
    updateProject.set_colors(settingsMap["colors"].toMap());
    updateProject.set_offlineMapUrl(settingsMap["offlineMapUrl"].toString());
    updateProject.saveToQmlFile(updateFolder + "/Project.qml");

    // Reset the project database state.
    m_projectManager->reset(projectUid, true);

    return Success();
}
