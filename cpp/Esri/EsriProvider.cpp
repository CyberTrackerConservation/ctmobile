#include "EsriProvider.h"
#include "App.h"
#include "Form.h"
#include "XlsForm.h"
#include "TrackFile.h"

constexpr bool CACHE_ENABLE = true;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tasks

namespace {

bool requireLogin(int code)
{
    return code == 401 || code == 498;
}

// UploadSightingTask.
class UploadSightingTask: public Task
{
public:
    UploadSightingTask(QObject* parent): Task(parent)
    {}

    static QString getName()
    {
        return QString(ESRI_PROVIDER) + "_SaveSighting";
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
            return;
        }

        // Failed: prompt for login.
        taskManager->pauseTask(projectUid, uid);
        if (requireLogin(output["status"].toInt()))
        {
            emit App::instance()->projectManager()->projectTriggerLogin(projectUid);
        }
    }

    bool doWork() override
    {
        if (!App::instance()->uploadAllowed())
        {
            qDebug() << "Upload not allowed";
            return false;
        }

        QNetworkAccessManager networkAccessManager;

        auto project = App::instance()->projectManager()->loadPtr(projectUid());

        auto input = this->input();
        auto serviceUrl = project->connectorParams()["serviceUrl"].toString() + "/applyEdits";
        auto features = input["features"].toList();
        auto payload = QJsonDocument(QJsonArray::fromVariantList(features)).toJson(QJsonDocument::Compact);

        auto sendData = [&]() -> QVariantMap
        {
            auto query = QUrlQuery();
            query.addQueryItem("f", "json");
            query.addQueryItem("edits", payload);
            query.addQueryItem("useGlobalIds", "true");
            query.addQueryItem("rollbackOnFailure", "true");
            query.addQueryItem("token", project->accessToken());
            auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(&networkAccessManager, serviceUrl, query));

            auto results = QVariantList();
            if (response.success)
            {
                results = QJsonDocument::fromJson(response.data).array().toVariantList();
            }

            return QVariantMap
            {
                { "success", response.success },
                { "status", response.status },
                { "errorString", response.errorString },
                { "reason", response.reason },
                { "response", results }
            };
        };

        auto output = sendData();
        if (requireLogin(output["status"].toInt()))
        {
            if (App::instance()->createConnectorPtr(project->connector())->refreshAccessToken(&networkAccessManager, project.get()))
            {
                output = sendData();
            }
        }

        update_output(output);

