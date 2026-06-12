#include "Connector.h"
#include "App.h"

namespace CyberTracker {

Connector::Connector(QObject* parent): QObject(parent)
{
    m_networkAccessManager = App::instance()->networkAccessManager();
    m_database = App::instance()->database();
    m_projectManager = App::instance()->projectManager();
}

Connector::~Connector()
{
}

void Connector::setNetworkAccessManager(QNetworkAccessManager* networkAccessManager)
{
    m_networkAccessManager = networkAccessManager;
}

void Connector::migrateProject(Project* /*project*/) const
{
}

bool Connector::canExportData(Project* /*project*/) const
{
    return false;
}

bool Connector::canSharePackage(Project* /*project*/) const
{
    return false;
}

bool Connector::canShareAuth(Project* /*project*/) const
{
    return false;
}

QVariantMap Connector::getShareData(Project* /*project*/, bool /*auth*/) const
{
    return QVariantMap();
}

bool Connector::canLogin(Project* /*project*/) const
{
    return false;
}

bool Connector::loggedIn(Project* /*project*/) const
{
    return true;
}

bool Connector::login(Project* /*project*/, const QString& /*server*/, const QString& /*username*/, const QString& /*password*/) const
{
    return true;
}

void Connector::logout(Project* /*project*/) const
{
}

ApiResult Connector::refreshAccessToken(Project* /*project*/, QString* /*oldRefreshTokenOut*/, QString* /*newAccessTokenOut*/, QString* /*newRefreshTokenOut*/)
{
    return NotSupported();
}

ApiResult Connector::hasUpdate(Project* /*project*/)
{
    return NotSupported();
}

ApiResult Connector::hasAnnouncements(Project* /*project*/, QVariantList* /*announcementsOut*/)
{
    return NotSupported();
}

ApiResult Connector::uploadData(Project* /*project*/, const Database::DbQueueItem& /*queueItem*/, QVariantMap* /*state*/, const CommitState& /*commitState*/)
{
    return NotSupported();
}

QString Connector::getFormFilePath(Project* /*project*/) const
{
    return "Form.qml";
}

ApiResult Connector::bootstrap(const QVariantMap& /*params*/) const
{
    return Failure(tr("Not implemented"));
}

bool Connector::canUpdate(Project* /*project*/) const
{
    return false;
}

ApiResult Connector::update(Project* /*project*/) const
{
    return Failure(tr("Not implemented"));
}

ApiResult Connector::buildSchema(Project* /*project*/, const QString& /*path*/) const
{
    return Success();
}

void Connector::reset(Project* /*project*/) const
{
}

void Connector::remove(Project* /*project*/) const
{
}

void Connector::deleteState(Project* /*project*/) const
{
}

void Connector::deleteData(Project* /*project*/) const
{
}

ApiResult Connector::ExpectedError(const QString& errorString)
{
    return ApiResult { false, errorString, true };
}

ApiResult Connector::Failure(const QString& errorString)
{
    return ApiResult { false, errorString, false };
}

ApiResult Connector::Success(const QString& errorString)
{
    return ApiResult { true, errorString, true };
}

ApiResult Connector::Unauthorized()
{
    return ApiResult { false, "Unauthorized", false };
}

ApiResult Connector::NotImplemented()
{
    return ApiResult { false, "Not implemented", false };
}

ApiResult Connector::NotSupported()
{
    return ApiResult { false, "Not supported", false };
}

ApiResult Connector::AlreadyUpToDate()
{
    return ApiResult { false, tr("Up to date"), true };
}

ApiResult Connector::AlreadyConnected()
{
    return ApiResult { false, tr("Already connected"), true };
}

ApiResult Connector::UpdateAvailable()
{
    return ApiResult { true, tr("Update available"), true };
}

ApiResult Connector::Failure(const Utils::HttpResponse& response)
{
    if (response.unauthorized())
    {
        return Unauthorized();
    }

    auto errorString = response.errorString;
    if (errorString.isEmpty())
    {
        errorString = "Operation failed";
    }

    return ApiResult { false, QString("%1: %2").arg(response.status).arg(errorString), false };
}

ApiResult Connector::ResultFromResponse(const Utils::HttpResponse& response)
{
    return response.success ? Success() : Failure(response);
}

ApiResult Connector::bootstrapUpdate(const QString& projectUid) const
{
    auto result = ApiResult::fromMap(m_projectManager->update(projectUid, false));
    if (!result.success)
    {
        m_projectManager->remove(projectUid);
        return Failure(result.errorString);
    }

    return Success();
}

ApiResult Connector::uploadDataODK(Project* project, const QString& url, const QByteArray& payload, const QStringList& attachments) const
{
    auto multipart = QHttpMultiPart(QHttpMultiPart::FormDataType);
    multipart.setBoundary(Utils::makeMultiPartBoundary());

    // Add payload.
    auto xmlPart = QHttpPart();
    xmlPart.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
    xmlPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"xml_submission_file\"; filename=\"xml_submission_file\"");
    xmlPart.setBody(payload);
    multipart.append(xmlPart);

    // Add attachments.
    auto mimeDatabase = QMimeDatabase();
    auto attachmentFiles = QList<QFile*>();
    auto cleanupFilesOnExit = qScopeGuard([&] { qDeleteAll(attachmentFiles); });
    for (auto attachmentIt = attachments.constBegin(); attachmentIt != attachments.constEnd(); attachmentIt++)
    {
        auto file = new QFile(App::instance()->getMediaFilePath(*attachmentIt));
        qFatalIf(!file->open(QIODevice::ReadOnly), "Failed to read attachment: " + *attachmentIt);
        attachmentFiles.append(file);

        auto part = QHttpPart();
        part.setHeader(QNetworkRequest::ContentTypeHeader, "arbitrary");
        auto fileInfo = QFileInfo(file->fileName());
        auto partName = fileInfo.fileName();
        part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + partName + "\"; filename=\"" + partName + "\"");
        QMimeType mimeType = mimeDatabase.mimeTypeForFile(file->fileName());
        part.setHeader(QNetworkRequest::ContentTypeHeader, mimeType.name());
        part.setBodyDevice(file);
        multipart.append(part);
    }

    // Post the request.
    auto request = Utils::httpCreateRequest(url);
    request.setRawHeader("X-OpenRosa-Version", "1.0");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + multipart.boundary());

    auto token = project->accessToken();
    auto tokenType = project->getConnectorParam("tokenType", "Bearer").toString();
    if (!token.isEmpty())
    {
        request.setRawHeader("Authorization", (tokenType + " " + token).toLocal8Bit());
    }

    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->post(request, &multipart));

    Utils::httpWait(reply.get());

    return ResultFromResponse(Utils::HttpResponse::fromReply(url, reply.get()));
}

} // namespace CyberTracker