#include "App.h"
#include "SMARTProvider.h"
#include "Form.h"

constexpr bool CACHE_ENABLE                = true;
constexpr bool COMPRESS_UPLOAD             = false;
constexpr bool COMPACT_JSON                = false;

constexpr char TYPE_INCIDENT[]             = "Incident";
constexpr char TYPE_PATROL[]               = "Patrol";
constexpr char TYPE_COLLECT[]              = "Collect";

constexpr char FIELD_MODEL[]               = "IncidentRecord";
constexpr char FIELD_MODEL_TREE[]          = "ModelTree";
constexpr char FIELD_GROUPS[]              = "GroupList";
constexpr char FIELD_LOCATION[]            = "Location";
constexpr char FIELD_DISTANCE[]            = "Distance";
constexpr char FIELD_BEARING[]             = "Bearing";
constexpr char FIELD_PHOTOS[]              = "Photos";
constexpr char FIELD_PHOTOS_REQUIRED[]     = "PhotosRequired";
constexpr char FIELD_AUDIO[]               = "Audio";

constexpr char FIELD_DATA_TYPE[]           = "SMART_DataType";
constexpr char FIELD_SIGHTING_GROUP_ID[]   = "SMART_SightingGroupId";
constexpr char FIELD_OBSERVATION_TYPE[]    = "SMART_ObservationType";
constexpr char FIELD_PATROL_ID[]           = "SMART_PatrolID";
constexpr char FIELD_PATROL_START_DATE[]   = "SMART_PatrolStartDate";
constexpr char FIELD_PATROL_START_TIME[]   = "SMART_PatrolStartTime";
constexpr char FIELD_PATROL_LEG_COUNTER[]  = "SMART_PatrolLegCounter";
constexpr char FIELD_PATROL_LEG_DISTANCE[] = "SMART_PatrolLegDistance";
constexpr char FIELD_OBS_COUNTER[]         = "SMART_ObsCounter";
constexpr char FIELD_NEW_WAYPOINT[]        = "SMART_NewWaypoint";
constexpr char FIELD_COLLECT_USER[]        = "SMART_CollectUser";
constexpr char FIELD_SAMPLING_UNIT[]       = "SMART_SamplingUnit";
constexpr char FIELD_PATROL_TRANSPORT[]    = "SMART_PatrolTransport";
constexpr char FIELD_OBSERVER[]            = "SMART_Observer";
constexpr char FIELD_PHOTO_PREFIX[]        = "SMART_Photo";
constexpr char FIELD_AUDIO_PREFIX[]        = "SMART_Audio";

constexpr char VALUE_NEW_PATROL[]          = "NewPatrol";
constexpr char VALUE_STOP_PATROL[]         = "StopPatrol";
constexpr char VALUE_PAUSE_PATROL[]        = "PausePatrol";
constexpr char VALUE_RESUME_PATROL[]       = "ResumePatrol";
constexpr char VALUE_CHANGE_PATROL[]       = "ChangePatrol";
constexpr char VALUE_OBSERVATION[]         = "Observation";

constexpr int DEFAULT_PHOTO_RESOLUTION_X   = 1600;
constexpr int DEFAULT_PHOTO_RESOLUTION_Y   = 1200;

constexpr QJsonDocument::JsonFormat JSON_FORMAT = COMPACT_JSON ? QJsonDocument::Compact : QJsonDocument::Indented;

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
        return QString(SMART_PROVIDER) + "_SaveSighting";
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
            auto sightingUids = output.value("sightingUids").toStringList();
            for (auto sightingIt = sightingUids.constBegin(); sightingIt != sightingUids.constEnd(); sightingIt++)
            {
                auto sightingUid = *sightingIt;
                App::instance()->database()->setSightingFlags(projectUid, sightingUid, Sighting::DB_UPLOADED_FLAG);

                emit App::instance()->sightingModified(projectUid, sightingUid, output["stateSpace"].toString());
            }
        }
        else
        {
            taskManager->pauseTask(projectUid, uid);
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

        // Prepare work.
        auto input = this->input();
        auto connector = input["DATA_SERVER"].toMap();
        auto payload = Utils::variantMapToJson(input["payload"].toMap(), JSON_FORMAT);
        auto usePUT = input["usePUT"].toBool();

        QNetworkAccessManager networkAccessManager;
        auto request = QNetworkRequest();
        request.setRawHeader("Connection", "keep-alive");
        request.setRawHeader("Accept", "application/json, text/plain, */*");

        auto serverUrl = connector["URL"].toString();
        auto apiKey = connector["API_KEY"].toString();
        auto username = connector["USERNAME"].toString();
        auto password = connector["PASSWORD"].toString();

        if (!username.isEmpty() && !password.isEmpty())
        {
            auto concatenated = username + ":" + password;
            auto data = concatenated.toLocal8Bit().toBase64();
            auto headerData = QString("Basic " + data).toLocal8Bit();
            request.setRawHeader("Authorization", headerData);
        }

        if (!apiKey.isEmpty())
        {
            serverUrl.append("?api_key=" + apiKey);
        }

        request.setRawHeader("Content-Type", "application/json; charset=utf-8");
        request.setUrl(serverUrl);

        auto compressedPayload = QByteArray();
        auto deflate = COMPRESS_UPLOAD && connector["PROTOCOL"].toString() == "GEOJSON_COMPRESSED";
        if (deflate)
        {
            request.setRawHeader("Content-Encoding", "deflate");
            compressedPayload = qCompress(payload);
            compressedPayload.remove(0, 4);    // Remove the first 4 bytes which are just a header.
        }

        auto reply = std::unique_ptr<QNetworkReply>();
        if (usePUT)
        {
            reply.reset(networkAccessManager.put(request, deflate ? compressedPayload : payload));
        }
        else
        {
            reply.reset(networkAccessManager.post(request, deflate ? compressedPayload : payload));
        }

        reply->ignoreSslErrors();

        QEventLoop eventLoop;
        QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();

        auto output = QVariantMap();
        output["errorCode"] = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        output["stateSpace"] = input.value("stateSpace");
        output["sightingUids"] = input.value("sightingUids");
        update_output(output);

        return reply->error() == QNetworkReply::NoError;
    }
};
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Provider

SMARTProvider::SMARTProvider(QObject *parent) : Provider(parent)
{
    update_name(SMART_PROVIDER);
    m_uploadFrequencyMinutes = -1;

#if defined(Q_OS_ANDROID)
    m_smartDataPath = Utils::androidGetExternalFilesDir() + "/SMARTdata";
    qFatalIf(!Utils::ensurePath(m_smartDataPath), "Failed to create SMARTdata folder");
#elif defined(Q_OS_IOS)
    m_smartDataPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DocumentsLocation);
    qFatalIf(!Utils::ensurePath(m_smartDataPath), "Failed to create SMARTdata folder");
#else
    m_smartDataPath = App::instance()->exportPath();
#endif
}

QVariantList SMARTProvider::buildConfigModel()
{
    auto boolToValue = [&](QVariant value) -> QString
    {
        return value.toBool() ? tr("Yes") : tr("No");
    };

    auto type = tr("None");
    if (m_hasPatrol && m_surveyMode)
    {
        type = tr("Survey");
    }
    else if (m_hasPatrol)
    {
        type = tr("Patrol");
    }
    else if (m_hasCollect)
    {
        type = tr("Collect");
    }

    auto upload = tr("Cable");
    if (m_hasDataServer)
    {
        if (m_manualUpload)
        {
            upload = tr("On export");
        }
        else
        {
            upload = m_uploadFrequencyMinutes == 0 ? tr("Immediate") : QString("%1 %2").arg(QString::number(m_uploadFrequencyMinutes), tr("minutes"));
        }
    }

    auto trackInterval = m_profile.value("WAYPOINT_TIMER", 0).toInt();
    auto trackTimer = tr("Off");
    if (trackInterval != 0)
    {
        if (m_profile.value("WAYPOINT_TIMER_TYPE", "") == "DISTANCE")
        {
            trackTimer = App::instance()->getDistanceText(trackInterval);
        }
        else
        {
            trackTimer = QString("%1 %2").arg(QString::number(trackInterval), tr("seconds"));
        }
    }

    auto resizeImage = !m_profile.value("RESIZE_IMAGE").toBool() ?
        QString("%1 x %2").arg(DEFAULT_PHOTO_RESOLUTION_X).arg(DEFAULT_PHOTO_RESOLUTION_Y) :
        QString("%1 x %2").arg(m_profile.value("IMAGE_WIDTH").toInt()).arg(m_profile.value("IMAGE_HEIGHT").toInt());

    auto skipButtonTimeout = m_profile.value("SKIP_BUTTON_TIMEOUT", 3).toInt();

    struct NV
    {
        QString name, value = QString();
    };

    NV model[] =
    {
        { tr("Type"), type },
        { tr("Incidents"), boolToValue(m_hasIncident) },
        { tr("Connection"), m_hasDataServer ? tr("Online") : tr("Desktop") },
        { tr("Upload"), upload },
        { "-" },
        { tr("Observer"), boolToValue(m_profile.value("RECORD_OBSERVER")) },
        { tr("Distance and Bearing"), boolToValue(m_profile.value("RECORD_DISTANCE_BEARING")) },
        { "-" },
        { tr("Incident group UI"), boolToValue(m_profile.value("INCIDENT_GROUP_UI")) },
        { tr("Kiosk mode"), boolToValue(m_profile.value("KIOSK_MODE")) },
        { tr("Can pause"), boolToValue(m_profile.value("CAN_PAUSE")) },
        { tr("Disable editing"), boolToValue(m_profile.value("DISABLE_EDITING")) },
        { "-" },
        { tr("Fix count"), m_profile.value("SIGHTING_FIX_COUNT", 0).toString() },
        { tr("Track timer"), trackTimer },
        { tr("Use time from GPS"), boolToValue(getUseGPSTime()) },
        { tr("Skip button timeout"), skipButtonTimeout == 0 ? tr("Off") : QString("%1 %2").arg(QString::number(skipButtonTimeout), tr("seconds")) },
        { tr("Allow manual GPS"), boolToValue(m_allowManualGPS) },
        { "-" },
        { tr("Maximum photos"), m_profile.value("MAX_PHOTO_COUNT", 3).toString() },
        { tr("Resize photos"), resizeImage },
        { "-" },
        { tr("Position updates"), boolToValue(m_profile["POSITION_UPDATES"].toMap()["FREQUENCY_MIN"].toInt() != 0) },
        { tr("Alerts"), boolToValue(m_profile.contains("ALERTS")) },
    };

    auto result = QVariantList();
    for (auto nv: model)
    {
        result.append(QVariantMap {{ "name", nv.name }, { "value", nv.value }});
    }

    return result;
}

void SMARTProvider::savePatrolSighting(const QString& observationType, bool saveTrackPoint)
{
    auto form = qobject_cast<Form*>(parent());
    auto sighting = form->createSightingPtr();
    sighting->regenerateUids();

    auto rootRecord = sighting->getRecord(sighting->rootRecordUid());
    rootRecord->resetFieldValue(FIELD_MODEL);
    rootRecord->resetFieldValue(FIELD_GROUPS);
    rootRecord->setFieldValue(FIELD_OBSERVATION_TYPE, observationType);

    form->saveSighting(sighting.get(), saveTrackPoint);

    form->resetFieldValue(FIELD_LOCATION);
    form->resetFieldValue(FIELD_DISTANCE);
    form->resetFieldValue(FIELD_BEARING);
}

void SMARTProvider::startPatrol()
{
    qDebug() << "Patrol start: " << QDateTime::currentDateTime();

    // Flush the last patrol if not using Connect.
    // In the Connect case, everything goes through the "Export data" button.
    if (!m_hasDataServer)
    {
        clearCompletedData();
    }

    auto form = qobject_cast<Form*>(parent());
    qFatalIf(form->readonly(), "Form is readonly");
    qFatalIf(form->editing(), "Form is editing");
    qFatalIf(!m_hasPatrol, "Patrol not available");
    qFatalIf(!m_patrolForm, "Not a patrol form");
    qFatalIf(m_patrolStarted, "Patrol already started");
    qFatalIf(m_patrolPaused, "Patrol should not be paused");
    qFatalIf(!m_patrolUid.isEmpty(), "Patrol uid must be empty to start a patrol");

    auto dataType = form->getFieldValue(FIELD_DATA_TYPE);
    qFatalIf(dataType != "patrol" && dataType != "survey", "Bad data type for patrol");

    // Setup the patrol metadata.
    m_patrolStarted = true;

    form->setFieldValue(FIELD_OBSERVATION_TYPE, VALUE_OBSERVATION);
    m_patrolUid = QUuid::createUuid().toString(QUuid::Id128);
    form->setFieldValue(FIELD_PATROL_ID, m_patrolUid);

    auto currentTime = App::instance()->timeManager()->currentDateTime();
    m_patrolStartTimestamp = App::instance()->timeManager()->currentDateTimeISO();
    form->setFieldValue(FIELD_PATROL_START_DATE, currentTime.toString("yyyy/MM/dd"));
    form->setFieldValue(FIELD_PATROL_START_TIME, currentTime.toString("HH:mm:ss"));

    // Setup the leg stats.
    update_patrolLegs(QVariantList { QVariantMap {{ "startTime", m_patrolStartTimestamp }} });
    form->setFieldValue(FIELD_PATROL_LEG_COUNTER, 1);
    form->setFieldValue(FIELD_PATROL_LEG_DISTANCE, 0.0);

    // Reset the upload state.
    m_uploadObsCounter = 1;
    m_uploadSightingLastTaskUid = "";

    // Initialize the ping uid.
    m_pingUuid = QUuid::createUuid().toString(QUuid::Id128);

    // Start the track.
    startTrack(0, 0);

    // Create a patrol state change sighting.
    // Note: do this after starting the track so we get a corresponding location sighting.
    savePatrolSighting(VALUE_NEW_PATROL, true);
    saveState();

    emit patrolStartedChanged();

    qDebug() << "Patrol start success: " << QDateTime::currentDateTime();
}

void SMARTProvider::stopPatrol()
{
    qDebug() << "Patrol stop: " << QDateTime::currentDateTime();

    auto form = qobject_cast<Form*>(parent());
    qFatalIf(form->readonly(), "Form is readonly");
    qFatalIf(form->editing(), "Form is editing");
    qFatalIf(!m_hasPatrol, "Patrol not available");
    qFatalIf(!m_patrolForm, "Not a patrol form");
    qFatalIf(!m_patrolStarted, "Patrol not started");
    qFatalIf(m_patrolPaused, "Patrol should not be paused");

    // Create a patrol state change sighting.
    // Note: do this before stopping the track so we get a corresponding location sighting.
    savePatrolSighting(VALUE_STOP_PATROL, false);

    // Stop the track timer.
    stopTrack();

    // Stop the patrol and commit the form state.
    m_patrolStarted = false;
    m_patrolStartTimestamp.clear();
    m_patrolUid.clear();
    form->newSighting();

    // Reset the upload state.
    m_uploadObsCounter = 1;
    m_uploadSightingLastTaskUid = "";

    // Finalize the leg.
    setPatrolLegValue("stopTime", App::instance()->timeManager()->currentDateTimeISO());

    saveState();

    emit patrolStartedChanged();

    qDebug() << "Patrol stop success: " << QDateTime::currentDateTime();
}

void SMARTProvider::pausePatrol()
{
    qDebug() << "Patrol pause: " << QDateTime::currentDateTime();

    auto form = qobject_cast<Form*>(parent());
    qFatalIf(form->readonly(), "Form is readonly");
    qFatalIf(form->editing(), "Form is editing");
    qFatalIf(!m_hasPatrol, "Patrol not available");
    qFatalIf(!m_patrolForm, "Not a patrol form");
    qFatalIf(!m_patrolStarted, "Patrol not started");
    qFatalIf(m_patrolPaused, "Patrol should not be paused");

    // Create a patrol state change sighting.
    // Note: do this before stopping the track so we get a corresponding location sighting.
    savePatrolSighting(VALUE_PAUSE_PATROL, false);

    // Store the distance to be resumed and pause the track.
    m_patrolPausedDistance = form->trackStreamer()->distanceCovered();
    stopTrack();
    m_patrolPaused = true;

    // Commit the form state.
    saveState();

    emit patrolPausedChanged();

    qDebug() << "Patrol pause success: " << QDateTime::currentDateTime();
}

void SMARTProvider::resumePatrol()
{
    qDebug() << "Patrol resume: " << QDateTime::currentDateTime();

    auto form = qobject_cast<Form*>(parent());
    qFatalIf(form->readonly(), "Form is readonly");
    qFatalIf(form->editing(), "Form is editing");
    qFatalIf(!m_hasPatrol, "Patrol not available");
    qFatalIf(!m_patrolForm, "Not a patrol form");
    qFatalIf(!m_patrolStarted, "resumePatrol: patrol not started");
    qFatalIf(!m_patrolPaused, "resumePatrol: patrol not paused");

    // Restart the track timer.
    startTrack(m_patrolPausedDistance, 0);

    // Create a patrol state change sighting.
    // Note: do this after starting the track so we get a corresponding location sighting.
    savePatrolSighting(VALUE_RESUME_PATROL, false);

    // Resume the patrol and commit the form state.
    m_patrolPaused = false;
    saveState();

    emit patrolPausedChanged();

    qDebug() << "Patrol resume success: " << QDateTime::currentDateTime();
}

bool SMARTProvider::isPatrolMetadataValid()
{
    auto form = qobject_cast<Form*>(parent());
    auto parentField = m_fieldManager->rootField();
    auto metadataTag = "metadata";
    bool result = true;

    for (int i = 0; i < parentField->fieldCount(); i++)
    {
        auto field = parentField->field(i);

        auto fieldTag = field->tag();
        if (fieldTag.isEmpty())
        {
            continue;
        }

        if (!fieldTag.contains(metadataTag))
        {
            continue;
        }

        if (fieldTag.value(metadataTag) == true)
        {
            if (!form->getFieldValueValid(form->rootRecordUid(), field->uid())) {
                qDebug() << field->uid() << " value not valid";
                result = false;
            }
        }
    }

    return result;
}

void SMARTProvider::setPatrolLegValue(const QString& name, const QVariant& value)
{
    if (m_patrolLegs.count() == 0)
    {
        // This should not be possible, but if the app crashes on the metadata screen, it might happen.
        qDebug() << "No patrol legs found!";
        return;
    }

    auto lastLeg = m_patrolLegs.last().toMap();
    lastLeg[name] = value;
    m_patrolLegs.replace(m_patrolLegs.count() - 1, lastLeg);

    emit patrolLegsChanged();
}