        return output["success"] == true;
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
        return QString(ESRI_PROVIDER) + "_UploadFile";
    }

    static Task* createInstance(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput)
    {
        auto task = new UploadFileTask(nullptr);
        task->init(projectUid, uid, input, parentOutput, true);

        return task;
    }

    static void completed(TaskManager* taskManager, const QString& projectUid, const QString& uid, const QVariantMap& output, bool success)
    {
        if (success)
        {
            return;
        }

        // Failed: prompt for login.
        taskManager->pauseTask(projectUid, uid);
        if (requireLogin(output["status"].toInt()))
        {
            emit App::instance()->projectManager()->projectTriggerLogin(projectUid);
        }
    }

    bool doWork() override
    {
        if (!App::instance()->uploadAllowed())
        {
            qDebug() << "Upload not allowed";
            return false;
        }

        QNetworkAccessManager networkAccessManager;
        auto project = App::instance()->projectManager()->loadPtr(projectUid());
        auto connectorParams = project->connectorParams();

        // Get the objectid for each record.
        auto parentOutput = this->parentOutput();
        auto lookupObjectId = [&](const QString& recordUid, int* serviceIdOut, qint64* objectIdOut) -> bool
        {
            auto globalId = Utils::addBracesToUuid(recordUid);
            auto resultsList = parentOutput["response"].toList();
            for (auto resultsMapIt = resultsList.constBegin(); resultsMapIt != resultsList.constEnd(); resultsMapIt++)
            {
                auto resultsMap = resultsMapIt->toMap();
                auto addResults = resultsMap["addResults"].toList();
                for (auto resultMapIt = addResults.constBegin(); resultMapIt != addResults.constEnd(); resultMapIt++)
                {
                    auto resultMap = resultMapIt->toMap();
                    if (resultMap["globalId"] != globalId)
                    {
                        continue;
                    }

                    *serviceIdOut = resultsMap["id"].toInt();
                    *objectIdOut = resultMap["objectId"].toLongLong();
                    return true;
                }
            }

            return false;
        };

        // Send data.
        auto input = this->input();
        auto recordUid = input["recordUid"].toString();
        auto filename = input["filename"].toString();

        int serviceId = -1;
        qint64 objectId = 0;
        if (!lookupObjectId(recordUid, &serviceId, &objectId))
        {
            qDebug() << "Error: lost attachment - no object found";
            return true; // Fall through to next task.
        }

        auto filePath = App::instance()->getMediaFilePath(filename);
        QFile file(filePath);
        QFileInfo fileInfo(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            qDebug() << "Error: lost attachment - file could not be opened";
            return true; // Fall through to next task.
        }

        auto serviceUrl = connectorParams["serviceUrl"].toString() + '/' + QString::number(serviceId) + '/' + QString::number(objectId) + "/addAttachment";

        auto sendData = [&]() -> QVariantMap
        {
            auto accessToken = project->accessToken();

            QHttpMultiPart multipart(QHttpMultiPart::FormDataType);

            auto part1 = QHttpPart();
            part1.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"f\"");
            part1.setBody("json");
            multipart.append(part1);

            auto part2 = QHttpPart();
            part2.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"token\"");
            part2.setBody(accessToken.toUtf8());
            multipart.append(part2);

            auto part3 = QHttpPart();
            part3.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"attachment\"");
            part3.setBody(fileInfo.fileName().toUtf8());
            multipart.append(part3);

            QMimeDatabase mimeDatabase;
            QMimeType mimeType;
            mimeType = mimeDatabase.mimeTypeForFile(fileInfo);

            auto part4 = QHttpPart();
            part4.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; token=\"" + accessToken + "\"; name=\"attachment\"; filename=\"" + fileInfo.fileName() + "\"");
            part4.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, mimeType.name());
            part4.setBodyDevice(&file);
            multipart.append(part4);

            auto response = Utils::esriDecodeResponse(Utils::httpPostMultiPart(&networkAccessManager, serviceUrl, &multipart));

            if (!response.success)
            {
                return QVariantMap
                {
                    { "success", false },
                    { "status", response.status },
                    { "errorString", response.errorString },
                    { "reason", response.reason },
                };
            }

            auto result = QJsonDocument::fromJson(response.data).object().toVariantMap().value("addAttachmentResult").toMap();

            return QVariantMap
            {
                { "success", result.value("success").toBool() },
                { "status", response.status },
                { "errorString", response.errorString },
                { "reason", response.reason },
                { "result", result },
            };
        };

        auto output = sendData();
        if (requireLogin(output["status"].toInt()))
        {
            if (App::instance()->createConnectorPtr(project->connector())->refreshAccessToken(&networkAccessManager, project.get()))
            {
                output = sendData();
            }
        }

        update_output(output);

        return output["success"] == true;
    }
};

// UploadCompleteTask.
class UploadCompleteTask: public Task
{
public:
    UploadCompleteTask(QObject* parent): Task(parent)
    {}

    static QString getName()
    {
        return QString(ESRI_PROVIDER) + "_UploadComplete";
    }

    static Task* createInstance(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput)
    {
        auto task = new UploadCompleteTask(nullptr);
        task->init(projectUid, uid, input, parentOutput, true);

        return task;
    }

    static void completed(TaskManager* /*taskManager*/, const QString& projectUid, const QString& /*uid*/, const QVariantMap& output, bool /*success*/)
    {
        auto sightingUid = output.value("sightingUid").toString();
        App::instance()->database()->setSightingFlags(projectUid, sightingUid, Sighting::DB_UPLOADED_FLAG);

        emit App::instance()->sightingFlagsChanged(projectUid, sightingUid);
    }

    bool doWork() override
    {
        update_output(input());

        return true;
    }
};

// UploadLocationsTask.
class UploadLocationsTask: public Task
{
public:
    UploadLocationsTask(QObject* parent): Task(parent)
    {}

    static QString getName()
    {
        return QString(ESRI_PROVIDER) + "_SaveLocations";
    }

    static Task* createInstance(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput)
    {
        auto task = new UploadLocationsTask(nullptr);
        task->init(projectUid, uid, input, parentOutput);

        return task;
    }

    static void completed(TaskManager* taskManager, const QString& projectUid, const QString& uid, const QVariantMap& output, bool success)
    {
        // Success: clean up snapped locations. Submitted is sufficient, since the task has the data.
        if (success)
        {
            App::instance()->database()->deleteSightings(projectUid, "", Sighting::DB_LOCATION_FLAG | Sighting::DB_SUBMITTED_FLAG | Sighting::DB_SNAPPED_FLAG);
            return;
        }

        // Failed: prompt for login.
        taskManager->pauseTask(projectUid, uid);
        if (requireLogin(output["status"].toInt()))
        {
            emit App::instance()->projectManager()->projectTriggerLogin(projectUid);
        }
    }

