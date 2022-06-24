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
            emit App::instance()->sightingModified(projectUid, sightingUid);
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
        multipart.setBoundary("------WebKitFormBoundarytoHka8LUGjq34sBN");
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

        return responseXml.contains("successful", Qt::CaseInsensitive);
    }
};

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ODKProvider

ODKProvider::ODKProvider(QObject *parent) : Provider(parent)
{
    update_name(ODK_PROVIDER);
}

bool ODKProvider::connectToProject(bool newBuild)
{
    m_taskManager->registerTask(UploadSightingTask::getName(), UploadSightingTask::createInstance, UploadSightingTask::completed);

    auto projectUid = m_project->uid();

    // Delete uploaded sightings after 2 seconds.
    auto form = qobject_cast<Form*>(parent());
    connect(form, &Form::sightingModified, this, [=](const QString& sightingUid)
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
    auto elementsFilePath = getProjectFilePath("Elements.qml");
    auto fieldsFilePath = getProjectFilePath("Fields.qml");

    // Early out if possible.
    if (!CACHE_ENABLE || newBuild || !QFile::exists(elementsFilePath) || !QFile::exists(fieldsFilePath))
    {
        XlsFormUtils::parseXlsForm(getProjectFilePath("form.xlsx"), getProjectFilePath(""), &m_parserError);
    }

    // Update the languages.
    m_settings = Utils::variantMapFromJsonFile(getProjectFilePath("Settings.json"));
    m_project->set_languages(m_settings.value("languages").toList());

    return true;
}

bool ODKProvider::requireUsername() const
{
    return m_project->username().isEmpty() && App::instance()->settings()->username().isEmpty();
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

bool ODKProvider::sendSighting(Form* form, const QString& sightingUid)
{
    auto app = App::instance();
    auto projectUid = m_project->uid();

    // Ensure this is not already submitted.
    uint flags = m_database->getSightingFlags(projectUid, sightingUid);
    if ((flags & Sighting::DB_COMPLETED_FLAG) == 0)
    {
        qDebug() << "Not completed: " << sightingUid;
        return false;
    }

    if (flags & Sighting::DB_SUBMITTED_FLAG)
    {
        qDebug() << "Already submitted: " << sightingUid;
        return false;
    }

    // Build the payload.
    auto formId = m_project->connectorParams()["exportId"].toString();
    auto formVersion = m_project->connectorParams()["version"].toString();

    auto attachments = QStringList();
    auto formXml = form->createSightingPtr(sightingUid)->toXml(formId, formVersion, &attachments);

    auto inputJson = QJsonObject();
    inputJson.insert("payload", formXml);
    inputJson.insert("attachments", QJsonArray::fromStringList(attachments));
    inputJson.insert("sightingUid", sightingUid);

    // Create the task for the main sighting.
    auto baseUid = Task::makeUid(UploadSightingTask::getName(), sightingUid);
    auto parentUids = QStringList();
    m_taskManager->getTasks(projectUid, baseUid, -1, &parentUids);
    auto parentUid = !parentUids.isEmpty() ? parentUids.last(): QString();
    auto sightingTaskUid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
    m_taskManager->addTask(projectUid, sightingTaskUid, parentUid, inputJson.toVariantMap());

    // Mark the sighting as submitted.
    m_database->setSightingFlags(projectUid, sightingUid, Sighting::DB_SUBMITTED_FLAG);
    emit app->sightingModified(projectUid, sightingUid);

    return true;
}

void ODKProvider::submitData()
{
    auto projectUid = m_project->uid();
    auto sightingUids = QStringList();
    auto form = qobject_cast<Form*>(parent());

    m_database->getSightings(projectUid, form->stateSpace(), Sighting::DB_SIGHTING_FLAG, &sightingUids);
    for (auto it = sightingUids.constBegin(); it != sightingUids.constEnd(); it++)
    {
        // Call is smart enough not to resend already sent data.
        sendSighting(form, *it);
    }

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
