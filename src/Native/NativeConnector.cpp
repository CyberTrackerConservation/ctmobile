#include "NativeConnector.h"
#include "NativeProvider.h"
#include "App.h"
#include "XlsForm.h"

namespace CyberTracker {

NativeConnector::NativeConnector(QObject *parent) : Connector(parent)
{
    update_name(CONNECTOR_NAME);
}

NativeConnector::~NativeConnector()
{
}

bool NativeConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool NativeConnector::canShareAuth(Project* /*project*/) const
{
    return false;
}

QVariantMap NativeConnector::getShareData(Project* project, bool /*auth*/) const
{
    if (project->webUpdateUrl().isEmpty())
    {
        return QVariantMap();
    }

    return QVariantMap
    {
        { "connector", CONNECTOR_NAME },
        { "webUpdateUrl", project->webUpdateUrl() },
        { "projectUid", project->uid() },
        { "launch", true }
    };
}

bool NativeConnector::canLogin(Project* /*project*/) const
{
    return false;
}

QString NativeConnector::getFormFilePath(Project* project) const
{
    auto xlsForm = m_projectManager->getFilePath(project->uid(), "form.xlsx");
    auto qmlForm = m_projectManager->getFilePath(project->uid(), "Form.qml");

    return QFile::exists(xlsForm) ? xlsForm : qmlForm;
}

ApiResult NativeConnector::bootstrap(const QVariantMap& params) const
{
    // Get update url.
    auto webUpdateUrl = params.value("webUpdateUrl").toString();
    if (webUpdateUrl.isEmpty())
    {
        return Failure(tr("Install kind not found"));
    }

    // Load the metadata and check for a matching project.
    auto response = Utils::httpGet(m_networkAccessManager, webUpdateUrl);
    if (!response.success)
    {
        return Failure(response);
    }

    auto metadata = Utils::variantMapFromJson(response.data);

    auto projectUid = metadata.value("projectUid").toString();
    if (projectUid.isEmpty())
    {
        return Failure(tr("Missing projectUid"));
    }

    auto projects = m_projectManager->buildListPtr();
    for (auto it = projects.constBegin(); it != projects.constEnd(); it++)
    {
        auto project = *it;
        if (project->uid() == projectUid)
        {
            return AlreadyConnected();
        }

        if (project->webUpdateUrl().compare(webUpdateUrl, Qt::CaseInsensitive) == 0)
        {
            qDebug() << "Project uids are different, but another project matches the webUpdateUrl";
            return AlreadyConnected();
        }
    }

    // Bootstrap the project.
    auto connectorParams = metadata.value("connectorParams").toMap();
    m_projectManager->init(projectUid, NativeProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, connectorParams);

    auto project = m_projectManager->loadPtr(projectUid);
    project->set_webUpdateUrl(webUpdateUrl);

    return bootstrapUpdate(projectUid);
}

ApiResult NativeConnector::hasUpdate(Project* project)
{
    auto url = project->webUpdateUrl();

    // ETag mode: webUpdateETag is set (stamped at install time by webInstall).
    if (!project->webUpdateETag().isEmpty())
    {
        auto response = Utils::httpGetHead(m_networkAccessManager, url);
        if (!response.success)
        {
            return Failure(response);
        }

        if (response.etag == project->webUpdateETag())
        {
            return AlreadyUpToDate();
        }

        return UpdateAvailable();
    }

    auto currentVersion = project->webUpdateMetadata().value("projectVersion", 0).toInt();

    auto request = Utils::httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");

    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->get(request));

    Utils::httpWait(reply.get());

    auto response = Utils::HttpResponse::fromReply(url, reply.get());
    if (!response.success)
    {
        emit Failure(response);
    }

    auto metadata = Utils::variantMapFromJson(response.data);
    if (metadata.value("projectUid") != project->uid())
    {
        return Failure(tr("Package mismatch"));
    }

    auto deployedVersion = metadata.value("projectVersion").toInt();
    if (deployedVersion == currentVersion)
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

ApiResult NativeConnector::uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* /*state*/, const CommitState& /*commitState*/)
{
    auto multipart = QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Add JSON payload.
    auto jsonPart = QHttpPart();
    jsonPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    jsonPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"json_submission_file\"; filename=\"json_submission_file\"");
    jsonPart.setBody(queueItem.payload);
    multipart.append(jsonPart);

    // Add attachments.
    auto mimeDatabase = QMimeDatabase();
    auto attachmentFiles = QList<QFile*>();
    auto cleanupFilesOnExit = qScopeGuard([&] { qDeleteAll(attachmentFiles); });
    for (auto attachmentIt = queueItem.attachments.constBegin(); attachmentIt != queueItem.attachments.constEnd(); attachmentIt++)
    {
        auto file = new QFile(App::instance()->getMediaFilePath(*attachmentIt));
        qFatalIf(!file->open(QIODevice::ReadOnly), "Failed to read attachment: " + *attachmentIt);
        attachmentFiles.append(file);

        auto part = QHttpPart();
        auto fileInfo = QFileInfo(file->fileName());
        auto partName = fileInfo.fileName();
        part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + partName + "\"; filename=\"" + partName + "\"");
        part.setHeader(QNetworkRequest::ContentTypeHeader, mimeDatabase.mimeTypeForFile(file->fileName()).name());
        part.setBodyDevice(file);
        multipart.append(part);
    }

    // Post the request.
    auto token = project->accessToken();
    auto tokenType = project->getConnectorParam("tokenType", "Bearer").toString();

    return ResultFromResponse(Utils::httpPostMultiPart(m_networkAccessManager, queueItem.url, &multipart, token, tokenType));
}

