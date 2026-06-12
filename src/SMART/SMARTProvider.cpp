#include "App.h"
#include "SMARTProvider.h"
#include "SMARTRenderer.h"
#include "SMARTSchema.h"
#include "Form.h"
#include "TrackFile.h"

namespace CyberTracker {

namespace {
constexpr bool COMPRESS_UPLOAD = false;
constexpr bool COMPACT_JSON    = true;
constexpr QJsonDocument::JsonFormat JSON_FORMAT = COMPACT_JSON ? QJsonDocument::Compact : QJsonDocument::Indented;
}

SMARTProvider::SMARTProvider(Form* form) : Provider(form)
{
    update_name(PROVIDER_NAME);
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
            upload = tr("On save");
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

    auto projectionFormat = tr("Unknown");
    switch (m_profile.value("PROJECTION", 1).toInt())
    {
    case 0:
        projectionFormat = tr("Degrees minutes seconds");
        break;
    case 1:
        projectionFormat = tr("Decimal degrees");
        break;
    case 2:
        projectionFormat = tr("UTM");
        break;
    }

    auto resizeImage = !m_profile.value("RESIZE_IMAGE").toBool() ?
        QString("%1 x %2").arg(SMARTSchema::DEFAULT_PHOTO_RESOLUTION_X).arg(SMARTSchema::DEFAULT_PHOTO_RESOLUTION_Y) :
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
        { tr("Use time from GPS"), boolToValue(requireGPSTime()) },
        { tr("Format"), projectionFormat },
        { tr("Skip button timeout"), skipButtonTimeout == 0 ? tr("Off") : QString("%1 %2").arg(QString::number(skipButtonTimeout), tr("seconds")) },
        { tr("Allow manual GPS"), boolToValue(m_allowManualGPS) },
        { tr("Units"), m_useMetric ? tr("Metric") : tr("Imperial") },
        { "-" },
        { tr("Maximum photos"), m_profile.value("MAX_PHOTO_COUNT", 3).toString() },
        { tr("Resize photos"), resizeImage },
        { "-" },
        { tr("Position updates"), boolToValue(m_profile["POSITION_UPDATES"].toMap()["FREQUENCY_MIN"].toInt() != 0) },
        { tr("Alerts"), boolToValue(m_profile.contains("ALERTS")) },
        { "-" },
        { tr("Allow history"), boolToValue(m_profile.value("HISTORY_MODE", true)) },
        { tr("Allow archive"), boolToValue(m_profile.value("ARCHIVE_MODE", true)) },
    };

    auto result = QVariantList();
    for (auto nv: model)
    {
        result.append(QVariantMap {{ "name", nv.name }, { "value", nv.value }});
    }

    return result;
}

void SMARTProvider::savePatrolSighting(const QString& observationType, const QString& timestamp, bool snapTrack)
{
    auto sighting = m_form->createSightingPtr();
    sighting->regenerateUids();

    sighting->set_sessionId(m_patrolUid);

    sighting->set_createTime(timestamp);
    sighting->set_updateTime(timestamp);

    auto rootRecord = sighting->recordManager()->rootRecord();
    rootRecord->resetFieldValue(SMARTSchema::FIELD_MODEL);
    rootRecord->resetFieldValue(SMARTSchema::FIELD_GROUPS);
    rootRecord->setFieldValue(SMARTSchema::FIELD_OBSERVATION_TYPE, observationType);

    if (snapTrack && m_form->trackStreamer()->running())
    {
        m_form->snapTrack(sighting.get());
    }

    m_form->saveSighting(sighting.get());

    m_form->resetFieldValue(SMARTSchema::FIELD_LOCATION);
    m_form->resetFieldValue(SMARTSchema::FIELD_DISTANCE);
    m_form->resetFieldValue(SMARTSchema::FIELD_BEARING);
}

void SMARTProvider::savePatrolLocation(Location* location)
{
    if (location->accuracy() == 0.0)
    {
        qDebug() << "Warning: savePatrolLocation: manual location added to track";
    }

    m_form->trackStreamer()->pushUpdate(QUuid::createUuid().toString(QUuid::Id128), location);
}

void SMARTProvider::startPatrol()
{
    auto now = App::instance()->timeManager()->currentDateTime();
    auto nowISO = Utils::encodeTimestamp(now);
    qDebug() << "Patrol start: " << nowISO;

    // Flush the last patrol if not using Connect.
    // In the Connect case, everything goes through the "Export data" button.
    if (!m_hasDataServer)
    {
        clearCompletedData();
    }

    qFatalIf(m_form->readonly(), "Form is readonly");
    qFatalIf(m_form->editing(), "Form is editing");
    qFatalIf(!m_hasPatrol, "Patrol not available");
    qFatalIf(!m_patrolForm, "Not a patrol form");
    qFatalIf(m_patrolStarted, "Patrol already started");
    qFatalIf(m_patrolPaused, "Patrol should not be paused");
    qFatalIf(!m_patrolUid.isEmpty(), "Patrol uid must be empty to start a patrol");

    auto dataType = m_form->getFieldValue(SMARTSchema::FIELD_DATA_TYPE).toString().toLower();
    qFatalIf(dataType != "patrol" && dataType != "survey", "Bad data type for patrol");
    auto location = m_form->sighting()->locationPtr();

    auditAdd("Start patrol");

    // Setup the patrol metadata.
    m_patrolStarted = true;

    m_form->setFieldValue(SMARTSchema::FIELD_OBSERVATION_TYPE, VALUE_OBSERVATION);
    m_form->newSessionId();
    m_patrolUid = m_form->sessionId();
    m_form->setFieldValue(SMARTSchema::FIELD_PATROL_ID, m_patrolUid);

    // Capture the patrol's timezone from its start location so that all exported
    // data for this patrol uses a single consistent zone.
    auto patrolTimeZone = Utils::detectTimeZone(location->x(), location->y());
    m_patrolTimeZone = patrolTimeZone.id();

    auto nowInZone = patrolTimeZone.isValid() ? now.toTimeZone(patrolTimeZone) : now;
    m_patrolStartTimestamp = Utils::encodeTimestamp(now);
    m_form->setFieldValue(SMARTSchema::FIELD_PATROL_START_DATE, nowInZone.toString("yyyy/MM/dd"));
    m_form->setFieldValue(SMARTSchema::FIELD_PATROL_START_TIME, nowInZone.toString("HH:mm:ss"));

    // Setup the leg stats.
    update_patrolLegs(QVariantList { QVariantMap {{ "startTime", m_patrolStartTimestamp }} });
    m_form->setFieldValue(SMARTSchema::FIELD_PATROL_LEG_COUNTER, 1);
    m_form->setFieldValue(SMARTSchema::FIELD_PATROL_LEG_DISTANCE, 0.0);

    // Reset the upload state.
    m_uploadObsCounter = 1;

    // Initialize the ping uid.
    m_pingUuid = QUuid::createUuid().toString(QUuid::Id128);

    // Start the track timer.
    startTrack(0, 0);

    // Save new patrol sighting with initial location.
    savePatrolSighting(VALUE_NEW_PATROL, m_patrolStartTimestamp, false);

    // Save track point: snap to patrol start time.
    location->set_timestamp(m_patrolStartTimestamp);
    savePatrolLocation(location.get());

    // Save the state.
    saveProjectState();

    emit patrolStartedChanged();

    qDebug() << "Patrol start success: " << nowISO;

    // Set global patrol active.
    App::instance()->settings()->set_patrolProjectUid(m_form->projectUid());
}

void SMARTProvider::stopPatrol()
{
    auto now = App::instance()->timeManager()->currentDateTimeISO();
    qDebug() << "Patrol stop: " << now;

    qFatalIf(m_form->readonly(), "Form is readonly");
    qFatalIf(m_form->editing(), "Form is editing");
    qFatalIf(!m_hasPatrol, "Patrol not available");
    qFatalIf(!m_patrolForm, "Not a patrol form");
    qFatalIf(!m_patrolStarted, "Patrol not started");
    qFatalIf(m_patrolPaused, "Patrol should not be paused");

    auto location = m_form->sighting()->locationPtr();

    // Save stop patrol sighting with final location.
    // location timestamp is guaranteed to be older than the patrol sighting (now).
    savePatrolLocation(location.get());
    savePatrolSighting(VALUE_STOP_PATROL, now, true /* snapTrack */);

    // Stop the track timer.
    stopTrack();

    // Stop the patrol and commit the form state.
    m_patrolStarted = false;
    m_patrolStartTimestamp.clear();
    m_patrolUid.clear();
    m_form->newSighting();

    // Reset the upload state.
    m_uploadObsCounter = 1;

    // Finalize the leg.
    setPatrolLegValue("stopTime", now);

    saveProjectState();

    emit patrolStartedChanged();

    qDebug() << "Patrol stop success: " << now;

    // Reset global patrol state.
    App::instance()->settings()->set_patrolProjectUid("");

    auditAdd("Stop patrol");
}

void SMARTProvider::pausePatrol()
{
    auto now = App::instance()->timeManager()->currentDateTimeISO();
    qDebug() << "Patrol pause: " << now;

    qFatalIf(m_form->readonly(), "Form is readonly");
    qFatalIf(m_form->editing(), "Form is editing");
    qFatalIf(!m_hasPatrol, "Patrol not available");
    qFatalIf(!m_patrolForm, "Not a patrol form");
    qFatalIf(!m_patrolStarted, "Patrol not started");
    qFatalIf(m_patrolPaused, "Patrol should not be paused");

    auto location = m_form->sighting()->locationPtr();

    // Store the distance to be resumed and pause the track.
    m_patrolPausedDistance = m_form->trackStreamer()->distanceCovered();

    // Save pause patrol sighting with location.
    // location timestamp is guaranteed to be older than the patrol sighting (now).
    savePatrolLocation(location.get());
    savePatrolSighting(VALUE_PAUSE_PATROL, now, true /* snapTrack */);

    // Stop the track timer.
    stopTrack();

    // Set the state to paused.
    m_patrolPaused = true;

    // Commit the form state.
    saveProjectState();

    emit patrolPausedChanged();

    qDebug() << "Patrol pause success: " << now;

    auditAdd("Pause patrol");

    /// The patrol pause feature was used.
    /// @category Actions
    /// @docs https://cybertrackerwiki.org/smart/user-manual/8#813-pause-patrol
    setUsage("pausePatrol", true);
}