QVariantMap SMARTProvider::getPatrolLeg(int index)
{
    qFatalIf(index >= m_patrolLegs.count(), "Bad leg index");

    // Show the last leg first if the patrol is running.
    if (m_patrolStarted)
    {
        if (index == 0)
        {
            index = m_patrolLegs.count() - 1;
        }
        else
        {
            index--;
        }
    }

    // Build a model of the leg.
    auto result = QVariantMap();

    auto legMap = m_patrolLegs[index].toMap();
    auto lastLeg = index == m_patrolLegs.count() - 1;
    auto timeManager = App::instance()->timeManager();
    auto distance = legMap.value("distance", 0.0).toDouble();
    auto startTime = legMap["startTime"].toString();
    auto stopTime = legMap.contains("stopTime") ? legMap["stopTime"].toString() : timeManager->currentDateTimeISO();
    auto rate = legMap.value("rate", "??");
    auto transportElementUid = legMap.value("transportElementUid").toString();

    auto id = QString("%1 %2").arg(tr("Leg"), QString::number(index + 1));

    if (!transportElementUid.isEmpty())
    {
        id += " - " + qobject_cast<Form*>(parent())->getElementName(transportElementUid);
    }

    if (lastLeg && m_patrolStarted)
    {
        id += " - " + (m_patrolPaused ? tr("Paused") : tr("Active"));
    }

    result["id"] = id;
    result["active"] = m_patrolStarted && lastLeg;
    result["startDate"] = timeManager->getDateText(startTime);
    result["startTime"] = timeManager->getTimeText(startTime);
    result["distance"] = App::instance()->getDistanceText(distance);
    result["speed"] = App::instance()->getSpeedText(distance / (Utils::timestampDeltaMs(startTime, stopTime) / 1000));
    result["rate"] = rate;

    return result;
}

void SMARTProvider::changePatrol()
{
    auto form = qobject_cast<Form*>(parent());

    // Store the counter and stop the track.
    auto trackCounter = form->trackStreamer()->counter();
    stopTrack();

    // Update the legs.
    auto currentTime = App::instance()->timeManager()->currentDateTimeISO();
    setPatrolLegValue("stopTime", currentTime);
    m_patrolLegs.append(QVariantMap {{ "startTime", currentTime }});
    form->setFieldValue(FIELD_PATROL_LEG_COUNTER, form->getFieldValue(FIELD_PATROL_LEG_COUNTER).toInt() + 1);
    form->setFieldValue(FIELD_PATROL_LEG_DISTANCE, 0);

    // To retain backward compatibility, a metadata change in a survey comes through as an observation.
    // The backend will not create a new incident as a result.
    savePatrolSighting(m_surveyMode ? VALUE_OBSERVATION : VALUE_CHANGE_PATROL, true);
    saveState();

    // Resume the track with the counter.
    startTrack(0, trackCounter);
}

void SMARTProvider::startIncident()
{
    qDebug() << "Incident start: " << QDateTime::currentDateTime();

    auto form = qobject_cast<Form*>(parent());
    qFatalIf(!m_hasIncident, "Incident not available");
    qFatalIf(m_incidentStarted, "Incident already started");
    qFatalIf(form->getFieldValue(FIELD_DATA_TYPE) != "incident", "Bad data type for incident");

    if (form->sighting()->createTime().isEmpty())
    {
        form->sighting()->snapCreateTime();
    }

    m_incidentStarted = true;

    saveState();

    emit incidentStartedChanged();

    qDebug() << "Incident start success: " << QDateTime::currentDateTime();
}

void SMARTProvider::stopIncident(bool /*cancel*/)
{
    qDebug() << "Incident stop: " << QDateTime::currentDateTime();

    auto form = qobject_cast<Form*>(parent());
    qFatalIf(!m_hasIncident, "Incident not available");
    qFatalIf(!m_incidentStarted, "Incident not started");

    m_incidentStarted = false;

    if (!form->editing())
    {
        form->newSighting();
    }

    saveState();

    emit incidentStartedChanged();

    qDebug() << "Incident stop success: " << QDateTime::currentDateTime();
}

void SMARTProvider::startCollect()
{
    qDebug() << "Collect start: " << QDateTime::currentDateTime();

    auto form = qobject_cast<Form*>(parent());
    qFatalIf(!m_hasCollect, "Collect not available");
    qFatalIf(m_collectStarted, "Collect already started");
    qFatalIf(form->getFieldValue(FIELD_DATA_TYPE) != "smartcollect", "Bad data type for collect");

    form->setFieldValue(FIELD_COLLECT_USER, m_collectUser);

    startTrack(0, 0);

    m_collectStarted = true;
    saveState();

    emit collectStartedChanged();

    qDebug() << "Collect start success: " << QDateTime::currentDateTime();
}

void SMARTProvider::stopCollect()
{
    qDebug() << "Collect stop: " << QDateTime::currentDateTime();

    auto form = qobject_cast<Form*>(parent());
    qFatalIf(!m_hasCollect, "Collect not available");
    qFatalIf(!m_collectStarted, "Collect not started");

    stopTrack();

    m_collectStarted = false;
    form->newSighting();
    saveState();

    emit collectStartedChanged();

    qDebug() << "Collect stop success: " << QDateTime::currentDateTime();
}

void SMARTProvider::setCollectUser(const QString& user)
{
    update_collectUser(user);

    saveState();
}

QJsonObject SMARTProvider::locationToJson(const QString& sightingUid, const QVariantMap& data, const QVariantMap& extraProps)
{
    // SMART requires that latitude and longitude look like doubles not like longs.
    auto locationX = data.value("x").toDouble();
    if (!QString::number(locationX).contains("."))
    {
        locationX += 0.0000000001;
    }
    auto locationY = data.value("y").toDouble();
    if (!QString::number(locationY).contains("."))
    {
        locationY += 0.0000000001;
    }

    // Geometry.
    auto geometry = QJsonObject();
    geometry.insert("type", "Point");

    auto coordinates = QJsonArray();
    coordinates.append(locationX);
    coordinates.append(locationY);
    geometry.insert("coordinates", coordinates);

    // Properties.
    auto properties = QJsonObject();
    properties.insert(FIELD_OBSERVATION_TYPE, "waypoint");
    properties.insert("dateTime", data["ts"].toString());
    properties.insert("deviceId", App::instance()->settings()->deviceId());
    properties.insert("id", sightingUid);
    auto accuracy = data["a"].toDouble();
    properties.insert("accuracy", qIsNaN(accuracy) ? 10000 : accuracy);
    auto altitude = data["z"].toDouble();
    properties.insert("altitude", qIsNaN(altitude) ? 0 : altitude);

    // Extra properties.
    for (auto it = extraProps.constKeyValueBegin(); it != extraProps.constKeyValueEnd(); it++)
    {
        properties.insert(it->first, QJsonValue::fromVariant(it->second));
    }

    // Full location object.
    auto result = QJsonObject();
    result.insert("geometry", geometry);
    result.insert("properties", properties);
    result.insert("type", "Feature");

    return result;
}

QJsonObject SMARTProvider::addRecordToJson(const QJsonObject& baseData, Form* form, Sighting* sighting, const QString& recordUid, bool childRecords, int attributeIndex)
{
    auto result = QJsonObject(baseData);

    auto record = sighting->getRecord(recordUid);
    auto fieldFilter = QStringList();

    if (record->hasFieldValue(FIELD_MODEL_TREE))
    {
        auto value = record->getFieldValue(FIELD_MODEL_TREE).toString();
        auto element = form->elementManager()->getElement(value);
        if (element)
        {
            fieldFilter = element->fieldUids();
        }

        fieldFilter.append(FIELD_SIGHTING_GROUP_ID);
    }

    record->enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
    {
        // Skip internal fields.
        auto fieldUid = fieldValue.fieldUid();
        if (fieldUid == FIELD_GROUPS ||
            fieldUid == FIELD_LOCATION ||
            fieldUid == FIELD_DISTANCE || fieldUid == FIELD_BEARING)
        {
            return;
        }

        auto field = fieldValue.field();

        // Special handling of the category selection.
        if (fieldUid == FIELD_MODEL_TREE)
        {
            auto element = form->elementManager()->getElement(fieldValue.value().toString());
            result.insert("c:" + QString::number(attributeIndex), element->exportUid());
            return;
        }

        // Skip fields which aren't part of the model.
        if (!fieldFilter.empty() && !fieldFilter.contains(fieldUid))
        {
            return;
        }

        // Skip fields which are not relevant.
        if (!fieldValue.isRelevant())
        {
            return;
        }

        // When we encounter a record field, we must replicate each of the attributes in the parent as well.
        // SMART expects:
        //    ParentAttributes
        //    Child0Attributes
        //    ParentAttributes
        //    Child1Attributes
        // etc.
        if (field->isRecordField())
        {
            auto recordField = RecordField::fromField(field);
            if (!childRecords)
            {
                return;
            }

            auto orgAttributeIndex = attributeIndex;
            auto childRecordUids = sighting->getRecord(recordUid)->getFieldValue(fieldUid).toStringList();
            for (auto i = 0; i < childRecordUids.count(); i++)
            {
                auto childRecordUid = childRecordUids[i];

                // In matrix mode, skip records if they have not been changed.
                if (recordField->matrixCount() > 0)
                {
                    if (sighting->getRecord(childRecordUid)->isEmpty())
                    {
                        continue;
                    }
                }

                // Add the parent record for each child.
                result = addRecordToJson(result, form, sighting, recordUid, false, attributeIndex);

                // Add the child record.
                result = addRecordToJson(result, form, sighting, childRecordUid, false, attributeIndex);

                // Add another index.
                attributeIndex++;
            }

            attributeIndex = orgAttributeIndex;
            return;
        }

        // Build the export name.
        auto exportName = field->safeExportUid();

        // Update the export name based on the attribute index.
        if (exportName.startsWith("a:0:") && attributeIndex > 0)
        {
            exportName.remove(0, 3);
            exportName = "a:" + QString::number(attributeIndex) + exportName;
        }

        // Build the export value.
        auto exportValue = record->getFieldValue(fieldUid, field->defaultValue());

        // Decode text field.
        bool fieldIsList = !field->listElementUid().isEmpty();
        if (fieldIsList && qobject_cast<StringField*>(field))
        {
            auto element = form->elementManager()->getElement(exportValue.toString());
            if (element && !element->exportUid().isEmpty())
            {
                exportValue = element->exportUid();
            }
        }
        // Decode date field.
        else if (qobject_cast<DateField*>(field))
        {
            exportValue = QDate::fromString(exportValue.toString(), Qt::DateFormat::ISODate).toString("yyyy/MM/dd");
        }
        // Decode check list.
        else if (fieldIsList && qobject_cast<CheckField*>(field))
        {
            bool master = field->getTagValue("master").toBool();

            auto valueMap = exportValue.toMap();
            auto orderedKeys = QStringList(valueMap.keys());
            form->elementManager()->sortByIndex(orderedKeys);

            for (auto keyIt = orderedKeys.constBegin(); keyIt != orderedKeys.constEnd(); keyIt++)
            {
                auto key = *keyIt;

                // If this is a master field, we know we will get one record for each
                // of the checked items. So filter to just the one we need.
                if (master && orderedKeys.indexOf(key) != attributeIndex)
                {
                    continue;
                }

                auto exportKey = key;
                auto element = form->elementManager()->getElement(exportKey);
                if (element && !element->exportUid().isEmpty())
                {
                    exportKey = element->exportUid();
                }

                if (exportKey.startsWith("l:"))
                {
                    exportKey = exportKey.insert(2, QString::number(attributeIndex) + ":");
                }

                result.insert(exportKey, valueMap[key].toBool());
            }
            return;
        }
        // Decode number list.
        else if (fieldIsList && qobject_cast<NumberField*>(field))
        {
            bool master = field->getTagValue("master").toBool();

            auto valueMap = exportValue.toMap();
            auto orderedKeys = QStringList(valueMap.keys());
            form->elementManager()->sortByIndex(orderedKeys);

            for (auto keyIt = orderedKeys.constBegin(); keyIt != orderedKeys.constEnd(); keyIt++)
            {
                auto key = *keyIt;

                // If this is a master field, we know we will get one record for each
                // of the checked items. So filter to just the one we need.
                if (master && orderedKeys.indexOf(key) != attributeIndex)
                {
                    continue;
                }

                auto exportKey = key;
                auto element = form->elementManager()->getElement(exportKey);
                if (element && !element->exportUid().isEmpty())
                {
                    exportKey = element->exportUid();
                }

                // SMART expects that the list item be prefixed with:
                //  "ml:<counter>:<exportUid minus prefix>".
                auto exportPrefix = field->exportUid();
                if (!exportPrefix.isEmpty())
                {
                    qFatalIf(!exportPrefix.startsWith("a:0"), "Bad export uid on number list field");
                    exportPrefix = exportPrefix.remove(0, 3);
                    exportPrefix = "ml:" + QString::number(attributeIndex) + exportPrefix + ":";
                }

                bool ok = false;
                result.insert(exportPrefix + exportKey, valueMap[key].toDouble(&ok));
                qFatalIf(!ok, "Bad double in number list field value");
            }

            return;
        }
        else if (qobject_cast<PhotoField*>(field))
        {
            auto filenames = exportValue.toStringList();
            auto photoIndex = 0;
            for (auto filenameIt = filenames.constBegin(); filenameIt != filenames.constEnd(); filenameIt++)
            {
                auto filePath = App::instance()->getMediaFilePath(*filenameIt);
                QFile file(filePath);
                if (!filePath.isEmpty() && file.exists() && file.open(QIODevice::ReadOnly))
                {
                    auto hexString = QString(file.readAll().toBase64());
                    auto exportKey = exportName + QString::number(photoIndex++);
                    if (m_smartVersion >= 700)
                    {
                        exportKey += ":" + QString::number(attributeIndex);
                    }
                    result.insert(exportKey, hexString);
                }
            }

            return;
        }
        else if (qobject_cast<AudioField*>(field))
        {
            auto filePath = App::instance()->getMediaFilePath(exportValue.toMap().value("filename").toString());
            QFile file(filePath);
            if (!filePath.isEmpty() && file.exists() && file.open(QIODevice::ReadOnly))
            {
                auto hexString = QString(file.readAll().toBase64());
                auto exportKey = exportName + QString::number(0);
                if (m_smartVersion >= 700)
                {
                    exportKey += ":" + QString::number(attributeIndex);
                }
                result.insert(exportKey, hexString);
            }

            return;
        }
        else if (qobject_cast<SketchField*>(field))
        {
            auto filePath = App::instance()->getMediaFilePath(exportValue.toMap().value("filename").toString());
            QFile file(filePath);
            if (!filePath.isEmpty() && file.exists() && file.open(QIODevice::ReadOnly))
            {
                auto hexString = QString(file.readAll().toBase64());
                auto exportKey = exportName + ":" + QString::number(attributeIndex);
                result.insert(exportKey, hexString);
            }

            return;
        }

        // Special handling for gps attributes.
        auto attributeKey = field->tag().value("attributeKey").toString();
        if (attributeKey.startsWith("gps_") && field->hidden())
        {
            auto location = sighting->getRootFieldValue(FIELD_LOCATION).toMap();
            attributeKey.remove(0, 4);

            if (location.contains(attributeKey))
            {
                exportValue = location.value(attributeKey).toDouble();
            }
        }

        result.insert(exportName, exportValue.toJsonValue());
    });

    return result;
}

QJsonArray SMARTProvider::sightingToJson(Form* form, const QVariantMap& data, int* obsCounter, const QString& filterRecordUid, const QVariantMap& extraProps)
{
    auto sighting = form->createSightingPtr();
    sighting->load(data);

    // Write the sighting.
    auto result = QJsonArray();
    auto rootRecordUid = sighting->rootRecordUid();
    auto recordUids = sighting->recordManager()->recordUids();

    // SMART requires that latitude and longitude look like doubles not like longs.
    auto location = sighting->getRootFieldValue(FIELD_LOCATION).toJsonObject();
    auto locationX = location.value("x").toDouble();
    if (!QString::number(locationX).contains("."))
    {
        locationX += 0.0000000001;
    }
    auto locationY = location.value("y").toDouble();
    if (!QString::number(locationY).contains("."))
    {
        locationY += 0.0000000001;
    }
    auto locationZ = location.value("z").toDouble();

    // Geometry.
    auto geometry = QJsonObject();
    geometry.insert("type", "Point");
    auto coordinates = QJsonArray();
    coordinates.append(locationX);
    coordinates.append(locationY);
    geometry.insert("coordinates", coordinates);

    // Properties.
    auto properties = QJsonObject();
    properties.insert("appName", m_project->title());
    properties.insert("ctVersion", App::instance()->buildString());
    properties.insert("dateTime", sighting->createTime());
    properties.insert("deviceId", App::instance()->settings()->deviceId());
    properties.insert("latitude", locationY);
    properties.insert("longitude", locationX);
    properties.insert("altitude", locationZ);
    properties.insert("rootId", rootRecordUid);

    // Distance and bearing.
    if (m_useDistanceAndBearing)
    {
        auto distance = sighting->getRootFieldValue(FIELD_DISTANCE);
        auto bearing = sighting->getRootFieldValue(FIELD_BEARING);
        if (distance.isValid() && bearing.isValid())
        {
            auto distanceValue = distance.toDouble();
            if (distanceValue > 0)
            {
                properties.insert("distance", distance.toDouble());
                properties.insert("bearing", bearing.toDouble());
            }
        }
    }

    for (auto it = extraProps.constKeyValueBegin(); it != extraProps.constKeyValueEnd(); it++)
    {
        properties.insert(it->first, QJsonValue::fromVariant(it->second));
    }

    // Metadata.
    auto baseData = QJsonObject();
    baseData = addRecordToJson(baseData, form, sighting.get(), rootRecordUid);

    // These are required by the SMART desktop parser.
    baseData.insert("SMART_DefaultAttributeValues", "{}");

    auto isPatrolForm = form->stateSpace() == TYPE_PATROL;
    if (isPatrolForm)
    {
        baseData.insert("SMART_DefaultPatrolValues", "{}");
    }

    // Incidents use a local counter, i.e. they always start at 1.
    auto incidentObsCounter = 1;
    if (!isPatrolForm && obsCounter)
    {
        obsCounter = &incidentObsCounter;
    }

    // Patrols use the global obsCounter which always increments.
    // New patrol must start at 1.
    if (isPatrolForm && obsCounter && baseData.value(FIELD_OBSERVATION_TYPE).toString() == VALUE_NEW_PATROL)
    {
        *obsCounter = 1;
    }

    // Add the data records.
    auto firstRecord = true;
    for (auto recordIt = recordUids.constBegin(); recordIt != recordUids.constEnd(); recordIt++)
    {
        auto recordUid = *recordIt;

        // Skip root metadata record unless this is a metadata only sighting.
        if ((recordUid == rootRecordUid) && (recordUids.count() != 1))
        {
            continue;
        }

        // Filter to just one record.
        if (!filterRecordUid.isEmpty() && recordUid != filterRecordUid)
        {
            continue;
        }

        // Skip child records - they will be aggregated during addRecordToJson().
        auto parentRecordUid = sighting->getRecord(recordUid)->parentRecordUid();
        if (!parentRecordUid.isEmpty() && parentRecordUid != rootRecordUid)
        {
            continue;
        }

        // Append to base data.
        auto oneRecord = QJsonObject();
        oneRecord["geometry"] = geometry;
        oneRecord["type"] = "Feature";

        auto recordJson = addRecordToJson(baseData, form, sighting.get(), recordUid, true, 0);

        if (obsCounter)
        {
            recordJson.insert(FIELD_OBS_COUNTER, *obsCounter);
            *obsCounter += 1;
        }

        recordJson.insert(FIELD_NEW_WAYPOINT, firstRecord);
        firstRecord = false;

        properties["sighting"] = recordJson;

        properties["id"] = recordUid;
        oneRecord["properties"] = properties;

        // Build and add the sighting.
        result.append(oneRecord);
    }

    return result;
}

