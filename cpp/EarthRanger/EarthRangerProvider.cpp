#include "EarthRangerProvider.h"
#include "EarthRangerConnector.h"
#include "App.h"
#include "Form.h"

constexpr bool CACHE_ENABLE     = true;
constexpr char FIELD_LOCATION[] = "__location";
constexpr char FIELD_PHOTOS[]   = "__photos";
constexpr char FIELD_NOTES[]    = "__notes";

constexpr char ELEMENT_REPORTED_BY[] = "__reportedBy";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tasks

namespace {

static void handleTaskUploadError(const QVariantMap& output, const QString& projectUid)
{
    if (output["status"] == 403)
    {
        emit App::instance()->projectManager()->projectTriggerLogin(projectUid);
    }
}

static bool isGoodButUnpatchableResult(const QString& dataAccessString, int httpResult)
{
    return (httpResult == 201) && dataAccessString.isEmpty();
}

static bool isSuccessResult(const QString& dataAccessString, int httpResult)
{
    // note: can't rely on 201+empty access string anymore.
    auto httpOk = (httpResult >= 200) && (httpResult <= 299);
    auto okAndReadWrite = httpOk && !dataAccessString.isEmpty();
    auto okAndReadOnly = httpOk && dataAccessString.isEmpty();

    return okAndReadWrite || okAndReadOnly;
}

static void logAnyErrors(const QString& action, const QString& server, QNetworkReply* reply)
{
    if (reply)
    {
        auto httpResult = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if ((reply->error() != QNetworkReply::NoError) || (httpResult < 200) || (httpResult > 299))
        {
            qDebug() << "error: " << action << " failed to: " << server << " (http error " << QString::number(httpResult) << "); QNetwork error = " << reply->error();
        }
    }
}

bool refreshAccessToken(QNetworkAccessManager* networkAccessManager, Project* project)
{
    auto connectorParams = project->connectorParams();
    auto server = connectorParams["server"].toString();

    auto accessToken = project->accessToken();
    auto refreshToken = project->refreshToken();

    if (!Utils::httpRefreshOAuthToken(networkAccessManager, server, EARTH_RANGER_MOBILE_CLIENT_ID, refreshToken, &accessToken, &refreshToken))
    {
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
        return QString(EARTH_RANGER_PROVIDER) + "_SaveSighting";
    }

    static Task* createInstance(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput)
    {
        auto task = new UploadSightingTask(nullptr);
        task->init(projectUid, uid, input, parentOutput, true);

        return task;
    }

    static void completed(TaskManager* taskManager, const QString& projectUid, const QString& uid, const QVariantMap& output, bool success)
    {
        if (success)
        {
            // 201_ERROR_HANDLING: succeeded but deemed unpatchable
            auto unpatchable = output.value("unpatchable").toBool();
            auto sightingUid = output.value("sightingUid").toString();

            if (unpatchable)
            {
                App::instance()->database()->setSightingFlags(projectUid, sightingUid, Sighting::DB_READONLY_FLAG);
            }
        }
        else
        {
            taskManager->pauseTask(projectUid, uid);
            handleTaskUploadError(output, projectUid);
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
        auto parentOutput = this->parentOutput();
        auto payloadJson = Utils::variantMapToJson(input["payload"].toMap());

        QNetworkAccessManager networkAccessManager;
        QNetworkRequest request;
        QEventLoop eventLoop;
        request.setRawHeader("Connection", "keep-alive");
        request.setRawHeader("Accept", "application/json, text/plain, */*");
        request.setRawHeader("Content-Type", "application/json; charset=utf-8");

        auto sendData = [&]() -> QVariantMap
        {
            std::unique_ptr<QNetworkReply> reply;

            auto accessToken = project->accessToken();
            auto server = connectorParams["server"].toString();

            request.setRawHeader("Authorization", QString("Bearer " + accessToken).toUtf8());

            // POST for a new sighting, PATCH for update of an existing sighting.
            if (parentOutput.isEmpty())
            {
                request.setUrl(server + "/api/v1.0/activity/events");
                reply.reset(networkAccessManager.post(request, payloadJson));
            }
            else
            {
                request.setUrl(parentOutput["sightingUrl"].toString());
                reply.reset(networkAccessManager.sendCustomRequest(request, "PATCH", payloadJson));
            }

            // Submit data.
            QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
            eventLoop.exec();

            auto output = QVariantMap();
            auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            output["status"] = status;

            if (reply->error() == QNetworkReply::NoError)
            {
                auto jsonObj = QJsonDocument::fromJson(reply->readAll()).object().value("data").toObject();
                output["eventId"] = jsonObj.value("id").toString();
                auto sightingUrl = jsonObj.value("url").toString();
                output["sightingUrl"] = sightingUrl; // For subsequnt tasks.
                output["unpatchable"] = isGoodButUnpatchableResult(sightingUrl, status);
                output["sightingUid"] = input.value("sightingUid");
            }

            logAnyErrors("sighting upload", server, reply.get());

            return output;
        };

        auto output = sendData();
        if (output["status"] == 401)
        {
            if (refreshAccessToken(&networkAccessManager, project.get()))
            {
                output = sendData();
            }
        }

        // The sightingUrl is the input for subsequent PATCH operations.
        update_output(output);

        return isSuccessResult(output["sightingUrl"].toString(), output["status"].toInt());
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
        return QString(EARTH_RANGER_PROVIDER) + "_UploadFile";
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
            handleTaskUploadError(output, projectUid);
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
        auto parentOutput = this->parentOutput();
        auto payload = input["payload"].toMap();

        auto sourceFilename = payload.value("sourceFilename").toString();
        if (sourceFilename.contains('/'))
        {
            // TODO(justin): remove after some time (4/13/2022).
            sourceFilename = QFileInfo(sourceFilename).fileName();
        }

        auto sourceFilePath = App::instance()->getMediaFilePath(sourceFilename);
        auto targetFilename = payload.value("targetFilename").toString();

        QHttpMultiPart multipart(QHttpMultiPart::FormDataType);
        multipart.setBoundary("------WebKitFormBoundarytoHka8LUGjq34sBN");
        auto part = QHttpPart();
        part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"filecontent.file\"; filename=\"" + targetFilename + "\"");
        QFile file(sourceFilePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            qDebug() << "Error - failed to read file: " + sourceFilePath;
            return false;
        }
        part.setBodyDevice(&file);
        multipart.append(part);

        QNetworkAccessManager networkAccessManager;
        QNetworkRequest request;
        QEventLoop eventLoop;
        request.setRawHeader("Connection", "keep-alive");
        request.setRawHeader("Accept", "application/json, text/plain, */*");
        request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + multipart.boundary());
        request.setUrl(Utils::trimUrl(parentOutput["sightingUrl"].toString()) + "/files/");

        auto sendData = [&]() -> QVariantMap
        {
            std::unique_ptr<QNetworkReply> reply;

            auto accessToken = project->accessToken();
            auto server = connectorParams["server"].toString();
            request.setRawHeader("Authorization", QString("Bearer " + accessToken).toUtf8());

            reply.reset(networkAccessManager.post(request, &multipart));

            QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
            eventLoop.exec();

            auto output = QVariantMap();
            auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            output["status"] = status;

            if (reply->error() == QNetworkReply::NoError)
            {
                auto jsonObj = QJsonDocument::fromJson(reply->readAll()).object().value("data").toObject();
                output["eventId"] = jsonObj.value("id").toString();
                auto sightingUrl = jsonObj.value("url").toString();
                output["sightingUrl"] = parentOutput["sightingUrl"]; // Chain for next task.
                output["unpatchable"] = isGoodButUnpatchableResult(sightingUrl, status);
                output["sightingUid"] = input.value("sightingUid");
            }

            logAnyErrors("file upload", server, reply.get());

            return output;
        };

        auto output = sendData();
        if (output["status"] == 401)
        {
            if (refreshAccessToken(&networkAccessManager, project.get()))
            {
                output = sendData();
            }
        }

        update_output(output);

        return isSuccessResult(output["sightingUrl"].toString(), output["status"].toInt());
    }
};