void SMARTProvider::resumePatrol()
{
    auto now = App::instance()->timeManager()->currentDateTimeISO();
    qDebug() << "Patrol resume: " << now;

    qFatalIf(m_form->readonly(), "Form is readonly");
    qFatalIf(m_form->editing(), "Form is editing");
    qFatalIf(!m_hasPatrol, "Patrol not available");
    qFatalIf(!m_patrolForm, "Not a patrol form");
    qFatalIf(!m_patrolStarted, "resumePatrol: patrol not started");
    qFatalIf(!m_patrolPaused, "resumePatrol: patrol not paused");

    auto location = m_form->sighting()->locationPtr();

    // Restart the track timer.
    startTrack(m_patrolPausedDistance, 0);

    // Save resume patrol sighting with location.
    savePatrolSighting(VALUE_RESUME_PATROL, now, false /* snapTrack */);

    // Save track point: snap to current time.
    location->set_timestamp(now);
    savePatrolLocation(location.get());

    // Resume the patrol and commit the form state.
    m_patrolPaused = false;
    saveProjectState();

    emit patrolPausedChanged();

    qDebug() << "Patrol resume success: " << now;

    auditAdd("Resume patrol");
}

bool SMARTProvider::isPatrolMetadataValid()
{
    auto parentField = m_form->schema()->rootField();
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
            if (!m_form->getFieldValueValid(m_form->rootRecordUid(), field->uid())) {
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

QVariantMap SMARTProvider::getPatrolLeg(int index) const
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
        id += " - " + m_form->getElementName(transportElementUid);
    }

    if (lastLeg && m_patrolStarted)
    {
        id += " - " + (m_patrolPaused ? tr("Paused") : tr("Active"));
    }

    return QVariantMap
    {
        { "id", id },
        { "active", m_patrolStarted && lastLeg },
        { "startDate", timeManager->getDateText(startTime) },
        { "startTime", timeManager->getTimeText(startTime) },
        { "distance", App::instance()->getDistanceText(distance) },
        { "speed", App::instance()->getSpeedText(distance / (Utils::timestampDeltaMs(startTime, stopTime) / 1000)) },
        { "rate", rate },
    };
}

void SMARTProvider::snapLegDistance()
{
    if (m_form->trackStreamer()->running())
    {
        auto trackDistance = m_form->trackStreamer()->distanceCovered();
        m_form->setFieldValue(SMARTSchema::FIELD_PATROL_LEG_DISTANCE, trackDistance);
        setPatrolLegValue("distance", trackDistance);
        saveProjectState();
    }
}

void SMARTProvider::changePatrol()
{
    // Hack to make sure we are always at least a second ahead of the track timer.
    // This means that when we write the first location point, it is guaranteed to be at least a second beyond the 
    // last track point. This is needed to keep the parser happy.
    auto now = App::instance()->timeManager()->currentDateTime().addSecs(1); 
    auto nowISO = Utils::encodeTimestamp(now);
    qDebug() << "Change patrol: " << nowISO;

    auto location = m_form->sighting()->locationPtr();

    // Update the legs.
    setPatrolLegValue("stopTime", nowISO);
    m_patrolLegs.append(QVariantMap {{ "startTime", nowISO }});
    m_form->setFieldValue(SMARTSchema::FIELD_PATROL_LEG_COUNTER, m_form->getFieldValue(SMARTSchema::FIELD_PATROL_LEG_COUNTER).toInt() + 1);
    m_form->setFieldValue(SMARTSchema::FIELD_PATROL_LEG_DISTANCE, 0);

    // Save the final location before the change.
    location->set_timestamp(Utils::encodeTimestamp(now.addSecs(-1))); // Hack so location is always at least a second before.
    savePatrolLocation(location.get());

    // Create a patrol state change sighting.
    savePatrolSighting(VALUE_CHANGE_PATROL, nowISO, true);

    // Stop the track.
    stopTrack();

    // Resume the track.
    startTrack(0, 0);

    // Save the first location of the new track.
    location->set_timestamp(Utils::encodeTimestamp(now.addSecs(+1))); // Hack so location is always at least a second after.
    savePatrolLocation(location.get());

    saveProjectState();

    auditAdd("Change patrol");

    /// The user used the patrol change feature.
    /// @category Actions
    /// @docs https://cybertrackerwiki.org/smart/user-manual/#815-change-patrol
    setUsage("changePatrol", true);
}

void SMARTProvider::startIncident()
{
    auto now = App::instance()->timeManager()->currentDateTimeISO();
    qDebug() << "Incident start: " << now;

    qFatalIf(!m_hasIncident, "Incident not available");
    qFatalIf(m_incidentStarted, "Incident already started");
    qFatalIf(m_form->getFieldValue(SMARTSchema::FIELD_DATA_TYPE).toString().toLower() != "incident", "Bad data type for incident");

    if (m_form->sighting()->createTime().isEmpty())
    {
        m_form->snapCreateTime();
    }

    m_incidentStarted = true;

    saveProjectState();

    emit incidentStartedChanged();

    qDebug() << "Incident start success: " << now;

    if (m_patrolStarted)
    {
        /// An independent incident was recorded during a patrol. Only set on the Incident form type.
        /// @category Actions
        /// @docs https://cybertrackerwiki.org/smart/user-manual/8#82-independent-incidents
        setUsage("incidentDuringPatrol", true);
    }
}

void SMARTProvider::stopIncident(bool /*cancel*/)
{
    auto now = App::instance()->timeManager()->currentDateTimeISO();
    qDebug() << "Incident stop: " << now;

    qFatalIf(!m_hasIncident, "Incident not available");
    qFatalIf(!m_incidentStarted, "Incident not started");

    m_incidentStarted = false;

    if (!m_form->editing())
    {
        m_form->newSighting();
    }

    saveProjectState();

    emit incidentStartedChanged();

    qDebug() << "Incident stop success: " << now;
}

void SMARTProvider::startCollect()
{
    auto now = App::instance()->timeManager()->currentDateTimeISO();
    qDebug() << "Collect start: " << now;

    qFatalIf(!m_hasCollect, "Collect not available");
    qFatalIf(m_collectStarted, "Collect already started");
    qFatalIf(m_form->getFieldValue(SMARTSchema::FIELD_DATA_TYPE).toString().toLower() != "smartcollect", "Bad data type for collect");

    auditAdd("Start collect");

    m_form->newSessionId();

    m_form->setFieldValue(SMARTSchema::FIELD_COLLECT_USER, m_collectUser);

    startTrack(0, 0);

    m_collectStarted = true;
    saveProjectState();

    emit collectStartedChanged();

    qDebug() << "Collect start success: " << now;
}

void SMARTProvider::stopCollect()
{
    auto now = App::instance()->timeManager()->currentDateTimeISO();
    qDebug() << "Collect stop: " << now;

    qFatalIf(!m_hasCollect, "Collect not available");
    qFatalIf(!m_collectStarted, "Collect not started");

    stopTrack();

    m_collectStarted = false;
    m_form->newSighting();
    saveProjectState();

    emit collectStartedChanged();

    qDebug() << "Collect stop success: " << now;

    auditAdd("Stop collect");
}

void SMARTProvider::setCollectUser(const QString& user)
{
    update_collectUser(user);

    saveProjectState();
}

QString SMARTProvider::convertToZone(const QString& isoTimestamp, const QString& timeZoneId) const
{
    if (timeZoneId.isEmpty())
    {
        return isoTimestamp;
    }

    auto timeZone = QTimeZone(timeZoneId.toUtf8());
    if (!timeZone.isValid())
    {
        return isoTimestamp;
    }
    
    return Utils::encodeTimestamp(Utils::decodeTimestamp(isoTimestamp).toTimeZone(timeZone));
}

QJsonObject SMARTProvider::locationToJson(const QString& locationUid, Location* location, const QVariantMap& extraProps, const QString& timeZoneOverride) const
{
    // SMART requires that latitude and longitude look like doubles not like longs.
    auto locationX = location->x();
    if (!QString::number(locationX).contains("."))
    {
        locationX += 0.0000000001;
    }
    auto locationY = location->y();
    if (!QString::number(locationY).contains("."))
    {
        locationY += 0.0000000001;
    }

    // Geometry.
    auto geometry = QJsonObject
    {
        { "type", "Point" },
        { "coordinates", QJsonArray { locationX, locationY } }
    };

    // Properties.
    auto properties = QJsonObject
    {
        { SMARTSchema::FIELD_OBSERVATION_TYPE, "waypoint" },
        { "dateTime", convertToZone(location->timestamp(), timeZoneOverride) },
        { "deviceId", App::instance()->settings()->deviceId() },
        { "id", locationUid },
        { "accuracy", std::isnan(location->accuracy()) ? 10000 : location->accuracy() },
        { "altitude", std::isnan(location->z()) ? 0 : location->z() },
    };

    // Extra properties.
    for (auto it = extraProps.constKeyValueBegin(); it != extraProps.constKeyValueEnd(); it++)
    {
        properties.insert(it->first, QJsonValue::fromVariant(it->second));
    }

    // Full location object.
    return QJsonObject {{ "type", "Feature" }, { "geometry", geometry }, { "properties", properties }};
}