    bool doWork() override
    {
        if (!App::instance()->uploadAllowed())
        {
            qDebug() << "Upload not allowed";
            return false;
        }

        QNetworkAccessManager networkAccessManager;
        auto project = App::instance()->projectManager()->loadPtr(projectUid());
        auto serviceUrl = project->esriLocationServiceUrl();

        auto input = this->input();
        auto locationUids = input["locationUids"].toStringList();
        auto features = input["features"].toList();

        auto sendData = [&]() -> QVariantMap
        {
            auto results = QVariantList();
            auto response = Utils::esriApplyEdits(&networkAccessManager, serviceUrl, "adds", 0, features, project->accessToken(), &results);

            return QVariantMap
            {
                { "locationUids", locationUids },
                { "success", response.success },
                { "status", response.status },
                { "errorString", response.errorString },
                { "reason", response.reason },
                { "response", results }
            };
        };

        auto output = sendData();
        if (requireLogin(output["status"].toInt()))
        {
            if (App::instance()->createConnectorPtr(project->connector())->refreshAccessToken(&networkAccessManager, project.get()))
            {
                output = sendData();
            }
        }

        update_output(output);

        return output["success"] == true;
    }
};

// UpdateLastLocationTask.
class UpdateLastLocationTask: public Task
{
public:
    UpdateLastLocationTask(QObject* parent): Task(parent)
    {}

    static QString getName()
    {
        return QString(ESRI_PROVIDER) + "_UpdateLastLocation";
    }

    static Task* createInstance(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput)
    {
        auto task = new UpdateLastLocationTask(nullptr);
        task->init(projectUid, uid, input, parentOutput);

        return task;
    }

    static void completed(TaskManager* taskManager, const QString& projectUid, const QString& uid, const QVariantMap& output, bool success)
    {
        if (success)
        {
            return;
        }

        // Failed: prompt for login.
        taskManager->pauseTask(projectUid, uid);
        if (requireLogin(output["status"].toInt()))
        {
            emit App::instance()->projectManager()->projectTriggerLogin(projectUid);
        }
    }

    bool doWork() override
    {
        if (!App::instance()->uploadAllowed())
        {
            qDebug() << "Upload not allowed";
            return false;
        }

        auto project = App::instance()->projectManager()->loadPtr(projectUid());
        if (project->sendLocationInterval() == 0)
        {
            qDebug() << "Skip sending location, since no longer enabled";
            return true;
        }

        QNetworkAccessManager networkAccessManager;
        auto serviceUrl = project->esriLocationServiceUrl();

        auto input = this->input();
        auto feature = input["feature"].toMap();

        auto sendData = [&]() -> QVariantMap
        {
            auto results = QVariantList();
            auto response = Utils::esriApplyEdits(&networkAccessManager, serviceUrl, "updates", 1, feature, project->accessToken(), &results);

            // Normally the last location state will be initialized during project install.
            // In case the last location table is reset, attempt to reinitialize it.
            if (response.success && results.length() == 1)
            {
                auto updateResults = results.constFirst().toMap()["updateResults"].toList();
                if (updateResults.length() == 1)
                {
                    auto errorCode = updateResults.constFirst().toMap()["error"].toMap()["code"].toInt();
                    if (errorCode == 1019)
                    {
                        auto esriLocationServiceState = QVariantMap();
                        auto initResponse = Utils::esriInitLocationTracking(&networkAccessManager, serviceUrl, project->accessToken(), &esriLocationServiceState);
                        if (initResponse.success)
                        {
                            project->set_esriLocationServiceState(esriLocationServiceState);
                        }
                    }
                }
            }

            return QVariantMap
            {
                { "success", response.success },
                { "status", response.status },
                { "errorString", response.errorString },
                { "reason", response.reason },
                { "response", results }
            };
        };

        auto output = sendData();
        if (requireLogin(output["status"].toInt()))
        {
            if (App::instance()->createConnectorPtr(project->connector())->refreshAccessToken(&networkAccessManager, project.get()))
            {
                output = sendData();
            }
        }

        update_output(output);

        return output["success"] == true;
    }
};

// UploadTrackTask.
class UploadTrackTask: public Task
{
public:
    UploadTrackTask(QObject* parent): Task(parent)
    {}

    static QString getName()
    {
        return QString(ESRI_PROVIDER) + "_UploadTrack";
    }

    static Task* createInstance(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput)
    {
        auto task = new UploadTrackTask(nullptr);
        task->init(projectUid, uid, input, parentOutput, true);

        return task;
    }

    static void completed(TaskManager* taskManager, const QString& projectUid, const QString& uid, const QVariantMap& output, bool success)
    {
        if (success)
        {
            App::instance()->database()->deleteSightings(projectUid, "", Sighting::DB_LOCATION_FLAG | Sighting::DB_SUBMITTED_FLAG | Sighting::DB_SNAPPED_FLAG);
            return;
        }

        // Failed: prompt for login.
        taskManager->pauseTask(projectUid, uid);
        if (requireLogin(output["status"].toInt()))
        {
            emit App::instance()->projectManager()->projectTriggerLogin(projectUid);
        }
    }