bool NativeConnector::canUpdate(Project* project) const
{
    return !project->webUpdateUrl().isEmpty();
}

ApiResult NativeConnector::update(Project* project) const
{
    auto projectUid = project->uid();
    auto webUpdateUrl = project->webUpdateUrl();

    // ETag mode: webUpdateETag is set (stamped at install time by webInstall).
    if (!project->webUpdateETag().isEmpty())
    {
        auto headResponse = Utils::httpGetHead(m_networkAccessManager, webUpdateUrl);
        if (!headResponse.success)
        {
            return Failure(headResponse);
        }

        if (headResponse.etag == project->webUpdateETag())
        {
            return AlreadyUpToDate();
        }

        auto zipFilePath = App::instance()->downloadFile(webUpdateUrl);
        if (zipFilePath.isEmpty())
        {
            return Failure(tr("Download failed"));
        }
        auto cleanupZipFile = qScopeGuard([&] { QFile::remove(zipFilePath); });

        return installPackageZip(project, zipFilePath, webUpdateUrl, QVariantMap(), headResponse.etag);
    }

    // Get the update metadata file.
    auto response = Utils::httpGet(m_networkAccessManager, webUpdateUrl);
    if (!response.success)
    {
        return Failure(response);
    }

    auto metadata = Utils::variantMapFromJson(response.data);
    if (metadata.value("projectUid") != projectUid)
    {
        return Failure(tr("Package mismatch"));
    }

    auto currentVersion = project->webUpdateMetadata().value("projectVersion", 0).toInt();
    auto metadataVersion = metadata.value("projectVersion").toInt();

    // Check for up to date.
    if (currentVersion >= metadataVersion)
    {
        return AlreadyUpToDate();
    }

    // Online version is newer, so download the package.
    auto zipFilePath = App::instance()->downloadFile(metadata.value("packageUrl").toString());
    if (zipFilePath.isEmpty())
    {
        return Failure(tr("Download failed"));
    }
    auto cleanupZipFile = qScopeGuard([&] { QFile::remove(zipFilePath); });

    return installPackageZip(project, zipFilePath, webUpdateUrl, metadata, QString());
}

ApiResult NativeConnector::installPackageZip(Project* project, const QString& zipFilePath, const QString& webUpdateUrl, const QVariantMap& metadata, const QString& eTag) const
{
    auto projectUid = project->uid();

    // Unpack the package and sanity check it.
    auto packagePath = Utils::zipUnpack(App::instance()->tempPath(), "Package", zipFilePath);
    if (packagePath.isEmpty())
    {
        return Failure(tr("Bad package"));
    }
    auto cleanupPackagePath = qScopeGuard([&] { QDir(packagePath).removeRecursively(); });

    auto projectJson = Utils::variantMapFromJsonFile(packagePath + "/project.json");
    if (!projectJson.contains("uid"))
    {
        return Failure(tr("Not a package"));
    }

    // Copy the project files to the update folder.
    auto projectPath = packagePath + "/project";
    if (!QDir(projectPath).exists())
    {
        return Failure(QString(tr("No %1 in package")).arg(App::instance()->alias_project()));
    }

    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    Utils::copyPath(projectPath, updateFolder);

    // Set up the new project.
    Project newProject;
    newProject.set_uid(projectUid);
    projectJson["uid"] = projectUid; // Ignore project uid from package.

    // Load settings for new projects (version == 0).
    newProject.load(projectJson, project->version() == 0);

    // Store the url, metadata and ETag.
    newProject.set_webUpdateUrl(webUpdateUrl);
    newProject.set_webUpdateMetadata(metadata);
    newProject.set_webUpdateETag(eTag);

    return buildSchema(&newProject, updateFolder);
}

ApiResult NativeConnector::buildSchema(Project* project, const QString& path) const
{
    if (!QFile::exists(path + "/form.xlsx"))
    {
        project->saveToQmlFile(path + "/Project.qml");
        return Success();
    }

    // Parse the form.
    auto schema = Schema();
    auto errorString = QString();
    if (!XlsFormParser::parse(path + "/form.xlsx", &schema, &errorString))
    {
        return Failure(tr("Bad form"));
    }

    // Modify project settings.
    XlsFormParser::configureProject(project, &schema);

    // Commit the project and schema file.
    project->saveToQmlFile(path + "/Project.qml");
    schema.setProjectInfo(project->title(), project->subtitle(), project->icon());
    schema.saveToQmlFile(path + "/Form.qml");

    return Success();
}

void NativeConnector::reset(Project* project) const
{
    if (QFile::exists(m_projectManager->getFilePath(project->uid(), "form.xlsx")))
    {
        QFile::remove(m_projectManager->getFilePath(project->uid(), "Form.qml"));
    }
}

} // namespace CyberTracker