QJsonObject SMARTProvider::addRecordToJson(const QJsonObject& baseData, Sighting* sighting, const QString& recordUid, bool childRecords, int attributeIndex, bool skipAttachments) const
{
    auto result = QJsonObject(baseData);
    auto schema = sighting->schema();

    // Default skip list.
    auto excludeFieldUids = QStringList { SMARTSchema::FIELD_MODEL_TREE, SMARTSchema::FIELD_GROUPS, SMARTSchema::FIELD_LOCATION, SMARTSchema::FIELD_DISTANCE, SMARTSchema::FIELD_BEARING };
    auto modelFieldUids = QStringList();

    // Skip employee list for incidents.
    if (sighting->stateSpace() == SMARTSchema::TYPE_INCIDENT)
    {
        excludeFieldUids.append(SMARTSchema::FIELD_EMPLOYEES);
    }

    // Filter to only model fields.
    auto record = sighting->getRecord(recordUid);
    if (record->hasFieldValue(SMARTSchema::FIELD_MODEL_TREE))
    {
        auto value = record->getFieldValue(SMARTSchema::FIELD_MODEL_TREE).toString();
        auto element = schema->getElement(value);
        if (element)
        {
            modelFieldUids.append(SMARTSchema::FIELD_SIGHTING_GROUP_ID);
            modelFieldUids.append(element->fieldUids());
            result.insert("c:" + QString::number(attributeIndex), element->exportUid());
        }
    }

    // Enumerate values.
    record->enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
    {
        auto field = fieldValue.field();
        auto fieldUid = field->uid();

        // Skip excluded fields.
        if (excludeFieldUids.contains(fieldUid))
        {
            return;
        }

        // Skip non-model fields.
        if (!modelFieldUids.isEmpty() && !modelFieldUids.contains(fieldUid))
        {
            return;
        }

        // Skip fields which are not relevant as long as no default value is set.
        if (!fieldValue.isRelevant() && !field->defaultValue().isValid())
        {
            return;
        }

        // Skip record fields, they are handled later.
        // Since they replicate the parent, it must have been completely processed.
        if (field->isRecordField())
        {
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

        // Skip CheckFields with undefined values.
        if (!exportValue.isValid() && qobject_cast<CheckField*>(field))
        {
            return;
        }

        // StringField.
        bool fieldIsList = !field->listElementUid().isEmpty();
        if (fieldIsList && qobject_cast<StringField*>(field))
        {
            auto element = schema->getElement(exportValue.toString());
            if (element && !element->exportUid().isEmpty())
            {
                exportValue = element->exportUid();
            }
        }
        // DateField.
        else if (qobject_cast<DateField*>(field))
        {
            exportValue = QDate::fromString(exportValue.toString(), Qt::DateFormat::ISODate).toString("yyyy/MM/dd");
        }
        // Check list.
        else if (fieldIsList && qobject_cast<CheckField*>(field))
        {
            bool master = field->getTagValue("master").toBool();

            auto valueMap = exportValue.toMap();
            auto orderedKeys = QStringList(valueMap.keys());
            schema->sortByIndex(orderedKeys);

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
                auto element = schema->getElement(exportKey);
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
        // Number list.
        else if (fieldIsList && qobject_cast<NumberField*>(field))
        {
            bool master = field->getTagValue("master").toBool();

            auto valueMap = exportValue.toMap();
            auto orderedKeys = QStringList(valueMap.keys());
            schema->sortByIndex(orderedKeys);

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
                auto element = schema->getElement(exportKey);
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
                auto v = valueMap[key].toDouble(&ok);
                if (!ok)    // Fix "." because keypad validation was broken.
                {
                    v = 0.0;
                    qDebug() << "Ignoring invalid number list value: " << valueMap[key];
                }

                result.insert(exportPrefix + exportKey, v);
            }

            return;
        }
        // PhotoField.
        else if (qobject_cast<PhotoField*>(field))
        {
            if (skipAttachments)
            {
                return;
            }

            // Restart the index from the last one available.
            auto photoIndex = 0;
            for (auto baseDataIt = baseData.constBegin(); baseDataIt != baseData.constEnd(); baseDataIt++)
            {
                auto key = baseDataIt.key();
                if (key.startsWith(exportName) && key.split(':').constLast() == QString::number(attributeIndex))
                {
                    photoIndex++;
                }
            }

            auto filenames = exportValue.toStringList();
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

                    // Add attachment tags to end of image data stream.
                    auto attachmentTags = Utils::getImageTags(filePath);
                    for (auto tagIt = attachmentTags.constBegin(); tagIt != attachmentTags.constEnd(); tagIt++)
                    {
                        auto tagElement = schema->getElement(*tagIt);
                        if (!tagElement)
                        {
                            qDebug() << "Error: element not found for tag: " << *tagIt;
                            continue;
                        }

                        hexString += ":" + tagElement->exportUid();
                    }

                    result.insert(exportKey, hexString);
                }
            }

            return;
        }
        // AudioField.
        else if (qobject_cast<AudioField*>(field))
        {
            if (skipAttachments)
            {
                return;
            }

            // Restart the index from the last one available.
            auto audioIndex = 0;
            for (auto baseDataIt = baseData.constBegin(); baseDataIt != baseData.constEnd(); baseDataIt++)
            {
                auto key = baseDataIt.key();
                if (key.startsWith(exportName) && key.split(':').constLast() == QString::number(attributeIndex))
                {
                    audioIndex++;
                }
            }

            auto filePath = App::instance()->getMediaFilePath(exportValue.toMap().value("filename").toString());
            QFile file(filePath);
            if (!filePath.isEmpty() && file.exists() && file.open(QIODevice::ReadOnly))
            {
                auto hexString = QString(file.readAll().toBase64());
                auto exportKey = exportName + QString::number(audioIndex++);
                if (m_smartVersion >= 700)
                {
                    exportKey += ":" + QString::number(attributeIndex);
                }
                result.insert(exportKey, hexString);
            }

            return;
        }
        // SketchField.
        else if (qobject_cast<SketchField*>(field))
        {
            if (skipAttachments)
            {
                return;
            }

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
        else if (qobject_cast<LineField*>(field))
        {
            auto points = exportValue.toList();
            if (points.length() < 2)
            {
                return;
            }

            exportValue = Utils::pointsToGeoJsonLine(points);
        }
        else if (qobject_cast<AreaField*>(field))
        {
            auto points = exportValue.toList();
            if (points.length() < 2)
            {
                return;
            }

            exportValue = Utils::pointsToGeoJsonArea(points);
        }

        // Special handling for gps attributes.
        auto attributeKey = field->tag().value("attributeKey").toString();
        if (attributeKey.startsWith("gps_") && field->hidden())
        {
            auto location = sighting->getRootFieldValue(SMARTSchema::FIELD_LOCATION).toMap();
            attributeKey.remove(0, 4);

            if (location.contains(attributeKey))
            {
                exportValue = location.value(attributeKey).toDouble();
            }
        }

        if (attributeKey == "device_id" && field->hidden())
        {
            exportValue = App::instance()->settings()->deviceId();
        }

        if (attributeKey == "device_username" && field->hidden())
        {
            exportValue = App::instance()->settings()->username();
        }

        result.insert(exportName, exportValue.toJsonValue());
    });

    // For child records, replicate each of the attributes in the parent as well.
    // SMART expects:
    //    ParentAttributes
    //    Child0Attributes
    //    ParentAttributes
    //    Child1Attributes
    // etc.
    if (childRecords)
    {
        record->enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
        {
            auto field = fieldValue.field();

            // Skip non-record fields.
            if (!field->isRecordField())
            {
                return;
            }

            // Skip non-model fields.
            if (!modelFieldUids.isEmpty() && !modelFieldUids.contains(fieldValue.fieldUid()))
            {
                return;
            }

            auto recordField = RecordField::fromField(field);

            auto orgAttributeIndex = attributeIndex;
            auto childRecordUids = fieldValue.value().toStringList();
            for (auto childRecordIt = childRecordUids.constBegin(); childRecordIt != childRecordUids.constEnd(); childRecordIt++)
            {
                // In matrix mode, skip records if they have not been changed.
                if (recordField->matrixCount() > 0)
                {
                    if (sighting->getRecord(*childRecordIt)->isEmpty())
                    {
                        continue;
                    }
                }

                // Add the parent record for each child.
                // Skip parent attachments for child records.
                result = addRecordToJson(result, sighting, recordUid, false, attributeIndex, true /* skipAttachments */);

                // Add the child record.
                result = addRecordToJson(result, sighting, *childRecordIt, false, attributeIndex);

                // Add another index.
                attributeIndex++;
            }

            attributeIndex = orgAttributeIndex;
        });
    }

    // Hack: to retain backward compatibility, a metadata change in a survey comes through as an observation.
    // The backend will not create a new incident as a result.
    if (m_surveyMode && result.value(SMARTSchema::FIELD_OBSERVATION_TYPE).toString() == VALUE_CHANGE_PATROL)
    {
        result.insert(SMARTSchema::FIELD_OBSERVATION_TYPE, VALUE_OBSERVATION);
    }

    return result;
}