bool SMARTProvider::exportSightings(Form* form, QFile& exportFile, const std::function<bool()>& startFile, const std::function<bool()>& completeFile, const std::function<void(const QString&, const QVariantMap&, uint)>& doneOne, ExportEnumState* enumState)
{
    auto projectUid = m_project->uid();
    auto sightingUids = QStringList();

    m_database->getSightings(projectUid, form->stateSpace(), Sighting::DB_SIGHTING_FLAG | Sighting::DB_LOCATION_FLAG, &sightingUids);

    App::instance()->initProgress(projectUid, "SMARTExportData", { sightingUids.count() });

    auto index = 0;

    for (auto sightingIt = sightingUids.constBegin(); sightingIt != sightingUids.constEnd(); sightingIt++)
    {
        auto sightingUid = *sightingIt;

        // Load from the database.
        auto data = QVariantMap();
        auto flags = (uint)0;
        auto timestamp = QString();
        m_database->loadSighting(projectUid, sightingUid, &data, &flags);
        index++;

        // Skip already exported sightings.
        if (flags & Sighting::DB_EXPORTED_FLAG)
        {
            continue;
        }

        // Open a new file as needed.
        if (!exportFile.isOpen() && !startFile())
        {
            return false;
        }

        auto featureArray = QJsonArray();
        if (flags & Sighting::DB_LOCATION_FLAG) // Locations.
        {
            timestamp = data.value("ts").toString();

            // A: Hack to import legacy times.
            if (timestamp.isEmpty())
            {
                auto l = data["t"].toLongLong();
                timestamp = (l == 0 || l == -1) ? "" : App::instance()->timeManager()->formatDateTime(l);
                data["ts"] = timestamp;
            }
            if (!timestamp.isEmpty())
            // B: Hack end.
            {
            featureArray.append(locationToJson(sightingUid, data));
            }
        }
        else                                    // Sightings.
        {
            timestamp = data.value("createTime").toString();

            // A: Hack to import legacy times.
            if (data.value("createTime").userType() != QMetaType::QString)
            {
                auto l = data.value("createTime").toLongLong();
                timestamp = (l == 0 || l == -1) ? "" : App::instance()->timeManager()->formatDateTime(l);
                data["createTime"] = timestamp;
            }
            // B: Hack end.

            featureArray = sightingToJson(form, data, &enumState->obsCounter);
        }

        // Commit the features.
        for (auto featureIt = featureArray.constBegin(); featureIt != featureArray.constEnd(); featureIt++)
        {
            auto featureJson = QJsonDocument(featureIt->toObject());

            // Write the comma separator.
            if (!enumState->first)
            {
                if (!exportFile.write(QString(",").toLatin1()))
                {
                    qDebug() << "Failed to write separator: " << exportFile.errorString();
                    return false;
                }
            }

            enumState->first = false;

            if (!exportFile.write(featureJson.toJson(JSON_FORMAT)))
            {
                qDebug() << "Failed to write data record: " << exportFile.errorString();
                return false;
            }
        }

        // Complete the file if it is over maxFileSizeBytes.
        if ((exportFile.size() > enumState->maxFileSizeBytes) && !completeFile())
        {
            return false;
        }

        // Success.
        doneOne(sightingUid, data, flags);

        if ((index + 100) % 100 == 0)
        {
            App::instance()->sendProgress(index);
        }
    }

    App::instance()->sendProgress(sightingUids.count());

    return true;
}

bool SMARTProvider::exportData()
{
    return exportDataInternal(-1, [&](const QString& filePath, const ExportFileInfo& exportFileInfo) -> bool
    {
        auto smartDataFilePath = m_smartDataPath + "/" + QFileInfo(filePath).fileName();

        auto result = QFile::rename(filePath, smartDataFilePath);
        if (result)
        {
            App::instance()->registerExportFile(smartDataFilePath, exportFileInfo.toMap());
            Utils::mediaScan(smartDataFilePath);
        }

        if (!result)
        {
            m_lastExportError = "Error 4a";
            qDebug() << "Failed to copy export file";
        }

        return result;
    });
}

bool SMARTProvider::exportDataInternal(int maxFileSizeBytes, const std::function<bool(const QString& filePath, const ExportFileInfo& info)>& finalizeFile)
{
    auto projectUid = m_project->uid();

    m_lastExportError.clear();

    // Create a working path.
    auto workPath = App::instance()->workPath() + "/Export_" + m_project->uid();
    QDir(workPath).removeRecursively();
    qFatalIf(!Utils::ensurePath(workPath), "Failed to create working folder");

    // Generate a filename based on export time.
    auto now = App::instance()->timeManager()->currentDateTime();
    auto exportFileNameRoot = QString("%1_%2_%3").arg("CTDATA", now.toString("yyyy-MM-ddThh_mm_ss"), "Patrol");
    auto exportFileName = QString();
    auto fileSuffixNext = 1;
    QFile file;

    auto exportFileInfo = ExportFileInfo { projectUid, SMART_PROVIDER, m_project->title(), now.toString("yyyy-MM-dd hh:mm:ss") };

    auto enumState = ExportEnumState();
    enumState.maxFileSizeBytes = maxFileSizeBytes == -1 ? 1024 * 1024 * 64 : maxFileSizeBytes;

    auto startFile = [&]() -> bool
    {
        enumState.first = true;
        exportFileInfo.reset();

        exportFileName = exportFileNameRoot + "-" + QString::number(fileSuffixNext).rightJustified(4, '0') + ".json";
        file.setFileName(workPath + "/" + exportFileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            m_lastExportError = "Error 1";
            qDebug() << "Failed to init export file: " << file.errorString();
            return false;
        }

        auto openBracket = QString("{\"type\": \"FeatureCollection\", \"features\": [");
        if (!file.write(openBracket.toLatin1()))
        {
            m_lastExportError = "Error 2";
            qDebug() << "Failed to init export file: " << file.errorString();
            return false;
        }

        fileSuffixNext += 1;

        return true;
    };

    auto completeFile = [&]() -> bool
    {
        auto closeBracket = QString("]}");
        if (!file.write(closeBracket.toLatin1()))
        {
            m_lastExportError = "Error 3";
            qDebug() << "Failed to complete export file: " << file.errorString();
            return false;
        }

        if (!file.flush())
        {
            m_lastExportError = "Error 4";
            qDebug() << "Failed to complete export file: " << file.errorString();
            return false;
        }

        file.close();
        Utils::mediaScan(file.fileName());

        return finalizeFile(file.fileName(), exportFileInfo);
    };

    auto doneOne = [&](const QString& /*sightingUid*/, const QVariantMap& data, uint flags) -> void
    {
        auto timestamp = QString();

        if (flags & Sighting::DB_SIGHTING_FLAG)
        {
            exportFileInfo.sightingCount++;
            timestamp = data["createTime"].toString();
        }
        else
        {
            exportFileInfo.locationCount++;
            timestamp = data["ts"].toString();
        }

        if (exportFileInfo.startTime.isEmpty())
        {
            exportFileInfo.startTime = timestamp;
        }

        exportFileInfo.stopTime = timestamp;
    };

    // Export patrol sightings.
    if (m_hasPatrol)
    {
        auto formPatrol = App::instance()->createFormPtr(projectUid, TYPE_PATROL);
        if (!exportSightings(formPatrol.get(), file, startFile, completeFile, doneOne, &enumState))
        {
            m_lastExportError = "Error 5";
            qDebug() << "Failed to export patrol data";
            return false;
        }
    }

    // Export independent incidents.
    if (m_hasIncident)
    {
        auto formIncident = App::instance()->createFormPtr(projectUid, TYPE_INCIDENT);
        if (!exportSightings(formIncident.get(), file, startFile, completeFile, doneOne, &enumState))
        {
            m_lastExportError = "Error 6";
            qDebug() << "Failed to export incident data";
            return false;
        }
    }

    // Complete the file.
    if (file.isOpen() && !completeFile())
    {
        m_lastExportError = "Error 7";
        qDebug() << "Failed to complete data";
        return false;
    }

    // Mark all sightings from this project as exported and read-only.
    m_database->setSightingFlagsAll(projectUid, Sighting::DB_EXPORTED_FLAG | Sighting::DB_READONLY_FLAG);

    QDir(workPath).removeRecursively();
    return true;
}

bool SMARTProvider::uploadData()
{
    // Create a working path.
    auto workPath = App::instance()->workPath() + "/Upload_" + m_project->uid();
    // Do not clean up, in case there are leftovers from a previous incomplete upload.
    qFatalIf(!Utils::ensurePath(workPath), "Failed to create upload folder");

    // Export data to small-ish files and move them to the upload folder.
    auto result = exportDataInternal(1024 * 256, [&](const QString& filePath, const ExportFileInfo& /*exportFileInfo*/) -> bool
    {
        auto fileInfo = QFileInfo(filePath);
        auto uploadFilePath = workPath + "/" + fileInfo.fileName();
        auto result = QFile::rename(filePath, uploadFilePath);

        if (!result)
        {
            qDebug() << "Failed to rename export file: " << filePath;
        }

        return result;
    });

    if (!result)
    {
        qDebug() << "Failed to export data to upload folder";
        return false;
    }

    // Get connection parameters.
    auto connector = m_profile["DATA_SERVER"].toMap();
    auto serverUrl = connector["URL"].toString();
    auto apiKey = connector["API_KEY"].toString();
    auto username = connector["USERNAME"].toString();
    auto password = connector["PASSWORD"].toString();
    auto deflate = COMPRESS_UPLOAD && connector["PROTOCOL"].toString() == "GEOJSON_COMPRESSED";

    auto tokenType = QString();
    auto token = QString();
    if (!username.isEmpty() && !password.isEmpty())
    {
        tokenType = "Basic";
        token = QString((username + ":" + password).toLocal8Bit().toBase64()).toLocal8Bit();
    }

    if (!apiKey.isEmpty())
    {
        serverUrl.append("?api_key=" + apiKey);
    }

    // Enumerate the files and upload each one.
    auto fileInfos = QDir(workPath).entryInfoList(QDir::Files, QDir::Time);
    App::instance()->initProgress("SMART", "Upload", { fileInfos.count() });
    auto index = 0;

    // Sort the paths, so the server receives them in order.
    auto filePaths = QStringList();
    for (auto it = fileInfos.constBegin(); it != fileInfos.constEnd(); it++)
    {
        filePaths.append(it->filePath());
    }

    filePaths.sort();

    for (auto it = filePaths.constBegin(); it != filePaths.constEnd(); it++)
    {
        QFile dataFile(*it);
        qFatalIf(!dataFile.open(QFile::ReadOnly | QFile::Text), "Failed to read data file");

        auto response = Utils::httpPostData(App::instance()->networkAccessManager(), serverUrl, dataFile.readAll(), token, tokenType, deflate);
        if (!response.success)
        {
            qDebug() << "Error uploading: " << response.errorString;
            return false;
        }

        dataFile.close();
        dataFile.remove();
        Utils::mediaScan(*it);

        App::instance()->sendProgress(index, 0);
        index++;
    }

    // Data has been accepted by the server, so cleanup the folder.
    QDir(workPath).removeRecursively();

    return true;
}

bool SMARTProvider::recoverAndClearData()
{
    auto projectUid = m_project->uid();

    // Wait for all tasks to complete or fail.
    App::instance()->taskManager()->rundownTasks(projectUid);
    m_database->deleteTasks(projectUid, Task::Active);
    m_database->deleteTasks(projectUid, Task::Paused);
    m_database->deleteTasks(projectUid, Task::Complete);

    // Remove the exported flag so we always get all the data.
    m_database->setSightingFlagsAll(projectUid, Sighting::DB_EXPORTED_FLAG, false);

    // Export the data.
    if (exportData())
    {
        clearCompletedData();
        return true;
    }

    return false;
}

bool SMARTProvider::clearCompletedData()
{
    auto projectUid = m_project->uid();
    auto app = App::instance();

    if (app->taskManager()->getIncompleteTaskCount(projectUid) != 0)
    {
        qDebug() << "Error: Cannot clear completed data when tasks are outstanding";
        return false;
    }

    // Backup the database.
    app->backupDatabase("SMART: Clear completed data");

    // Recover from conditions where the export at the end of patrol timed out.
    if (!m_hasDataServer)
    {
        exportData();
    }

    // Clear collect sightings.
    if (m_hasCollect)
    {
        app->removeSightingsByFlag(projectUid, TYPE_COLLECT, Sighting::DB_UPLOADED_FLAG);
        app->removeSightingsByFlag(projectUid, TYPE_COLLECT, Sighting::DB_EXPORTED_FLAG);
    }

    // Clear patrol sightings.
    if (m_hasPatrol)
    {
        app->removeSightingsByFlag(projectUid, TYPE_PATROL, Sighting::DB_UPLOADED_FLAG);
        app->removeSightingsByFlag(projectUid, TYPE_PATROL, Sighting::DB_EXPORTED_FLAG);
        update_patrolLegs(QVariantList());
    }

    // Clear independent incidents.
    if (m_hasIncident)
    {
        app->removeSightingsByFlag(projectUid, TYPE_INCIDENT, Sighting::DB_UPLOADED_FLAG);
        app->removeSightingsByFlag(projectUid, TYPE_INCIDENT, Sighting::DB_EXPORTED_FLAG);
    }

    // Clear completed tasks.
    m_uploadSightingLastTaskUid = "";
    saveState();
    m_database->deleteTasks(projectUid, Task::Complete);

    // Reset form states.
    m_database->deleteFormState(projectUid, TYPE_COLLECT);
    m_database->deleteFormState(projectUid, TYPE_PATROL);
    m_database->deleteFormState(projectUid, TYPE_INCIDENT);

    // Compact the database.
    m_database->compact();

    // Clean up unreferenced media.
    app->garbageCollectMedia();

    return true;
}

void SMARTProvider::startTrack(double initialTrackDistance, int initialTrackCounter)
{
    auto form = qobject_cast<Form*>(parent());

    auto pingUpdateInterval = 0;
    auto trackUpdateInterval = 0;
    auto trackDistanceFilter = 0.0;

    if (m_smartVersion >= 700) // SMART 7+.
    {
        // Outlier detection.
        auto transportElementUid = form->getFieldValue(FIELD_PATROL_TRANSPORT).toString();
        if (!transportElementUid.isEmpty())
        {
            setPatrolLegValue("transportElementUid", transportElementUid);

            auto transportElement = form->elementManager()->getElement(transportElementUid);
            auto max_speed = transportElement->getTagValue("max_speed").toInt();
            App::instance()->settings()->set_locationOutlierMaxSpeed(max_speed);
        }

        // Ping.
        if (m_profile.contains("POSITION_UPDATES") && shouldUploadData())
        {
            auto frequencyMinutes = m_profile["POSITION_UPDATES"].toMap()["FREQUENCY_MIN"].toInt();
            pingUpdateInterval = frequencyMinutes * 60;
        }

        // Track.
        auto tt = form->getFieldValue(FIELD_PATROL_TRANSPORT);
        auto te = form->getElement(tt.toString());
        auto ct_settings = te->getTagValue("ct_settings").toMap();

        auto distanceMode = false;
        auto interval = 0;

        if (!ct_settings.isEmpty())
        {
            distanceMode = ct_settings.value("waypoint_timer_type", "TIME") == "DISTANCE";
            interval = ct_settings.value("waypoint_timer", 0).toInt();
        }
        else
        {
            distanceMode = m_profile.value("WAYPOINT_TIMER_TYPE", "") == "DISTANCE";
            interval = m_profile.value("WAYPOINT_TIMER", 0).toInt();
        }

        trackUpdateInterval = distanceMode ? 1 : interval;
        trackDistanceFilter = distanceMode ? interval : 0;
    }
    else // SMART 6.
    {
        // Ping.
        m_pingData.clear();
        if (shouldUploadData())
        {
            for (auto it = m_extraData.constBegin(); it != m_extraData.constEnd(); it++)
            {
                auto extraDataMap = it->toMap();
                if (extraDataMap.value("id") == "connect_ct_properties")
                {
                    m_pingData = extraDataMap;
                    pingUpdateInterval = extraDataMap.value("ping_frequency").toInt() * 60;
                    break;
                }
            }
        }

        // Track.
        trackUpdateInterval = m_profile.value("WAYPOINT_TIMER", 0).toInt();
        trackDistanceFilter = 0;
    }

    // Disable track settings for Collect, ping still allowed.
    if (m_hasCollect)
    {
        trackUpdateInterval = 0;
        trackDistanceFilter = 0;
    }

    // Configure ping streamer.
    form->pointStreamer()->start(pingUpdateInterval * 1000, 0);

    // Configure track streamer.
    form->trackStreamer()->start(trackUpdateInterval * 1000, trackDistanceFilter, initialTrackDistance, initialTrackCounter, true);
    setPatrolLegValue("rate", form->trackStreamer()->rateText());

    qDebug() << "Track start: " << form->trackStreamer()->rateText();
}

void SMARTProvider::stopTrack()
{
    auto form = qobject_cast<Form*>(parent());

    qDebug() << "Track stop: " << form->trackStreamer()->rateText();

    form->trackStreamer()->stop();
    form->pointStreamer()->stop();

    // Disable outlier detection.
    App::instance()->settings()->set_locationOutlierMaxSpeed(0);
}

bool SMARTProvider::uploadPing(const QVariantMap& locationMap)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");

    auto result = false;

    if (m_profile.contains("POSITION_UPDATES"))
    {
        result = uploadPing7(locationMap);
    }
    else
    {
        result = uploadPing6(locationMap);
    }

    return result;
}