// UploadNoteTask.
class UploadNoteTask: public Task
{
public:
    UploadNoteTask(QObject* parent): Task(parent)
    {}

    static QString getName()
    {
        return QString(EARTH_RANGER_PROVIDER) + "_UploadNote";
    }

    static Task* createInstance(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput)
    {
        auto task = new UploadNoteTask(nullptr);
        task->init(projectUid, uid, input, parentOutput, true);

        return task;
    }

    static void completed(TaskManager* taskManager, const QString& projectUid, const QString& uid, const QVariantMap& output, bool success)
    {
        if (!success)
        {
            taskManager->pauseTask(projectUid, uid);
            handleTaskUploadError(output, projectUid);
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
        auto parentOutput = this->parentOutput();
        auto payload = input["payload"].toMap();

        QNetworkAccessManager networkAccessManager;
        QNetworkRequest request;
        QEventLoop eventLoop;
        request.setRawHeader("Connection", "keep-alive");
        request.setRawHeader("Accept", "application/json, text/plain, */*");
        request.setRawHeader("Content-Type", "application/json; charset=utf-8");

        auto sendData = [&]() -> QVariantMap
        {
            std::unique_ptr<QNetworkReply> reply;

            auto accessToken = project->accessToken();
            auto server = connectorParams["server"].toString();

            request.setRawHeader("Authorization", QString("Bearer " + accessToken).toUtf8());

            auto noteId = parentOutput.value("noteId").toString();
            if (noteId.isEmpty())
            {
                request.setUrl(parentOutput["sightingUrl"].toString() + "/notes/");
                reply.reset(networkAccessManager.post(request, Utils::variantMapToJson(payload)));
            }
            else
            {
                payload.insert("id", noteId);
                request.setUrl(parentOutput["sightingUrl"].toString() + "/note/" + noteId + "/");
                reply.reset(networkAccessManager.sendCustomRequest(request, "PATCH", Utils::variantMapToJson(payload)));
                noteId.clear();
            }

            QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
            eventLoop.exec();

            auto output = QVariantMap();
            auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            output["status"] = status;

            if (reply->error() == QNetworkReply::NoError)
            {
                auto jsonObj = QJsonDocument::fromJson(reply->readAll()).object().value("data").toObject();
                noteId = jsonObj.value("id").toString();
                output["noteId"] = noteId;
                output["unpatchable"] = isGoodButUnpatchableResult(noteId, status);
                output["sightingUid"] = input.value("sightingUid");
                output["sightingUrl"] = parentOutput.value("sightingUrl"); // Chain for next task.
            }

            logAnyErrors("note upload", server, reply.get());

            return output;
        };

        auto output = sendData();
        if (output["status"] == 401)
        {
            if (refreshAccessToken(&networkAccessManager, project.get()))
            {
                output = sendData();
            }
        }

        update_output(output);

        return isSuccessResult(output["noteId"].toString(), output["status"].toInt());
    }
};

// UploadCleanupTask.
class UploadCompleteTask: public Task
{
public:
    UploadCompleteTask(QObject* parent): Task(parent)
    {}