    bool doWork() override
    {
        if (!App::instance()->uploadAllowed())
        {
            qDebug() << "Upload not allowed";
            return false;
        }

        QNetworkAccessManager networkAccessManager;
        auto project = App::instance()->projectManager()->loadPtr(projectUid());
        auto serviceState = project->esriLocationServiceState();
        auto serviceUrl = project->esriLocationServiceUrl();

        auto input = this->input();
        auto sightingUid = input["sightingUid"].toString();
        auto trackFilename = input["trackFile"].toString();

        TrackFile trackFile(App::instance()->getMediaFilePath(trackFilename));
        auto features = trackFile.createEsriFeatures(sightingUid, serviceState.value("fullName").toString());

        auto sendData = [&]() -> QVariantMap
        {
            auto results = QVariantList();
            auto response = Utils::esriApplyEdits(&networkAccessManager, serviceUrl, "adds", 2, features, project->accessToken(), &results);

            return QVariantMap
            {
                { "success", response.success },
                { "status", response.status },
                { "errorString", response.errorString },
                { "reason", response.reason },
                { "response", results }
            };
        };

        auto output = sendData();
        if (requireLogin(output["status"].toInt()))
        {
            if (App::instance()->createConnectorPtr(project->connector())->refreshAccessToken(&networkAccessManager, project.get()))
            {
                output = sendData();
            }
        }

        output["sightingUid"] = sightingUid;
        update_output(output);

        return output["success"] == true;
    }
};

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EsriProvider

EsriProvider::EsriProvider(QObject *parent) : Provider(parent)
{
    update_name(ESRI_PROVIDER);
}

void EsriProvider::setFieldTags(const QString& serviceInfoFilePath, const QString& fieldsFilePath)
{
    auto serviceInfo = Utils::variantMapFromJsonFile(serviceInfoFilePath);
    FieldManager fieldManager;
    fieldManager.loadFromQmlFile(fieldsFilePath);

    auto processFields = [&](const QVariantMap& serviceInfo)
    {
        auto serviceId = serviceInfo["id"].toInt();

        auto fields = serviceInfo["fields"].toList();
        auto fieldMap = QMap<QString, int>();
        for (auto i = 0; i < fields.count(); i++)
        {
            fieldMap.insert(fields[i].toMap().value("name").toString(), i);
        }

        auto recordField = qobject_cast<RecordField*>(serviceId == 0 ? fieldManager.rootField() : fieldManager.getField(serviceInfo["name"].toString()));
        if (!recordField)
        {
            qDebug() << "EsriProvider - skipping table: " << serviceInfo["name"];
            return;
        }

        recordField->set_tag(QVariantMap {{ "serviceId", serviceId }, { "geometryType", serviceInfo.value("geometryType", "") } });
        auto recordFields = QList<RecordField*>{ recordField };

        while (!recordFields.isEmpty())
        {
            recordFields.first()->enumFields([&](BaseField* field)
            {
                // Handle child records.
                auto childRecordField = qobject_cast<RecordField*>(field);
                if (childRecordField)
                {
                    if (childRecordField->group())
                    {
                        recordFields.append(childRecordField);
                    }

                    return;
                }

                auto fieldIndex = fieldMap.value(Utils::lastPathComponent(field->uid()), -1);
                if (fieldIndex == -1)
                {
                    return;
                }

                auto fieldMeta = fields[fieldIndex].toMap();

                // Skip parentglobalid.
                if (fieldMeta["name"] == "parentglobalid")
                {
                    return;
                }

                // Skip read-only fields.
                if (!fieldMeta["editable"].toBool())
                {
                    return;
                }
                fieldMeta.remove("editable");

                // Remove domain tag, because it repeats list elements and makes the qml needlessly large.
                fieldMeta.remove("domain");

                field->set_tag(fieldMeta);
            });

            recordFields.removeFirst();
        }
    };

    auto tables = serviceInfo["tables"].toList();
    for (auto i = 0; i < tables.count(); i++)
    {
        processFields(tables[i].toMap()["serviceInfo"].toMap());
    }

    auto layers = serviceInfo["layers"].toList();
    for (auto i = 0; i < layers.count(); i++)
    {
        processFields(layers[i].toMap()["serviceInfo"].toMap());
    }

    fieldManager.saveToQmlFile(fieldsFilePath);
}