bool SMARTProvider::uploadPing6(const QVariantMap& locationMap)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");

    // Create a ping from the base server URL.
    auto dataServer = m_profile["DATA_SERVER"].toMap();
    auto urlParts = dataServer["URL"].toString().split('/');

    // Build the CaUuid from the current server URL.
    auto caUuid = urlParts.last().remove('-');
    caUuid.insert(8, '-');
    caUuid.insert(13, '-');
    caUuid.insert(18, '-');
    caUuid.insert(23, '-');

    // Build the typeUuid.
    auto typeUuid = m_pingData["ping_alert_type"].toString().remove('-');
    typeUuid.insert(8, '-');
    typeUuid.insert(13, '-');
    typeUuid.insert(18, '-');
    typeUuid.insert(23, '-');

    // Construct a url.
    urlParts.removeLast();
    urlParts.removeLast();
    urlParts.append("connectalert");
    urlParts.append(m_patrolUid);
    auto url = urlParts.join('/');

    // Create and fixup the sighting JSON.
    auto extraProps = QVariantMap();
    extraProps.insert("typeUuid", typeUuid);
    extraProps.insert("caUuid", caUuid);
    extraProps.insert("level", 5);
    extraProps.insert("patrolId", m_patrolUid);
    extraProps.insert("description", "");

    auto locationJson = locationToJson(QUuid::createUuid().toString(QUuid::Id128), locationMap, extraProps);
    auto features = QVariantList();
    features.append(locationJson);

    auto event = QVariantMap();
    event["type"] = "FeatureCollection";
    event["features"] = features;
    auto payloadJson = Utils::variantMapToJson(event, JSON_FORMAT);

    // POST.
    QNetworkAccessManager networkAccessManager;
    QNetworkReply* reply;
    auto request = QNetworkRequest();
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");

    auto concatenated = dataServer["USERNAME"].toString() + ":" + dataServer["PASSWORD"].toString();
    auto data = concatenated.toLocal8Bit().toBase64();
    auto headerData = QString("Basic " + data).toLocal8Bit();
    request.setRawHeader("Authorization", headerData);

    request.setRawHeader("Content-Type", "application/json; charset=utf-8");
    request.setUrl(url);
    reply = networkAccessManager.post(request, payloadJson);
    reply->ignoreSslErrors();

    // Submit data.
    QEventLoop eventLoop;
    auto uploadSuccess = false;
    auto errorCode = QNetworkReply::UnknownServerError;
    connect(reply, &QNetworkReply::finished, this, [&]()
    {
        errorCode = reply->error();
        uploadSuccess = errorCode == QNetworkReply::NoError;
        reply->deleteLater();
        eventLoop.exit();
    });

    eventLoop.exec();

    if (uploadSuccess)
    {
        qDebug() << "Ping success";
    }
    else
    {
        qDebug() << "Failed to ping server: " << errorCode;
    }

    return true;
}

bool SMARTProvider::uploadPing7(const QVariantMap& locationMap)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");
    qFatalIf(!m_profile.contains("POSITION_UPDATES"), "Nowhere to send");

    // Create a ping from the base server URL.
    auto positionUpdates = m_profile["POSITION_UPDATES"].toMap();
    auto apiKey = positionUpdates["API_KEY"].toString();
    auto username = positionUpdates["USERNAME"].toString();
    auto password = positionUpdates["PASSWORD"].toString();
    auto serverUrl = positionUpdates["URL"].toString();

    // Create and fixup the sighting JSON.
    auto positionUpdatesMetadata = positionUpdates["METADATA"].toMap();
    auto extraProps = QVariantMap
    {
        { "typeUuid", positionUpdatesMetadata["TYPEUUID"] },
        { "caUuid", positionUpdatesMetadata["CAUUID"] },
        { "level", positionUpdatesMetadata["LEVEL"] },
        { "patrolId", m_patrolUid },
        { "description", "" },
    };

    auto locationJson = locationToJson(QUuid::createUuid().toString(QUuid::Id128), locationMap, extraProps);
    auto features = QVariantList();
    features.append(locationJson);

    auto event = QVariantMap();
    event["type"] = "FeatureCollection";
    event["features"] = features;
    auto payloadJson = Utils::variantMapToJson(event, JSON_FORMAT);

    // POST.
    QNetworkAccessManager networkAccessManager;
    QNetworkReply* reply;
    auto request = QNetworkRequest();
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/json; charset=utf-8");

    if (!username.isEmpty() && !password.isEmpty())
    {
        auto concatenated = username + ":" + password;
        auto data = concatenated.toLocal8Bit().toBase64();
        auto headerData = QString("Basic " + data).toLocal8Bit();
        request.setRawHeader("Authorization", headerData);
    }

    if (!apiKey.isEmpty())
    {
        serverUrl.append(m_pingUuid);
        serverUrl.append("?api_key=" + apiKey);
    }

    request.setUrl(serverUrl);
    reply = networkAccessManager.put(request, payloadJson);
    reply->ignoreSslErrors();

    // Submit data.
    QEventLoop eventLoop;
    auto uploadSuccess = false;
    auto errorCode = QNetworkReply::UnknownServerError;
    auto errorString = QString();
    connect(reply, &QNetworkReply::finished, this, [&]()
    {
        errorCode = reply->error();
        errorString = reply->errorString();
        uploadSuccess = errorCode == QNetworkReply::NoError;
        reply->deleteLater();
        eventLoop.exit();
    });

    eventLoop.exec();

    if (uploadSuccess)
    {
        qDebug() << "Ping success";
    }
    else
    {
        qDebug() << "Failed to ping server: " << errorString;
    }

    return true;
}

bool SMARTProvider::uploadAlerts(Form* form, const QString& sightingUid)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");

    auto result = false;

    if (m_profile.contains("ALERTS"))
    {
        result = uploadAlerts7(form, sightingUid);
    }
    else
    {
        result = uploadAlerts6(form, sightingUid);
    }

    return result;
}

bool SMARTProvider::uploadAlerts6(Form* form, const QString& sightingUid)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");

    auto projectUid = m_project->uid();

    auto sighting = form->createSightingPtr(sightingUid);
    auto recordUids = sighting->recordManager()->recordUids();
    auto sightingData = sighting->save();

    for (auto extraItemIt = m_extraData.constBegin(); extraItemIt != m_extraData.constEnd(); extraItemIt++)
    {
        auto alertItem = extraItemIt->toMap();
        if (alertItem.value("id", "") != "connect_ct_alert")
        {
            continue;
        }

        auto fieldUid = alertItem["attribute"].toString();
        auto fieldValue = alertItem["alert_item"].toString();

        for (auto recordIt = recordUids.constBegin(); recordIt != recordUids.constEnd(); recordIt++)
        {
            auto recordUid = *recordIt;

            if (fieldUid.isEmpty())
            {
                auto modelElementUid = sighting->getRecord(recordUid)->getFieldValue(FIELD_MODEL_TREE).toString();
                if (modelElementUid.isEmpty())
                {
                    continue;
                }

                auto modelElement = m_elementManager->getElement(modelElementUid);
                while (modelElement)
                {
                    if (modelElement->uid() == fieldValue)
                    {
                        break;
                    }

                    modelElement = qobject_cast<Element*>(modelElement->parent());
                }

                if (!modelElement)
                {
                    continue;
                }
            }
            else if (sighting->getRecord(recordUid)->getFieldValue(fieldUid) != fieldValue)
            {
                continue;
            }

            // Record has matched: fire alert.
            // Create an alert URL from the base server URL.
            auto dataServer = m_profile["DATA_SERVER"].toMap();
            auto url = dataServer["URL"].toString().split('/');

            // Build the CaUuid from the current server URL.
            auto caUuid = url.last();
            caUuid.insert(8, '-');
            caUuid.insert(13, '-');
            caUuid.insert(18, '-');
            caUuid.insert(23, '-');

            // Construct a connectalert api.
            url.removeLast();
            url.removeLast();
            url.append("connectalert");
            url.append(recordUid);
            dataServer["URL"] = url.join('/');
            dataServer["PROTOCOL"] = "GEOJSON";

            // Create and fixup the sighting JSON.
            auto extraProps = QVariantMap();
            extraProps.insert("typeUuid", alertItem["type"]);
            extraProps.insert("caUuid", caUuid);
            extraProps.insert("level", alertItem["level"]);
            extraProps.insert("description", "");

            auto event = QVariantMap();
            event["type"] = "FeatureCollection";
            event["features"] = sightingToJson(form, sightingData, nullptr, recordUid, extraProps);

            auto inputJson = QJsonObject();
            inputJson.insert("DATA_SERVER", QJsonObject::fromVariantMap(dataServer));
            inputJson.insert("payload", QJsonObject::fromVariantMap(event));

            // Parent to the last save task.
            auto baseUid = Task::makeUid(UploadSightingTask::getName(), recordUid + "_alert");
            auto uid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());

            // Create the task and parent to the last uploaded sighting.
            m_taskManager->addTask(projectUid, uid, "", inputJson.toVariantMap());
        }
    }

    return true;
}

bool SMARTProvider::uploadAlerts7(Form* form, const QString& sightingUid)
{
    qFatalIf(!m_profile.contains("ALERTS"), "Nowhere to send");

    auto projectUid = m_project->uid();

    auto sighting = form->createSightingPtr(sightingUid);
    auto recordUids = sighting->recordManager()->recordUids();
    auto sightingData = sighting->save();

    auto alerts = m_profile["ALERTS"].toList();

    for (auto alertIt = alerts.constBegin(); alertIt != alerts.constEnd(); alertIt++)
    {
        auto alertMap = alertIt->toMap();
        auto nodeUid = alertMap.value("CMNODE_UUID").toString();
        auto fieldUid = alertMap.value("CMATTRIBUTE_UUID").toString();
        auto fieldValueList = alertMap.value("CMATTRIBUTE_LISTITEM_UUID").toString();
        auto fieldValueTree = alertMap.value("CMATTRIBUTE_TREENODE_UUID").toString();
        auto fieldValue = fieldValueTree.isEmpty() ? fieldValueList : fieldValueTree;

        for (auto recordIt = recordUids.constBegin(); recordIt != recordUids.constEnd(); recordIt++)
        {
            auto recordUid = *recordIt;

            if (!nodeUid.isEmpty())
            {
                auto modelElementUid = sighting->getRecord(recordUid)->getFieldValue(FIELD_MODEL_TREE).toString();
                if (modelElementUid.isEmpty())
                {
                    continue;
                }

                auto modelElement = m_elementManager->getElement(modelElementUid);
                while (modelElement)
                {
                    if (modelElement->uid() == nodeUid)
                    {
                        break;
                    }

                    modelElement = qobject_cast<Element*>(modelElement->parent());
                }

                if (!modelElement)
                {
                    continue;
                }
            }

            if (!fieldUid.isEmpty() && sighting->getRecord(recordUid)->getFieldValue(fieldUid) != fieldValue)
            {
                continue;
            }

            // Record has matched: fire alert.
            auto dataServer = QVariantMap();
            dataServer["URL"] = alertMap["URL"].toString() + sightingUid;
            dataServer["API_KEY"] = alertMap["API_KEY"];
            dataServer["USERNAME"] = alertMap["USERNAME"];
            dataServer["PASSWORD"] = alertMap["PASSWORD"];
            dataServer["PROTOCOL"] = "GEOJSON";

            // Create and fixup the sighting JSON.
            auto alertMetadata = alertMap["METADATA"].toMap();
            auto extraProps = QVariantMap
            {
                { "typeUuid", alertMetadata["TYPEUUID"] },
                { "caUuid", alertMetadata["CAUUID"] },
                { "level", alertMetadata["LEVEL"] },
                { "description", "" },
            };

            auto event = QVariantMap();
            event["type"] = "FeatureCollection";
            event["features"] = sightingToJson(form, sightingData, nullptr, recordUid, extraProps);

            auto inputJson = QJsonObject();
            inputJson.insert("DATA_SERVER", QJsonObject::fromVariantMap(dataServer));
            inputJson.insert("payload", QJsonObject::fromVariantMap(event));
            inputJson.insert("usePUT", true);

            // Parent to the last save task.
            auto baseUid = Task::makeUid(UploadSightingTask::getName(), recordUid + "_alert");
            auto uid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());

            // Create the task and parent to the last uploaded sighting.
            m_taskManager->addTask(projectUid, uid, "", inputJson.toVariantMap());
        }
    }

    return true;
}

bool SMARTProvider::uploadSighting(Form* form, const QString& sightingUid, int* obsCounter)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");

    auto projectUid = m_project->uid();
    auto data = QVariantMap();
    uint flags;
    m_database->loadSighting(projectUid, sightingUid, &data, &flags);

    auto event = QVariantMap();
    event["type"] = "FeatureCollection";
    event["features"] = sightingToJson(form, data, obsCounter);

    auto inputJson = QJsonObject();
    inputJson.insert("DATA_SERVER", m_profile["DATA_SERVER"].toJsonObject());
    inputJson.insert("payload", QJsonObject::fromVariantMap(event));
    inputJson.insert("stateSpace", form->stateSpace());
    inputJson.insert("sightingUids", QJsonArray::fromStringList(QStringList { sightingUid }));

    // Parent to the last save task.
    auto baseUid = Task::makeUid(UploadSightingTask::getName(), sightingUid);
    auto uid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());

    // Create the task and parent to the last uploaded sighting (or none if empty).
    m_taskManager->addTask(projectUid, uid, m_uploadSightingLastTaskUid, inputJson.toVariantMap());

    // Subsequent sightings will only upload after this one is complete.
    m_uploadSightingLastTaskUid = uid;
    m_uploadSightingLastTime = data["createTime"].toString();

    // Mark the sighting as read-only.
    m_database->setSightingFlags(projectUid, sightingUid, Sighting::DB_READONLY_FLAG);

    return true;
}

void SMARTProvider::uploadPendingLocations(Form* form)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");

    auto projectUid = m_project->uid();
    auto sightingUids = QStringList();
    m_database->getSightings(projectUid, form->stateSpace(), Sighting::DB_PENDING_FLAG, &sightingUids);

    if (sightingUids.count() == 0)
    {
        return;
    }

    auto event = QVariantMap();
    event["type"] = "FeatureCollection";
    auto featureArray = QJsonArray();

    for (auto sightingIt = sightingUids.constBegin(); sightingIt != sightingUids.constEnd(); sightingIt++)
    {
        auto sightingUid = *sightingIt;
        auto data = QVariantMap();
        m_database->loadSighting(projectUid, sightingUid, &data);
        featureArray.append(locationToJson(sightingUid, data));
    }

    event["features"] = featureArray;

    auto inputJson = QJsonObject();
    inputJson.insert("DATA_SERVER", m_profile["DATA_SERVER"].toJsonObject());
    inputJson.insert("payload", QJsonObject::fromVariantMap(event));
    inputJson.insert("stateSpace", form->stateSpace());
    inputJson.insert("sightingUids", QJsonArray::fromStringList(sightingUids));

    // Create the task and parent to the last uploaded sighting.
    auto baseUid = Task::makeUid(UploadSightingTask::getName(), sightingUids.first());
    auto uid = baseUid + '.' + QString::number(sightingUids.count());
    m_taskManager->addTask(projectUid, uid, m_uploadSightingLastTaskUid, inputJson.toVariantMap());

    // Remove the pending flag: the task owns this now.
    for (auto sightingIt = sightingUids.constBegin(); sightingIt != sightingUids.constEnd(); sightingIt++)
    {
        m_database->setSightingFlags(projectUid, *sightingIt, Sighting::DB_PENDING_FLAG | Sighting::DB_READONLY_FLAG, false);
    }

    m_uploadLastTime = App::instance()->timeManager()->currentDateTimeISO();
}

bool SMARTProvider::uploadLocation(Form* form, const QString& sightingUid)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");
    qFatalIf(form->stateSpace() != TYPE_PATROL && form->stateSpace() != TYPE_COLLECT, "Can only send locations for patrol & collect");
    qFatalIf(!m_patrolStarted && !m_collectStarted, "Patrol/Collect not started");
    qFatalIf(m_patrolPaused, "Patrol paused");

    auto projectUid = m_project->uid();
    auto data = QVariantMap();
    uint flags;
    m_database->loadSighting(projectUid, sightingUid, &data, &flags);

    auto event = QVariantMap();
    event["type"] = "FeatureCollection";
    auto featureArray = QJsonArray();
    featureArray.append(locationToJson(sightingUid, data));
    event["features"] = featureArray;

    auto inputJson = QJsonObject();
    inputJson.insert("DATA_SERVER", m_profile["DATA_SERVER"].toJsonObject());
    inputJson.insert("payload", QJsonObject::fromVariantMap(event));
    inputJson.insert("stateSpace", form->stateSpace());
    inputJson.insert("sightingUids", QJsonArray::fromStringList(QStringList { sightingUid }));

    auto baseUid = Task::makeUid(UploadSightingTask::getName(), sightingUid);
    auto uid = baseUid + '.' + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch());

    // Create the task and parent to the last uploaded sighting.
    m_taskManager->addTask(projectUid, uid, m_uploadSightingLastTaskUid, inputJson.toVariantMap());

    // Mark the location as read-only.
    m_database->setSightingFlags(projectUid, sightingUid, Sighting::DB_READONLY_FLAG);

    return true;
}

QVariant SMARTProvider::getProfileValue(const QString& key, const QVariant& defaultValue)
{
    return m_profile.value(key, defaultValue);
}

void SMARTProvider::loadState()
{
    auto map = m_project->providerState();

    update_patrolUid(map.value("patrolUid").toString());
    update_patrolStarted(map.value("patrolStarted", false).toBool());
    update_patrolPaused(map.value("patrolPaused", false).toBool());
    update_patrolPausedDistance(map.value("patrolPausedDistance", 0).toDouble());
    update_patrolLegs(map.value("patrolLegs", QVariantList()).toList());
    update_incidentStarted(map.value("incidentStarted", false).toBool());
    update_collectStarted(map.value("collectStarted", false).toBool());
    update_collectUser(map.value("collectUser").toString());
    update_uploadObsCounter(map.value("uploadObsCounter", 1).toInt());
    update_uploadLastTime(map.value("lastUploadTime").toString());
    update_uploadSightingLastTaskUid(map.value("lastUploadSightingTaskUid").toString());
    update_uploadSightingLastTime(map.value("lastUploadSightingTime").toString());
    m_pingData = map.value("pingData").toMap();
    m_pingUuid = map.value("pingUuid").toString();
    m_patrolStartTimestamp = map.value("patrolStartTimestamp").toString();
}

void SMARTProvider::saveState()
{
    auto map = QVariantMap();
    map["patrolUid"] = m_patrolUid;
    map["patrolStarted"] = m_patrolStarted;
    map["patrolPaused"] = m_patrolPaused;
    map["patrolPausedDistance"] = m_patrolPausedDistance;
    map["patrolLegs"] = m_patrolLegs;
    map["incidentStarted"] = m_incidentStarted;
    map["collectStarted"] = m_collectStarted;
    map["collectUser"] = m_collectUser;
    map["uploadObsCounter"] = m_uploadObsCounter;
    map["lastUploadTime"] = m_uploadLastTime;
    map["lastUploadSightingTaskUid"] = m_uploadSightingLastTaskUid;
    map["lastUploadSightingTime"] = m_uploadSightingLastTime;
    map["pingData"] = m_pingData;
    map["pingUuid"] = m_pingUuid;
    map["patrolStartTimestamp"] = m_patrolStartTimestamp;

    auto form = qobject_cast<Form*>(parent());
    form->saveState();

    m_project->set_providerState(map);
}

