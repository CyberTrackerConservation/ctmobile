#include "CTOConnector.h"
#include "../ODK/ODKProvider.h"
#include "App.h"
#include "XlsForm.h"

namespace CyberTracker {

CTOConnector::CTOConnector(QObject *parent) : Connector(parent)
{
    update_name(CONNECTOR_NAME);
    update_icon("qrc:/CTO/logo.svg");
}

CTOConnector::~CTOConnector()
{
}

void CTOConnector::migrateProject(Project* project) const
{
    // Hack: try to find the access token in the logs.
    // Around build 530, we shipped a change which lost tokens.
    if (!project->accessToken().isEmpty())
    {
        return;
    }

    auto rootPath = QFileInfo(App::instance()->logFilePath()).path();
    Utils::enumFiles(rootPath, [&](const QFileInfo& fileInfo)
    {
        if (!fileInfo.baseName().startsWith("Log"))
        {
            return;
        }

        auto file = QFile(fileInfo.filePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "Failed to open log file: " << file.fileName();
            return;
        }

        auto stream = QTextStream(&file);
        while (!stream.atEnd())
        {
            auto line = stream.readLine();
            constexpr char APPLINK_PREFIX[] = "https://cybertrackerwiki.org/applink?x=";
            auto applinkIndex = line.indexOf(APPLINK_PREFIX);
            if (applinkIndex == -1)
            {
                continue;
            }

            line.remove(0, applinkIndex);
            line.remove(0, QString(APPLINK_PREFIX).length());
            line.removeLast();

            auto applink = App::instance()->appLink()->decode(line);
            if (applink.value("connector") != CONNECTOR_NAME)
            {
                continue;
            }

            if (applink.value("formId") != project->getConnectorParam("formId"))
            {
                continue;
            }

            auto token = applink.value("token").toString();
            if (token.isEmpty())
            {
                continue;
            }

            qDebug() << "Recovered access token for CTO form: " << applink.value("formId").toString();
            project->set_accessToken(token);
            return;
        }
    });
}

bool CTOConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool CTOConnector::canShareAuth(Project* /*project*/) const
{
    return false;
}

QVariantMap CTOConnector::getShareData(Project* project, bool /*auth*/) const
{
    if (project->connectorParams().isEmpty())
    {
        return QVariantMap();
    }

    auto result = QVariantMap
    {
        { "connector", CONNECTOR_NAME },
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

bool CTOConnector::login(Project* /*project*/, const QString& /*server*/, const QString& /*username*/, const QString& /*password*/) const
{
    return false;
}

void CTOConnector::logout(Project* project) const
{
    project->set_accessToken("");
}

QString CTOConnector::getFormFilePath(Project* project) const
{
    return m_projectManager->getFilePath(project->uid(), "form.xlsx");
}

ApiResult CTOConnector::bootstrap(const QVariantMap& params) const
{
    auto server = params["server"].toString();
    server = Utils::removeTrailingSlash(server);

    auto formId = params["formId"].toString();
    auto projectUid = "CTO_" + formId;
    auto token = params.value("token").toString();

    if (m_projectManager->exists(projectUid))
    {
        // Update the access token.
        App::instance()->vault()->setAccessToken(projectUid, token);

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

    if (!m_projectManager->init(projectUid, ODKProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, connectorParams))
    {
        return Failure(QString(tr("Failed to create %1")).arg(App::instance()->alias_project()));
    }

    m_projectManager->modify(projectUid, [&](Project* project)
    {
        project->set_accessToken(token);
    });

    return bootstrapUpdate(projectUid);
}

ApiResult CTOConnector::hasUpdate(Project* project)
{
    auto server = project->getConnectorParam("server").toString();
    auto formId = project->getConnectorParam("formId").toString();
    auto token = project->accessToken();
    auto currentVersion = project->getConnectorParam("version").toString();
    auto url = server + "/api/applications/" + formId + "/version";

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

    auto formMap = QJsonDocument::fromJson(response.data).object().toVariantMap();
    auto deployedVersion = formMap.value("version").toString();
    if (deployedVersion.isEmpty())
    {
        return Failure(tr("No deployed versions"));
    }

    if (deployedVersion == currentVersion)
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

ApiResult CTOConnector::uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* /*state*/, const CommitState& /*commitState*/)
{
    return uploadDataODK(project, queueItem.url, queueItem.payload, queueItem.attachments);
}

bool CTOConnector::canUpdate(Project* project) const
{
    return loggedIn(project);
}

ApiResult CTOConnector::update(Project* project) const
{
    // Skip update if not logged in.
    auto projectUid = project->uid();
    if (!loggedIn(project))
    {
        return Failure(tr("Logged out"));
    }

    // Skip updates if there are any unsent sightings.
    auto sightingUids = QStringList();
    m_database->getData(project->uid(), "", Database::DataFlag::DB_SIGHTING_FLAG, &sightingUids);
    if (sightingUids.count() > 0)
    {
        return Failure(tr("Unsent data"));
    }

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
    auto cleanupZipFile = qScopeGuard([&] { QFile::remove(zipFilePath); });

    // Unpack the project.
    auto packagePath = Utils::zipUnpack(App::instance()->tempPath(), "Package", zipFilePath);
    if (packagePath.isEmpty())
    {
        return Failure(tr("Bad package"));
    }
    auto cleanupPackagePath = qScopeGuard([&] { QDir(packagePath).removeRecursively(); });

    // Copy to the update folder.
    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    if (!Utils::copyPath(packagePath + "/project", updateFolder))
    {
        return Failure(tr("Copy failed"));
    }

    // Load the project.
    auto projectJson = Utils::variantMapFromJsonFile(packagePath + "/project.json");

    // Create a new project from the json file.
    Project updateProject;
    updateProject.load(projectJson, false);
    updateProject.set_uid(project->uid());
    updateProject.set_connector(CONNECTOR_NAME);
    updateProject.set_connectorParams(project->connectorParams());

    // Reset the project.
    m_projectManager->reset(projectUid, true);

    // Prepare the project.
    return buildSchema(&updateProject, updateFolder);
}

ApiResult CTOConnector::buildSchema(Project* project, const QString& path) const
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

    // Get the form version and add it to the connectorParams.
    project->set_version(1);
    project->set_provider(ODKProvider::PROVIDER_NAME);
    project->set_defaultWizardMode(true);
    project->setConnectorParam("version", schema.version());

    if (project->icon().isEmpty())
    {
        project->set_icon("qrc:/app/appicon.svg");
    }

    // Commit the project and schema file.
    project->saveToQmlFile(path + "/Project.qml");
    schema.setProjectInfo(project->title(), project->subtitle(), project->icon());
    schema.saveToQmlFile(path + "/Form.qml");

    return Success();
}

void CTOConnector::reset(Project* project) const
{
    QFile::remove(m_projectManager->getFilePath(project->uid(), "Form.qml"));
}

} // namespace CyberTracker
