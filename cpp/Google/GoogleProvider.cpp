#include "GoogleProvider.h"
#include "App.h"
#include "Form.h"

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
        return QString(GOOGLE_PROVIDER) + "_SaveSighting";
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
            return;
        }

        // Failed: prompt for login.
        taskManager->pauseTask(projectUid, uid);
    }

    bool doWork() override
    {
        // Check if uploads are allowed.
        if (!App::instance()->uploadAllowed())
        {
            qDebug() << "Upload blocked because not on WiFi";
            return false;
        }

        QNetworkAccessManager networkAccessManager;

        auto project = App::instance()->projectManager()->loadPtr(projectUid());
        auto connectorParams = project->connectorParams();

        auto input = this->input();
        auto publishedUrl = connectorParams["publishedUrl"].toString();
        publishedUrl.replace("/viewform", "/formResponse?", Qt::CaseInsensitive);

        auto payload = input["payload"].toString();

        auto attachments = parentOutput();
        for (auto attachmentIt = attachments.constKeyValueBegin(); attachmentIt != attachments.constKeyValueEnd(); attachmentIt++)
        {
            auto filename = attachmentIt->first;
            auto fileLink = attachmentIt->second.toString();
            auto encoded = QUrl(fileLink).toEncoded();
            if (payload.contains(filename))
            {
                payload.replace(filename, encoded);
            }
        }

        publishedUrl += payload;
        publishedUrl += "&emailAddress=" + connectorParams.value("email", "unknown@unknown.com").toString();

        auto result = Utils::httpGet(&networkAccessManager, publishedUrl);
         if (!result.data.contains("response has been recorded"))
        {
            qDebug() << "Failed to upload: " + result.data;
            result.success = false;
            result.status = 304;
        }

        if (!result.success)
        {
            qDebug() << "Google Forms upload failed: " << result.errorString;
        }

        auto output = result.toMap();
        output["sightingUid"] = input["sightingUid"];
        update_output(output);

        return result.success;
    }
};

// UploadFileTask.
class UploadFileTask: public Task
{
public:
    UploadFileTask(QObject* parent): Task(parent)
    {}

    static QString getName()
    {
        return QString(GOOGLE_PROVIDER) + "_SaveFile";
    }

    static Task* createInstance(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput)
    {
        auto task = new UploadFileTask(nullptr);
        task->init(projectUid, uid, input, parentOutput);

        return task;
    }