QVariantMap SMARTProvider::parseProject()
{
    auto projectPath = getProjectFilePath("project.json");
    QFile projectFile(projectPath);
    qFatalIf(!projectFile.open(QFile::ReadOnly | QFile::Text), "Failed to read project.json from project folder");
    QByteArray projectFileData = projectFile.readAll();
    auto projectJson = QJsonDocument::fromJson(projectFileData);

    qFatalIf(!projectJson.isObject(), "project.json is not an object");

    return projectJson.object().toVariantMap();
}

void SMARTProvider::addMetadataItem(const QString& fieldName, const QVariantMap& fieldValues, ElementManager* elementManager, FieldManager* fieldManager, QVariantMap* /*outSettings*/)
{
    // Extract labels from an object - anything that starts with "label_".
    auto findLabels = [&](const QVariantMap& values) -> QJsonObject
    {
        auto result = QJsonObject();
        for (auto it = values.constKeyValueBegin(); it != values.constKeyValueEnd(); it++)
        {
            if (it->first.startsWith("label_"))
            {
                auto languageCode = QString(it->first).remove(0, 6);
                result.insert(languageCode, it->second.toString());
            }
        }

        return result;
    };

    auto baseElement = new Element(nullptr);
    baseElement->set_uid(fieldName);

    // SMART 6 just has the label attribute - the translation comes from a language pack on the server.
    // SMART 7 supports translations, e.g. label_en, label_de etc.
    if (fieldValues.contains("label"))
    {
        baseElement->set_name(fieldValues["label"].toString());
    }
    else
    {
        baseElement->set_names(findLabels(fieldValues));
    }

    elementManager->rootElement()->appendElement(baseElement);

    BaseField* field = nullptr;
    auto defaultValue = fieldValues["default"];
    auto required = fieldValues["isRequired"].toBool();

    auto fieldType = fieldValues["type"].toString();
    auto isFixed = fieldValues["isFixed"].toBool();
    auto requiredBy = fieldValues.value("required_by", QVariantMap()).toMap();

    if (fieldType == "TEXT")
    {
        auto stringField = new StringField();
        stringField->set_multiLine(true);
        field = stringField;
    }
    else if (fieldType == "SINGLE_CHOICE")
    {
        field = new StringField();

        // Sort employee and station lists.
        field->set_listSorted(fieldName == "SMART_Leader" || fieldName == "SMART_Pilot" || fieldName == "SMART_Station");
    }
    else if (fieldType == "MULTI_CHOICE")
    {
        field = new CheckField();

        // Sort employee lists.
        field->set_listSorted(fieldName == "SMART_Employees");

        if (fieldValues.contains("defaults"))
        {
            auto valueMap = QVariantMap();
            auto defaults = fieldValues["defaults"].toList();
            for (auto it = defaults.constBegin(); it != defaults.constEnd(); it++)
            {
                valueMap.insert(it->toString(), true);
            }
            defaultValue = valueMap;
        }
    }
    else if (fieldType == "BOOLEAN")
    {
        field = new CheckField();
        if (required)
        {
            defaultValue = defaultValue.toBool();
        }
    }
    else if (fieldType == "NUMERIC")
    {
        field = new NumberField();
    }
    else if (fieldType == "DATE")
    {
        if (fieldName == FIELD_PATROL_START_DATE)
        {
            field = new StringField();
        }
        else
        {
            field = new DateField();
        }
    }
    else if ((fieldType == "DATE_AND_TIME") || (fieldType == "TIME"))
    {
        if (fieldName == FIELD_PATROL_START_TIME)
        {
            field = new StringField();
        }
        else
        {
            field = new TimeField();
        }
    }
    else if (fieldType == "UUID")
    {
        field = new StringField();
    }
    else if (fieldType == "MULTI_SELECT_LIST")
    {
        field = new CheckField(); // Haven't seen this yet.
    }
    else if (fieldType == "IMAGE")
    {
        field = new StringField();
    }
    else
    {
        qFatalIf(true, "Unknown field type: " + fieldType);
    }

    field->set_uid(fieldName);
    field->set_nameElementUid(fieldName);

    if (!fieldValues["options"].isNull())
    {
        std::pair<QString, QString> optionPrefixes[] = {
            { "SMART_Employees",       "e:"  },
            { "SMART_Mandate",         "pm:" },
            { "SMART_PatrolTransport", "tt:" },
            { "SMART_Station",         "ps:" },
            { "SMART_Team",            "pt:" },
            { "SMART_MissionAttList",  "ml:" },
            { "SMART_MissionAtt",      "ma:" },
            { "SMART_SamplingUnit",    "su:" },
        };

        QString optionPrefix;
        for (auto item: optionPrefixes)
        {
            if (fieldName == item.first)
            {
                optionPrefix = item.second;
                break;
            }
        }

        auto options = fieldValues["options"].toJsonArray();
        auto optionUids = QStringList();
        for (auto optionIt = options.constBegin(); optionIt != options.constEnd(); optionIt++)
        {
            auto optionMap = optionIt->toObject().toVariantMap();
            auto optionUid = optionMap["uuid"].toString();
            optionUids.append(optionUid);

            auto optionElement = new Element(nullptr);
            optionElement->set_uid(optionUid);
            optionElement->set_exportUid(optionPrefix + optionUid);

            if (!optionMap["label"].isNull())
            {
                optionElement->set_name(optionMap["label"].toString());
            }
            else
            {
                for (auto labelIt = optionMap.constKeyValueBegin(); labelIt != optionMap.constKeyValueEnd(); labelIt++)
                {
                    if (labelIt->first.startsWith("label_"))
                    {
                        auto names = optionElement->names();
                        auto keyToAdd = labelIt->first.split("_").last();
                        names.insert(keyToAdd, labelIt->second.toString());
                        optionElement->set_names(names);
                    }
                }
            }

            if (optionMap.contains("ct_settings"))
            {
                optionElement->addTag("ct_settings", optionMap["ct_settings"]);
            }

            if (optionMap.contains("max_speed"))
            {
                optionElement->addTag("max_speed", optionMap["max_speed"]);
            }

            qFatalIf(optionElement->name().isEmpty() && optionElement->names().isEmpty(), "No name for list option for field " + fieldName);

            baseElement->appendElement(optionElement);
        }

        field->set_listElementUid(fieldName);

        // Groups: SMART 7.5+.
        if (!fieldValues["groups"].toJsonArray().isEmpty())
        {
            auto groupsElement = new Element(nullptr);
            groupsElement->set_uid(fieldName + "/groups");
            groupsElement->set_elementUids(optionUids);
            elementManager->rootElement()->appendElement(groupsElement);

            auto groups = fieldValues["groups"].toJsonArray();

            auto index = 1;
            for (auto groupIt = groups.constBegin(); groupIt != groups.constEnd(); groupIt++)
            {
                auto groupMap = groupIt->toObject().toVariantMap();

                auto groupElement = new Element(nullptr);
                groupElement->set_uid(fieldName + "/group/" + QString::number(index++));
                groupElement->set_names(findLabels(groupMap));
                groupElement->set_elementUids(groupMap.value("options").toStringList());
                groupsElement->appendElement(groupElement);
            }

            field->set_listElementUid(groupsElement->uid());
        }
    }
    else if (!fieldValues["parent"].isNull())
    {
        auto parentValue = fieldValues["parent"].toString();
        field->set_listElementUid(parentValue);
        field->set_listFilterByField(parentValue);
    }

    field->addTag("metadata", true);

    if (isFixed)
    {
        field->addTag("fixed", true);
    }

    auto requiredIf = QString();
    if (!requiredBy.isEmpty())
    {
        for (auto requiredByIt = requiredBy.constKeyValueBegin(); requiredByIt != requiredBy.constKeyValueEnd(); requiredByIt++)
        {
            auto requiredValues = requiredByIt->second.toStringList();
            if (requiredValues.isEmpty())
            {
                continue;
            }

            if (!requiredIf.isEmpty())
            {
                requiredIf += " or ";
            }

            requiredIf += '(';

            for (auto i = 0; i < requiredValues.count(); i++)
            {
                if (i > 0)
                {
                    requiredIf += " or ";
                }

                requiredIf += "${" + requiredByIt->first + "} = '" + requiredValues[i] + "'";
            }

            requiredIf += ')';
        }

        field->set_relevant(requiredIf.isEmpty() ? "false()" : requiredIf);
    }

    field->set_hidden(!fieldValues["isVisible"].toBool());
    field->set_required(requiredIf.isEmpty() ? QVariant(required) : requiredIf);
    field->set_defaultValue(defaultValue);

    fieldManager->rootField()->appendField(field);
}

void SMARTProvider::parseMetadata(const QString& fileName, ElementManager* elementManager, FieldManager* fieldManager, QVariantMap* outSettings)
{
    auto path = getProjectFilePath(fileName);
    QFile file(path);
    qFatalIf(!file.open(QFile::ReadOnly | QFile::Text), "Failed to read " + fileName + " from project folder");
    QByteArray fileData = file.readAll();
    auto json = QJsonDocument::fromJson(fileData);
    qFatalIf(!json.isArray(), fileName + " is not an array");

    auto groupListField = new ListField(this);
    groupListField->set_uid(FIELD_GROUPS);
    groupListField->set_required(true);
    groupListField->set_hidden(true);
    fieldManager->rootField()->appendField(groupListField);

    auto locationField = new LocationField(this);
    locationField->set_uid(FIELD_LOCATION);
    locationField->set_required(true);
    locationField->set_allowManual(m_profile.value("MANUAL_GPS", false).toBool() || m_profile.value("ALLOW_SKIP_MANUAL_GPS", false).toBool());
    locationField->addTag("metadata", true);
    locationField->set_fixCount(m_profile.value("SIGHTING_FIX_COUNT", 0).toInt());
    fieldManager->rootField()->appendField(locationField);

    auto distanceField = new NumberField(this);
    distanceField->set_uid(FIELD_DISTANCE);
    distanceField->set_minValue(0.0);
    distanceField->set_maxValue(10000.0);
    distanceField->set_decimals(2);
    distanceField->set_hidden(true);
    fieldManager->rootField()->appendField(distanceField);

    auto bearingField = new NumberField(this);
    bearingField->set_uid(FIELD_BEARING);
    bearingField->set_minValue(0.0);
    bearingField->set_maxValue(359.0);
    bearingField->set_decimals(2);
    bearingField->set_hidden(true);
    fieldManager->rootField()->appendField(bearingField);

    auto observerField = new StringField(this);
    observerField->set_uid(FIELD_OBSERVER);
    observerField->set_hidden(true);
    observerField->set_required(true);
    observerField->set_listSorted(true);
    fieldManager->rootField()->appendField(observerField);

    auto sightingTypeField = new StringField(this);
    sightingTypeField->set_uid(FIELD_OBSERVATION_TYPE);
    sightingTypeField->set_required(true);
    sightingTypeField->set_hidden(true);
    sightingTypeField->addTag("metadata", true);
    fieldManager->rootField()->appendField(sightingTypeField);

    auto jsonArray = json.array();

    for (auto itemIt = jsonArray.constBegin(); itemIt != jsonArray.constEnd(); itemIt++)
    {
        qFatalIf(!itemIt->isObject(), "Expects an array of objects");
        auto itemObject = itemIt->toObject();
        qFatalIf(itemObject.count() != 1, "Expects each object to have one value");

        auto fieldName = itemObject.keys().constFirst();
        auto fieldValues = itemObject[fieldName].toObject().toVariantMap();

        // Found employees, so connect to the observer field.
        // Collect instances do not have employee lists, but keep the observer field around
        // so that there is less special casing between flavors.
        if (fieldName == "SMART_Employees")
        {
            observerField->set_listElementUid("SMART_Employees");

            // Filter employees for patrols and surveys based on those selected.
            if (!fileName.startsWith("incident"))
            {
                observerField->set_listFilterByField("SMART_Employees");
            }
        }

        // Make the sampling units look like other attributes - also correct for a typo coming from SMART.
        // Create a goto manager layer from the sampling unit data.
        if (fieldName == "SMART_SampingUnit" || fieldName == FIELD_SAMPLING_UNIT)
        {
            auto options = QVariantList();
            auto optionNull = QVariantMap();
            optionNull["label"] = "tr:None";
            optionNull["id"] = "__sampling_unit_uid__";
            optionNull["uuid"] = "null";
            options.append(optionNull);

            auto samplingUnits = itemObject[fieldName].toArray();
            auto gotoItems = QVariantList();

            for (auto samplingUnitIt = samplingUnits.constBegin(); samplingUnitIt != samplingUnits.constEnd(); samplingUnitIt++)
            {
                auto samplingUnitMap = samplingUnitIt->toObject().toVariantMap();
                auto id = samplingUnitMap["id"];
                samplingUnitMap["label"] = id;
                options.append(samplingUnitMap);

                auto gotoItem = QVariantMap();
                auto gotoItemProps = QVariantMap();
                gotoItemProps["id"] = id;
                gotoItem["geometry"] = samplingUnitMap["geometry"];
                gotoItem["type"] = "Feature";
                gotoItem["properties"] = gotoItemProps;
                gotoItems.append(gotoItem);
            }

            auto gotoLayer = QVariantMap();
            gotoLayer["decoder"] = "sourceparser_smartsurveylayer";
            gotoLayer["features"] = gotoItems;
            gotoLayer["filename"] = m_project->uid() + ".json";
            gotoLayer["name"] = m_project->title();
            gotoLayer["type"] = "FeatureCollection";
            App::instance()->gotoManager()->install(gotoLayer);

            fieldValues["default"] = optionNull["uuid"];
            fieldValues["isFixed"] = false;
            fieldValues["type"] = "SINGLE_CHOICE";
            fieldValues["isRequired"] = true;
            fieldValues["isVisible"] = true;
            fieldValues["options"] = options;

            fieldName = FIELD_SAMPLING_UNIT;
            outSettings->insert("samplingUnits", true);
        }

        addMetadataItem(fieldName, fieldValues, elementManager, fieldManager, outSettings);
    }
}

PhotoField* SMARTProvider::createPhotoField(const QString& fieldUidSuffix, bool required) const
{
    auto photoField = new PhotoField();
    photoField->set_uid(QString(required ? FIELD_PHOTOS_REQUIRED : FIELD_PHOTOS) + fieldUidSuffix);
    photoField->set_nameElementUid(FIELD_PHOTOS);
    photoField->set_exportUid(FIELD_PHOTO_PREFIX);
    photoField->set_required(required);
    photoField->set_minCount(0);
    photoField->set_maxCount(m_profile.value("MAX_PHOTO_COUNT", 3).toInt());

    auto resizeImage = m_profile.value("RESIZE_IMAGE", false).toBool();
    photoField->set_resolutionX(resizeImage ? m_profile.value("IMAGE_WIDTH").toInt() : DEFAULT_PHOTO_RESOLUTION_X);
    photoField->set_resolutionY(resizeImage ? m_profile.value("IMAGE_HEIGHT").toInt() : DEFAULT_PHOTO_RESOLUTION_Y);

    return photoField;
}

AudioField* SMARTProvider::createAudioField(const QString& fieldUidSuffix) const
{
    auto audioField = new AudioField();
    audioField->set_uid(QString(FIELD_AUDIO) + fieldUidSuffix);
    audioField->set_nameElementUid(FIELD_AUDIO);
    audioField->set_exportUid(FIELD_AUDIO_PREFIX);

    return audioField;
}

void SMARTProvider::parseModel(const QString& fileName, ElementManager* elementManager, FieldManager* fieldManager, QVariantMap* outSettings, QVariantList* outExtraData)
{
    auto modelField = new RecordField(this);
    modelField->set_uid(FIELD_MODEL);
    fieldManager->rootField()->appendField(modelField);

    auto groupField = new StringField(this);
    groupField->set_uid(FIELD_SIGHTING_GROUP_ID);
    modelField->appendField(groupField);

    auto treeField = new StringField(this);
    treeField->set_uid(FIELD_MODEL_TREE);
    treeField->set_listElementUid(FIELD_MODEL_TREE);
    modelField->appendField(treeField);

    auto patrolLegCounterField = new NumberField(this);
    patrolLegCounterField->set_uid(FIELD_PATROL_LEG_COUNTER);
    patrolLegCounterField->set_hidden(true);
    patrolLegCounterField->set_defaultValue(1);
    patrolLegCounterField->set_decimals(0);
    modelField->appendField(patrolLegCounterField);

    auto patrolLegDistanceField = new NumberField(this);
    patrolLegDistanceField->set_uid(FIELD_PATROL_LEG_DISTANCE);
    patrolLegDistanceField->set_hidden(true);
    patrolLegDistanceField->set_defaultValue(0);
    patrolLegDistanceField->set_decimals(4);
    modelField->appendField(patrolLegDistanceField);

    // Photo fields.
    auto photoElement = new Element();
    photoElement->set_uid(FIELD_PHOTOS);
    photoElement->set_name(FIELD_PHOTOS);
    elementManager->rootElement()->appendElement(photoElement);

    modelField->appendField(createPhotoField("", false));
    modelField->appendField(createPhotoField("", true));

    // Process the model.
    auto modelPath = getProjectFilePath(fileName);
    QFile modelFile(modelPath);
    qFatalIf(!modelFile.open(QFile::ReadOnly | QFile::Text), "Failed to read " + fileName + " from project folder");

    SMARTModelHandler modelHandler(fieldManager, modelField, elementManager, outSettings, outExtraData);
    QXmlSimpleReader xmlReader;
    xmlReader.setContentHandler(&modelHandler);
    xmlReader.setErrorHandler(&modelHandler);

    QXmlInputSource xmlInputSource(&modelFile);
    qFatalIf(!xmlReader.parse(xmlInputSource), "Failed to parse " + fileName);

    modelField->appendField(createAudioField(""));

    // Audio field.
    if (m_smartVersion >= 700)
    {
        auto audioElement = new Element();
        audioElement->set_uid(FIELD_AUDIO);
        audioElement->set_name(FIELD_AUDIO);
        elementManager->rootElement()->appendElement(audioElement);
    }

    // Complete model processing.
    postProcessModel(elementManager, fieldManager, modelField, outSettings);
}