QJsonArray SMARTProvider::sightingToJson(Sighting* sighting, int* obsCounter, const QString& filterRecordUid, const QVariantMap& extraProps) const
{
    auto result = QJsonArray();

    // Patrol sightings use the timezone captured when the patrol was started, so that
    // all exported data for the patrol shares a single consistent zone.
    auto timeZoneOverride = QString();
    if (sighting->stateSpace() == SMARTSchema::TYPE_PATROL)
    {
        timeZoneOverride = m_patrolTimeZone;
    }

    if (!sighting->trackFile().isEmpty())
    {
        auto trackFile = TrackFile(App::instance()->getMediaFilePath(sighting->trackFile()));
        trackFile.enumLocations([&](const QString& locationUid, Location* location)
        {
            result.append(locationToJson(locationUid, location, QVariantMap(), timeZoneOverride));
        });
    }

    auto rootRecordUid = sighting->rootRecordUid();

    // SMART requires that latitude and longitude look like doubles not like longs.
    auto location = sighting->locationPtr();
    auto locationX = location->x();
    if (!QString::number(locationX).contains("."))
    {
        locationX += 0.0000000001;
    }
    auto locationY = location->y();
    if (!QString::number(locationY).contains("."))
    {
        locationY += 0.0000000001;
    }

    // Geometry.
    auto geometry = QJsonObject
    {
        { "type", "Point" },
        { "coordinates", QJsonArray { locationX, locationY } }
    };

    // Properties.
    auto properties = QJsonObject
    {
        { "appName", m_project->title() },
        { "ctVersion", App::instance()->buildString() },
        { "dateTime", convertToZone(sighting->createTime(), timeZoneOverride) },
        { "deviceId", sighting->deviceId() },
        { "latitude", locationY },
        { "longitude", locationX },
        { "altitude", std::isnan(location->z()) ? 0 : location->z() },
        { "rootId", rootRecordUid },
    };

    // Distance and bearing.
    if (m_useDistanceAndBearing)
    {
        auto distance = sighting->getRootFieldValue(SMARTSchema::FIELD_DISTANCE);
        auto bearing = sighting->getRootFieldValue(SMARTSchema::FIELD_BEARING);
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
    baseData = addRecordToJson(baseData, sighting, rootRecordUid);

    // These are required by the SMART desktop parser.
    baseData.insert("SMART_DefaultAttributeValues", "{}");

    auto isPatrolForm = sighting->stateSpace() == SMARTSchema::TYPE_PATROL;
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
    if (isPatrolForm && obsCounter && baseData.value(SMARTSchema::FIELD_OBSERVATION_TYPE).toString() == VALUE_NEW_PATROL)
    {
        *obsCounter = 1;
    }

    // Add the data records.
    auto recordUids = sighting->getRootFieldValue(SMARTSchema::FIELD_MODEL).toStringList();
    if (!filterRecordUid.isEmpty())
    {
        // Filter to just one record.
        recordUids = QStringList { filterRecordUid };
    }
    else if (recordUids.count() == 0)
    {
        // Metadata only sighting.
        recordUids = QStringList { rootRecordUid };
    }

    auto firstRecord = true;
    for (auto recordIt = recordUids.constBegin(); recordIt != recordUids.constEnd(); recordIt++)
    {
        auto recordUid = *recordIt;

        // Append to base data.
        auto oneRecord = QJsonObject
        {
            { "type", "Feature" },
            { "geometry", geometry },
        };

        auto recordJson = addRecordToJson(baseData, sighting, recordUid, true, 0);

        if (obsCounter)
        {
            recordJson.insert(SMARTSchema::FIELD_OBS_COUNTER, *obsCounter);
            *obsCounter += 1;
        }

        recordJson.insert(SMARTSchema::FIELD_NEW_WAYPOINT, firstRecord);
        firstRecord = false;

        properties["sighting"] = recordJson;

        properties["id"] = recordUid;
        oneRecord["properties"] = properties;

        // Build and add the sighting.
        result.append(oneRecord);
    }

    return result;
}

bool SMARTProvider::exportSightings(Form* form, QFile& exportFile, const std::function<bool()>& startFile, const std::function<bool()>& completeFile, const std::function<void(const QString&, const QVariantMap&, Database::DataFlags, int)>& doneOne, ExportEnumState* enumState)
{
    auto projectUid = m_project->uid();
    auto sightingUids = QStringList();

    m_database->getData(projectUid, form->stateSpace(), Database::DataFlag::DB_SIGHTING_FLAG, &sightingUids);

    App::instance()->initProgress(projectUid, "SMARTExportData", { (int)sightingUids.count() });

    auto index = 0;
    auto success = true;

    m_database->enumData(projectUid, form->stateSpace(), Database::DataFlag::DB_SIGHTING_FLAG, [&](const QString& /*projectUid*/, const QString& sightingUid, const Database::DbData& dbData)
    {
        index++;
        if (!success)
        {
            return;
        }

        // Skip already exported sightings.
        if (dbData.flags.testFlag(Database::DataFlag::DB_EXPORTED_FLAG))
        {
            return;
        }

        // Open a new file as needed.
        if (!exportFile.isOpen() && !startFile())
        {
            qDebug() << "Error: failed to create a new export file";
            success = false;
            return;
        }

        auto sighting = form->createSightingPtr(dbData.data);

        auto featureArray = sightingToJson(sighting.get(), &enumState->obsCounter);

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
                    success = false;
                    return;
                }
            }

            enumState->first = false;

            if (!exportFile.write(featureJson.toJson(JSON_FORMAT)))
            {
                qDebug() << "Failed to write data record: " << exportFile.errorString();
                success = false;
                return;
            }
        }

        // Complete the file if it is over maxFileSizeBytes.
        if ((exportFile.size() > enumState->maxFileSizeBytes) && !completeFile())
        {
            success = false;
            return;
        }

        // Success.
        doneOne(sightingUid, dbData.data, dbData.flags, featureArray.count());

        if ((index + 100) % 100 == 0)
        {
            App::instance()->sendProgress(index);
        }
    });

    App::instance()->sendProgress(sightingUids.count());

    return success;
}

bool SMARTProvider::exportData()
{
    qDebug() << "Exporting data to: " << m_smartDataPath;

    return exportDataInternal(-1, [&](const QString& filePath, ExportFileInfo exportFileInfo) -> bool
    {
        qDebug() << "Exporting data file: " << QFileInfo(filePath).fileName();

        auto smartDataFilePath = m_smartDataPath + "/" + QFileInfo(filePath).fileName();

        auto result = QFile::rename(filePath, smartDataFilePath);
        if (!result)
        {
            if (!QFile::copy(filePath, smartDataFilePath))
            {
                m_lastExportError = "Error 4a";
                qDebug() << "Failed to copy export file";
                return false;
            }
        }

#if defined(Q_OS_ANDROID)
        exportFileInfo.downloadUri = App::instance()->copyToDownload(smartDataFilePath, "SMARTdata");
        if (!exportFileInfo.downloadUri.isEmpty())
        {
            qDebug() << "Exported file from " << smartDataFilePath << " to " << exportFileInfo.downloadUri;
        }
        else
        {
            qDebug() << "Failed to export file to download folder";
        }
#endif

        registerExportFile(smartDataFilePath, exportFileInfo.toMap());
        registerBackupFile(smartDataFilePath);

        Utils::mediaScan(smartDataFilePath);

        auditAdd("Export data success");

        return result;
    });
}

bool SMARTProvider::exportDataInternal(int maxFileSizeBytes, const std::function<bool(const QString& filePath, ExportFileInfo info)>& finalizeFile)
{
    auto projectUid = m_project->uid();

    m_lastExportError.clear();

    // Create a working path.
    auto workPath = App::instance()->workPath() + "/Export_" + m_project->uid();
    QDir(workPath).removeRecursively();
    qFatalIf(!Utils::ensurePath(workPath), "Failed to create working folder");

    // Generate a filename based on export time.
    auto prefix = QString("SMDATA_");
    auto caUuid = m_form->schema()->getSetting("caUuid").toString();
    if (!caUuid.isEmpty())
    {
        prefix += caUuid + '_';
    }

    auto now = App::instance()->timeManager()->currentDateTime();
    auto exportFileNameRoot = Utils::makeUniqueFilename(prefix, now, "_Patrol");
    auto exportFileName = QString();
    auto fileSuffixNext = 1;
    QFile file;

    auto exportFileInfo = ExportFileInfo { projectUid, SMARTProvider::PROVIDER_NAME, m_project->title(), now.toString("yyyy-MM-dd hh:mm:ss") };

    auto enumState = ExportEnumState();
    enumState.maxFileSizeBytes = maxFileSizeBytes == -1 ? 1024 * 1024 * 64 : maxFileSizeBytes;

    auto startFile = [&]() -> bool
    {
        enumState.first = true;
        exportFileInfo.reset();

        exportFileName = exportFileNameRoot + "-" + QString::number(fileSuffixNext).rightJustified(4, '0') + ".json";

        qDebug() << "ExportInternal: " << exportFileName;

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

    auto doneOne = [&](const QString& /*sightingUid*/, const QVariantMap& data, Database::DataFlags flags, int featureCount) -> void
    {
        if (!flags.testFlag(Database::DataFlag::DB_SIGHTING_FLAG))
        {
            return;
        }

        exportFileInfo.sightingCount++;
        exportFileInfo.locationCount += featureCount - 1;

        auto timestamp = data["createTime"].toString();

        if (exportFileInfo.startTime.isEmpty())
        {
            exportFileInfo.startTime = timestamp;
        }

        if (exportFileInfo.stopTime.isEmpty() || exportFileInfo.stopTime < timestamp)
        {
            exportFileInfo.stopTime = timestamp;
        }
    };

    // Export patrol sightings.
    if (m_hasPatrol)
    {
        auto formPatrol = App::instance()->createFormPtr(projectUid, SMARTSchema::TYPE_PATROL);
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
        auto formIncident = App::instance()->createFormPtr(projectUid, SMARTSchema::TYPE_INCIDENT);
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
    m_database->setDataFlagsAll(projectUid, Database::DataFlag::DB_EXPORTED_FLAG | Database::DataFlag::DB_READONLY_FLAG);

    QDir(workPath).removeRecursively();
    return true;
}

bool SMARTProvider::hasUploadData() const
{
    return hasUploadDataLegacy() || m_uploadQueue->hasItems(m_project->uid());
}

bool SMARTProvider::uploadData()
{
    if (!uploadDataLegacy())
    {
        return false;
    }

    qDebug() << "uploadData for project: " << m_project->uid();

    auto upload = [&](Form* form)
    {
        m_database->transaction([&]() -> bool
        {
            auto items = QList<Database::DbQueueItem>();
            auto obsCounter = 1;
            m_database->enumData(m_project->uid(), form->stateSpace(), Database::DataFlag::DB_SIGHTING_FLAG, Database::DataFlag::DB_SUBMITTED_FLAG, [&](const QString& projectUid, const QString& sightingUid, const Database::DbData& dbData)
            {
                auto sighting = form->createSightingPtr(dbData.data);
                auto featureArray = sightingToJson(sighting.get(), &obsCounter);
                auto dataServer = m_profile["DATA_SERVER"].toMap();
                auto payload = QVariantMap {{ "type", "FeatureCollection" }, { "features", featureArray }};

                auto item = Database::DbQueueItem();
                item.uid = Utils::createUuid();
                item.projectUid = projectUid;
                item.url = dataServer["URL"].toString();
                item.dataUid = sighting->rootRecordUid();
                item.webhook = form->renderer()->getSightingWebhook(m_project, sighting.get());
                item.summary = form->renderer()->getSightingSummaryText(sighting.get());

                auto payloadFilename = Utils::makeUniqueFilename(item.uid, App::instance()->timeManager()->currentDateTime(), QString("payload.json"));
                auto payloadFilePath = App::instance()->getMediaFilePath(payloadFilename);
                Utils::writeDataToFile(payloadFilePath, Utils::variantMapToJson(payload, JSON_FORMAT));

                item.attachments = QStringList { payloadFilename };
                item.payload = Utils::variantMapToJson(dataServer);

                items.append(item);

                m_database->appendQueueItem(item);
                m_database->setDataFlags(projectUid, sightingUid, Database::DataFlag::DB_SUBMITTED_FLAG);
            });

            return true;
        });
    };

    // Stop the upload queue processing.
    m_uploadQueue->stop();
    auto restartOnExit = qScopeGuard([&] { m_uploadQueue->start(); });

    // Patrols.
    if (m_hasPatrol)
    {
        auto formPatrol = App::instance()->createFormPtr(m_project->uid(), SMARTSchema::TYPE_PATROL);
        upload(formPatrol.get());
    }

    // Independent incidents.
    if (m_hasIncident)
    {
        auto formIncident = App::instance()->createFormPtr(m_project->uid(), SMARTSchema::TYPE_INCIDENT);
        upload(formIncident.get());
    }

    // Run synchronously.
    auto result = m_uploadQueue->processSync(m_project->uid());

    if (!result)
    {
        m_lastExportError = "Error 6";
        qDebug() << "Failed to upload data";
        auditAdd("Upload data failed");
    }
    else
    {
        auditAdd("Upload data success");
        clearCompletedData();
    }

    return result;
}

bool SMARTProvider::hasUploadDataLegacy() const
{
    auto workPath = App::instance()->workPath() + "/Upload_" + m_project->uid();
    qFatalIf(!Utils::ensurePath(workPath), "Failed to create upload folder");
    auto fileInfos = QDir(workPath).entryInfoList(QDir::Files, QDir::Time);

    return !fileInfos.isEmpty();
}

bool SMARTProvider::uploadDataLegacy()
{
    qDebug() << "Upload data for project: " << m_project->uid();

    // Create a working path.
    auto workPath = App::instance()->workPath() + "/Upload_" + m_project->uid();
    // Do not clean up, in case there are leftovers from a previous incomplete upload.
    qFatalIf(!Utils::ensurePath(workPath), "Failed to create upload folder");

    // Export data to small-ish files and move them to the upload folder.
    // auto result = exportDataInternal(1024 * 256, [&](const QString& filePath, const ExportFileInfo& /*exportFileInfo*/) -> bool
    // {
    //     auto fileInfo = QFileInfo(filePath);

    //     qDebug() << "Moving file to upload queue: " << fileInfo.fileName();

    //     auto uploadFilePath = workPath + "/" + fileInfo.fileName();
    //     auto result = QFile::rename(filePath, uploadFilePath);
    //     if (!result)
    //     {
    //         qDebug() << "Failed to rename export file: " << filePath;
    //     }

    //     return result;
    // });

    // if (!result)
    // {
    //     qDebug() << "Failed to export data to upload folder";
    //     return false;
    // }

    // Get connection parameters.
    auto connector = m_profile["DATA_SERVER"].toMap();
    auto serverUrl = connector["URL"].toString();
    auto apiKey = m_project->accessToken();
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
    App::instance()->initProgress("SMART", "Upload", { (int)fileInfos.count() });
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

        auto dataFilename = QFileInfo(dataFile.fileName()).fileName();
        auto serverHost = QUrl(serverUrl).host();
        qDebug() << "Uploading file: " << dataFilename << serverHost;

        auto response = Utils::httpPostData(App::instance()->networkAccessManager(), serverUrl, dataFile.readAll(), token, tokenType, deflate);
        if (!response.success)
        {
            qDebug() << "Error uploading: " << response.status << response.errorString << dataFilename << serverHost;
            return false;
        }

        qDebug() << "Upload success: " << response.status << dataFilename << serverHost;

        dataFile.close();

        // BUGBUG: Backup the data file.
        dataFile.copy(App::instance()->backupPath() + "/" + QFileInfo(dataFile.fileName()).fileName());
        qDebug() << "Backup created: " << dataFilename;

        dataFile.remove();
        Utils::mediaScan(*it);

        App::instance()->sendProgress(index, 0);
        index++;
    }

    // Data has been accepted by the server, so cleanup the folder.
    qDebug() << "Upload complete: removing local data";
    QDir(workPath).removeRecursively();

    return true;
}

bool SMARTProvider::recoverAndClearData()
{
    auto projectUid = m_project->uid();

    // Remove the exported flag so we always get all the data.
    m_database->setDataFlagsAll(projectUid, Database::DataFlag::DB_EXPORTED_FLAG, false);

    // Export the data.
    if (exportData())
    {
        auditAdd("Recover online data success");
        clearCompletedData();
        return true;
    }

    return false;
}

bool SMARTProvider::clearCompletedData()
{
    auto projectUid = m_project->uid();
    auto app = App::instance();
    auto disableArchive = m_project->disableArchive();

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
        app->deleteSightingsByFlag(projectUid, SMARTSchema::TYPE_COLLECT, Database::DataFlag::DB_UPLOADED_FLAG, disableArchive);
        app->deleteSightingsByFlag(projectUid, SMARTSchema::TYPE_COLLECT, Database::DataFlag::DB_EXPORTED_FLAG, disableArchive);
    }

    // Clear patrol sightings.
    if (m_hasPatrol)
    {
        app->deleteSightingsByFlag(projectUid, SMARTSchema::TYPE_PATROL, Database::DataFlag::DB_UPLOADED_FLAG, disableArchive);
        app->deleteSightingsByFlag(projectUid, SMARTSchema::TYPE_PATROL, Database::DataFlag::DB_EXPORTED_FLAG, disableArchive);
        update_patrolLegs(QVariantList());
    }

    // Clear independent incidents.
    if (m_hasIncident)
    {
        app->deleteSightingsByFlag(projectUid, SMARTSchema::TYPE_INCIDENT, Database::DataFlag::DB_UPLOADED_FLAG, disableArchive);
        app->deleteSightingsByFlag(projectUid, SMARTSchema::TYPE_INCIDENT, Database::DataFlag::DB_EXPORTED_FLAG, disableArchive);
    }

    // Clear completed data.
    saveProjectState();

    // Reset form states.
    m_database->deleteFormState(projectUid, SMARTSchema::TYPE_COLLECT);
    m_database->deleteFormState(projectUid, SMARTSchema::TYPE_PATROL);
    m_database->deleteFormState(projectUid, SMARTSchema::TYPE_INCIDENT);

    // Compact the database.
    m_database->compact();

    // Cleanup references.
    app->garbageCollectMedia();
    app->garbageCollectArchive();

    auditAdd("Clear completed data");

    return true;
}

