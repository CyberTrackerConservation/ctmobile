#include "ODKProvider.h"
#include "App.h"
#include "Form.h"
#include "XlsForm.h"

constexpr bool CACHE_ENABLE = true;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tasks

namespace {

// UploadSightingTask.
class UploadSightingTask: public Task
{
public:
    UploadSightingTask(QObject* parent): Task(parent)
    {}

    static QString getName()
    {
        return QString(ODK_PROVIDER) + "_SaveSighting";
    }

    static Task* createInstance(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput)
    {
        auto task = new UploadSightingTask(nullptr);
        task->init(projectUid, uid, input, parentOutput);

        return task;
    }

    static void completed(TaskManager* taskManager, const QString& projectUid, const QString& uid, const QVariantMap& output, bool success)
    {
        if (success)
        {
            auto sightingUid = output.value("sightingUid").toString();
            App::instance()->database()->setSightingFlags(projectUid, sightingUid, Sighting::DB_UPLOADED_FLAG);
            emit App::instance()->sightingFlagsChanged(projectUid, sightingUid);
        }
        else
        {
            taskManager->pauseTask(projectUid, uid);
            if (output["errorCode"] == 401)
            {
                emit App::instance()->projectManager()->projectTriggerLogin(projectUid);
            }
        }
    }

    bool doWork() override
    {
        // Check if uploads are allowed.
        if (!App::instance()->uploadAllowed())
        {
            qDebug() << "Upload blocked because not on WiFi";
            return false;
        }

        auto project = App::instance()->projectManager()->loadPtr(projectUid());
        auto connectorParams = project->connectorParams();

        // Prepare work.
        auto input = this->input();
        auto token = project->accessToken();
        auto tokenType = connectorParams["tokenType"].toString();
        auto submissionUrl = connectorParams["submissionUrl"].toString();

        auto payloadXml = input["payload"].toByteArray();

        QHttpMultiPart multipart(QHttpMultiPart::FormDataType);
        multipart.setBoundary(Utils::makeMultiPartBoundary());
        auto xmlPart = QHttpPart();
        xmlPart.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
        xmlPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"xml_submission_file\"; filename=\"xml_submission_file\"");
        xmlPart.setBody(payloadXml);
        multipart.append(xmlPart);

        auto attachments = input["attachments"].toStringList();
        for (auto attachmentIt = attachments.constBegin(); attachmentIt != attachments.constEnd(); attachmentIt++)
        {
            auto attachment = App::instance()->getMediaFilePath(*attachmentIt);
            QFileInfo fileInfo(attachment);

            auto part = QHttpPart();
            part.setHeader(QNetworkRequest::ContentTypeHeader, "arbitrary");

            auto partName = fileInfo.fileName();
            part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + partName + "\"; filename=\"" + partName + "\"");

            QMimeDatabase mimeDatabase;
            QMimeType mimeType = mimeDatabase.mimeTypeForFile(attachment);
            part.setHeader(QNetworkRequest::ContentTypeHeader, mimeType.name());

            QFile file(attachment);
            if (file.exists() && file.open(QIODevice::ReadOnly))
            {
                part.setBody(file.readAll());
            }

            multipart.append(part);
        }

        QNetworkAccessManager networkAccessManager;
        QNetworkRequest request;
        QEventLoop eventLoop;
        request.setRawHeader("X-OpenRosa-Version", "1.0");
        request.setRawHeader("Connection", "keep-alive");
        request.setRawHeader("Accept", "*/*");
        request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + multipart.boundary());
        request.setRawHeader("Authorization", (tokenType + " " + token).toLocal8Bit());
        request.setUrl(submissionUrl);

        auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager.post(request, &multipart));
        connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();

        auto output = QVariantMap();
        auto responseXml = QString(reply->readAll());
        output["responseXml"] = responseXml;
        output["errorCode"] = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        output["sightingUid"] = input["sightingUid"];
        update_output(output);

        if (reply->error() != QNetworkReply::NoError)
        {
            qDebug() << "ODK upload failed: " << reply->errorString();
        }

        return responseXml.contains("successful", Qt::CaseInsensitive) || responseXml.contains("duplicate", Qt::CaseInsensitive);
    }
};

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ODKProvider

ODKProvider::ODKProvider(QObject *parent) : Provider(parent)
{
    update_name(ODK_PROVIDER);
}