    static QString getName()
    {
        return QString(EARTH_RANGER_PROVIDER) + "_UploadComplete";
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
// EarthRangerProvider

EarthRangerProvider::EarthRangerProvider(QObject *parent) : Provider(parent)
{
    update_name(EARTH_RANGER_PROVIDER);
}

QString EarthRangerProvider::reportedBy()
{
    return m_project->reportedBy();
}

void EarthRangerProvider::setReportedBy(const QString& value)
{
    auto lastValue = reportedBy();
    if (value != lastValue)
    {
        m_project->set_reportedBy(value);
        emit reportedByChanged();
    }
}

void EarthRangerProvider::parseReportedBy()
{
    auto schema = Utils::variantMapFromJsonFile(getProjectFilePath("schema.json"));
    auto props = schema["properties"].toMap();

    auto reportedByElement = new Element(this);
    reportedByElement->set_uid(ELEMENT_REPORTED_BY);
    reportedByElement->set_name("Reported by");
    m_elementManager->appendElement(nullptr, reportedByElement);

    auto targetUsername = m_project->username().toLower();
    auto currReportedBy = reportedBy();

    auto reportedByEnum = props["reported_by"].toMap()["enum_ext"].toList();
    for (auto item: reportedByEnum)
    {
        auto itemMap = item.toMap();
        auto value = itemMap["value"].toMap();

        auto element = new Element(this);
        element->set_uid(value["id"].toString());
        element->set_name(itemMap["title"].toString());
        element->addTag("identity", value);
        m_elementManager->appendElement(reportedByElement, element);

        // Pick up the default by username.
        if (currReportedBy.isEmpty() && !targetUsername.isEmpty() && value.value("username").toString().toLower() == targetUsername)
        {
            currReportedBy = value["id"].toString();
            setReportedBy(currReportedBy);
        }
    }
}

void EarthRangerProvider::parseEventTypes()
{
    auto eventTypesUrl = getProjectFileUrl("eventtypes.json");
    QFile eventTypesFile(eventTypesUrl.toLocalFile());
    qFatalIf(!eventTypesFile.open(QFile::ReadOnly | QFile::Text), "Failed to read eventTypes.json from project folder");
    auto eventTypesJson = QJsonDocument::fromJson(eventTypesFile.readAll());
    qFatalIf(!eventTypesJson.isArray(), "Expected an array");

    auto rootElement = new Element(this);
    rootElement->set_uid("categories");
    rootElement->set_name("Categories");
    m_elementManager->appendElement(nullptr, rootElement);

    // Create Elements for each category.
    for (auto eventType: eventTypesJson.array())
    {
        auto eventTypeObj = eventType.toObject();
        auto categoryObj = eventTypeObj.value("category").toObject();
        auto categoryUid = categoryObj.value("value").toString();
        auto categoryName = categoryObj.value("display").toString();
        auto categoryFlag = categoryObj.value("flag").toString();
        auto permissions = categoryObj.value("permissions").toArray();

        if (categoryName == "HIDDEN")
        {
            continue;
        }

        if (categoryFlag == "system")
        {
            continue;
        }

        if (!permissions.contains("create"))
        {
            continue;
        }

        if (m_elementManager->getElement(categoryUid) == nullptr)
        {
            auto categoryElement = new Element(this);
            categoryElement->set_uid(categoryUid);
            categoryElement->set_name(categoryName);
            m_elementManager->appendElement(rootElement, categoryElement);
        }
    }

    // Location.
    auto locationElement = new Element(this);
    locationElement->set_uid(FIELD_LOCATION);
    m_elementManager->appendElement(nullptr, locationElement);

    auto locationField = new LocationField(this);
    locationField->set_uid(FIELD_LOCATION);
    locationField->set_nameElementUid(FIELD_LOCATION);
    locationField->set_required(true);
    locationField->set_fixCount(5);
    locationField->set_relevant("LOCATION_VISIBLE");
    m_fieldManager->appendField(nullptr, locationField);

    // Photos.
    auto photosElement = new Element(this);
    photosElement->set_uid(FIELD_PHOTOS);
    m_elementManager->appendElement(nullptr, photosElement);

    auto photosField = new PhotoField(this);
    photosField->set_uid(FIELD_PHOTOS);
    photosField->set_nameElementUid(FIELD_PHOTOS);
    photosField->set_maxCount(16);
    m_fieldManager->appendField(nullptr, photosField);

    // Notes.
    auto notesElement = new Element(this);
    notesElement->set_uid(FIELD_NOTES);
    m_elementManager->appendElement(nullptr, notesElement);

    auto notesField = new StringField(this);
    notesField->set_uid(FIELD_NOTES);
    notesField->set_nameElementUid(FIELD_NOTES);
    notesField->set_multiLine(true);
    m_fieldManager->appendField(nullptr, notesField);

    // Create a synthetic field which represents the report uid.
    auto rootField = new StringField(this);
    rootField->set_uid("reportUid");
    rootField->set_nameElementUid("categories");
    rootField->set_listElementUid("categories");
    m_fieldManager->appendField(nullptr, rootField);

    // Populate each category with reports.
    for (auto eventType: eventTypesJson.array())
    {
        auto eventTypeObj = eventType.toObject();
        auto categoryObj = eventTypeObj.value("category").toObject();
        auto categoryUid = categoryObj.value("value").toString();

        auto categoryElement = m_elementManager->getElement(categoryUid);
        if (categoryElement == nullptr)
        {
            continue;
        }

        auto reportElement = new Element(this);
        reportElement->set_uid(eventTypeObj.value("value").toString());
        reportElement->set_name(eventTypeObj.value("display").toString());
        reportElement->set_icon(eventTypeObj.value("icon_id").toString() + ".svg");
        m_elementManager->appendElement(categoryElement, reportElement);

        parseReport(reportElement);
    }
}

bool EarthRangerProvider::parseField(const QString& reportUid, RecordField* recordField, const QJsonObject& jsonField, QString* keyOut)
{
    auto result = QString();
    auto exportKey = jsonField.value("key").toString();
    auto key = reportUid + '.' + exportKey;

    auto type = jsonField.value("type").toString();
    auto fieldHtmlClass = jsonField.value("fieldHtmlClass").toString();
    auto format = jsonField.value("format").toString();
    auto title = jsonField.value("title").toString();

    // Create the field.
    BaseField* field = nullptr;
    if (format == "date")
    {
        auto dateField = new DateField(this);
        field = dateField;
    }
    else if (fieldHtmlClass.contains("date-time-picker"))
    {
        auto dateTimeField = new DateTimeField(this);
        field = dateTimeField;
    }
    else if (type == "string")
    {
        auto stringField = new StringField(this);
        stringField->set_multiLine(true);
        field = stringField;
    }
    else if (type == "number" || type == "integer")
    {
        auto numberField = new NumberField(this);

        auto restriction = jsonField.value("minimum");
        if (!restriction.isUndefined())
        {
            numberField->set_minValue(restriction.toInt());
        }

        restriction = jsonField.value("maximum");
        if (!restriction.isUndefined())
        {
            numberField->set_maxValue(restriction.toInt());
        }

        numberField->set_decimals(type == "integer" ? 0 : 4);

        field = numberField;
    }
    else if (type == "boolean" || type == "checkboxes")
    {
        auto checkField = new CheckField(this);
        field = checkField;
    }
    else if (type == "array")
    {
        auto jsonField2 = jsonField.value("items").toObject();
        if (jsonField2.isEmpty())
        {
            qDebug() << "No items for array type";
            return false;
        }

        if (jsonField2.value("type") != "object")
        {
            qDebug() << "Array type is not an object";
            return false;
        }

        auto props2 = jsonField2.value("properties").toObject();
        if (props2.isEmpty())
        {
            qDebug() << "No fields in child record";
            return false;
        }

        title = jsonField.value("title").toString();

        // Hack because JSON schema uses an object instead of an array, so the order is lost by the JSON object map.
        auto orderedKeys = QStringList(props2.keys());
        {
            auto reportPath = getProjectFilePath(reportUid + ".json");
            QFile reportFile(reportPath);
            qFatalIf(!reportFile.open(QFile::ReadOnly | QFile::Text), "Failed to read report json");

            auto reportData = QString(reportFile.readAll());
            reportData.replace(' ', "");
            reportData.replace('\n', "");
            reportData.replace('\r', "");
            reportData.replace('\t', "");

            std::sort(orderedKeys.begin(), orderedKeys.end(), [&](const QString& k1, const QString& k2) -> bool
            {
                auto offset1 = reportData.indexOf('"' + k1 + "\":{");
                auto offset2 = reportData.indexOf('"' + k2 + "\":{");
                return offset1 < offset2;
            });
        }

        auto recordField2 = new RecordField(this);
        for (auto key2: orderedKeys)
        {
            jsonField2 = props2.value(key2).toObject();
            jsonField2.insert("key", key2);
            auto key2temp = QString();
            parseField(key, recordField2, jsonField2, &key2temp);
        }

        field = recordField2;
    }
    else
    {
        qDebug() << "Failed to find type " << type << " for " << key << " 3";
        return false;
    }

    // Create the Element for the field title.
    auto nameElement = new Element(this);
    nameElement->set_uid(key);
    nameElement->set_name(title);
    m_elementManager->appendElement(nullptr, nameElement);

    // Create list elements.
    auto listItems = jsonField.value("enumNames");
    if (listItems.isObject())
    {
        auto listArray = jsonField.value("enum").toVariant().toStringList();
        auto listObject = listItems.toObject();

        for (auto listItem: listArray)
        {
            auto listElement = new Element(this);
            listElement->set_uid("id_" + QVariant(m_runningUid++).toString());

            auto name = listObject.value(listItem).toString();
            listElement->set_name(name);
            listElement->set_exportUid(listItem);

            m_elementManager->appendElement(nameElement, listElement);
        }
    }
    else if (jsonField.value("titleMap").isArray())
    {
        auto titleMapArray = jsonField.value("titleMap").toArray();
        for (auto titleItem: titleMapArray)
        {
            auto listElement = new Element(this);
            listElement->set_uid("id_" + QVariant(m_runningUid++).toString());

            auto titleItemMap = titleItem.toObject();
            listElement->set_name(titleItemMap.value("name").toString());
            listElement->set_exportUid(titleItemMap.value("value").toString());

            m_elementManager->appendElement(nameElement, listElement);
        }
    }

    if (nameElement->elementCount() > 0)
    {
        field->set_listElementUid(key);
    }

    // Append the field.
    field->set_uid(key);
    field->set_exportUid(exportKey);
    field->set_nameElementUid(key);
    recordField->appendField(field);

    *keyOut = key;

    return true;
}

QJsonObject EarthRangerProvider::resolveForm(const QJsonValue& form, const QJsonObject& props)
{
    auto formJson = QJsonObject();
    if (form.isString())
    {
        auto key = form.toString();
        formJson = props.value(key).toObject();
        formJson["key"] = key;
    }
    else if (form.isObject())
    {
        auto formObj = form.toObject();

        // Start with key.
        auto key = formObj.value("key").toString();
        auto type = QString();

        if (props.contains(key))
        {
            formJson = props.value(key).toObject();
            type = formJson.value("type").toString();
        }

        // Overlay with type.
        auto ftype = formObj.value("type").toString();
        if (props.contains(ftype))
        {
            auto propsObj = props.value(ftype).toObject();
            for (auto p: propsObj.keys())
            {
                formJson.insert(p, propsObj.value(p));
            }
            type = formJson.value("type").toString();
        }

        // Overlay with form.
        for (auto f: formObj.keys())
        {
            formJson.insert(f, formObj.value(f));
        }

        // Get the final type.
        if (type.isEmpty())
        {
            type = formObj.value("type").toString();
        }

        formJson.insert("type", type);
    }
    else
    {
        qDebug() << "Unknown form type";
    }

    // Validate.
    {
        auto key = formJson.value("key").toString();
        auto success = !key.isEmpty();
        if (!success)
        {
            qDebug() << "Failed to find key " << key;
        }

        auto type = formJson.value("type").toString();
        success = success && !type.isEmpty();
        if (!success)
        {
            qDebug() << "Failed to find type " << type << " for " << key << " 1";
        }

        auto validTypes = QStringList { "string", "integer", "boolean", "number", "object", "checkboxes", "array" };
        success = success && (validTypes.indexOf(type) != -1);
        if (!success)
        {
            qDebug() << "Failed to find type " << type << " for " << key << " 2";
            formJson.empty();
        }
    }

    return formJson;
}

void EarthRangerProvider::parseReport(Element* reportElement)
{
    auto reportPath = getProjectFilePath(reportElement->uid() + ".json");
    QFile reportFile(reportPath);
    qFatalIf(!reportFile.open(QFile::ReadOnly | QFile::Text), "Failed to read eventTypes.json from project folder");

    auto reportJson = QJsonDocument::fromJson(reportFile.readAll());
    qFatalIf(!reportJson.isObject(), "Expected an object");

    auto forms = reportJson.object().value("definition").toArray();
    auto props = reportJson.object().value("schema").toObject().value("properties").toObject();

    reportElement->appendFieldUid(FIELD_PHOTOS);
    reportElement->appendFieldUid(FIELD_LOCATION);

    auto processedKeys = QVariantMap();

    for (auto form: forms)
    {
        auto schemaItem = resolveForm(form, props);
        if (schemaItem.isEmpty())
        {
            continue;
        }

        auto schemaKey = schemaItem.value("key").toString();
        if (processedKeys.contains(schemaKey))
        {
            continue;
        }
        processedKeys.insert(schemaKey, true);

        auto key = QString();
        if (!parseField(reportElement->uid(), m_fieldManager->rootField(), schemaItem, &key))
        {
            continue;    
        }

        reportElement->appendFieldUid(key);
    }

    reportElement->appendFieldUid(FIELD_NOTES);

    m_fieldManager->completeUpdate();
}

bool EarthRangerProvider::connectToProject(bool newBuild)
{
    m_taskManager->registerTask(UploadSightingTask::getName(), UploadSightingTask::createInstance, UploadSightingTask::completed);
    m_taskManager->registerTask(UploadFileTask::getName(), UploadFileTask::createInstance, UploadFileTask::completed);
    m_taskManager->registerTask(UploadNoteTask::getName(), UploadNoteTask::createInstance, UploadNoteTask::completed);
    m_taskManager->registerTask(UploadCompleteTask::getName(), UploadCompleteTask::createInstance, UploadCompleteTask::completed);

    auto form = qobject_cast<Form*>(parent());
    connect(form, &Form::sightingSaved, this, [=](const QString& sightingUid)
    {
        sendSighting(form, sightingUid);
    });

    auto elementsFilePath = getProjectFilePath("Elements.qml");
    auto fieldsFilePath = getProjectFilePath("Fields.qml");

    if (CACHE_ENABLE && !newBuild && QFile::exists(elementsFilePath) && QFile::exists(fieldsFilePath) && !reportedBy().isEmpty())
    {
        return true;
    }

    // Parse the JSON files.
    m_runningUid = 0;
    parseReportedBy();
    parseEventTypes();

    // Commit the Elements and Fields.
    m_elementManager->saveToQmlFile(elementsFilePath);
    m_fieldManager->saveToQmlFile(fieldsFilePath);

    return true;
}

QUrl EarthRangerProvider::getStartPage()
{
    return QUrl("qrc:/EarthRanger/StartPage.qml");
}

void EarthRangerProvider::getElements(ElementManager* elementManager)
{
    elementManager->loadFromQmlFile(getProjectFilePath("Elements.qml"));
}

void EarthRangerProvider::getFields(FieldManager* fieldManager)
{
    fieldManager->loadFromQmlFile(getProjectFilePath("Fields.qml"));
}

bool EarthRangerProvider::sendSighting(Form* form, const QString& sightingUid)
{
    auto projectUid = m_project->uid();
    auto sighting = form->createSightingPtr(sightingUid);

    auto reportUid = sighting->getRecord(sightingUid)->getFieldValue("reportUid").toString();
    auto element = m_elementManager->getElement(reportUid);
    auto fieldUids = element->fieldUids();

    // Append non-record fields.
    auto appendFlatFieldValue = [&](QVariantMap& currDetail, BaseField* field, const QVariant& fieldValue)
    {
        if (field->isRecordField())
        {
            return;
        }

        // Skip values that were not entered.
        if (!fieldValue.isValid())
        {
            return;
        }

        auto value = field->save(fieldValue);

        // Fixup the list elements with their export ids.
        // TODO(justin): should the infrastructure handle this?
        if (!field->listElementUid().isEmpty())
        {
            auto remapToExportUid = [=](const QString& input) -> QString
            {
                auto element = m_elementManager->getElement(input);
                return element ? element->exportUid() : input;
            };

            if (!value.toList().isEmpty())
            {
                auto valueArr = value.toList();
                for (auto i = 0; i < valueArr.count(); i++)
                {
                    valueArr[i] = remapToExportUid(valueArr[i].toString());
                }
                value = valueArr;
            }
            else if (!value.toString().isEmpty())
            {
                value = remapToExportUid(value.toString());
            }
        }

        currDetail.insert(field->exportUid(), value);
    };

    auto detail = QVariantMap();
    for (auto fieldIt = fieldUids.constBegin(); fieldIt != fieldUids.constEnd(); fieldIt++)
    {
        auto fieldUid = *fieldIt;

        // Skip location, photos and notes.
        if (fieldUid == FIELD_LOCATION || fieldUid == FIELD_PHOTOS || fieldUid == FIELD_NOTES)
        {
            continue;
        }

        auto field = m_fieldManager->getField(fieldUid);
        auto recordField = RecordField::fromField(field);

        // Add flat fields.
        if (!recordField)
        {
            appendFlatFieldValue(detail, field, sighting->getRecord(sightingUid)->getFieldValue(fieldUid));
            continue;
        }

        // Add records.
        auto recordUids = sighting->getRecord(sightingUid)->getFieldValue(fieldUid).toStringList();
        auto recordArray = QVariantList();
        for (auto recordIt = recordUids.constBegin(); recordIt != recordUids.constEnd(); recordIt++)
        {
            auto recordUid = *recordIt;
            auto childDetail = QVariantMap();

            sighting->getRecord(recordUid)->enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
            {
                appendFlatFieldValue(childDetail, fieldValue.field(), fieldValue.value());
            });

            recordArray.append(childDetail);
        }

        if (recordArray.count() > 0)
        {
            detail.insert(field->exportUid(), recordArray);
        }
    }

    auto location = QVariantMap();
    auto locationValue = sighting->getRecord(sightingUid)->getFieldValue(FIELD_LOCATION).toMap();
    location["latitude"] = locationValue.value("y").toDouble();
    location["longitude"] = locationValue.value("x").toDouble();

    auto event = QVariantMap();
    event["event_type"] = reportUid;
    event["event_details"] = detail;
    event["time"] = sighting->createTime();
    event["location"] = location;

    auto reportedByElement = m_elementManager->getElement(reportedBy());
    if (reportedByElement)
    {
        event["reported_by"] = reportedByElement->getTagValue("identity").toMap();
    }
    else
    {
        qDebug() << "warning: sighting reported_by not found (" << reportedBy() << ")";
    }

    auto inputJson = QJsonObject();
    inputJson.insert("payload", QJsonObject::fromVariantMap(event));
    inputJson.insert("sightingUid", sightingUid);

    // Create the task for the main sighting.
    auto sightingTaskUid = QString();
    auto lastTaskUid = QString();
    {
        auto baseUid = Task::makeUid(UploadSightingTask::getName(), sightingUid);
        auto parentUids = QStringList();
        m_taskManager->getTasks(projectUid, baseUid, -1, &parentUids);
        auto parentUid = !parentUids.isEmpty() ? parentUids.last(): QString();

        sightingTaskUid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
        m_taskManager->addTask(projectUid, sightingTaskUid, parentUid, inputJson.toVariantMap());
        lastTaskUid = sightingTaskUid;
    }

    // Create the tasks for the photos (1 per photo).
    // Note we try not to send the same filename more than once.
    auto photos = sighting->getRecord(sightingUid)->getFieldValue(FIELD_PHOTOS).toStringList();
    photos.removeAll(QString(""));
    for (auto i = 0; i < photos.length(); i++)
    {
        auto filename = photos[i];
        auto filePath = App::instance()->getMediaFilePath(filename);
        auto fileInfo = QFileInfo(filePath);
        if (!fileInfo.exists())
        {
            qDebug() << "Error - file missing: " + filename;
            continue;
        }

        event.clear();
        event["sourceFilename"] = filename;
        event["targetFilename"] = "Photo" + QString::number(i + 1) + "." + fileInfo.suffix();
        inputJson.insert("payload", QJsonObject::fromVariantMap(event));

        auto baseUid = Task::makeUid(UploadFileTask::getName(), sightingUid) + '.' + fileInfo.fileName();
        auto photoTaskUid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());

        auto alreadySentTaskUids = QStringList();
        m_taskManager->getTasks(projectUid, baseUid, -1, &alreadySentTaskUids);
        if (alreadySentTaskUids.isEmpty())
        {
            m_taskManager->addTask(projectUid, photoTaskUid, lastTaskUid, inputJson.toVariantMap());
            lastTaskUid = photoTaskUid;
        }
    }