void SMARTProvider::startTrack(double initialTrackDistance, int initialTrackCounter)
{
    auto pingUpdateInterval = 0;
    auto trackUpdateInterval = 0;
    auto trackDistanceFilter = 0.0;

    if (m_smartVersion >= 700) // SMART 7+.
    {
        // Outlier detection.
        auto transportElementUid = m_form->getFieldValue(SMARTSchema::FIELD_PATROL_TRANSPORT).toString();
        if (!transportElementUid.isEmpty())
        {
            setPatrolLegValue("transportElementUid", transportElementUid);

            auto transportElement = m_form->getElement(transportElementUid);
            auto max_speed = transportElement->getTagValue("max_speed").toInt();
            App::instance()->settings()->set_locationOutlierMaxSpeedKph(max_speed);
        }

        // Ping.
        if (m_profile.contains("POSITION_UPDATES") && shouldUploadData())
        {
            auto frequencyMinutes = m_profile["POSITION_UPDATES"].toMap()["FREQUENCY_MIN"].toInt();
            pingUpdateInterval = frequencyMinutes * 60;
        }

        // Track.
        auto tt = m_form->getFieldValue(SMARTSchema::FIELD_PATROL_TRANSPORT);
        auto te = m_form->getElement(tt.toString());
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
            auto extraData = m_form->schema()->getSetting("extraData").toList();
            for (auto it = extraData.constBegin(); it != extraData.constEnd(); it++)
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
    m_form->pointStreamer()->start(pingUpdateInterval * 1000, 0);

    // Configure track streamer.
    m_form->trackStreamer()->start(trackUpdateInterval * 1000, trackDistanceFilter, initialTrackDistance, initialTrackCounter);
    setPatrolLegValue("rate", m_form->trackStreamer()->rateText());

    qDebug() << "Track start: " << m_form->trackStreamer()->rateText();
}

void SMARTProvider::stopTrack()
{
    qDebug() << "Track stop: " << m_form->trackStreamer()->rateText();

    m_form->trackStreamer()->stop();
    m_form->pointStreamer()->stop();

    // Disable outlier detection.
    App::instance()->settings()->set_locationOutlierMaxSpeedKph(0);
}

bool SMARTProvider::uploadPing(Location* location)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");

    auto result = false;

    if (m_smartVersion >= 700)
    {
        if (m_profile.contains("POSITION_UPDATES"))
        {
            result = uploadPing7(location);
        }
    }
    else
    {
        result = uploadPing6(location);
    }

    return result;
}

bool SMARTProvider::uploadPing6(Location* location)
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
    dataServer["URL"] = url;

    // Create and fixup the sighting JSON.
    auto extraProps = QVariantMap
    {
        { "typeUuid", typeUuid },
        { "caUuid", caUuid },
        { "level", 5 },
        { "patrolId", m_patrolUid },
        { "description", "" },
    };

    auto locationJson = locationToJson(QUuid::createUuid().toString(QUuid::Id128), location, extraProps);
    auto features = QJsonArray();
    features.append(locationJson);

    auto event = QJsonObject
    {
        { "type", "FeatureCollection" },
        { "features", features },
    };

    auto payloadJson = QJsonDocument(event).toJson(JSON_FORMAT);

    addToUploadQueue(m_form, nullptr, "PINGv6", tr("Send location"), dataServer, payloadJson);

    return true;
}

bool SMARTProvider::uploadPing7(Location* location)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");
    qFatalIf(!m_profile.contains("POSITION_UPDATES"), "Nowhere to send");

    // Create a ping from the base server URL.
    auto positionUpdates = m_profile["POSITION_UPDATES"].toMap();

    // Create and fixup the sighting JSON.
    auto positionUpdatesMetadata = positionUpdates["METADATA"].toMap();
    auto extraProps = QVariantMap
    {
        { "typeUuid", positionUpdatesMetadata["TYPEUUID"] },
        { "fieldIdentifier", processFieldIdentifier(positionUpdatesMetadata["FIELDIDENTIFIER"].toString()) },
        { "caUuid", positionUpdatesMetadata["CAUUID"] },
        { "level", positionUpdatesMetadata["LEVEL"] },
        { "patrolId", m_patrolUid },
        { "description", "" },
    };

    auto locationJson = locationToJson(QUuid::createUuid().toString(QUuid::Id128), location, extraProps);
    auto features = QJsonArray();
    features.append(locationJson);

    auto event = QJsonObject
    {
        { "type", "FeatureCollection" },
        { "features", features },
    };

    auto payloadJson = QJsonDocument(event).toJson(JSON_FORMAT);

    addToUploadQueue(m_form, nullptr, "PINGv7", tr("Send location"), positionUpdates, payloadJson);

    return true;
}

bool SMARTProvider::uploadAlerts(Form* form, const QString& sightingUid)
{
    qFatalIf(!m_hasDataServer, "No data server");

    auto result = false;

    if (m_smartVersion >= 700)
    {
        if (m_profile.contains("ALERTS"))
        {
            result = uploadAlerts7(form, sightingUid);
        }
    }
    else
    {
        result = uploadAlerts6(form, sightingUid);
    }

    return result;
}