bool ODKProvider::connectToProject(bool newBuild, bool* formChangedOut)
{
    m_taskManager->registerTask(UploadSightingTask::getName(), UploadSightingTask::createInstance, UploadSightingTask::completed);

    auto projectUid = m_project->uid();

    // Delete uploaded sightings after 2 seconds.
    auto form = qobject_cast<Form*>(parent());
    connect(form, &Form::sightingFlagsChanged, this, [=](const QString& sightingUid)
    {
        if (!m_database->testSighting(projectUid, sightingUid))
        {
            return;
        }

        auto flags = m_database->getSightingFlags(projectUid, sightingUid);
        if (!(flags & Sighting::DB_UPLOADED_FLAG))
        {
            return;
        }

        QTimer::singleShot(2000, form, &Form::removeUploadedSightings);
    });

    // Cleanup uploaded data.
    connect(form, &Form::connectedChanged, this, [=]()
    {
        form->removeUploadedSightings();
    });

    // Rebuild the form.
    auto formFilePath = getProjectFilePath("form.xlsx");
    auto formLastModified = QFileInfo(formFilePath).lastModified().toMSecsSinceEpoch();
    auto elementsFilePath = getProjectFilePath("Elements.qml");
    auto fieldsFilePath = getProjectFilePath("Fields.qml");

    // Early out if possible.
    if (!CACHE_ENABLE || newBuild || formLastModified != m_project->formLastModified() || !QFile::exists(elementsFilePath) || !QFile::exists(fieldsFilePath))
    {
        XlsFormParser::parse(formFilePath, getProjectFilePath(""), projectUid, &m_parserError);
        m_project->set_formLastModified(formLastModified);
        m_project->reload();
        *formChangedOut = true;
    }

    // Load settings.
    m_settings = Utils::variantMapFromJsonFile(getProjectFilePath("Settings.json"));

    return true;
}

bool ODKProvider::requireUsername() const
{
    return m_settings["requireUsername"].toBool() &&
           m_project->username().isEmpty() &&
           App::instance()->settings()->username().isEmpty();
}

QUrl ODKProvider::getStartPage()
{
    auto parameters = m_settings.value("parameters").toMap();
    if (parameters.contains("startPage"))
    {
        auto startPagePath = getProjectFilePath(parameters["startPage"].toString());
        if (QFile::exists(startPagePath))
        {
            return QUrl::fromLocalFile(startPagePath);
        }

        qDebug() << "Failed to load start page: " << startPagePath;
    }

    return QUrl("qrc:/ODK/StartPage.qml");
}

void ODKProvider::getElements(ElementManager* elementManager)
{
    elementManager->loadFromQmlFile(getProjectFilePath("Elements.qml"));
}

void ODKProvider::getFields(FieldManager* fieldManager)
{
    fieldManager->loadFromQmlFile(getProjectFilePath("Fields.qml"));
}

bool ODKProvider::canSubmitData() const
{
    return !m_project->connectorParams().value("submissionUrl").toString().isEmpty();
}

void ODKProvider::submitData()
{
    auto projectUid = m_project->uid();
    auto form = qobject_cast<Form*>(parent());

    // Backup database and delete old tasks.
    App::instance()->backupDatabase("ODK: submit data");
    m_database->deleteTasks(projectUid, Task::Complete);

    // Create tasks for sightings.
    m_database->enumSightings(projectUid, form->stateSpace(), Sighting::DB_SIGHTING_FLAG | Sighting::DB_COMPLETED_FLAG /* ON */, Sighting::DB_SUBMITTED_FLAG /* OFF */,
        [&](const QString& sightingUid, uint /*flags*/, const QVariantMap& data, const QStringList& /*attachments*/)
    {
        auto sighting = form->createSightingPtr(data);

        // Build the payload.
        auto formId = m_project->connectorParams()["exportId"].toString();
        auto formVersion = m_project->connectorParams()["version"].toString();

        auto attachments = QStringList();
        auto formXml = sighting->toXml(formId, formVersion, &attachments);

        auto inputJson = QJsonObject();
        inputJson.insert("payload", formXml);
        inputJson.insert("attachments", QJsonArray::fromStringList(attachments));
        inputJson.insert("sightingUid", sightingUid);

        // Create the task for the main sighting.
        auto baseUid = Task::makeUid(UploadSightingTask::getName(), sightingUid);
        auto sightingTaskUid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
        m_taskManager->addSingleTask(projectUid, sightingTaskUid, inputJson.toVariantMap());

        // Mark the sighting as submitted.
        form->setSightingFlag(sightingUid, Sighting::DB_SUBMITTED_FLAG);
    });

    // Remove snapped locations.
    m_database->deleteSightings(projectUid, form->stateSpace(), Sighting::DB_LOCATION_FLAG | Sighting::DB_SNAPPED_FLAG);

    m_taskManager->resumePausedTasks(projectUid);
}

bool ODKProvider::notify(const QVariantMap& message)
{
    if (message["AuthComplete"].toBool())
    {
        m_taskManager->resumePausedTasks(m_project->uid());
    }

    return true;
}

bool ODKProvider::finalizePackage(const QString& packageFilesPath) const
{
    // Cleanup cached files.
    QFile::remove(packageFilesPath + "/Elements.qml");
    QFile::remove(packageFilesPath + "/Fields.qml");
    QFile::remove(packageFilesPath + "/Settings.json");

    return true;
}