void SMARTProvider::postProcessModel(ElementManager* elementManager, FieldManager* fieldManager, RecordField* modelField, QVariantMap* settings)
{
    // Creator for audio and photo fields.
    auto createAttachmentFields = [&](RecordField* recordField, const QString& suffix, bool allowed, bool required)
    {
        if (allowed)
        {
            recordField->insertField(0, createPhotoField(suffix, required));
        }

        if (allowed  && m_smartVersion >= 700)
        {
            recordField->appendField(createAudioField(suffix));
        }
    };

    // Handle complex nodes.
    auto leafElements = elementManager->getLeafElements(elementManager->rootElement()->uid());
    for (auto leafElementIt = leafElements.constBegin(); leafElementIt != leafElements.constEnd(); leafElementIt++)
    {
        auto leafElement = *leafElementIt;
        auto fieldUids = leafElement->fieldUids();
        if (fieldUids.isEmpty())
        {
            continue;
        }

        auto photoFirst = settings->value("photoFirst").toBool();
        auto photoAllowed = photoFirst || leafElement->getTagValue("photoAllowed") == "true";
        auto photoRequired = leafElement->getTagValue("photoRequired") == "true" && !photoFirst;
        auto audioAllowed = photoAllowed && m_smartVersion >= 700;
        auto handled = false;

        // Dynamic observations.
        if (leafElement->getTagValue("collectMultipleObs") == "true")
        {
            auto recordFieldUid = leafElement->uid() + ".dynamic";
            auto recordField = new RecordField();
            recordField->set_uid(recordFieldUid);
            modelField->appendField(recordField);

            auto newFieldUids = QStringList();

            for (auto i = 0; i < fieldUids.count(); i++)
            {
                auto field = fieldManager->getField(fieldUids[i]);
                auto enterOnce = field->getTagValue("ENTER_ONCE").toString();

                // If ENTER_ONCE is empty, then this field is part of the dynamic set.
                if (enterOnce.isEmpty())
                {
                    if (!newFieldUids.contains(recordFieldUid))
                    {
                        newFieldUids.append(recordFieldUid);
                    }

                    modelField->detachField(field);
                    recordField->appendField(field);
                }
                else
                {
                    newFieldUids.append(fieldUids[i]);

                    if (enterOnce == "END")
                    {
                        modelField->detachField(field);
                        modelField->appendField(field);
                    }
                }
            }

            if (m_smartVersion >= 700)
            {
                createAttachmentFields(recordField, "." + recordFieldUid, photoAllowed, photoRequired);
            }

            fieldUids = newFieldUids;

            // Cleanup sub-record if it is empty.
            // This is a GIGO case, but nothing prevents it and this feature is not easy to use.
            if (recordField->fieldCount() == 0)
            {
                modelField->detachField(recordField);
                delete recordField;
            }

            handled = true;
        }

        // Multi-select: does not co-exist with dynamic observations (above).
        if (!handled)
        {
            auto newFieldUids = QStringList();

            for (auto i = 0; i < fieldUids.count(); i++)
            {
                auto fieldUid = fieldUids[i];
                auto field = fieldManager->getField(fieldUid);
                newFieldUids.append(fieldUid);

                // Skip this for MLISTs, which are just simple check lists.
                if (field->getTagValue("MLIST").toBool())
                {
                    field->removeTag("MLIST");
                    continue;
                }

                // Look for check-list and number-list fields.
                if (field->listElementUid().isEmpty() || (!qobject_cast<CheckField*>(field) && !qobject_cast<NumberField*>(field)))
                {
                    continue;
                }

                auto recordFieldUid = leafElement->uid() + ".multiselect";
                newFieldUids.append(recordFieldUid);

                auto recordField = new RecordField();
                recordField->set_uid(recordFieldUid);
                recordField->set_nameElementUid(field->nameElementUid());
                recordField->set_masterFieldUid(fieldUid);

                // Move the fields into the record.
                for (auto j = i + 1; j < fieldUids.count(); j++)
                {
                    auto field = fieldManager->getField(fieldUids[j]);
                    modelField->detachField(field);
                    recordField->appendField(field);
                }

                if (m_smartVersion >= 700)
                {
                    createAttachmentFields(recordField, "." + recordFieldUid, photoAllowed, photoRequired);
                }

                fieldUids = newFieldUids;

                // Tag this field so that export can cheaply know to give it special treatment.
                field->addTag("master", true);

                // Hide the record if there are no fields.
                recordField->set_hidden(recordField->fieldCount() == 0);
                modelField->appendField(recordField);

                handled = true;
                break;
            }
        }

        // Matrix questions.
        if (!handled)
        {
            auto newFieldUids = QStringList();
            auto recordField = (RecordField*)nullptr;

            auto i = 0;
            while (i < fieldUids.count())
            {
                auto fieldUid = fieldUids[i];
                auto field = fieldManager->getField(fieldUid);

                // Skip normal fields.
                if (!field->getTagValue("INPUT_GROUP").toBool())
                {
                    newFieldUids.append(fieldUid);
                    i++;
                    continue;
                }

                // Matrix field detected: move into a separate record field.
                if (recordField == nullptr)
                {
                    auto recordFieldUid = leafElement->uid() + ".matrix";
                    recordField = new RecordField();
                    recordField->set_uid(recordFieldUid);
                    newFieldUids.append(recordFieldUid);
                    modelField->appendField(recordField);
                }

                modelField->detachField(field);
                recordField->appendField(field);

                i++;
            }

            // Compute the number of matrix dimensions for the record.
            if (recordField)
            {
                fieldUids = newFieldUids;

                auto matrixCount = 0;
                for (matrixCount = 0; matrixCount < recordField->fieldCount(); matrixCount++)
                {
                    if (recordField->field(matrixCount)->listElementUid().isEmpty())
                    {
                        break;
                    }

                    recordField->field(matrixCount)->set_hidden(true);
                }

                recordField->set_matrixCount(matrixCount);

                //handled = true;
            }
        }

        // Prepend a photo field as needed.
        if (photoAllowed)
        {
            fieldUids.insert(0, photoRequired ? FIELD_PHOTOS_REQUIRED : FIELD_PHOTOS);
        }

        // Append an audio field as needed.
        if (audioAllowed)
        {
            fieldUids.append(FIELD_AUDIO);
        }

        leafElement->set_fieldUids(fieldUids);
    }

    fieldManager->completeUpdate();
}

bool SMARTProvider::initialize()
{
    auto project = parseProject();
    auto languages = QVariantList();
    auto languageIndex = -1;

    // Initialize main.
    {
        ElementManager elementManager;
        FieldManager fieldManager;
        QVariantMap settings;

        auto elementsFilePath = getProjectFilePath("Elements.qml");
        elementManager.saveToQmlFile(elementsFilePath);

        auto fieldsFilePath = getProjectFilePath("Fields.qml");
        fieldManager.saveToQmlFile(fieldsFilePath);

        auto settingsFilePath = getProjectFilePath("Settings.json");
        auto jsonDoc = QJsonDocument::fromVariant(settings);
        Utils::writeJsonToFile(settingsFilePath, jsonDoc.toJson());
    }

    // Initialize Collect.
    if (m_hasCollect)
    {
        auto metadataFile = project["metadata"].toString();
        auto definitionFile = project["definition"].toString();

        ElementManager elementManager;
        FieldManager fieldManager;
        QVariantMap settings;
        QVariantList extraData;

        parseMetadata(metadataFile, &elementManager, &fieldManager, &settings);
        parseModel(definitionFile, &elementManager, &fieldManager, &settings, &extraData);
        languages = settings.value("languages").toList();
        languageIndex = settings.value("languageIndex").toInt();

        auto elementsFilePath = getProjectFilePath("Elements" + QString(TYPE_COLLECT) + ".qml");
        elementManager.saveToQmlFile(elementsFilePath);

        auto fieldsFilePath = getProjectFilePath("Fields" + QString(TYPE_COLLECT) + ".qml");
        fieldManager.saveToQmlFile(fieldsFilePath);

        auto settingsFilePath = getProjectFilePath("Settings" + QString(TYPE_COLLECT) + ".json");
        if (!QFile::exists(settingsFilePath))
        {
            auto jsonDoc = QJsonDocument::fromVariant(settings);
            Utils::writeJsonToFile(settingsFilePath, jsonDoc.toJson());
        }

        auto extraDataFilePath = getProjectFilePath("ExtraData" + QString(TYPE_COLLECT) + ".json");
        auto jsonDoc = QJsonDocument::fromVariant(extraData);
        Utils::writeJsonToFile(extraDataFilePath, jsonDoc.toJson());
    }

    // Initialize Patrol or Survey.
    if (m_hasPatrol)
    {
        auto metadataFile = project["metadata"].toString();
        auto definitionFile = project["definition"].toString();

        ElementManager elementManager;
        FieldManager fieldManager;
        QVariantMap settings;
        QVariantList extraData;

        parseMetadata(metadataFile, &elementManager, &fieldManager, &settings);
        parseModel(definitionFile, &elementManager, &fieldManager, &settings, &extraData);
        languages = settings.value("languages").toList();
        languageIndex = settings.value("languageIndex").toInt();

        auto elementsFilePath = getProjectFilePath("Elements" + QString(TYPE_PATROL) + ".qml");
        elementManager.saveToQmlFile(elementsFilePath);

        auto fieldsFilePath = getProjectFilePath("Fields" + QString(TYPE_PATROL) + ".qml");
        fieldManager.saveToQmlFile(fieldsFilePath);

        auto settingsFilePath = getProjectFilePath("Settings" + QString(TYPE_PATROL) + ".json");
        if (!QFile::exists(settingsFilePath))
        {
            auto jsonDoc = QJsonDocument::fromVariant(settings);
            Utils::writeJsonToFile(settingsFilePath, jsonDoc.toJson());
        }

        auto extraDataFilePath = getProjectFilePath("ExtraData" + QString(TYPE_PATROL) + ".json");
        auto jsonDoc = QJsonDocument::fromVariant(extraData);
        Utils::writeJsonToFile(extraDataFilePath, jsonDoc.toJson());
    }

    // Initialize Incident.
    if (m_hasIncident)
    {
        auto metadataFile = project["metadata"].toString();
        auto definitionFile = project["definition"].toString();

        if (project.contains("incident"))
        {
            auto incident = project["incident"].toMap();
            metadataFile = incident["metadata"].toString();
            definitionFile = incident["definition"].toString();
        }

        ElementManager elementManager;
        FieldManager fieldManager;
        QVariantMap settings;
        QVariantList extraData;

        parseMetadata(metadataFile, &elementManager, &fieldManager, &settings);
        parseModel(definitionFile, &elementManager, &fieldManager, &settings, &extraData);
        languages = settings.value("languages").toList();
        languageIndex = settings.value("languageIndex").toInt();

        auto elementsFilePath = getProjectFilePath("Elements" + QString(TYPE_INCIDENT) + ".qml");
        elementManager.saveToQmlFile(elementsFilePath);

        auto fieldsFilePath = getProjectFilePath("Fields" + QString(TYPE_INCIDENT) + ".qml");
        fieldManager.saveToQmlFile(fieldsFilePath);

        auto settingsFilePath = getProjectFilePath("Settings" + QString(TYPE_INCIDENT) + ".json");
        if (!QFile::exists(settingsFilePath))
        {
            auto jsonDoc = QJsonDocument::fromVariant(settings);
            Utils::writeJsonToFile(settingsFilePath, jsonDoc.toJson());
        }

        auto extraDataFilePath = getProjectFilePath("ExtraData" + QString(TYPE_INCIDENT) + ".json");
        auto jsonDoc = QJsonDocument::fromVariant(extraData);
        Utils::writeJsonToFile(extraDataFilePath, jsonDoc.toJson());
    }

    // Update languages.
    m_project->set_languages(languages);
    if (m_project->languageIndex() == -1)
    {
        m_project->set_languageIndex(languageIndex);
    }

    return true;
}

bool SMARTProvider::connectToProject(bool newBuild)
{
    m_taskManager->registerTask(UploadSightingTask::getName(), UploadSightingTask::createInstance, UploadSightingTask::completed);

    m_profile = Utils::variantMapFromJsonFile(getProjectFilePath("ct_profile.json"));

    auto project = parseProject();
    m_smartVersion = qFloor(project.value("smart_version", 6.00).toDouble() * 100);

    auto metadataFile = project.value("metadata").toString();
    m_hasCollect = metadataFile == "smartcollect_metadata.json";
    m_hasPatrol = (metadataFile == "patrol_metadata.json") || (metadataFile == "survey_metadata.json");
    m_hasIncident = (metadataFile == "incident_metadata.json") || project.contains("incident");

    m_hasDataServer = m_profile.contains("DATA_SERVER");
    m_uploadFrequencyMinutes = m_profile.value("DATA_SERVER").toMap().value("FREQUENCY_MIN", -1).toInt();
    m_manualUpload = m_hasDataServer && (m_uploadFrequencyMinutes == -1);

    auto form = qobject_cast<Form*>(parent());
    auto stateSpace = form->stateSpace();
    m_patrolForm = stateSpace == TYPE_PATROL;
    m_incidentForm = stateSpace == TYPE_INCIDENT;
    m_collectForm = stateSpace == TYPE_COLLECT;

    auto elementsFilePath = getProjectFilePath("Elements" + stateSpace + ".qml");
    auto fieldsFilePath = getProjectFilePath("Fields" + stateSpace + ".qml");
    auto settingsFilePath = getProjectFilePath("Settings" + stateSpace + ".json");
    auto extraDataFilePath = getProjectFilePath("ExtraData" + stateSpace + ".json");

    // Convert projection units from SMART enum to the coordinateFormat enum.
    // This overrides the global app setting, but that is probably desirable.
    {
        auto projectionMap = QMap<int, int>
        {
            { 0, 1 }, // Degrees Minutes Seconds
            { 1, 0 }, // Decimal Degrees
            { 2, 3 }, // UTM
        };

        auto coordinateFormat = projectionMap.value(m_profile.value("PROJECTION", 1).toInt(), 0);

        App::instance()->settings()->set_coordinateFormat(coordinateFormat);
    }

    // Rebuild the cache if needed.
    if (!CACHE_ENABLE || newBuild ||
        !QFile::exists(elementsFilePath) || !QFile::exists(fieldsFilePath) || !QFile::exists(settingsFilePath))
    {
        initialize();
    }

    // Misc properties.
    auto settings = Utils::variantMapFromJsonFile(settingsFilePath);
    m_extraData = Utils::variantListFromJsonFile(extraDataFilePath);
    m_surveyMode = project.value("metadata").toString().contains("survey");
    m_samplingUnits = settings.value("samplingUnits", false).toBool();
    m_instantGPS = settings.value("instantGPS", false).toBool();
    m_allowManualGPS = m_profile.value("MANUAL_GPS", false).toBool() || m_profile.value("ALLOW_SKIP_MANUAL_GPS", false).toBool();
    m_useDistanceAndBearing = m_profile.value("RECORD_DISTANCE_BEARING", false) == true;
    m_useGroupUI = m_profile.value("INCIDENT_GROUP_UI", false) == true;
    m_useObserver = m_profile.value("RECORD_OBSERVER", false) == true;

    // Load the state.
    loadState();

    // Sighting saved.
    connect(form, &Form::sightingSaved, this, [=](const QString& sightingUid)
    {
        if (!shouldUploadData())
        {
            return;
        }

        uploadSighting(form, sightingUid, &m_uploadObsCounter);
        uploadPendingLocations(form);
        uploadAlerts(form, sightingUid);
        saveState();
    });

    // Location saved.
    connect(form, &Form::locationTrack, this, [=](const QVariantMap& locationMap, const QString& locationUid)
    {
        // Update the patrol leg.
        {
            auto trackDistance = form->trackStreamer()->distanceCovered();
            form->setFieldValue(FIELD_PATROL_LEG_DISTANCE, trackDistance);
            setPatrolLegValue("distance", trackDistance);
            saveState();
        }

        // No track point was saved, so nothing more to do.
        if (locationUid.isEmpty())
        {
            return;
        }

        auto projectUid = m_project->uid();

        // If the location happens outside of a patrol, just delete it. While this should not happen,
        // the track points may come through at any time and this eliminates a source of bugs.
        if (!m_patrolStarted || m_patrolPaused)
        {
            m_database->deleteSighting(projectUid, locationUid);
            return;
        }

        // Skip if before the start of the patrol.
        // Skip if too close to a sighting - since the sighting also emits a track point.
        auto locationTimestamp = locationMap["ts"].toString();
        auto timeSinceStart = Utils::timestampDeltaMs(m_patrolStartTimestamp, locationTimestamp);
        auto timeSinceLastUpload = Utils::timestampDeltaMs(m_uploadSightingLastTime, locationTimestamp);

        if (timeSinceStart < 0 || timeSinceLastUpload < 1000)
        {
            m_database->deleteSighting(projectUid, locationUid);
            return;
        }

        // Batch behavior for locations.
        if (shouldUploadData())
        {
            // Check for no batching.
            if (m_uploadFrequencyMinutes == 0)
            {
                uploadLocation(form, locationUid);
                return;
            }

            // Mark the location as pending.
            m_database->setSightingFlags(projectUid, locationUid, Sighting::DB_PENDING_FLAG);

            // Check the timeout.
            auto currentTime = App::instance()->timeManager()->currentDateTimeISO();
            if (m_uploadLastTime.isEmpty())
            {
                m_uploadLastTime = currentTime;
                saveState();
                return;
            }

            // Upload if the timeout has been met.
            if (Utils::timestampDeltaMs(m_uploadLastTime, currentTime) >= m_uploadFrequencyMinutes * 60 * 1000)
            {
                uploadPendingLocations(form);
                saveState();
                return;
            }
        }
    });

    // Pings.
    connect(form, &Form::locationPoint, this, [=](const QVariantMap& locationMap)
    {
        if (!shouldUploadData())
        {
            return;
        }

        if ((m_patrolStarted && !m_patrolPaused) || m_collectStarted)
        {
            uploadPing(locationMap);
        }
    });

    // Sync state across all forms.
    connect(form, &Form::projectSettingChanged, this, [&](const QString& name, const QVariant& /*value*/)
    {
        if (name == "providerState")
        {
            loadState();
        }
    });

    return true;
}

QUrl SMARTProvider::getStartPage()
{
    auto form = qobject_cast<Form*>(parent());
    auto stateSpace = form->stateSpace();

    if (stateSpace.isEmpty())
    {
        if (m_hasCollect)
        {
            return QUrl("qrc:/SMART/StartPageCollect.qml");
        }
        else
        {
            return QUrl("qrc:/SMART/StartPage.qml");
        }
    }
    else if (stateSpace == TYPE_INCIDENT)
    {
        return QUrl("qrc:/SMART/SightingHomePage.qml");
    }
    else if (stateSpace == TYPE_PATROL)
    {
        if (form->editing())
        {
            return QUrl("qrc:/SMART/SightingHomePage.qml");
        }
        else if (!m_patrolStarted)
        {
            form->resetFieldValue(FIELD_LOCATION);
            return QUrl("qrc:/SMART/MetadataPage.qml");
        }
        else if (m_patrolPaused)
        {
            return QUrl("qrc:/SMART/LocationPage.qml");
        }

        // Should not get here...
        return QUrl("qrc:/SMART/SightingHomePage.qml");
    }
    else if (stateSpace == TYPE_COLLECT)
    {
        return QUrl("qrc:/SMART/SightingHomePage.qml");
    }

    qFatal("stateSpace not recognized");

    return QUrl();
}

void SMARTProvider::getElements(ElementManager* elementManager)
{
    auto form = qobject_cast<Form*>(parent());
    auto stateSpace = form->stateSpace();
    elementManager->loadFromQmlFile(getProjectFilePath("Elements" + stateSpace + ".qml"));
}