bool SMARTProvider::uploadAlerts6(Form* form, const QString& sightingUid)
{
    qFatalIf(!m_hasDataServer, "No data server");

    auto sighting = form->createSightingPtr(sightingUid);
    auto recordUids = sighting->recordManager()->recordUids();

    auto extraData = form->schema()->getSetting("extraData").toList();
    for (auto extraItemIt = extraData.constBegin(); extraItemIt != extraData.constEnd(); extraItemIt++)
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
                auto modelElementUid = sighting->getRecord(recordUid)->getFieldValue(SMARTSchema::FIELD_MODEL_TREE).toString();
                if (modelElementUid.isEmpty())
                {
                    continue;
                }

                auto modelElement = form->getElement(modelElementUid);
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
            auto extraProps = QVariantMap
            {
                { "typeUuid", alertItem["type"] },
                { "caUuid", caUuid },
                { "level", alertItem["level"] },
                { "description", "" },
            };

            auto event = QVariantMap
            {
                { "type", "FeatureCollection" },
                { "features", sightingToJson(sighting.get(), nullptr, recordUid, extraProps) }
            };

            auto summary = QString("%1: %2").arg(tr("Alert"), form->renderer()->getSightingSummaryText(sighting.get()));

            // Add to upload queue.
            addToUploadQueue(form, sighting.get(), "ALERTv6", summary, dataServer, Utils::variantMapToJson(event, JSON_FORMAT));
        }
    }

    return true;
}

bool SMARTProvider::uploadAlerts7(Form* form, const QString& sightingUid)
{
    qFatalIf(!m_profile.contains("ALERTS"), "Nowhere to send");

    auto sighting = form->createSightingPtr(sightingUid);
    auto recordUids = sighting->recordManager()->recordUids();

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
                auto modelElementUid = sighting->getRecord(recordUid)->getFieldValue(SMARTSchema::FIELD_MODEL_TREE).toString();
                if (modelElementUid.isEmpty())
                {
                    continue;
                }

                auto modelElement = form->getElement(modelElementUid);
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
            auto dataServer = QVariantMap
            {
                { "URL", alertMap["URL"].toString() + recordUid },
                { "USERNAME", alertMap["USERNAME"].toString() },
                { "PASSWORD", alertMap["PASSWORD"].toString() },
            };

            // Create and fixup the sighting JSON.
            auto alertMetadata = alertMap["METADATA"].toMap();
            auto extraProps = QVariantMap
            {
                { "typeUuid", alertMetadata["TYPEUUID"] },
                { "fieldIdentifier", processFieldIdentifier(alertMetadata["FIELDIDENTIFIER"].toString()) },
                { "caUuid", alertMetadata["CAUUID"] },
                { "level", alertMetadata["LEVEL"] },
                { "description", sighting->summaryText(true) },
            };

            auto event = QVariantMap
            {
                { "type", "FeatureCollection" },
                { "features", sightingToJson(sighting.get(), nullptr, recordUid, extraProps) }
            };

            auto summary = QString("%1: %2").arg(tr("Alert"), form->renderer()->getSightingSummaryText(sighting.get()));

            // Add to upload queue.
            addToUploadQueue(form, sighting.get(), "ALERTv7", summary, dataServer, Utils::variantMapToJson(event, JSON_FORMAT));
        }
    }

    return true;
}

bool SMARTProvider::uploadSighting(Form* form, const QString& sightingUid, int* obsCounter)
{
    qFatalIf(!shouldUploadData(), "Upload disabled");

    auto sighting = form->createSightingPtr(sightingUid);

    auto event = QVariantMap
    {
        { "type", "FeatureCollection" },
        { "features", sightingToJson(sighting.get(), obsCounter) }
    };

    auto summary = form->renderer()->getSightingSummaryText(sighting.get());

    m_database->transaction([&]() -> bool
    {        
        addToUploadQueue(form, sighting.get(), "DATA", summary, m_profile["DATA_SERVER"].toMap(), Utils::variantMapToJson(event, JSON_FORMAT));
        m_database->setDataFlags(m_project->uid(), sightingUid, Database::DataFlag::DB_READONLY_FLAG);
        emit App::instance()->sightingFlagsChanged(m_project->uid(), sightingUid);

        return true;
    });

    return true;
}

void SMARTProvider::addToUploadQueue(Form* form, Sighting* sighting, const QString& type, const QString& summary, const QVariantMap& dataServer, const QByteArray& payload) const
{
    auto queueItem = Database::DbQueueItem();
    queueItem.projectUid = m_project->uid();
    queueItem.dataUid = sighting ? sighting->rootRecordUid() : QString();
    queueItem.type = type;
    queueItem.url = dataServer["URL"].toString();
    queueItem.summary = summary;
    queueItem.webhook = sighting ? form->renderer()->getSightingWebhook(m_project, sighting) : QByteArray();
    queueItem.flags |= type.startsWith("PING") ? Database::QueueFlag::DB_AUTO_DELETE : Database::QueueFlag::DB_NO_FLAG;

    auto payloadFilename = Utils::makeUniqueFilename("SM" + type + "_", App::instance()->timeManager()->currentDateTime(), QString("_upload.json"));
    auto payloadFilePath = App::instance()->getMediaFilePath(payloadFilename);
    Utils::writeDataToFile(payloadFilePath, payload);

    queueItem.attachments = QStringList { payloadFilename };
    queueItem.payload = Utils::variantMapToJson(dataServer);

    m_uploadQueue->addItem(queueItem);
    m_uploadQueue->kick();

    registerBackupFile(payloadFilePath);
}

QVariant SMARTProvider::getProfileValue(const QString& key, const QVariant& defaultValue) const
{
    return m_profile.value(key, defaultValue);
}

void SMARTProvider::loadProjectState()
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
    m_pingData = map.value("pingData").toMap();
    m_pingUuid = map.value("pingUuid").toString();
    m_patrolStartTimestamp = map.value("patrolStartTimestamp").toString();
    m_patrolTimeZone = map.value("patrolTimeZone").toString();
}

void SMARTProvider::saveProjectState()
{
    auto map = QVariantMap
    {
        { "patrolUid", m_patrolUid },
        { "patrolStarted", m_patrolStarted },
        { "patrolPaused", m_patrolPaused },
        { "patrolPausedDistance", m_patrolPausedDistance },
        { "patrolLegs", m_patrolLegs },
        { "incidentStarted", m_incidentStarted },
        { "collectStarted", m_collectStarted },
        { "collectUser", m_collectUser },
        { "uploadObsCounter", m_uploadObsCounter },
        { "lastUploadTime", m_uploadLastTime },
        { "pingData", m_pingData },
        { "pingUuid", m_pingUuid },
        { "patrolStartTimestamp", m_patrolStartTimestamp },
        { "patrolTimeZone", m_patrolTimeZone },
    };

    m_form->saveState();

    m_project->set_providerState(map);
}

