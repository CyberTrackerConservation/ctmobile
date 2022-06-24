#include "EsriProvider.h"
#include "App.h"
#include "Form.h"
#include "XlsForm.h"

constexpr bool CACHE_ENABLE = true;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tasks

namespace {

bool refreshAccessToken(QNetworkAccessManager* networkAccessManager, Project* project)
{
    auto accessToken = project->accessToken();
    auto refreshToken = project->refreshToken();

    auto result = Utils::esriRefreshOAuthToken(networkAccessManager, refreshToken, &accessToken, &refreshToken);
    if (!result.success)
    {
        qDebug() << "Failed to refresh token: " << result.errorString;
        return false;
    }

    project->set_accessToken(accessToken);
    project->set_refreshToken(refreshToken);

    return true;
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
        if (!success)
        {
            taskManager->pauseTask(projectUid, uid);
            if (output["status"] == 401 || output["status"] == 498)
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
        auto serviceUrl = connectorParams["serviceUrl"].toString() + "/applyEdits";
        auto payload = input["payload"].toByteArray();

        QNetworkAccessManager networkAccessManager;

        auto sendData = [&]() -> QVariantMap
        {
            auto query = QUrlQuery();
            query.addQueryItem("f", "json");
            query.addQueryItem("edits", QUrl::toPercentEncoding(payload));
            query.addQueryItem("useGlobalIds", "true");
            query.addQueryItem("rollbackOnFailure", "true");
            query.addQueryItem("token", project->accessToken());

            auto response = Utils::httpPostQuery(&networkAccessManager, serviceUrl, query);

            auto output = QVariantMap();
            if (!response.success)
            {
                output["success"] = false;
                output["status"] = response.status;
                output["errorString"] = response.errorString;
                return output;
            }

            auto errorMap = QJsonDocument::fromJson(response.data).object().toVariantMap();
            if (errorMap.contains("error"))
            {
                output["success"] = false;
                errorMap = errorMap.value("error").toMap();
                output["status"] = errorMap.value("code", -1).toInt();
                output["errorString"] = errorMap.value("message").toString();
                return output;
            }

            auto results = QJsonDocument::fromJson(response.data).array().toVariantList();
            if (results.count() == 0)
            {
                output["success"] = false;
                output["status"] = 404;
                return output;
            }

            output["success"] = true;
            output["response"] = results;
            output["status"] = response.status;
            output["sightingUid"] = input["sightingUid"];

            return output;
        };

        auto output = sendData();
        if (output["status"] == 401 || output["status"] == 498)
        {
            if (refreshAccessToken(&networkAccessManager, project.get()))
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
        if (!success)
        {
            taskManager->pauseTask(projectUid, uid);
            if (output["status"] == 401 || output["status"] == 498)
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
        QNetworkAccessManager networkAccessManager;

        // Get the objectid for each record.
        auto parentOutput = this->parentOutput();
        auto lookupObjectId = [&](const QString& recordUid, int* serviceIdOut, qint64* objectIdOut) -> bool
        {
            auto globalId = Utils::addBracesToUuid(recordUid);
            auto resultsList = parentOutput["response"].toList();
            for (auto results: resultsList)
            {
                auto resultsMap = results.toMap();

                auto addResults = resultsMap["addResults"].toList();
                for (auto result: addResults)
                {
                    auto resultMap = result.toMap();
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

        // Prepare work.
        auto serviceUrl = connectorParams["serviceUrl"].toString();
        auto input = this->input();
        auto payload = input["payload"].toMap();

        int serviceId;
        qint64 objectId;
        if (!lookupObjectId(payload["recordUid"].toString(), &serviceId, &objectId))
        {
            qDebug() << "Lost attachment: no object found";
            return true; // Fall through to next task.
        }

        auto filePath = App::instance()->getMediaFilePath(payload["filename"].toString());
        QFile file(filePath);
        QFileInfo fileInfo(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            qDebug() << "Lost attachment: file could not be opened";
            return true; // Fall through to next task.
        }

        serviceUrl = serviceUrl + '/' + QString::number(serviceId) + '/' + QString::number(objectId) + "/addAttachment";

        // Send data function.
        auto sendData = [&]() -> QVariantMap
        {
            auto accessToken = project->accessToken();

            QHttpMultiPart multipart(QHttpMultiPart::FormDataType);
            multipart.setBoundary("------WebKitFormBoundarytoHka8LUGjq34sBN");

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

            auto response = Utils::httpPostMultiPart(&networkAccessManager, serviceUrl, &multipart);

            auto output = QVariantMap();
            if (!response.success)
            {
                output["success"] = false;
                output["status"] = response.status;
                output["errorString"] = response.errorString;
                return output;
            }

            auto outputMap = QJsonDocument::fromJson(response.data).object().toVariantMap();
            if (outputMap.contains("error"))
            {
                output["success"] = false;
                auto errorMap = outputMap.value("error").toMap();
                output["status"] = errorMap.value("code", -1).toInt();
                output["errorString"] = errorMap.value("message");
                return output;
            }

            auto jsonObj = outputMap.value("addAttachmentResult").toMap();
            output["success"] = jsonObj.value("success").toBool();
            output["status"] = response.status;
            output["reason"] = response.reason;
            output["errorString"] = response.errorString;
            output["data"] = response.data;
            output["sightingUid"] = parentOutput.value("sightingUid");

            return output;
        };

        auto output = sendData();
        if (output["status"] == 401 || output["status"] == 498)
        {
            if (refreshAccessToken(&networkAccessManager, project.get()))
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
        emit App::instance()->sightingModified(projectUid, sightingUid);
    }

    bool doWork() override
    {
        update_output(QVariantMap { { "sightingUid", input()["sightingUid"] } });

        return true;
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

bool EsriProvider::connectToProject(bool newBuild)
{
    m_taskManager->registerTask(UploadSightingTask::getName(), UploadSightingTask::createInstance, UploadSightingTask::completed);
    m_taskManager->registerTask(UploadFileTask::getName(), UploadFileTask::createInstance, UploadFileTask::completed);
    m_taskManager->registerTask(UploadCompleteTask::getName(), UploadCompleteTask::createInstance, UploadCompleteTask::completed);

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

    QString elementsFilePath = getProjectFilePath("Elements.qml");
    QString fieldsFilePath = getProjectFilePath("Fields.qml");

    // Early out if possible.
    if (!CACHE_ENABLE || newBuild || !QFile::exists(elementsFilePath) || !QFile::exists(fieldsFilePath))
    {
        if (XlsFormUtils::parseXlsForm(getProjectFilePath("form.xlsx"), getProjectFilePath(""), &m_parserError))
        {
            setFieldTags(getProjectFilePath("serviceInfo.json"), fieldsFilePath);
        }
    }

    // Update the languages.
    m_settings = Utils::variantMapFromJsonFile(getProjectFilePath("Settings.json"));
    m_project->set_languages(m_settings.value("languages").toList());

    return true;
}

bool EsriProvider::requireUsername() const
{
    return m_project->username().isEmpty() && App::instance()->settings()->username().isEmpty();
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

QVariantList EsriProvider::buildSightingPayload(Form* form, const QString& sightingUid, QVariantMap* attachmentsMapOut)
{
    auto sighting = form->createSightingPtr(sightingUid);

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
    auto queueRecordUids = QStringList { sightingUid };

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
                    return;
                }
                else if (qobject_cast<LineField*>(field) || qobject_cast<AreaField*>(field))
                {
                    auto points = fieldValue.value().toList();
                    auto path = QVariantList();

                    for (auto it = points.constBegin(); it != points.constEnd(); it++)
                    {
                        auto point = it->toList();
                        auto x = point[0].toDouble();
                        auto y = point[1].toDouble();
                        path.append(QVariant(QVariantList { x, y }));
                    }

                    geometry.insert("paths", QVariantList { QVariant(path) });
                    return;
                }

                // Attachments.
                else if (qobject_cast<PhotoField*>(field) || qobject_cast<AudioField*>(field) || qobject_cast<SketchField*>(field))
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

bool EsriProvider::sendSighting(Form* form, const QString& sightingUid)
{
    auto app = App::instance();
    auto database = app->database();
    auto projectUid = m_project->uid();

    // Ensure this is not already submitted.
    uint flags = database->getSightingFlags(projectUid, sightingUid);

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
    auto attachmentsMap = QVariantMap();
    auto payload = buildSightingPayload(form, sightingUid, &attachmentsMap);
    auto payloadJson = QJsonDocument(QJsonArray::fromVariantList(payload)).toJson(QJsonDocument::Compact);
    auto inputMap = QVariantMap();
    inputMap.insert("payload", payloadJson);
    inputMap.insert("sightingUid", sightingUid);

    // Create the task for the sighting.
    auto sightingTaskUid = QString();
    auto lastTaskUid = QString();
    {
        auto baseUid = Task::makeUid(UploadSightingTask::getName(), sightingUid);
        auto parentUids = QStringList();
        m_taskManager->getTasks(projectUid, baseUid, -1, &parentUids);
        auto parentUid = !parentUids.isEmpty() ? parentUids.last(): QString();

        sightingTaskUid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
        m_taskManager->addTask(projectUid, sightingTaskUid, parentUid, inputMap);
        lastTaskUid = sightingTaskUid;
    }

    // Create the tasks for the attachments (1 per attachments).
    auto baseUid = Task::makeUid(UploadFileTask::getName(), sightingUid);
    for (auto recordUid: attachmentsMap.keys())
    {
        auto attachmentsTaskUid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
        auto attachments = attachmentsMap[recordUid].toStringList();
        auto counter = 0;
        for (auto filename: attachments)
        {
            auto inputMap = QVariantMap();
            inputMap["payload"] = QVariantMap { { "recordUid", recordUid }, { "filename", filename } };
            auto attachmentTaskUid = attachmentsTaskUid + "." + QString::number(counter);

            auto alreadySentTaskUids = QStringList();
            m_taskManager->getTasks(projectUid, baseUid, -1, &alreadySentTaskUids);
            if (alreadySentTaskUids.isEmpty())
            {
                m_taskManager->addTask(projectUid, attachmentTaskUid, lastTaskUid, inputMap);
                lastTaskUid = attachmentTaskUid;
            }

            counter++;
        }
    }

    // Create the completion task.
    m_taskManager->addTask(projectUid, Task::makeUid(UploadCompleteTask::getName(), sightingUid), lastTaskUid, QVariantMap { { "sightingUid", sightingUid } });

    // Mark the sighting as submitted.
    database->setSightingFlags(projectUid, sightingUid, Sighting::DB_SUBMITTED_FLAG);
    emit app->sightingModified(projectUid, sightingUid);

    return true;
}

void EsriProvider::submitData()
{
    auto app = App::instance();
    auto database = app->database();
    auto projectUid = m_project->uid();
    auto sightingUids = QStringList();
    auto form = qobject_cast<Form*>(parent());

    database->getSightings(projectUid, form->stateSpace(), Sighting::DB_SIGHTING_FLAG, &sightingUids);
    for (auto it = sightingUids.constBegin(); it != sightingUids.constEnd(); it++)
    {
        // Call is smart enough not to resend already sent data.
        sendSighting(form, *it);
    }

    app->taskManager()->resumePausedTasks(projectUid);
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