void SMARTProvider::getFields(FieldManager* fieldManager)
{
    auto form = qobject_cast<Form*>(parent());
    auto stateSpace = form->stateSpace();
    fieldManager->loadFromQmlFile(getProjectFilePath("Fields" + stateSpace + ".qml"));
}

QVariantList SMARTProvider::buildMapDataLayers()
{
    auto result = QVariantList();

    // Incident.
    if (m_hasIncident)
    {
        auto form = App::instance()->createFormPtr(m_project->uid(), TYPE_INCIDENT);
        auto getSightingSymbol = [](const Sighting* /*sighting*/) -> QVariantMap
        {
            auto symbol = QVariantMap();
            symbol["symbolType"] = "PictureMarkerSymbol";
            symbol["angle"] = 0.0;
            symbol["type"] = "esriPMS";
            symbol["url"] = "qrc:/icons/map_marker.png";
            symbol["width"] = 24;
            symbol["height"] = 24;
            symbol["yoffset"] = 12.0;

            return symbol;
        };

        result.append(form->buildSightingMapLayer(tr("Incident"), getSightingSymbol));
    }

    // Patrol.
    if (m_hasPatrol)
    {
        auto form = App::instance()->createFormPtr(m_project->uid(), TYPE_PATROL);

        // Sightings.
        auto getSightingSymbol = [](const Sighting* /*sighting*/) -> QVariantMap
        {
            auto symbol = QVariantMap();
//            auto observationType = sighting->getRootFieldValue(FIELD_OBSERVATION_TYPE).toString();

// TODO(justin): get different markers for different patrol parts
//            auto map = QVariantMap();
//            map[VALUE_NEW_PATROL] = 0;
//            map[VALUE_STOP_PATROL] = 15;
//            map[VALUE_PAUSE_PATROL] = 30;
//            map[VALUE_RESUME_PATROL] = 45;
//            map[VALUE_CHANGE_PATROL] = 60;
//            map[VALUE_OBSERVATION] = 90;

            symbol["symbolType"] = "PictureMarkerSymbol";
            symbol["angle"] = 0;//map[observationType].toDouble();
            symbol["type"] = "esriPMS";
            symbol["url"] = "qrc:/icons/map_marker.png";
            symbol["width"] = 24;
            symbol["height"] = 24;
            symbol["yoffset"] = 12.0;

            return symbol;
        };

        result.append(form->buildSightingMapLayer(tr("Patrol"), getSightingSymbol));

        // Track.
        auto getTrackSymbol = [&]() -> QVariantMap
        {
            auto symbol = QVariantMap();
            symbol["symbolType"] = "SimpleLineSymbol";

            auto trackColor = QColor("#" + getProfileValue("TRACK_COLOR", "0000ff").toString());
            auto symbolColor = QVariantList();
            symbolColor.append(trackColor.red());
            symbolColor.append(trackColor.green());
            symbolColor.append(trackColor.blue());
            symbolColor.append(200);

            symbol["color"] = symbolColor;
            symbol["style"] = "esriSLSDot";
            symbol["type"] = "esriSLS";
            symbol["width"] = 1.75;

            return symbol;
        };

        result.append(form->buildTrackMapLayer(tr("Tracks"), getTrackSymbol()));
    }

    // Collect.
    if (m_hasCollect)
    {
        auto form = App::instance()->createFormPtr(m_project->uid(), TYPE_COLLECT);

        // Sightings.
        auto getSightingSymbol = [](const Sighting* /*sighting*/) -> QVariantMap
        {
            auto symbol = QVariantMap();
            //auto observationType = sighting->getRootFieldValue(FIELD_OBSERVATION_TYPE).toString();
            symbol["symbolType"] = "PictureMarkerSymbol";
            symbol["angle"] = 0;
            symbol["type"] = "esriPMS";
            symbol["url"] = "qrc:/icons/map_marker.png";
            symbol["width"] = 24;
            symbol["height"] = 24;
            symbol["yoffset"] = 12.0;

            return symbol;
        };

        result.append(form->buildSightingMapLayer(tr("Collect"), getSightingSymbol));

        // Track.
        auto getTrackSymbol = [&]() -> QVariantMap
        {
            auto symbol = QVariantMap();
            symbol["symbolType"] = "SimpleLineSymbol";

            auto trackColor = QColor("#" + getProfileValue("TRACK_COLOR", "0000ff").toString());
            auto symbolColor = QVariantList();
            symbolColor.append(trackColor.red());
            symbolColor.append(trackColor.green());
            symbolColor.append(trackColor.blue());
            symbolColor.append(200);

            symbol["color"] = symbolColor;
            symbol["style"] = "esriSLSDot";
            symbol["type"] = "esriSLS";
            symbol["width"] = 1.75;

            return symbol;
        };

        result.append(form->buildTrackMapLayer(tr("Tracks"), getTrackSymbol()));
    }

    return result;
}

bool SMARTProvider::getUseGPSTime()
{
    return m_profile.value("USE_GPS_TIME", false) == true;
}

bool SMARTProvider::canEditSighting(Sighting* sighting, int /*flags*/)
{
    return !shouldUploadData() &&
           sighting->recordManager()->recordUids().count() > 1 &&
           getProfileValue("DISABLE_EDITING", false) == false;
}

void SMARTProvider::finalizeSighting(QVariantMap& sightingMap)
{
    auto pages = sightingMap.value("pages").toList();
    if (pages.isEmpty())
    {
        return;
    }

    auto lastPage = pages.last().toMap();
    if (lastPage.value("pageUrl").toString().contains("LocationPage.qml"))
    {
        pages.removeLast();
        sightingMap["pages"] = pages;
    }
}

bool SMARTProvider::shouldUploadData()
{
    return m_hasDataServer && !m_manualUpload;
}

QString SMARTProvider::getFieldName(const QString& fieldUid)
{
    // Handle simple static fields.
    auto map = QVariantMap
    {
        { FIELD_LOCATION,        tr("Location")      },
        { FIELD_DISTANCE,        tr("Distance") + " (m)" },
        { FIELD_BEARING,         tr("Bearing") + " (°)"  },
        { FIELD_PHOTOS,          tr("Photos")        },
        { FIELD_PHOTOS_REQUIRED, tr("Photos")        },
        { FIELD_AUDIO,           tr("Audio note")    },
        { FIELD_SAMPLING_UNIT,   tr("Sampling unit") },
        { FIELD_OBSERVER,        tr("Observer")      },
    };

    auto result = map.value(fieldUid).toString();
    if (!result.isEmpty())
    {
        return result;
    }

    // Field name -> English name
    // Used to detect English on non-English systems.
    static auto enMap = QVariantMap
    {
        { "SMART_PatrolTransport", "Patrol Transport Type" },
        { "SMART_Armed",           "Is Armed"              },
        { "SMART_Team",            "Team"                  },
        { "SMART_Station",         "Station"               },
        { "SMART_Mandate",         "Mandate"               },
        { "SMART_Objective",       "Objective"             },
        { "SMART_Comments",        "Comment"               },
        { "SMART_Employees",       "Employees"             },
        { "SMART_Members",         "Members"               },
        { "SMART_Leader",          "Leader"                },
        { "SMART_Pilot",           "Pilot"                 },
        { "SMART_SamplingUnit",    "Sampling Unit"         },
    };

    // Field name -> translated name.
    auto trMap = QVariantMap
    {
        { "SMART_PatrolTransport", tr("Transport Type")    },
        { "SMART_Armed",           tr("Is Armed")          },
        { "SMART_Team",            tr("Team")              },
        { "SMART_Station",         tr("Station")           },
        { "SMART_Mandate",         tr("Mandate")           },
        { "SMART_Objective",       tr("Objective")         },
        { "SMART_Comments",        tr("Comment")           },
        { "SMART_Employees",       tr("Employees")         },
        { "SMART_Members",         tr("Members")           },
        { "SMART_Leader",          tr("Leader")            },
        { "SMART_Pilot",           tr("Pilot")             },
        { "SMART_SamplingUnit",    tr("Sampling Unit")     },
    };

    qFatalIf(enMap.count() != trMap.count(), "Bad map sizes");

    // Check if we care about this field.
    if (!enMap.contains(fieldUid))
    {
        return QString();
    }

    // Lookup the field.
    auto field = m_fieldManager->getField(fieldUid);
    if (!field)
    {
        return QString();
    }

    // Lookup the name Element.
    auto nameElementUid = field->nameElementUid();
    if (nameElementUid.isEmpty())
    {
        return QString();
    }

    auto nameElement = m_elementManager->getElement(nameElementUid);
    if (!nameElement)
    {
        return QString();
    }

    // If the name Element has "names" set, then we are on SMART 7 and the server has provided translations.
    if (!nameElement->names().isEmpty())
    {
        return QString();
    }

    // If the name does not match the English name we have, then it must have been translated by
    // SMART desktop via a language pack, so leave it alone.
    auto form = qobject_cast<Form*>(parent());
    auto name = m_elementManager->getElementName(nameElementUid, form->languageCode());

    if (name != enMap.value(fieldUid))
    {
        return QString();
    }

    // The name matches the English exactly, so use the translation.
    return trMap.value(fieldUid).toString();
}

QVariantList SMARTProvider::buildSightingView(Sighting* sighting)
{
    auto recordModel = QVariantList();
    auto form = qobject_cast<Form*>(parent());

    auto groupList = sighting->getRootFieldValue(FIELD_GROUPS).toStringList();
    auto recordUids = sighting->getRootFieldValue(FIELD_MODEL).toStringList();
    auto rootRecordUid = sighting->rootRecordUid();

    // Add timestamp.
    recordModel.append(QVariantMap {
        { "delegate", "timestamp" },
        { "createTime", sighting->createTime() } });

    // Add location.
    auto locationFieldValue = sighting->getRootFieldValue(FIELD_LOCATION);
    if (locationFieldValue.isValid())
    {
        recordModel.append(QVariantMap {
            { "delegate", "location" },
            { "value", locationFieldValue } });
    }

    // Add observer.
    if (m_useObserver)
    {
        auto observer = sighting->getRootFieldValue(FIELD_OBSERVER).toString();
        if (!observer.isEmpty())
        {
            recordModel.append(QVariantMap {
                { "delegate", "field" },
                { "fieldName", getFieldName(FIELD_OBSERVER) },
                { "fieldValue", sighting->getRecord(rootRecordUid)->getFieldDisplayValue(FIELD_OBSERVER) },
                { "recordUid", rootRecordUid },
                { "depth", 1 } });
        }
    }

    // Add distance and bearing.
    if (m_useDistanceAndBearing)
    {
        auto distance = sighting->getRootFieldValue(FIELD_DISTANCE);
        auto bearing = sighting->getRootFieldValue(FIELD_BEARING);
        if (distance.isValid() && bearing.isValid())
        {
            recordModel.append(QVariantMap {
                { "delegate", "field" },
                { "fieldName", getFieldName(FIELD_DISTANCE) },
                { "fieldValue", distance },
                { "recordUid", rootRecordUid },
                { "depth", 1 } });

            recordModel.append(QVariantMap {
                { "delegate", "field" },
                { "fieldName", getFieldName(FIELD_BEARING) },
                { "fieldValue", bearing },
                { "recordUid", rootRecordUid },
                { "depth", 1 } });
        }
    }

    // Build records group at a time.
    auto modelStart = recordModel.count();

    if (!groupList.isEmpty())
    {
        for (auto groupIt = groupList.constBegin(); groupIt != groupList.constEnd(); groupIt++)
        {
            auto groupUid = *groupIt;

            recordModel.append(QVariantMap {
                { "delegate", "group" },
                { "caption", m_useGroupUI ? tr("Group") + " " + QString::number(groupList.indexOf(groupUid) + 1) : tr("Observation") }
            });

            auto modelFieldUids = QStringList();

            sighting->buildRecordModel(&recordModel, [&](const FieldValue& fieldValue, int depth) -> bool
            {
                if (depth == 0)
                {
                    return false;
                }

                // Skip records from other groups.
                auto groupRecordUid = depth == 1 ? fieldValue.recordUid() : fieldValue.record()->parentRecordUid();
                if (sighting->getRecord(groupRecordUid)->getFieldValue(FIELD_SIGHTING_GROUP_ID) != groupUid)
                {
                    return false;
                }

                // Skip group id.
                if (fieldValue.fieldUid() == FIELD_SIGHTING_GROUP_ID)
                {
                    return false;
                }

                // Special handling for the model row.
                if (fieldValue.fieldUid() == FIELD_MODEL_TREE)
                {
                    auto elementUid = fieldValue.value().toString();

                    recordModel.append(QVariantMap {
                        { "delegate", "element" },
                        { "elementUid", elementUid } });
                    modelFieldUids = form->getElement(elementUid)->fieldUids();
                    return false;
                }

                // Skip fields which are not part of the selected model. This happens when defaults are specified.
                if (depth == 1 && modelFieldUids.indexOf(fieldValue.fieldUid()) == -1)
                {
                    return false;
                }

                return true;
            });
        }
    }

    // Fancy logic to add headers and dividers to make complex records more readable.
    auto modelIndex = modelStart;
    while (modelIndex <= recordModel.count() - 1)
    {
       auto lastMap = recordModel.at(modelIndex - 1).toMap();
       auto lastDepth = lastMap.value("depth", 0).toInt();
       auto lastRecordUid = lastMap.value("recordUid").toString();
       auto currMap = recordModel.at(modelIndex).toMap();
       auto currDepth = currMap.value("depth", 0).toInt();
       auto currRecordUid = currMap.value("recordUid").toString();

       auto addDivider = false;
       addDivider = addDivider || (currDepth > 1 && lastMap.value("depth") != currMap.value("depth"));
       addDivider = addDivider || (currDepth > 1 && lastMap.value("recordUid") != currMap.value("recordUid"));
       addDivider = addDivider || (lastDepth > currDepth && lastDepth > 1);
       if (addDivider)
       {
           lastMap["divider"] = true;
           recordModel.replace(modelIndex - 1, lastMap);
       }

       auto addHeader = currDepth > 1 && lastRecordUid != currRecordUid;
       if (addHeader)
       {
           recordModel.insert(modelIndex, QVariantMap {
               { "delegate", "recordHeader" },
               { "text", sighting->getRecord(currRecordUid)->name() },
               { "divider", true } });
           modelIndex++;
       }

       modelIndex++;
    }

    // Add patrol metadata.
    if (form->stateSpace() == TYPE_PATROL)
    {
        // Add title.
        auto map = QVariantMap();
        map["delegate"] = "group";
        auto observationType = sighting->getRootFieldValue(FIELD_OBSERVATION_TYPE).toString();
        map["caption"] = getSightingTypeText(observationType);
        recordModel.append(map);

        // Add leg data if on patrol.
        auto legText = "#" + sighting->getRootFieldValue(FIELD_PATROL_LEG_COUNTER).toString() + " - " +
                       App::instance()->getDistanceText(sighting->getRootFieldValue(FIELD_PATROL_LEG_DISTANCE, 0.0).toDouble());

        recordModel.append(QVariantMap {
            { "delegate", "field" },
            { "fieldName", tr("Leg") },
            { "fieldValue", legText },
            { "recordUid", rootRecordUid },
            { "depth", 1 } });

        sighting->buildRecordModel(&recordModel, [&](const FieldValue& fieldValue, int depth) -> bool
        {
            if (depth != 0)
            {
                return false;
            }

            if (fieldValue.fieldUid() == FIELD_LOCATION)
            {
                return false;
            }

            if (fieldValue.recordUid() != rootRecordUid)
            {
                return false;
            }

            if (!fieldValue.field()->getTagValue("metadata").toBool())
            {
                return false;
            }

            return true;
        });
    }

    return recordModel;
}

QString SMARTProvider::getPatrolText()
{
    return m_surveyMode ? tr("Survey") : tr("Patrol");
}

QString SMARTProvider::getSightingTypeText(const QString& observationType)
{
    if (observationType == VALUE_NEW_PATROL)
    {
        return m_surveyMode ? tr("Start survey") : tr("Start patrol");
    }
    else if (observationType == VALUE_STOP_PATROL)
    {
        return m_surveyMode ? tr("End survey") : tr("End patrol");
    }
    else if (observationType == VALUE_PAUSE_PATROL)
    {
        return m_surveyMode ? tr("Pause survey") : tr("Pause patrol");
    }
    else if (observationType == VALUE_RESUME_PATROL)
    {
        return m_surveyMode ? tr("Resume survey") : tr("Resume patrol");
    }
    else if (observationType == VALUE_CHANGE_PATROL)
    {
        return m_surveyMode ? tr("Change survey") : tr("Change patrol");
    }
    else if (observationType == VALUE_OBSERVATION)
    {
        return m_surveyMode ? tr("Survey") : tr("Patrol");
    }
    else
    {
        return "";
    }
}

bool SMARTProvider::getFieldTitleVisible(const QString& fieldUid)
{
    return fieldUid != "Photos";
}