bool SMARTProvider::connectToProject()
{
    m_profile = Utils::variantMapFromJsonFile(getProjectFilePath("ct_profile.json"));

    auto project = Utils::variantMapFromJsonFile(getProjectFilePath("project.json"));
    m_smartVersion = qFloor(project.value("smart_version", 6.00).toDouble() * 100);

    // Hack, because SIMPLE_CAMERA should only be enabled on 810 and above.
    if (m_smartVersion < 810 && m_project->simpleCamera())
    {
        m_project->set_simpleCamera(false);
    }

    auto metadataFile = project.value("metadata").toString();
    m_hasCollect = metadataFile == "smartcollect_metadata.json";
    m_hasPatrol = (metadataFile == "patrol_metadata.json") || (metadataFile == "survey_metadata.json");
    m_hasIncident = (metadataFile == "incident_metadata.json") || project.contains("incident");

    m_hasDataServer = m_profile.contains("DATA_SERVER");
    m_uploadFrequencyMinutes = m_profile.value("DATA_SERVER").toMap().value("FREQUENCY_MIN", -1).toInt();
    m_manualUpload = m_hasDataServer && (m_uploadFrequencyMinutes == -1);

    auto stateSpace = m_form->stateSpace();
    m_patrolForm = stateSpace == SMARTSchema::TYPE_PATROL;
    m_incidentForm = stateSpace == SMARTSchema::TYPE_INCIDENT;
    m_collectForm = stateSpace == SMARTSchema::TYPE_COLLECT;

    // This overrides the global setting, but that is desirable.
    if (m_profile.contains("PROJECTION_EPSG"))
    {
        auto epsg = m_profile.value("PROJECTION_EPSG").toInt();
        auto projection = App::instance()->settings()->projection();
        if (epsg != projection.value("i", -1).toInt())
        {
            auto projections = Utils::variantListFromJsonFile(":/config/projections.json");
            auto found = false;
            for (auto projectionIt = projections.constBegin(); projectionIt != projections.constEnd(); projectionIt++)
            {
                if (projectionIt->toMap().value("i").toInt() == epsg)
                {
                    App::instance()->settings()->set_projection(projectionIt->toMap());
                    found = true;
                    break;
                }
            }

            if (found)
            {
                qDebug() << "Projection found: " << App::instance()->settings()->projection()["d"];
            }
            else
            {
                qDebug() << "Projection not found for EPSG: " << epsg;
            }
        }
    }

    // Misc properties.
    m_surveyMode = project.value("metadata").toString().contains("survey");
    m_allowManualGPS = m_profile.value("MANUAL_GPS", false).toBool() || m_profile.value("USE_MAP_ON_SKIP", false).toBool();
    m_useDistanceAndBearing = m_profile.value("RECORD_DISTANCE_BEARING", false) == true;
    m_useGroupUI = m_profile.value("INCIDENT_GROUP_UI", false) == true;
    m_useObserver = m_profile.value("RECORD_OBSERVER", false) == true;
    m_instantGPS = m_samplingUnits = m_useIncidentType = false;
    m_allowHistory = m_profile.value("HISTORY_MODE", true) == true;

    // Units.
    m_useMetric = m_profile.value("UNITS", "METRIC").toString() == "METRIC";
    App::instance()->settings()->set_metricUnits(m_useMetric);

    // Globals.
    /// Dark theme was enabled.
    /// @category Settings
    /// @default false
    /// @docs https://cybertrackerwiki.org/smart/user-manual/7#743-dark-theme
    setUsage("darkTheme", App::instance()->settings()->darkTheme());
    /// Setting: UI font scale percent (e.g. 100, 125, 150).
    /// @category Settings
    /// @default 125
    /// @docs https://cybertrackerwiki.org/smart/user-manual/#745-font-size
    setUsage("fontSize", App::instance()->settings()->fontSize());
    /// Setting: required GPS accuracy in metres for a fix.
    /// @category GPS
    /// @default 80
    /// @docs https://cybertrackerwiki.org/smart/user-manual/#747-gps-accuracy-meters
    setUsage("locationAccuracy", App::instance()->settings()->locationAccuracyMeters());
    /// Speed threshold (kph) above which GPS fixes are rejected as outliers. This can be configured per-patrol type, e.g. it can be different for foot and vehicle patrols.
    /// @category GPS
    /// @default 0
    setUsage("locationOutlierSpeed", App::instance()->settings()->locationOutlierMaxSpeedKph());

    // Record cumulative package-configured features for telemetry.
    /// Project title
    /// @category Project
    /// @hidden true
    setUsage("title", m_project->title());

    /// SMART Desktop version for this package. Shown as an aggregate distribution on the Summary tab, so the raw per-form field is hidden.
    /// @category Project
    /// @hidden true
    setUsage("smartVersion", m_smartVersion);

    /// Metadata file used by the project (patrol/incident/survey/smartcollect).
    /// @category Data model
    /// @hidden true
    setUsage("metadataFile", metadataFile);

    /// Survey package type
    /// @category Data model
    /// @default false
    setUsage("surveyMode", m_surveyMode);

    /// Independent incident included in package. Independent incidents are observations which are not necessarily part of a patrol. Some packages only contain independent incidents, i.e. no patrol support.
    /// @category Data model
    /// @default false
    /// @docs https://cybertrackerwiki.org/smart/user-manual/8#82-independent-incidents
    setUsage("hasIncident", m_hasIncident);

    /// When in areas of poor GPS reception, the user can specify the location manually, i.e. specifying the coordinates in an edit box or by positioning a pointer on a map. If the GPS location is current, then the user will not get the option of specifying this.
    /// @category GPS
    /// @default false
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#652-gps-tab
    setUsage("allowManualGPS", m_allowManualGPS);

    /// Add fields for distance and bearing on sighting home page. These are collected in addition to location.
    /// @category GPS
    /// @default false
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#6262-distance-and-bearing
    setUsage("useDistanceAndBearing", m_useDistanceAndBearing);

    /// Incident group UI is enabled on sighting home page.
    /// @category Config model
    /// @default false
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#6511-incident-group-ui
    setUsage("useGroupUI", m_useGroupUI);

    /// Observer name can be specified for each observation.
    /// @category Config model
    /// @default false
    setUsage("useObserver", m_useObserver);

    /// History lets the user see the captured data in the current session. It is cleared when the patrol ends and data is uploaded. Disallowing it is about making the UI a bit simpler.
    /// @category Control
    /// @default true
    /// @docs https://cybertrackerwiki.org/smart/user-manual/#83-history
    setUsage("allowHistory", m_allowHistory);

    /// Units are metric (true) or imperial (false).
    /// @category Settings
    /// @default true
    /// @docs https://cybertrackerwiki.org/smart/user-manual/7#744-metric-units
    setUsage("useMetric", m_useMetric);

    /// Use the time from the GPS rather than the system clock. Useful for devices which never connect to a network. This is automatically disabled for devices using Connect server.
    /// @category GPS
    /// @default true
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#652-gps-tab
    setUsage("useGpsTime", requireGPSTime());

    /// Data is uploaded to SMART Connect. false means that this is an entirely offline scenario with data uploaded over USB.
    /// @category Connect
    /// @default false
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#636-connect-tab
    setUsage("hasDataServer", m_hasDataServer);

    /// Data is uploaded automatically or manually (-1).
    /// @category Connect
    /// @default -1
    /// @hidden true
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#636-connect-tab
    setUsage("uploadFrequencyMinutes",m_uploadFrequencyMinutes);

    /// Data archiving is active, i.e. the user enabled data archiving.
    /// @category Control
    /// @default false
    /// @docs https://cybertrackerwiki.org/smart/features/#archive
    setUsage("archiveMode", m_profile.value("ARCHIVE_MODE", false).toBool());

    /// Alerts are configured for this package. Alerts send a notification to the server when a condition matches on an observation, e.g. species is a specific value. This is sent immediately, regardless of the upload frequency configuration. If offline, they are sent when a connection if restored. Only relevant when a data server is configured.
    /// @category Connect
    /// @default false
    /// @normalizeWhen hasDataServer
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#637-alerts
    setUsage("alerts", m_profile.contains("ALERTS"));

    /// Device location is sent periodically. Only relevant when a data server is configured.
    /// @category Connect
    /// @default false
    /// @normalizeWhen hasDataServer
    setUsage("positionUpdates", m_profile.contains("POSITION_UPDATES"));

    /// Kiosk mode is enabled.
    /// @category Settings
    /// @default false
    /// @docs https://cybertrackerwiki.org/smart/user-manual/4
    setUsage("kioskMode", App::instance()->kioskMode());

    /// Patrol pause is allowed.
    /// @category Control
    /// @default true
    /// @docs https://cybertrackerwiki.org/smart/user-manual/#6514-can-pause
    setUsage("canPause", m_profile.value("CAN_PAUSE", true).toBool());

    /// Legacy projection: 0 = Degrees Minutes Seconds, 1 = Decimal degrees, 2 = UTM. Pre-SMART 8 setting, superseded by projectionEpsg; shown only for projects before the 8.0 data model.
    /// @category GPS
    /// @default 1
    /// @until 800
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#635-basemap-settings-tab
    setUsage("projection", m_profile.value("PROJECTION", 1).toInt());

    /// Map projection EPSG code. SMART 8+ feature; shown only for projects on the 8.0+ data model.
    /// @category GPS
    /// @default -1
    /// @since 800
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#635-basemap-settings-tab
    setUsage("projectionEpsg", m_profile.value("PROJECTION_EPSG", -1).toInt());

    /// Number of GPS fixes to take for each sighting. When the track timer is enabled, this is immediate. This is to account for GPS warmup, whereby the first reading or two may be invalid.
    /// @category GPS
    /// @default 10
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#652-gps-tab
    setUsage("sightingFixCount", m_profile.value("SIGHTING_FIX_COUNT", 0).toInt());

    /// Number of seconds until "skip" button appears on GPS acquisition screen.
    /// @category GPS
    /// @default 3
    /// @discrete true
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#6525-skip-button-timeout
    setUsage("skipButtonTimeout", m_profile.value("SKIP_BUTTON_TIMEOUT", 3).toInt());

    /// Track interval, interpreted in seconds when waypointTimerType is TIME or metres when DISTANCE. Split into two unit-correct charts in the dashboard, so the raw mixed-unit field is hidden.
    /// @category GPS
    /// @default -1
    /// @hidden true
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#652-gps-tab
    setUsage("waypointTimer", m_profile.value("WAYPOINT_TIMER", -1).toInt());

    /// Track type: DISTANCE or TIME.
    /// @category GPS
    /// @default TIME
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#652-gps-tab
    setUsage("waypointTimerType", m_profile.value("WAYPOINT_TIMER_TYPE", "").toString());

    /// Maximum number of photos allowed per observation.
    /// @category Photos
    /// @default 3
    /// @docs https://cybertrackerwiki.org/smart/user-manual/#62311-additional-photo-settings
    setUsage("maxPhotoCount", m_profile.value("MAX_PHOTO_COUNT", 3).toInt());

    /// Photos are resized.
    /// @category Photos
    /// @default false
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#62311-additional-photo-settings
    setUsage("resizeImage", m_profile.value("RESIZE_IMAGE", false).toBool());

    /// Photos are resized to this width. 0 = no resizing.
    /// @category Photos
    /// @default 1600
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#62311-additional-photo-settings
    setUsage("imageWidth", m_profile.value("IMAGE_WIDTH", 0).toInt());

    /// Photos are resized to this height. 0 = no resizing.
    /// @category Photos
    /// @default 1200
    /// @docs https://cybertrackerwiki.org/smart/user-manual/6#62311-additional-photo-settings
    setUsage("imageHeight", m_profile.value("IMAGE_HEIGHT", 0).toInt());

    // Load the state.
    loadProjectState();

    // Connected.
    connect(m_form, &Form::connectedChanged, this, [=]()
    {
        auto schema = m_form->schema();

        update_samplingUnits(schema->getSetting("samplingUnits", false).toBool());
        update_instantGPS(schema->getSetting("instantGPS", false).toBool());
        update_useIncidentType(schema->getSetting("useIncidentType", false).toBool());

        // List of sampling units for this survey.
        /// Schema includes sampling units.
        /// @category Data model
        /// @default false
        /// @hidden true
        setUsage("samplingUnits", m_samplingUnits);

        /// Number of sampling units defined for a survey. Shown only for survey-mode projects (surveyMode = true).
        /// @category Data model
        /// @default 0
        /// @normalizeWhen surveyMode
        setUsage("samplingUnitCount", schema->getSetting("samplingUnitCount", 0).toInt());

        /// User can specify the GPS position manually on the sighting home page.
        /// @category GPS
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/6#621-top-level-group
        setUsage("instantGPS", m_instantGPS);

        /// Sightings can have photos.
        /// @category Photos
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/6#6231-photos
        setUsage("photoFirst", schema->getSetting("photoFirst", false).toBool());

        /// Incident type can be changed on sighting home page.
        /// @category Config model
        /// @default false
        setUsage("useIncidentType", m_useIncidentType);

        /// Data upload is manual, typically at the end of a patrol. During the patrol, alerts will be sent immediately, but data and track data will not. Only relevant when a data server is configured.
        /// @category Connect
        /// @default false
        /// @normalizeWhen hasDataServer
        /// @docs https://cybertrackerwiki.org/smart/user-manual/6#636-connect-tab
        setUsage("manualUpload", schema->getSetting("manualUpload", false).toBool());

        /// Sighting editing is disabled. Editing only works if the data is batched, i.e. not uploaded immediately. Disabling may be for data integrity or UI simplicity reasons.
        /// @category Control
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/#6515-disable-editing
        setUsage("disableEditing", schema->getSetting("disableEditing", false).toBool());

        /// Data archiving is allowed for this package. Normally the user can activate data retention (archiving) for any captured data. This override lets the desktop disallow this behavior. SMART 8+ feature; pre-8 packages report the default only.
        /// @category Control
        /// @default true
        /// @since 800
        /// @docs https://cybertrackerwiki.org/smart/features/#archive
        setUsage("allowArchive", schema->getSetting("allowArchive", true).toBool());

        /// One or more model categories support collection of multiple data points within an observation. Multiple observations are like repeat groups.
        /// @category Data model
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/#625-collect-multiple-observations
        setUsage("hasCollectMultipleObs", schema->getSetting("hasCollectMultipleObs", false).toBool());

        /// A matrix field is used.
        /// @category Data model
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/#6246-attribute-groups
        setUsage("hasMatrixQuestions", schema->getSetting("hasMatrixQuestions", false).toBool());

        /// A multi-select with subsequent records is used. This is a checklist with a follow-on question for each checklist item. Checking an item triggers a detail record for that item.
        /// @category Config model
        /// @default false
        setUsage("hasMultiSelectFollowups", schema->getSetting("hasMultiSelectFollowups", false).toBool());

        /// One or more metadata items contains an option group.
        /// @category Data model
        /// @default false
        setUsage("hasOptionGroups", schema->getSetting("hasOptionGroups", false).toBool());

        /// One or more metadata items contains a cascading list.
        /// @category Data model
        /// @default false
        /// @hidden true
        setUsage("hasCascadingLists", schema->getSetting("hasCascadingLists", false).toBool());

        /// One or more configurable model observations uses conditional fields. Conditional fields are used to specify whether specific subsequent fields are visible.
        /// @category Config model
        /// @default false
        setUsage("hasConditionalFields", schema->getSetting("hasConditionalFields", false).toBool());

        /// Bar codes are used. QR codes and numerous other bar code types are supported.
        /// @category Data model
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/#6243-text
        setUsage("hasBarcodeFields", schema->getSetting("hasBarcodeFields", false).toBool());

        /// A text editor hint to default to a number keypad for text fields, e.g. phone number.
        /// @category Config model
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/6#6243-text
        setUsage("hasNumpadFields", schema->getSetting("hasNumpadFields", false).toBool());

        /// An area field is used.
        /// @category Data model
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/features/#new-attributes
        setUsage("hasAreaField", schema->getSetting("hasAreaField", false).toBool());

        /// For area fields, manual locations are allowed. Manual means the user can drop a point on the map by tapping.
        /// @category Config model
        /// @default false
        setUsage("areaAllowManual", schema->getSetting("areaAllowManual", false).toBool());

        /// For area fields, auto GPS is allowed. Auto GPS adds a position automatically as the user moves around.
        /// @category Config model
        /// @default false
        setUsage("areaAllowAutoGPS", schema->getSetting("areaAllowAutoGPS", false).toBool());

        /// For area fields, a button appears which adds the current location to the area.
        /// @category Config model
        /// @default false
        setUsage("areaAllowManualGPS", schema->getSetting("areaAllowManualGPS", false).toBool());

        /// For area fields, for auto GPS, this is the GPS timer frequency.
        /// @category Config model
        /// @default false
        setUsage("areaAutoGPSTimeInSeconds", schema->getSetting("areaAutoGPSTimeInSeconds", 0).toInt());

        /// A line field is used.
        /// @category Data model
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/features/#new-attributes
        setUsage("hasLineField", schema->getSetting("hasLineField", false).toBool());

        /// For line fields, manual locations are allowed.
        /// @category Config model
        /// @default false
        setUsage("lineAllowManual", schema->getSetting("lineAllowManual", false).toBool());

        /// For line fields, auto GPS is allowed.
        /// @category Config model
        /// @default false
        setUsage("lineAllowAutoGPS", schema->getSetting("lineAllowAutoGPS", false).toBool());

        /// For line fields, manual GPS is allowed.
        /// @category Config model
        /// @default false
        setUsage("lineAllowManualGPS", schema->getSetting("lineAllowManualGPS", false).toBool());

        /// For line fields, auto GPS timeout in seconds.
        /// @category Config model
        /// @default false
        setUsage("lineAutoGPSTimeInSeconds", schema->getSetting("lineAutoGPSTimeInSeconds", 0).toInt());

        /// Tree flattening was used. This makes hierarchical tree field appear as a flat list.
        /// @category Config model
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/#6245-tree
        setUsage("hasFlattenedTree", schema->getSetting("hasFlattenedTree", false).toBool());

        /// Field help is used. Field help adds an info button next to a field which pops up a page with image and caption.
        /// @category Config model
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/#6241-help-content
        setUsage("hasFieldHelp", schema->getSetting("hasFieldHelp", false).toBool());

        /// Signatures are used.
        /// @category Data model
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/user-manual/6#6261-signatures
        setUsage("hasSignatures", schema->getSetting("hasSignatures", false).toBool());

        /// Photo tags are used. Photo tags allow the user to set custom tags on photos directly from the photo screen. These tags are embedded in the photo as exif tags, so they travel with the photo.
        /// @category Photos
        /// @default false
        /// @docs https://cybertrackerwiki.org/smart/features/#other-features
        setUsage("hasAttachmentTags",     schema->getSetting("hasAttachmentTags", false).toBool());

        // Conservation area and data model statistics.
        /// Conservation Area UUID.
        /// @category Data model
        /// @default ""
        /// @hidden true
        setUsage("caUuid", schema->getSetting("caUuid", "").toString());

        /// Total number of input fields defined by the form schema across the entire data model. Configured models may be a subset of the full data model, so the full model may be larger (but not smaller).
        /// @category Data model
        /// @default 0
        setUsage("fieldCount", schema->getSetting("fieldCount", 0).toInt());

        /// Number of incident types defined in the schema. This is an additional list field which lets the user categorize observations and groups of observations on the sighting home page.
        /// @category Config model
        /// @default 0
        setUsage("incidentTypeCount", schema->getSetting("incidentTypeCount", 0).toInt());
    });

    // Sighting saved.
    connect(m_form, &Form::sightingSaved, this, [=](const QString& sightingUid)
    {
        if (m_database->getDataStateSpace(m_form->projectUid(), sightingUid) != m_form->stateSpace())
        {
            return;
        }

        if (shouldUploadData())
        {
            uploadSighting(m_form, sightingUid, &m_uploadObsCounter);
        }

        if (m_hasDataServer)
        {
            uploadAlerts(m_form, sightingUid);
        }
        saveProjectState();
    });

    // Location saved.
    connect(m_form, &Form::locationSaved, this, [=](Location* /*location*/, const QString& /*locationUid*/)
    {
        if (m_form->trackStreamer()->running())
        {
            setPatrolLegValue("distance", m_form->trackStreamer()->distanceCovered());
        }
    });

    // Pings.
    connect(m_form, &Form::locationPoint, this, [=](Location* location)
    {
        if (!shouldUploadData())
        {
            return;
        }

        if ((m_patrolStarted && !m_patrolPaused) || m_collectStarted)
        {
            uploadPing(location);
        }
    });

    // Sync state across all forms.
    connect(m_form, &Form::projectSettingChanged, this, [&](const QString& name, const QVariant& /*value*/)
    {
        if (name == "providerState")
        {
            loadProjectState();
        }
    });

    return true;
}