    static void completed(TaskManager* taskManager, const QString& projectUid, const QString& uid, const QVariantMap& output, bool success)
    {
        if (success)
        {
            return;
        }

        // Failed - probably a bad file folder.
        taskManager->pauseTask(projectUid, uid);
        if (output["status"].toInt() == 400)
        {
            emit App::instance()->showError(QString(tr("Incorrect %1 tag")).arg("#fileFolder"));
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
        auto driveFolderId = connectorParams["driveFolderId"].toString();

        auto input = this->input();
        auto filename = input["filename"].toString();
        auto filePath = App::instance()->getMediaFilePath(filename);

        auto resultUrl = QString();

        QNetworkAccessManager networkAccessManager;
        auto response = Utils::googleUploadFile(&networkAccessManager, driveFolderId, filePath, &resultUrl);

        auto output = parentOutput();
        output[filename] = resultUrl;
        update_output(output);

        return response.success;
    }
};

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GoogleProvider

GoogleProvider::GoogleProvider(QObject *parent) : Provider(parent)
{
    update_name(GOOGLE_PROVIDER);
}

bool GoogleProvider::connectToProject(bool /*newBuild*/, bool* /*formChangedOut*/)
{
    m_taskManager->registerTask(UploadSightingTask::getName(), UploadSightingTask::createInstance, UploadSightingTask::completed);
    m_taskManager->registerTask(UploadFileTask::getName(), UploadFileTask::createInstance, UploadFileTask::completed);

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

    // Load settings.
    m_settings = Utils::variantMapFromJsonFile(getProjectFilePath("Settings.json"));

    // Show a message if we are missing a file folder for uploads.
    if (m_settings.value("driveFolderId").toString().isEmpty() && m_settings.value("needsFileFolder").toBool())
    {
        emit App::instance()->showError(QString(tr("Missing %1 attribute")).arg("#fileFolder"));
    }

    return true;
}

bool GoogleProvider::requireUsername() const
{
    return m_settings["requireUsername"].toBool() &&
           m_project->username().isEmpty() &&
           App::instance()->settings()->username().isEmpty();
}

QUrl GoogleProvider::getStartPage()
{
    if (m_settings.value("hasFileUploadItem").toBool())
    {
        return QUrl("qrc:/Google/UnsupportedPage.qml");
    }

    return QUrl("qrc:/ODK/StartPage.qml");
}

void GoogleProvider::getElements(ElementManager* elementManager)
{
    elementManager->loadFromQmlFile(getProjectFilePath("Elements.qml"));
}

void GoogleProvider::getFields(FieldManager* fieldManager)
{
    fieldManager->loadFromQmlFile(getProjectFilePath("Fields.qml"));
}

QUrlQuery GoogleProvider::buildSighting(Sighting* sighting)
{
    auto result = QUrlQuery();

    auto addDate = [&](const QString& exportUid, const QString& isoDateTime, bool hideYear)
    {
        auto date = Utils::decodeTimestamp(isoDateTime).date();

        if (!hideYear)
        {
            result.addQueryItem(exportUid + "_year", QString::number(date.year()));
        }

        result.addQueryItem(exportUid + "_month", QString::number(date.month()));
        result.addQueryItem(exportUid + "_day", QString::number(date.day()));
    };

    auto addTime = [&](const QString& exportUid, const QString& isoDateTime)
    {
        auto time = Utils::decodeTimestamp(isoDateTime).time();

        result.addQueryItem(exportUid + "_hour", QString::number(time.hour()));
        result.addQueryItem(exportUid + "_minute", QString::number(time.minute()));
    };

    sighting->recordManager()->enumFieldValues(
        sighting->rootRecordUid(), true,
        nullptr, // recordBeginCallback.
        [&](const FieldValue& fieldValue) // fieldValueCallback.
        {
            auto field = fieldValue.field();
            auto exportUid = fieldValue.exportUid();
            auto valueString = fieldValue.value().toString();

            // String and single select.
            auto stringField = qobject_cast<StringField*>(field);
            if (stringField && !valueString.isEmpty())
            {
                if (!stringField->listElementUid().isEmpty())
                {
                    auto valueElement = m_elementManager->getElement(valueString);
                    result.addQueryItem(exportUid, valueElement->exportUid());
                    return;
                }

                if (stringField->tag().value("duration", false).toBool())
                {
                    auto values = valueString.split(':');
                    qFatalIf(values.length() != 3, "Bad duration");
                    result.addQueryItem(exportUid + "_hour", values[0]);
                    result.addQueryItem(exportUid + "_minute", values[1]);
                    result.addQueryItem(exportUid + "_second", values[2]);
                    return;
                }

                result.addQueryItem(exportUid, valueString);
                return;
            }

            // Check list.
            auto checkField = qobject_cast<CheckField*>(field);
            if (checkField)
            {
                auto valueMap = fieldValue.value().toMap();
                if (valueMap.isEmpty())
                {
                    return;
                }

                m_elementManager->enumChildren(checkField->listElementUid(), true, [&](Element* element)
                {
                    auto elementUid = element->uid();
                    if (valueMap.contains(elementUid))
                    {
                        result.addQueryItem(exportUid, element->safeExportUid());
                    }
                });

                return;
            }

            // Date.
            auto dateField = qobject_cast<DateField*>(field);
            if (dateField && !valueString.isEmpty())
            {
                addDate(exportUid, valueString, dateField->hideYear());
                return;
            }

            // Time.
            auto timeField = qobject_cast<TimeField*>(field);
            if (timeField && !valueString.isEmpty())
            {
                addTime(exportUid, valueString);
                return;
            }

            // Datetime.
            auto dateTimeField = qobject_cast<DateTimeField*>(field);
            if (dateTimeField && !valueString.isEmpty())
            {
                addDate(exportUid, valueString, dateTimeField->hideYear());
                addTime(exportUid, valueString);
                return;
            }

            // Location.
            auto locationField = qobject_cast<LocationField*>(field);
            if (locationField)
            {
                auto valueMap = fieldValue.value().toMap();
                if (valueMap.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, QString("%1 %2").arg(valueMap["y"].toString(), valueMap["x"].toString()));
                return;
            }

            // Line field.
            auto lineField = qobject_cast<LineField*>(field);
            if (lineField)
            {
                auto value = fieldValue.xmlValue();
                if (value.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, value);
                return;
            }

            // Area field.
            auto areaField = qobject_cast<AreaField*>(field);
            if (areaField)
            {
                auto value = fieldValue.xmlValue();
                if (value.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, value);
                return;
            }

            // Photo.
            auto photoField = qobject_cast<PhotoField*>(field);
            if (photoField)
            {
                auto valueList = fieldValue.value().toStringList();
                if (valueList.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, valueList.join(", "));
                return;
            }

            // Audio.
            auto audioField = qobject_cast<AudioField*>(field);
            if (audioField)
            {
                auto valueMap = fieldValue.value().toMap();
                auto filename = valueMap["filename"].toString();
                if (filename.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, filename);
                return;
            }

            // Sketch.
            auto sketchField = qobject_cast<SketchField*>(field);
            if (sketchField)
            {
                auto valueMap = fieldValue.value().toMap();
                auto filename = valueMap["filename"].toString();
                if (filename.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, filename);
                return;
            }

            // File.
            auto fileField = qobject_cast<FileField*>(field);
            if (fileField)
            {
                auto filename = fieldValue.value().toString();
                if (filename.isEmpty())
                {
                    return;
                }

                result.addQueryItem(exportUid, filename);
                return;
            }

            // Everything else.
            if (!valueString.isEmpty())
            {
                result.addQueryItem(exportUid, valueString);
                return;
            }
        },
        nullptr // recordEndCallback.
    );

    result.addQueryItem("submit", "Submit");

    return result;
}

void GoogleProvider::submitData()
{
    auto projectUid = m_project->uid();
    auto form = qobject_cast<Form*>(parent());

    // Backup database and delete old tasks.
    App::instance()->backupDatabase("GF: submit data");
    m_database->deleteTasks(projectUid, Task::Complete);

    // Create tasks for sightings.
    m_database->enumSightings(projectUid, form->stateSpace(), Sighting::DB_SIGHTING_FLAG | Sighting::DB_COMPLETED_FLAG /* ON */, Sighting::DB_SUBMITTED_FLAG /* OFF */,
        [&](const QString& sightingUid, uint /*flags*/, const QVariantMap& data, const QStringList& /*attachments*/)
    {
        auto sighting = form->createSightingPtr(data);

        // Create a task for each attachment.
        auto attachments = sighting->recordManager()->attachments();
        for (auto attachmentIt = attachments.constBegin(); attachmentIt != attachments.constEnd(); attachmentIt++)
        {
            auto baseUid = Task::makeUid(UploadFileTask::getName(), sightingUid);
            auto fileTaskUid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
            m_taskManager->addSerialTask(projectUid, fileTaskUid, QVariantMap {{ "filename", *attachmentIt }});
        }

        // Build the payload.
        auto payload = buildSighting(sighting.get()).toString(QUrl::FullyEncoded);
        auto inputJson = QJsonObject();
        inputJson.insert("payload", payload);
        inputJson.insert("sightingUid", sightingUid);

        // Create the task for the main sighting.
        auto baseUid = Task::makeUid(UploadSightingTask::getName(), sightingUid);
        auto sightingTaskUid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
        m_taskManager->addSerialTask(projectUid, sightingTaskUid, inputJson.toVariantMap());

        // Mark the sighting as submitted.
        form->setSightingFlag(sightingUid, Sighting::DB_SUBMITTED_FLAG);
    });

    m_taskManager->resumePausedTasks(projectUid);
}