bool SMARTProvider::finalizePackage(const QString& packageFilesPath) const
{
    auto types = QStringList { "", TYPE_INCIDENT, TYPE_PATROL, TYPE_COLLECT };

    for (auto it = types.constBegin(); it != types.constEnd(); it++)
    {
        QFile::remove(packageFilesPath + "/Elements" + *it + ".qml");
        QFile::remove(packageFilesPath + "/Fields" + *it + ".qml");
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SMARTModelHandler

SMARTModelHandler::SMARTModelHandler(FieldManager* fieldManager, RecordField* modelField, ElementManager* elementManager, QVariantMap* outSettings, QVariantList* outExtraData)
{
    m_fieldManager = fieldManager;
    m_modelField = modelField;
    m_elementManager = elementManager;
    m_settings = outSettings;
    m_extraData = outExtraData;
}

SMARTModelHandler::~SMARTModelHandler()
{

}

bool SMARTModelHandler::matchParent(const QString& parentAttributeName)
{
    return m_stack.last().compare(parentAttributeName) == 0;
}

Element* SMARTModelHandler::getCurrentElement()
{
    return m_elementStack.last();
}

void SMARTModelHandler::elementFromXml(Element* element, const QXmlAttributes& attributes, const QString& exportUidPrefix)
{
    auto tag = element->tag();
    auto id = QString();
    auto dmUuid = QString();

    for (int i = 0; i < attributes.length(); i++)
    {
        auto name = attributes.qName(i);
        auto value = attributes.value(i);
        if (name == "id")
        {
            id = value;
            element->set_uid(value);
        }
        else if (name == "imageFile")
        {
            element->set_icon(value);
        }
        else if (name == "dmUuid")
        {
            dmUuid = value;
            if (!exportUidPrefix.isNull())
            {
                element->set_exportUid(exportUidPrefix + value);
            }
        }
        else if (name == "isActive")
        {
            element->set_hidden(value == "false");
        }
        else if (name.startsWith("photo"))
        {
            tag.insert(name, value);
        }
        else if (name == "collectMultipleObs" && value != "false")
        {
            tag.insert(name, value);
        }
    }

    element->set_tag(tag);

    // Default values for lists and trees are specified with a dmUuid, but we cannot use dmUuid as an
    // element id, because it is not unique. This code remaps the dmUuid to the uid for the default values
    // for lists and trees. Note that Elements are populated after all the fields have been specified.
    if (!id.isEmpty() && !dmUuid.isEmpty())
    {
         auto remaps = m_remapDefaultValue.values(dmUuid);
         if (remaps.count() > 0)
         {
             for (auto remapIt = remaps.begin(); remapIt != remaps.end(); remapIt++)
             {
                 // Hack start: fix the default value in the check field list. The default value is specified as a comma-separated string.
                 // This converts it to a map, so that this behavior (default value as string instead of a map) does not leak into the engine.
                 auto checkField = qobject_cast<CheckField*>(*remapIt);
                 if (checkField && !checkField->listElementUid().isEmpty())
                 {
                     auto defaultValue = (*remapIt)->defaultValue();
                     auto defaultValueMap = defaultValue.toMap();

                     // Convert the default value to a map as needed.
                     if (!defaultValue.toString().isEmpty())
                     {
                         auto defaultValues = defaultValue.toString().split(',');
                         for (auto it = defaultValues.constBegin(); it != defaultValues.constEnd(); it++)
                         {
                            defaultValueMap.insert(*it, true);
                         }

                         (*remapIt)->set_defaultValue(defaultValueMap);
                     }

                     // Remap the current element.
                     defaultValueMap.remove(dmUuid);
                     defaultValueMap.insert(id, true);
                     (*remapIt)->set_defaultValue(defaultValueMap);

                     continue;
                 }
                 // Hack end.

                 (*remapIt)->set_defaultValue(id);
             }

             m_remapDefaultValue.remove(dmUuid);
         }
     }
}

bool SMARTModelHandler::startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& attributes)
{
    Q_UNUSED(namespaceURI)
    Q_UNUSED(localName)

    if (qName == "ConfigurableModel")
    {
        auto element = new Element(nullptr);
        elementFromXml(element, attributes);
        element->set_uid(FIELD_MODEL_TREE);
        m_elementManager->rootElement()->appendElement(element);
        m_elementStack.append(element);

        m_settings->insert("instantGPS", attributes.value("instantGps") == "true");
        m_settings->insert("photoFirst", attributes.value("photoFirst") == "true");
    }
    else if (qName == "language")
    {
        qFatalIf(!matchParent("languages"), "language has wrong parent");
        auto languageCode = attributes.value("code");
        qFatalIf(languageCode.isEmpty(), "Missing language code");
        m_languages.append(languageCode);
        if (attributes.value("is_default") == "true")
        {
            m_defaultLanguage = languageCode;
        }
    }
    else if (qName == "signatureType")
    {
        qFatalIf(!matchParent("signatures"), "signatureType has wrong parent");
        auto element = new Element(nullptr);
        auto uid = attributes.value("uuid");
        element->set_uid(uid);
        m_elementManager->rootElement()->appendElement(element);
        m_elementStack.append(element);

        auto field = new SketchField();
        field->set_uid(uid);
        field->set_nameElementUid(uid);
        field->set_exportUid("SMART_Signature_" + attributes.value("keyid"));
        field->addTag("ENTER_ONCE", "END"); // Signatures can only be at the root level.
        m_modelField->appendField(field);
    }
    else if (qName == "signature")
    {
        // Add this field to the current uid list.
        auto signatureUid = attributes.value("uuid");
        m_nodeStack.last()->appendFieldUid(signatureUid);
    }
    else if (qName == "node")
    {
        qFatalIf(!matchParent("nodes") && !matchParent("node"), "node has wrong parent");
        auto element = new Element(nullptr);
        elementFromXml(element, attributes, "");
        m_elementStack.last()->appendElement(element);
        m_elementStack.append(element);
        m_nodeStack.append(element);
    }
    else if (qName == "name")
    {
        if (matchParent("node") || matchParent("ConfigurableModel") || matchParent("attributeConfig") ||
            matchParent("listItem") || matchParent("treeNode") || matchParent("children") || matchParent("attribute") ||
            matchParent("signatureType"))
        {
            auto jsonObject = m_elementStack.last()->names();
            jsonObject[attributes.value("language_code")] = attributes.value("value");
            m_elementStack.last()->set_names(jsonObject);
        }
    }
    else if (qName == "attributeConfig")
    {
        qFatalIf(!matchParent("ConfigurableModel"), "attributeConfig in wrong place");
        auto element = new Element(nullptr);
        elementFromXml(element, attributes);
        m_elementManager->rootElement()->appendElement(element);
        m_elementStack.append(element);
    }
    else if (qName == "listItem")
    {
        qFatalIf(!matchParent("attributeConfig") && !matchParent("treeNode") && !matchParent("children"), "attributeConfig child has wrong parent");
        auto element = new Element(nullptr);
        elementFromXml(element, attributes, "l:");
        m_elementStack.last()->appendElement(element);
        m_elementStack.append(element);
    }
    else if ((qName == "treeNode") || (qName == "children"))
    {
        qFatalIf(!matchParent("attributeConfig") && !matchParent("treeNode") && !matchParent("children"), "attributeConfig child has wrong parent");
        auto element = new Element(nullptr);
        elementFromXml(element, attributes, "t:");
        m_elementStack.last()->appendElement(element);
        m_elementStack.append(element);
    }
    else if (qName == "attribute")
    {
        // Create name Element.
        qFatalIf(!matchParent("node"), "attribute has wrong parent");
        auto element = new Element(nullptr);
        auto elementUid = attributes.value("id");
        element->set_uid(elementUid);
        element->set_icon(attributes.value("imageFile"));
        m_elementManager->rootElement()->appendElement(element);
        m_elementStack.append(element);

        // Build attribute metadata so we can turn it into a field later.
        qFatalIf(!matchParent("node"), "attribute has bad parent");
        qFatalIf(m_attribute.count(), "Attribute map should be clear");

        auto dataType = attributes.value("type");
        m_attribute["type"] = dataType;
        m_attribute["uid"] = attributes.value("id");
        m_attribute["exportUid"] = "a:0:" + attributes.value("dmUuid");
        m_attribute["nameElementUid"] = attributes.value("id");
        m_attribute["tag"] = QVariantMap();
        m_attribute["minValue"] = attributes.value("minValue");
        m_attribute["maxValue"] = attributes.value("maxValue");
        m_attribute["key"] = attributes.value("attributeKey");

        auto listElementUid = attributes.value("configId");
        if (!listElementUid.isEmpty() && (dataType == "LIST" || dataType == "TREE" || dataType == "MLIST"))
        {
            m_attribute["listElementUid"] = listElementUid;
        }
        m_attribute["required"] = attributes.value("required") != "false";

        m_attribute["regex"] = attributes.value("regex");

        // Add this field to the current uid list.
        m_nodeStack.last()->appendFieldUid(m_attribute["uid"].toString());
    }
    else if (qName == "option")
    {
        qFatalIf(!matchParent("attribute"), "option has wrong parent");
        qFatalIf(m_attribute.count() == 0, "No last field");

        auto tag = m_attribute["tag"].toMap();
        auto doubleValue = attributes.value("doubleValue");
        auto stringValue = attributes.value("stringValue");
        auto uuidValue = attributes.value("dmUuid");

        auto optionId = attributes.value("id");
        if (optionId == "IS_VISIBLE")
        {
            m_attribute["hidden"] = doubleValue == "0.0";
            m_attribute["relevant"] = doubleValue == "2.0" ? stringValue.trimmed() : "";
        }
        else if (optionId == "MULTISELECT")
        {
            if (doubleValue == "1.0")
            {
                m_attribute["type"] = "BOOLEAN";
            }
        }
        else if (optionId == "DEFAULT_VALUE")
        {
            QVariant defaultValue;
            auto attributeType = m_attribute["type"];
            if (attributeType == "NUMERIC")
            {
                defaultValue = doubleValue.toDouble();
            }
            else if (attributeType == "BOOLEAN")
            {
                if (doubleValue == "1.0")
                {
                    defaultValue = m_attribute["required"].isValid();
                }
            }
            else if (!uuidValue.isEmpty())
            {
                defaultValue = uuidValue;
            }
            else
            {
                defaultValue = stringValue;
            }

            m_attribute["default_value"] = defaultValue;
        }
        else if (optionId == "FLATTEN_TREE")
        {
            if (doubleValue == "1.0")
            {
                m_attribute["listFlatten"] = true;
                m_attribute["listSorted"] = true;
            }
        }
        else if (optionId == "NUMERIC")
        {
            if (doubleValue == "1.0")
            {
                m_attribute["numeric"] = true;
            }
        }
        else if (optionId == "ENTER_ONCE")
        {
            if (stringValue != "NONE")
            {
                tag["ENTER_ONCE"] = stringValue;
            }
        }
        else if (optionId == "INPUT_GROUP")
        {
            if (doubleValue == "1.0")
            {
                tag.remove("ENTER_ONCE"); // Suppress enter_once mechanism - should be done by desktop.
                tag["INPUT_GROUP"] = true;
            }
        }
        else if (optionId.startsWith("HELP_"))
        {
            auto hint = m_attribute["hint"].toMap();
            hint.insert(optionId, stringValue);
            m_attribute["hint"] = hint;
        }
        else if (optionId == "QR_CODE")
        {
            m_attribute["barcode"] = doubleValue == "1.0";
        }
        else if (optionId == "USE_NUMPAD")
        {
            m_attribute["numbers"] = doubleValue == "1.0";
        }
//        else
//        {
//            tag.insert(optionId, !doubleValue.isNull() ? doubleValue : stringValue);
//        }

        m_attribute["tag"] = tag;
    }
    else if (qName == "extraData")
    {
        auto map = QVariantMap();
        map.insert("id", attributes.value("type"));
        m_extraData->append(map);
    }
    else if ((qName == "stringKey") || (qName == "integerKey"))
    {
        qFatalIf(!matchParent("extraData"), "data has wrong parent");
        auto map = m_extraData->last().toMap();
        auto value = QVariant(attributes.value("value"));
        if (qName == "integerKey")
        {
            value = value.toInt();
        }
        map.insert(attributes.value("key"), value);
        m_extraData->removeLast();
        m_extraData->append(map);
    }

    m_stack.append(qName);

    return true;
}

bool SMARTModelHandler::endElement(const QString& namespaceURI, const QString& localName, const QString& qName)
{
    Q_UNUSED(namespaceURI)
    Q_UNUSED(localName)

    if (qName == "ConfigurableModel")
    {
        // Clear any trees+lists that have a default that is not a valid Element uid.
        // This happens if the data model specifies a node rather than a leaf.
        for (auto field: m_remapDefaultValue.values())
        {
            qDebug() << "Error: field has invalid default value: " << field->uid();
            field->set_defaultValue(QVariant());
        }
        m_remapDefaultValue.clear();

        m_fieldManager->completeUpdate();
        m_elementManager->completeUpdate();
    }
    else if (qName == "languages")
    {
        auto languageList = QVariantList();

        for (auto languageIt = m_languages.constBegin(); languageIt != m_languages.constEnd(); languageIt++)
        {
            auto code = *languageIt;
            auto locale = QLocale(code);

            languageList.append(QVariantMap
            {
                { "code", code },
                { "name", locale.languageToString(locale.language()) + " (" + locale.countryToString(locale.country()) + ")" }
            });
        }

        std::sort(languageList.begin(), languageList.end(), [](const QVariant& r1, const QVariant& r2) -> bool
        {
            auto n1 = r1.toMap()["name"].toString();
            auto n2 = r2.toMap()["name"].toString();
            return n1.compare(n2) == 0;
        });

        for (auto i = 0; i < languageList.count(); i++)
        {
            if (languageList[i].toMap()["code"].toString() == m_defaultLanguage)
            {
                m_settings->insert("languageIndex", i);
                break;
            }
        }

        m_settings->insert("languages", languageList);
    }
    else if (qName == "signatureType")
    {
        m_elementStack.removeLast();
    }
    else if (qName == "signature")
    {
        m_attribute.clear();
    }
    else if (qName == "nodes")
    {
        m_elementStack.removeLast();
    }
    else if (qName == "node")
    {
        m_elementStack.removeLast();
        m_nodeStack.removeLast();
        m_lastAttribute.clear();
    }
    else if (qName == "attributeConfig")
    {
        m_elementStack.removeLast();
    }
    else if ((qName == "listItem") || (qName == "treeNode") || (qName == "children"))
    {
        m_elementStack.removeLast();
    }
    else if (qName == "attribute")
    {
        m_elementStack.removeLast();
        qFatalIf(m_attribute.count() == 0, "Last field should be valid");
        BaseField *field = nullptr;
        auto fieldType = m_attribute.value("type");

        if (fieldType == "TEXT")
        {
            auto stringField = new StringField();
            stringField->set_multiLine(!m_attribute["numbers"].toBool());
            stringField->set_numbers(m_attribute["numbers"].toBool());
            stringField->set_barcode(m_attribute["barcode"].toBool());
            if (!m_attribute.value("regex").toString().isEmpty())
            {
                stringField->set_constraint("regex(., '" + m_attribute["regex"].toString() + "')");
            }
            field = stringField;
        }
        else if (fieldType == "LIST")
        {
            field = new StringField();
        }
        else if (fieldType == "TREE")
        {
            field = new StringField();
        }
        else if (fieldType == "MLIST")
        {
            field = new CheckField();

            // Tag this as an MLIST so our post-processor does not turn subsequent fields into a record.
            auto tag = m_attribute["tag"].toMap();
            tag["MLIST"] = true;
            m_attribute["tag"] = tag;

            // Add the default items to the remap list.
            auto defaultValue = m_attribute.value("default_value").toString();
            if (!defaultValue.isEmpty())
            {
                auto defaultValues = defaultValue.split(',');
                for (auto it = defaultValues.constBegin(); it != defaultValues.constEnd(); it++)
                {
                    m_remapDefaultValue.insert(*it, field);
                }
            }
        }
        else if (fieldType == "BOOLEAN")
        {
            field = new CheckField();
        }
        else if (fieldType == "NUMERIC")
        {
            auto numberField = new NumberField();
            field = numberField;

            auto valueText = m_attribute.value("minValue").toString();
            bool ok = false;
            auto value = valueText.toDouble(&ok);
            if (ok)
            {
                numberField->set_minValue(value);
            }

            valueText = m_attribute.value("maxValue").toString();
            value = valueText.toDouble(&ok);
            if (ok)
            {
                numberField->set_maxValue(value);
            }

            numberField->set_decimals(3);

            // Special handling for GPS fields.
            auto attributeKey = m_attribute["key"].toString();
            if (attributeKey.startsWith("gps_"))
            {
                m_attribute["hidden"] = true;
                m_attribute["default_value"] = 0;

                auto tag = m_attribute["tag"].toMap();
                tag["attributeKey"] = attributeKey;
                m_attribute["tag"] = tag;
            }

            //
            // SMART Desktop does not have support for number lists directly. Instead, it requires
            // two attributes: a check list followed by a number field. The number field must be
            // tagged with the "numeric" option.
            //
            // Since this is discovered only when processing the number field, go back to the prior
            // attribute, pick up the relevant attributes (like name, list, etc) and then remove the
            // prior attribute from the model.
            //
            if (m_lastAttribute["type"] == "BOOLEAN" && !m_lastAttribute["listElementUid"].toString().isEmpty() &&
                m_attribute.value("numeric").toBool())
            {
                auto lastField = m_modelField->field(m_modelField->fieldCount() - 1);
                m_remapDefaultValue.remove(m_lastAttribute["default_value"].toString(), lastField);

                m_modelField->removeLastField();
                m_nodeStack.last()->removeFieldUid(m_lastAttribute["uid"].toString());

                m_attribute["nameElementUid"] = m_lastAttribute["nameElementUid"];
                m_attribute["listElementUid"] = m_lastAttribute["listElementUid"];
                m_attribute["required"] = m_lastAttribute["required"];
                m_attribute["hidden"] = m_lastAttribute["hidden"];
                m_attribute["listFlatten"] = m_lastAttribute["listFlatten"];
                m_attribute["listSorted"] = m_lastAttribute["listSorted"];
                m_attribute["hint"] = m_lastAttribute["hint"];
            }
        }
        else if (fieldType == "DATE")
        {
            field = new DateField();
        }
        else if (fieldType == "DATE_AND_TIME")
        {
            field = new DateTimeField();
        }
        else if (fieldType == "TIME")
        {
            field = new TimeField();
        }
        else if (fieldType == "UUID")
        {
            field = new StringField();
        }
        else if (fieldType == "IMAGE")
        {
            field = new StringField();
        }
        else
        {
            qFatalIf(true, "Unknown field type");
        }

        // For trees and single select lists, remap the default value.
        if (m_attribute["default_value"].isValid() && m_attribute.contains("listElementUid") && fieldType != "MLIST")
        {
            m_remapDefaultValue.insert(m_attribute["default_value"].toString(), field);
        }

        field->set_uid(m_attribute["uid"].toString());
        field->set_exportUid(m_attribute["exportUid"].toString());
        field->set_nameElementUid(m_attribute["nameElementUid"].toString());
        field->set_listElementUid(m_attribute["listElementUid"].toString());
        field->set_required(m_attribute["required"]);
        field->set_hidden(m_attribute["hidden"].toBool());
        field->set_listFlatten(m_attribute["listFlatten"].toBool());
        field->set_listSorted(m_attribute["listSorted"].toBool());
        field->set_defaultValue(m_attribute["default_value"]);

        auto hintMap = m_attribute["hint"].toMap();
        if (!hintMap.isEmpty())
        {
            // No longer using web view for help content.
            //field->set_hintLink(hintMap["HELP_HTML_FILE"].toString());

            auto hintElementUid = field->uid() + "/hint";
            auto hintElement = new Element(nullptr);
            hintElement->set_uid(hintElementUid);
            hintElement->set_name(hintMap["HELP_TEXT"].toString());
            hintElement->set_icon(hintMap["HELP_IMAGE_FILE"].toString());
            hintElement->addTag("iconLocation", hintMap["HELP_IMAGE_LOCATION"].toString());
            m_elementManager->rootElement()->appendElement(hintElement);

            field->set_hintElementUid(hintElementUid);
        }

        field->set_relevant(m_attribute["relevant"].toString());
        field->set_tag(m_attribute["tag"].toMap());
        m_modelField->appendField(field);

        m_lastAttribute = m_attribute;
        m_attribute.clear();
    }

    qFatalIf(m_stack.last() != qName, "Invalid XML");
    m_stack.removeLast();

    return true;
}

bool SMARTModelHandler::characters(const QString& str)
{
    Q_UNUSED(str)
    return true;
}

bool SMARTModelHandler::fatalError(const QXmlParseException& exception)
{
    qFatalIf(true, "Parse error at line reading xml: " + exception.message());
    return false;
}

QString SMARTModelHandler::errorString() const
{
    return "";
}