QString SMARTProvider::getStartPage() const
{
    auto stateSpace = m_form->stateSpace();

    if (stateSpace.isEmpty())
    {
        if (m_hasCollect)
        {
            return "qrc:/SMART/StartPageCollect.qml";
        }
        else
        {
            return "qrc:/SMART/StartPage.qml";
        }
    }
    else if (stateSpace == SMARTSchema::TYPE_INCIDENT)
    {
        return "qrc:/SMART/SightingHomePage.qml";
    }
    else if (stateSpace == SMARTSchema::TYPE_PATROL)
    {
        if (m_form->editing())
        {
            return "qrc:/SMART/SightingHomePage.qml";
        }
        else if (!m_patrolStarted)
        {
            m_form->resetFieldValue(SMARTSchema::FIELD_LOCATION);
            return "qrc:/SMART/MetadataPage.qml";
        }
        else if (m_patrolPaused)
        {
            return "qrc:/SMART/LocationPage.qml";
        }

        // Should not get here...
        return "qrc:/SMART/SightingHomePage.qml";
    }
    else if (stateSpace == SMARTSchema::TYPE_COLLECT)
    {
        return "qrc:/SMART/SightingHomePage.qml";
    }

    qFatal("stateSpace not recognized");

    return QString();
}

bool SMARTProvider::requireGPSTime() const
{
    // Having a data server implies an internet connection, so the time would be correct.
    return !m_hasDataServer && getProfileValue("USE_GPS_TIME", false) == true;
}

void SMARTProvider::finalizeSightingData(QVariantMap* sightingData) const
{
    auto pages = sightingData->value("pages").toList();
    if (pages.isEmpty())
    {
        return;
    }

    auto lastPage = pages.last().toMap();
    if (lastPage.value("pageUrl").toString().contains("LocationPage.qml"))
    {
        pages.removeLast();
        sightingData->insert("pages", pages);
    }
}

bool SMARTProvider::shouldUploadData() const
{
    return m_hasDataServer && !m_manualUpload;
}

QString SMARTProvider::getPatrolText() const
{
    return SMARTRenderer().getPatrolText(m_surveyMode);
}

QString SMARTProvider::getSightingTypeText(const QString& observationType) const
{
    return SMARTRenderer().getSightingTypeText(observationType, m_surveyMode);
}

bool SMARTProvider::finalizePackage(const QString& packageFilesPath) const
{
    auto types = QStringList { "", SMARTSchema::TYPE_INCIDENT, SMARTSchema::TYPE_PATROL, SMARTSchema::TYPE_COLLECT };

    for (auto it = types.constBegin(); it != types.constEnd(); it++)
    {
        QFile::remove(packageFilesPath + "/Form" + *it + ".qml");
    }

    return true;
}

void SMARTProvider::setUsage(const QString& name, const QVariant& value) const
{
    if (m_form->stateSpace().isEmpty())
    {
        return;
    }

    Provider::setUsage(name, value);
}

QString SMARTProvider::processFieldIdentifier(const QString& fieldIdentifier) const
{
    if (!m_patrolStarted)
    {
        return "";
    }

    static const auto fieldUids = QStringList
    {
        "SMART_Leader",
        "SMART_PatrolTransport",
        "SMART_Mandate",
        "SMART_Station",
        "SMART_Team",
        "SMART_PatrolStartDate",
        "SMART_PatrolStartTime"
    };

    auto schema = m_form->schema();
    auto result = fieldIdentifier;
    for (auto fieldIt = fieldUids.constBegin(); fieldIt != fieldUids.constEnd(); fieldIt++)
    {
        // Do not want to crash if the field is missing for surveys.
        if (schema->getField(*fieldIt) == nullptr)
        {
            continue;
        }

        auto fieldValue = m_form->getFieldDisplayValue(m_form->rootRecordUid(), *fieldIt);
        result = result.replace(QString("{%1}").arg(*fieldIt), fieldValue);
    }

    return result;
}

} // namespace CyberTracker