    // Create the task for the notes.
    {
        auto baseUid = Task::makeUid(UploadNoteTask::getName(), sightingUid);
        auto parentUids = QStringList();
        m_taskManager->getTasks(projectUid, baseUid, -1, &parentUids);
        auto parentUid = !parentUids.isEmpty() ? parentUids.last(): sightingTaskUid;

        auto note = sighting->getRecord(sightingUid)->getFieldValue(FIELD_NOTES).toString().trimmed();

        // Send if this sighting already had a note or the note is not empty.
        if (!note.isEmpty() || !parentUids.isEmpty())
        {
            // Server does not understand empty notes.
            if (note.isEmpty())
            {
                note = "[removed]";
            }

            event.clear();
            event["text"] = note;
            inputJson.insert("payload", QJsonObject::fromVariantMap(event));
            auto noteTaskUid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());
            m_taskManager->addTask(projectUid, noteTaskUid, lastTaskUid, inputJson.toVariantMap());
            lastTaskUid = noteTaskUid;
        }
    }

    // Create the completion task.
    m_taskManager->addTask(projectUid, Task::makeUid(UploadCompleteTask::getName(), sightingUid), lastTaskUid, QVariantMap { { "sightingUid", sightingUid } });

    return true;
}

bool EarthRangerProvider::notify(const QVariantMap& message)
{
    if (message["AuthComplete"].toBool())
    {
        m_taskManager->resumePausedTasks(m_project->uid());
    }

    return true;
}

bool EarthRangerProvider::finalizePackage(const QString& packageFilesPath) const
{
    // Cleanup cached files.
    QFile::remove(packageFilesPath + "/Elements.qml");
    QFile::remove(packageFilesPath + "/Fields.qml");

    return true;
}

QString EarthRangerProvider::getFieldName(const QString& fieldUid)
{
    static auto map = QVariantMap
    {
        { FIELD_LOCATION, tr("Location") },
        { FIELD_PHOTOS,   tr("Photos")   },
        { FIELD_NOTES,    tr("Notes")    },
    };

    return map.value(fieldUid).toString();
}

bool EarthRangerProvider::getFieldTitleVisible(const QString& fieldUid)
{
    return fieldUid != FIELD_PHOTOS;
}

