#include "GoogleConnector.h"
#include "GoogleProvider.h"
#include "GoogleFormParser.h"
#include "App.h"

namespace CyberTracker {

GoogleConnector::GoogleConnector(QObject *parent) : Connector(parent)
{
    update_name(CONNECTOR_NAME);
    update_icon("qrc:/Google/logo.svg");
}

GoogleConnector::~GoogleConnector()
{
}

QString GoogleConnector::getFormFilePath(Project* project) const
{
    return m_projectManager->getFilePath(project->uid(), "form.json");
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
        { "connector", CONNECTOR_NAME },
        { "formId", project->getConnectorParam("formId") },
        { "launch", true },
    };
}

ApiResult GoogleConnector::hasUpdate(Project* project)
{
    auto formId = project->getConnectorParam("formId").toString();
    auto currentVersion = project->getConnectorParam("version").toString();

    auto query = QUrlQuery();
    query.addQueryItem("formId", formId);
    auto url = QUrl(Utils::googleServerUrl() + "/api/form/ver");
    url.setQuery(query);

    auto request = Utils::httpCreateRequest();
    request.setUrl(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");
    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->get(request));

    Utils::httpWait(reply.get());

    auto response = Utils::HttpResponse::fromReply(url.toString(), reply.get());
    if (!response.success)
    {
        return Failure(response);
    }

    auto deployedVersion = response.data;
    if (deployedVersion == currentVersion)
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

ApiResult GoogleConnector::uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* state, const CommitState& commitState)
{
    // Upload attachments.
    auto driveFolderId = project->getConnectorParam("driveFolderId").toString();

    for (auto attachmentIt = queueItem.attachments.constBegin(); attachmentIt != queueItem.attachments.constEnd(); attachmentIt++)
    {
        auto filename = *attachmentIt;
        if (state->contains(filename))
        {
            continue;
        }

        auto filePath = App::instance()->getMediaFilePath(filename);

        auto resultUrl = QString();
        auto response = Utils::googleUploadFile(m_networkAccessManager, driveFolderId, filePath, &resultUrl);

        if (!response.success)
        {
            return ResultFromResponse(response);
        }

        state->insert(filename, resultUrl);
        commitState();
    }

    // Upload data.
    auto publishedUrl = queueItem.url;
    publishedUrl.replace("/viewform", "/formResponse?", Qt::CaseInsensitive);

    auto payloadData = QString(queueItem.payload);

    for (auto attachmentIt = queueItem.attachments.constBegin(); attachmentIt != queueItem.attachments.constEnd(); attachmentIt++)
    {
        auto filename = *attachmentIt;
        auto fileLink = state->value(filename).toString();
        auto encoded = QUrl(fileLink).toEncoded();
        if (payloadData.contains(filename))
        {
            payloadData.replace(filename, encoded);
        }
    }

    publishedUrl += payloadData;
    publishedUrl += "&emailAddress=" + project->getConnectorParam("email", "unknown@unknown.com").toString();

    auto response = Utils::httpGet(m_networkAccessManager, publishedUrl);
    if (!response.data.contains("Submit another"))
    {
        qDebug() << "Failed to upload: " + response.data;
        // TODO: we fall through, because we have no way to detect a real failure.
        //result.success = false;
        //result.status = 304;
    }

    if (!response.success)
    {
        qDebug() << "Google Forms upload failed: " << response.errorString;
    }

    return ResultFromResponse(response);
}

ApiResult GoogleConnector::bootstrap(const QVariantMap& params) const
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
    if (!m_projectManager->init(projectUid, GoogleProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, connectorParams))
    {
        return Failure(QString(tr("Failed to create %1")).arg(App::instance()->alias_project()));
    }

    return bootstrapUpdate(projectUid);
}

bool GoogleConnector::canUpdate(Project* /*project*/) const
{
    return true;
}

ApiResult GoogleConnector::update(Project* project) const
{
    // Skip updates if there are any unsent sightings.
    if (m_database->hasData(project->uid(), "", Database::DataFlag::DB_SIGHTING_FLAG))
    {
        return Failure(tr("Unsent data"));
    }

    auto projectUid = project->uid();
    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);

    // Check if an update is required.
    auto connectorParams = project->connectorParams();
    auto formId = connectorParams["formId"].toString();
    auto formMap = QVariantMap();

    auto response = Utils::googleFetchForm(m_networkAccessManager, formId, &formMap);
    if (!response.success)
    {
        return Failure(response);
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
    auto schema = std::unique_ptr<Schema>(qobject_cast<Schema*>(Utils::loadQmlFromFile(App::instance()->qmlEngine(), updateFolder + "/Form.qml")));
    auto settingsMap = schema->settings();

    // Update the project.
    connectorParams["formId"] = formId;
    connectorParams["version"] = version;
    connectorParams["publishedUrl"] = formMap["publishedUrl"];
    connectorParams["driveFolderId"] = settingsMap["driveFolderId"].toString();
    connectorParams["email"] = settingsMap["email"];

    Project updateProject;
    updateProject.set_uid(projectUid);
    updateProject.set_provider(GoogleProvider::PROVIDER_NAME);
    updateProject.set_connector(CONNECTOR_NAME);
    updateProject.set_connectorParams(connectorParams);
    updateProject.set_version(1);
    updateProject.set_name(settingsMap["title"].toString());
    updateProject.set_subtitle(settingsMap["subtitle"].toString());
    updateProject.set_icon(settingsMap["icon"].toString());
    updateProject.set_iconDark(settingsMap["iconDark"].toString());
    updateProject.set_defaultWizardMode(settingsMap["wizardMode"].toBool());
    updateProject.set_immersive(settingsMap["immersive"].toBool());
    updateProject.set_defaultSubmitInterval(settingsMap["submitInterval"].toInt());
    updateProject.set_colors(settingsMap["colors"].toMap());
    updateProject.set_offlineMapUrl(settingsMap["offlineMapUrl"].toString());
    updateProject.set_requireLocation(settingsMap["hasLocation"].toBool());

    updateProject.saveToQmlFile(updateFolder + "/Project.qml");

    // Reset the project database state.
    m_projectManager->reset(projectUid, true);

    return buildSchema(project, updateFolder);
}

ApiResult GoogleConnector::buildSchema(Project* /*project*/, const QString& /*path*/) const
{
    return Success();
}

void GoogleConnector::reset(Project* /*project*/) const
{

}

} // namespace CyberTracker