bool EsriProvider::connectToProject(bool newBuild, bool* formChangedOut)
{
    m_taskManager->registerTask(UploadSightingTask::getName(), UploadSightingTask::createInstance, UploadSightingTask::completed);
    m_taskManager->registerTask(UploadFileTask::getName(), UploadFileTask::createInstance, UploadFileTask::completed);
    m_taskManager->registerTask(UploadTrackTask::getName(), UploadTrackTask::createInstance, UploadTrackTask::completed);
    m_taskManager->registerTask(UploadCompleteTask::getName(), UploadCompleteTask::createInstance, UploadCompleteTask::completed);
    m_taskManager->registerTask(UploadLocationsTask::getName(), UploadLocationsTask::createInstance, UploadLocationsTask::completed);
    m_taskManager->registerTask(UpdateLastLocationTask::getName(), UpdateLastLocationTask::createInstance, UpdateLastLocationTask::completed);

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

    // Last known location.
    connect(form, &Form::locationPoint, this, [=](Location* update)
    {
        sendLastLocation(update);
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
        if (XlsFormParser::parse(formFilePath, getProjectFilePath(""), projectUid, &m_parserError))
        {
            setFieldTags(getProjectFilePath("serviceInfo.json"), fieldsFilePath);
        }
        m_project->set_formLastModified(formLastModified);
        m_project->reload();
        *formChangedOut = true;
    }

    // Load settings.
    m_settings = Utils::variantMapFromJsonFile(getProjectFilePath("Settings.json"));

    return true;
}

bool EsriProvider::requireUsername() const
{
    return m_settings["requireUsername"].toBool() &&
           m_project->username().isEmpty() &&
           App::instance()->settings()->username().isEmpty();
}

bool EsriProvider::supportLocationPoint() const
{
    return !m_project->esriLocationServiceUrl().isEmpty();
}

bool EsriProvider::supportLocationTrack() const
{
    // Location service.
    auto hasLocationService = !m_project->esriLocationServiceUrl().isEmpty();
    auto hasTrackFileField = !qobject_cast<Form*>(parent())->sighting()->getTrackFileFieldUid().isEmpty();

    if (hasLocationService && hasTrackFileField)
    {
        qWarning() << "Both location service and track file field detected: location service will be ignored";
    }

    return hasLocationService || hasTrackFileField;
}

QUrl EsriProvider::getStartPage()
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

void EsriProvider::getElements(ElementManager* elementManager)
{
    elementManager->loadFromQmlFile(getProjectFilePath("Elements.qml"));
}

void EsriProvider::getFields(FieldManager* fieldManager)
{
    fieldManager->loadFromQmlFile(getProjectFilePath("Fields.qml"));
}

QVariantList EsriProvider::buildSightingPayload(Sighting* sighting, QVariantMap* attachmentsMapOut)
{
    // Build the per-record payload.
    struct RecordData
    {
        QString recordUid;
        int serviceId = -1;
        QVariantMap payload;
    };

    attachmentsMapOut->clear();
    auto recordDataList = QVector<RecordData>();

    // Iterate over the records and build a RecordData structure for each service table.
    auto queueRecordUids = QStringList { sighting->rootRecordUid() };

    while (!queueRecordUids.isEmpty())
    {
        auto currRecordUid = queueRecordUids.first();
        queueRecordUids.removeFirst();
        auto recordField = sighting->getRecord(currRecordUid)->recordField();

        auto geometry = QVariantMap();
        auto geometryType = recordField->getTagValue("geometryType").toString();
        if (!geometryType.isEmpty())
        {
            geometry = QVariantMap { { "geometryType", geometryType }, { "spatialReference", QVariantMap { { "wkid", 4326 } } } };
        }

        auto attributes = QVariantMap();
        attributes.insert("globalid", Utils::addBracesToUuid(currRecordUid));

        auto parentRecordUid = sighting->getRecord(currRecordUid)->parentRecordUid();
        if (!parentRecordUid.isEmpty())
        {
            attributes.insert("parentglobalid", Utils::addBracesToUuid(parentRecordUid));
        }

        auto attachments = QStringList();

        // Enumerate the fields in this record and child groups.
        sighting->recordManager()->enumFieldValues(currRecordUid, true,
            [&](const Record& record, bool* skipRecordOut) // recordBeginCallback.
            {
                // Always use the record we are scanning.
                if (record.recordUid() == currRecordUid)
                {
                    return;
                }

                // Flatten groups.
                if (record.recordField()->group())
                {
                    return;
                }

                // Skip this record.
                *skipRecordOut = true;

                // Add complex records with a service as pending records.
                if (recordField->tag().contains("serviceId"))
                {
                    queueRecordUids.append(record.recordUid());
                }
            },
            [&](const FieldValue& fieldValue) // fieldValueCallback.
            {
                auto field = fieldValue.field();

                // Skip record fields:
                //   If it is a group, enum will enumerate all the children next
                //   If it is a complex record, the recordBeginCallback will track it for later.
                if (field->isRecordField())
                {
                    return;
                }

                auto fieldIsList = !field->listElementUid().isEmpty();
                auto finalFieldValue = fieldValue.value();

                // Single select.
                if (fieldIsList && qobject_cast<StringField*>(field))
                {
                    if (!fieldValue.value().toString().isEmpty())
                    {
                        finalFieldValue = m_elementManager->getElement(fieldValue.value().toString())->safeExportUid();
                    }
                }
                // Check list.
                else if (fieldIsList && qobject_cast<CheckField*>(field))
                {
                    auto fieldValueMap = fieldValue.value().toMap();
                    auto fieldValues = QStringList();

                    for (auto it = fieldValueMap.constKeyValueBegin(); it != fieldValueMap.constKeyValueEnd(); it++)
                    {
                        if (!it->second.toBool())
                        {
                            continue;
                        }

                        fieldValues.append(m_elementManager->getElement(it->first)->safeExportUid());
                    }

                    finalFieldValue = fieldValues.join(',');
                }
                // Location.
                else if (qobject_cast<LocationField*>(field))
                {
                    auto locationValue = fieldValue.value().toMap();
                    geometry.insert("x", locationValue["x"]);
                    geometry.insert("y", locationValue["y"]);
                    geometry.insert("z", locationValue["z"]);
                    return;
                }
                else if (qobject_cast<LineField*>(field))
                {
                    auto points = fieldValue.value().toList();
                    auto path = QVariantList();

                    for (auto it = points.constBegin(); it != points.constEnd(); it++)
                    {
                        auto point = it->toList();
                        auto x = point[0].toDouble();
                        auto y = point[1].toDouble();
                        auto z = point[2].toDouble();
                        path.append(QVariant(QVariantList { x, y, z }));
                    }

                    if (!path.isEmpty())
                    {
                        geometry.insert("paths", QVariantList { QVariant(path) });
                    }

                    return;
                }
                else if (qobject_cast<AreaField*>(field))
                {
                    auto points = fieldValue.value().toList();
                    auto path = QVariantList();

                    for (auto it = points.constBegin(); it != points.constEnd(); it++)
                    {
                        auto point = it->toList();
                        auto x = point[0].toDouble();
                        auto y = point[1].toDouble();
                        auto z = point[2].toDouble();
                        path.append(QVariant(QVariantList { x, y, z }));
                    }

                    if (!path.isEmpty())
                    {
                        path.append(path.constFirst()); // Last point same as first point.
                        geometry.insert("rings", QVariantList { QVariant(path) });
                    }

                    return;
                }

                // Attachments.
                else if (qobject_cast<PhotoField*>(field) ||
                         qobject_cast<AudioField*>(field) ||
                         qobject_cast<SketchField*>(field) ||
                         qobject_cast<FileField*>(field))
                {
                    attachments.append(fieldValue.attachments());
                    return;
                }

                // Get ESRI field type for custom conversions.
                auto esriFieldType = field->getTagValue("type").toString();
                if (esriFieldType.isEmpty())
                {
                    // Some calculation fields are only used internally to the form.
                    qWarning() << "Missing esriFieldType: " << fieldValue.fieldUid();
                }

                // Convert date to required format.
                if (esriFieldType == "esriFieldTypeDate")
                {
                    finalFieldValue = Utils::decodeTimestampMSecs(fieldValue.value().toString());
                }
                else if (esriFieldType == "esriFieldTypeInteger")
                {
                    finalFieldValue = fieldValue.value().toInt();
                }

                // Fill in the attribute.
                attributes.insert(field->safeExportUid(), finalFieldValue);
            },
            nullptr
        );

        // Add to the list.
        RecordData recordData;
        recordData.recordUid = currRecordUid;
        recordData.serviceId = recordField->getTagValue("serviceId", -1).toInt();
        recordData.payload["attributes"] = attributes;

        if (!geometry.isEmpty())
        {
            recordData.payload["geometry"] = geometry;
        }

        recordDataList.append(recordData);

        if (attachments.count() > 0)
        {
            attachmentsMapOut->insert(currRecordUid, attachments);
        }
    }

    // Sort by layer id.
    auto orderRecordUids = sighting->recordManager()->recordUids();
    std::sort(recordDataList.begin(), recordDataList.end(), [&](const RecordData& rd1, const RecordData& rd2) -> bool
    {
        // Order by original list order if the service ids are the same.
        if (rd1.serviceId == rd2.serviceId)
        {
            return orderRecordUids.indexOf(rd1.recordUid) < orderRecordUids.indexOf(rd2.recordUid);
        }

        // Order by service id.
        return rd1.serviceId < rd2.serviceId;
    });

    // Build the final output.
    auto result = QVariantList();

    auto lastServiceId = -1;
    auto lastServiceAdds = QVariantList();
    for (auto it = recordDataList.constBegin(); it != recordDataList.constEnd(); it++)
    {
        if (lastServiceId != -1 && it->serviceId != lastServiceId)
        {
            result.append(QVariantMap { { "id", lastServiceId }, { "adds", lastServiceAdds } });
            lastServiceAdds.clear();
        }

        lastServiceId = it->serviceId;
        lastServiceAdds.append(it->payload);
    }

    result.append(QVariantMap { { "id", lastServiceId }, { "adds", lastServiceAdds } });

    return result;
}

QVariantMap EsriProvider::buildLocationPayload(const QString& locationUid, const QVariantMap& location) const
{
    auto attributes = QVariantMap
    {
        { "globalid", Utils::addBracesToUuid(locationUid) },
        { "deviceid", Utils::addBracesToUuid(App::instance()->settings()->deviceId()) },
        { "appid", App::instance()->config()["title"] },
        { "counter", location["c"].toInt() },
        { "timestamp", Utils::decodeTimestampMSecs(location["ts"].toString()) },
        { "accuracy", location["a"] },
        { "altitude", location["z"] },
        { "speed", location["s"] },
        { "course", location["d"] },
        { "interval", location["interval"].toInt() / 1000 },
        { "distance_filter", location["distanceFilter"].toInt() },
        { "accuracy_filter", location["accuracyFilter"].toInt() },
        { "full_name", m_project->esriLocationServiceState().value("fullName", "Not Found") },
    };

    auto geometry = QVariantMap
    {
        { "spatialReference", QVariantMap { { "wkid", 4326 } } },
        { "x", location["x"] },
        { "y", location["y"] },
        { "z", location["z"] },
    };

    return QVariantMap
    {
        { "attributes", attributes },
        { "geometry", geometry },
    };
}

bool EsriProvider::sendSighting(Sighting* sighting)
{
    auto projectUid = m_project->uid();
    auto sightingUid = sighting->rootRecordUid();

    auto attachmentsMap = QVariantMap();
    auto features = buildSightingPayload(sighting, &attachmentsMap);

    auto inputMap = QVariantMap
    {
        { "sightingUid", sightingUid },
        { "features", features },
    };

    // Create the task for the sighting.
    auto sightingTaskUid = Task::makeUid(UploadSightingTask::getName(), sightingUid);
    m_taskManager->addSerialTask(projectUid, sightingTaskUid, inputMap);

    // Create the tasks for the attachments (1 per attachments).
    for (auto attachmentsIt = attachmentsMap.constKeyValueBegin(); attachmentsIt != attachmentsMap.constKeyValueEnd(); attachmentsIt++)
    {
        auto recordUid = attachmentsIt->first;
        auto filenames = attachmentsIt->second.toStringList();

        auto counter = 0;
        for (auto filenameIt = filenames.constBegin(); filenameIt != filenames.constEnd(); filenameIt++)
        {
            auto filename = *filenameIt;
            auto inputMap = QVariantMap {{ "recordUid", recordUid }, { "filename", filename }};

            auto taskUid = Task::makeUid(UploadFileTask::getName(), sightingUid) + ".attachment." + recordUid + "." + QString::number(counter++);
            m_taskManager->addTask(projectUid, taskUid, sightingTaskUid, inputMap);
        }
    }

    // Create the task for the track file.
    if (!sighting->trackFile().isEmpty() && m_project->esriLocationServiceUrl().isEmpty())
    {
        auto inputMap = QVariantMap
        {
            { "sightingUid", sightingUid },
            { "trackFile", sighting->trackFile() },
        };

        auto taskUid = Task::makeUid(UploadTrackTask::getName(), sightingUid);
        m_taskManager->addTask(projectUid, taskUid, sightingTaskUid, inputMap);
    }

    // Create the completion task.
    m_taskManager->addSerialTask(projectUid, Task::makeUid(UploadCompleteTask::getName(), sightingUid), QVariantMap { { "sightingUid", sightingUid } });

    return true;
}

void EsriProvider::sendLocations(const QStringList& locationUids, const QVariantList& features)
{
    auto inputMap = QVariantMap
    {
        { "locationUids", locationUids },
        { "features", features }
    };

    auto taskUid = Task::makeUid(UploadLocationsTask::getName(), locationUids.constFirst());
    m_taskManager->addSerialTask(m_project->uid(), taskUid, inputMap);
}

void EsriProvider::sendLastLocation(Location* location)
{
    auto serviceState = m_project->esriLocationServiceState();

    auto attributes = QVariantMap
    {
        { "globalid", serviceState.value("globalid") },
        { "deviceid", Utils::addBracesToUuid(location->deviceId()) },
        { "appid", App::instance()->config()["title"] },
        { "timestamp", Utils::decodeTimestampMSecs(location->timestamp()) },
        { "accuracy", location->accuracy() },
        { "altitude", location->z() },
        { "speed", location->speed() },
        { "course", location->direction() },
        { "interval", location->interval() },
        { "full_name", serviceState.value("fullName", "Not Found") },
    };

    auto geometry = QVariantMap
    {
        { "spatialReference", QVariantMap { { "wkid", 4326 } } },
        { "x", location->x() },
        { "y", location->y() },
        { "z", location->z() },
    };

    auto feature = QVariantMap
    {
        { "attributes", attributes },
        { "geometry", geometry },
    };

    auto inputMap = QVariantMap
    {
        { "feature", feature }
    };

    auto taskUid = Task::makeUid(UpdateLastLocationTask::getName(), "singleton");
    m_taskManager->resetTask(m_project->uid(), taskUid, inputMap);
}

void EsriProvider::sendTrackFile(Sighting* sighting)
{
    if (sighting->trackFile().isEmpty())
    {
        // Nothing to send.
        return;
    }

    if (m_project->esriLocationServiceUrl().isEmpty())
    {
        // Nowhere to send.
        qDebug() << "Error: nowhere to send track file";
        return;
    }

    auto sightingUid = sighting->rootRecordUid();
    auto inputMap = QVariantMap
    {
        { "sightingUid", sightingUid },
        { "trackFile", sighting->trackFile() },
    };

    auto taskUid = Task::makeUid(UploadTrackTask::getName(), sightingUid);
    m_taskManager->addSerialTask(m_project->uid(), taskUid, inputMap);
}

void EsriProvider::submitData()
{
    auto projectUid = m_project->uid();
    auto form = qobject_cast<Form*>(parent());

    // Backup database and delete old tasks.
    App::instance()->backupDatabase("ESRI: submit data");
    m_database->deleteTasks(projectUid, Task::Complete);

    // Create tasks for sightings.
    m_database->enumSightings(projectUid, form->stateSpace(), Sighting::DB_SIGHTING_FLAG | Sighting::DB_COMPLETED_FLAG /* ON */, Sighting::DB_SUBMITTED_FLAG /* OFF */,
        [&](const QString& sightingUid, uint /*flags*/, const QVariantMap& data, const QStringList& /*attachments*/)
    {
        auto sighting = form->createSightingPtr(data);

        // Mark the sighting as submitted.
        sendSighting(sighting.get());
        form->setSightingFlag(sightingUid, Sighting::DB_SUBMITTED_FLAG);

        // Send any attached track file.
        sendTrackFile(sighting.get());
    });

    // Create tasks for locations.
    if (!m_project->esriLocationServiceUrl().isEmpty())
    {
        auto batchLocationUids = QStringList();
        auto batchFeatures = QVariantList();
        auto lastFeature = QVariantMap();

        m_database->enumSightings(projectUid, form->stateSpace(), Sighting::DB_LOCATION_FLAG /* ON */, Sighting::DB_SUBMITTED_FLAG /* OFF */,
            [&](const QString& locationUid, uint /*flags*/, const QVariantMap& data, const QStringList& /*attachments*/)
        {
            lastFeature = buildLocationPayload(locationUid, data);
            batchLocationUids.append(locationUid);
            batchFeatures.append(lastFeature);

            if (batchLocationUids.count() == 1024)
            {
                sendLocations(batchLocationUids, batchFeatures);
                batchLocationUids.clear();
                batchFeatures.clear();
            }
        });

        // Send leftovers.
        if (!batchLocationUids.isEmpty())
        {
            sendLocations(batchLocationUids, batchFeatures);
        }

        // Mark locations as submitted.
        m_database->setSightingFlags(projectUid, form->stateSpace(), Sighting::DB_LOCATION_FLAG /* ON */, Sighting::DB_SUBMITTED_FLAG /* OFF */, Sighting::DB_SUBMITTED_FLAG);
    }

    // Start all tasks.
    App::instance()->taskManager()->resumePausedTasks(projectUid);
}

bool EsriProvider::notify(const QVariantMap& message)
{
    if (message["AuthComplete"].toBool())
    {
        m_taskManager->resumePausedTasks(m_project->uid());
    }

    return true;
}

bool EsriProvider::finalizePackage(const QString& packageFilesPath) const
{
    // Cleanup cached files.
    QFile::remove(packageFilesPath + "/Elements.qml");
    QFile::remove(packageFilesPath + "/Fields.qml");
    QFile::remove(packageFilesPath + "/Settings.json");

    return true;
}
