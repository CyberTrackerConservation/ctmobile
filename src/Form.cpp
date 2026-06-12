#include "pch.h"
#include "Form.h"
#include "App.h"
#include "qtcsv/stringdata.h"
#include "qtcsv/writer.h"

namespace CyberTracker {

Form::Form(QObject* /*parent*/)
{
    m_connected = false;
    m_editing = false;
    m_stateSpace = "";
    m_readonly = false;
    resetSessionId();

    m_languageCode = QLocale::system().name();
    m_depth = -1;
    m_tracePageChanges = false;
    m_requireUsername = false;

    m_database = App::instance()->database();
    m_timeManager = App::instance()->timeManager();
    m_projectManager = App::instance()->projectManager();
    m_transcribeQueue = App::instance()->transcribeQueue();
    m_schema = nullptr;
    m_trackStreamer = new LocationStreamer(this);
    m_trackStreamer->set_rateLabel(tr("Track"));
    m_pointStreamer = new LocationStreamer(this);
    m_pointStreamer->set_rateLabel(tr("Location"));
    m_fieldStreamer = new LocationStreamer(this);
    m_fieldStreamer->set_rateLabel(tr("Field"));
    m_sighting = createSightingPtr(QVariantMap {}).release();
    m_wizard = new Wizard(this);

    connect(this, &Form::languageCodeChanged, this, [&]()
    {
        m_sighting->set_languageCode(m_languageCode);
    });

    // Process transcribe results.
    connect(m_transcribeQueue, &TranscribeQueue::itemCompleted, this, &Form::processTranscribeResult);

    // Forward sighting change events.
    connect(m_sighting, &Sighting::recalculated, this, [&](const FieldValueChanges& fieldValueChanges, const FieldValueChanges& recordChildValueChanges)
    {
        // Ignore events coming before or after connection.
        if (!m_connected)
        {
            return;
        }

        // Send change events for fields.
        for (auto it = fieldValueChanges.constBegin(); it != fieldValueChanges.constEnd(); it++)
        {
            emit fieldValueChanged(it->recordUid, it->fieldUid, it->oldValue, m_sighting->getRecord(it->recordUid)->getFieldValue(it->fieldUid));
        }

        // Send change events for the owning records.
        for (auto it = recordChildValueChanges.constBegin(); it != recordChildValueChanges.constEnd(); it++)
        {
            emit recordChildValueChanged(it->recordUid, it->fieldUid);
        }

        // Update wizard buttons.
        m_wizard->updateButtons();
    });

    // Make sure sighting locations are part of the track.
    connect(this, &Form::fieldValueChanged, this, [&](const QString& /*recordUid*/, const QString& fieldUid, const QVariant& /*oldValue*/, const QVariant& newValue)
    {
        if (!m_connected || m_editing || m_readonly || !m_trackStreamer->running())
        {
            return;
        }

        auto locationField = qobject_cast<LocationField*>(m_schema->getField(fieldUid));
        if (!locationField)
        {
            return;
        }

        auto locationMap = newValue.toMap();
        if (locationMap.isEmpty())
        {
            return;
        }

        auto location = Location(locationMap);
        if (location.accuracy() == 0.0)
        {
            qDebug() << "Warning: fieldValueChanged: manual location not added to track";
            return;
        }

        m_trackStreamer->pushUpdate(QUuid::createUuid().toString(QUuid::Id128), &location);
    });

    // Update requireUsername and snap the username for the current sighting.
    connect(App::instance()->settings(), &Settings::usernameChanged, this, [&]()
    {
        if (m_connected)
        {
            update_requireUsername(m_provider->requireUsername());

            if (!m_editing && !m_readonly)
            {
                m_sighting->set_username(App::instance()->settings()->username());
            }
        }
    });

    // Update requireUsername and snap the username for the current sighting.
    connect(App::instance(), &App::aboutToQuit, this, [&]()
    {
        if (m_connected && !m_readonly)
        {
            saveState();
        }
    });

    // Forward global Project events.
    connect(m_projectManager, &ProjectManager::projectSettingChanged, this, [&](const QString& projectUid, const QString& name, const QVariant& value)
    {
        if (!m_connected || m_projectUid != projectUid)
        {
            return;
        }

        update_languageCode(m_project->languageCode());

        static auto trackProperties = QStringList { "locationServiceUrl", "trackFieldUid", "trackFileFormat", "locationPointInterval", "locationTrackInterval" };
        if (trackProperties.contains(name))
        {
            // If the user changes any track properties, disable streaming - it must be turned on manually.
            m_project->set_locationStreamingEnabled(false);

            emit trackSettingsChanged();
        }

        if (name == "locationStreamingEnabled")
        {
            updateLocationStreamers();
        }

        emit projectSettingChanged(name, value);
    });

    connect(m_projectManager, &ProjectManager::projectLoggedInChanged, this, [&](const QString& projectUid, bool loggedIn)
    {
        if (m_connected && m_projectUid == projectUid)
        {
            emit loggedInChanged(loggedIn);
        }
    });

    connect(m_projectManager, &ProjectManager::projectTriggerLogin, this, [&](const QString& projectUid)
    {
        if (m_connected && m_projectUid == projectUid)
        {
            emit triggerLogin();
        }
    });

    // Add a location point to the database.
    connect(m_trackStreamer, &LocationStreamer::locationsAvailable, this, [&](int /*count*/)
    {
        if (m_connected)
        {
            // Populate the track log.
            trackFile()->addLocationBatch([&]()
            {
                m_trackStreamer->enumQueue([&](const QString& locationUid, Location* location)
                {
                    trackFile()->addLocation(locationUid, location);
                });
            });

            saveState();

            // Emit locationSaved events.
            m_trackStreamer->enumQueue([&](const QString& locationUid, Location* location)
            {
                emit App::instance()->locationSaved(m_project->uid(), locationUid, location);
            });

            saveState();
        }
    });

    // Emit a point.
    connect(m_pointStreamer, &LocationStreamer::locationsAvailable, this, [&](int count)
    {
        if (m_connected)
        {
            // Emit the most recent point only.
            auto index = 0;
            m_pointStreamer->enumQueue([&](const QString& locationUid, Location* location)
            {
                index++;
                if (index == count)
                {
                    emit locationPoint(location);
                }
            });
        }
    });

    // Add a point to a line/polygon field.
    connect(m_fieldStreamer, &LocationStreamer::locationsAvailable, this, [&](int /*count*/)
    {
        if (m_connected && !m_fieldStreamerRecordUid.isEmpty() && !m_fieldStreamerFieldUid.isEmpty())
        {
            auto points = getFieldValue(m_fieldStreamerRecordUid, m_fieldStreamerFieldUid).toList();
            m_fieldStreamer->enumQueue([&](const QString& locationUid, Location* location)
            {
                points.append(QVariant(QVariantList { location->x(), location->y(), location->z(), location->accuracy() }));
            });

            setFieldValue(m_fieldStreamerRecordUid, m_fieldStreamerFieldUid, points);
            saveState();
        }
    });

    // Forward global Provider events.
    connect(App::instance(), &App::providerEvent, this, [&](const QString& providerName, const QString& projectUid, const QString& name, const QVariant& value)
    {
        if (m_connected && m_provider->name() == providerName && m_projectUid == projectUid)
        {
            emit providerEvent(name, value);
        }
    });

    // Process alarms.
    connect(App::instance(), &App::alarmFired, this, [&](const QString& alarmId)
    {
        if (m_connected && alarmId == m_submitAlarmId)
        {
            processAutoSubmit();
        }
    });

    // Forward global Sighting events.
    auto matchToThis = [](Form* form, const QString& projectUid) -> bool
    {
        return form->connected() && form->projectUid() == projectUid;
    };

    connect(App::instance(), &App::sightingsChanged, this, [&, this](const QString& projectUid)
    {
        if (matchToThis(this, projectUid))
        {
            emit sightingsChanged();
        }
    });

    connect(App::instance(), &App::sightingSaved, this, [&, this](const QString& projectUid, const QString& sightingUid)
    {
        if (matchToThis(this, projectUid))
        {
            emit sightingSaved(sightingUid);
        }
    });

    connect(App::instance(), &App::sightingDeleted, this, [&, this](const QString& projectUid, const QString& sightingUid)
    {
        if (matchToThis(this, projectUid)) // Removed goes to all, so it is easy to remove from any context.
        {
            emit sightingDeleted(sightingUid);
        }
    });

    connect(App::instance(), &App::sightingModified, this, [&, this](const QString& projectUid, const QString& sightingUid)
    {
        if (matchToThis(this, projectUid))
        {
            emit sightingModified(sightingUid);
        }
    });

    connect(App::instance(), &App::sightingFlagsChanged, this, [&, this](const QString& projectUid, const QString& sightingUid)
    {
        if (matchToThis(this, projectUid))
        {
            emit sightingFlagsChanged(sightingUid);
        }
    });

    connect(App::instance(), &App::locationSaved, this, [&, this](const QString& projectUid, const QString& locationUid, Location* location)
    {
        if (matchToThis(this, projectUid))
        {
            emit locationSaved(location, locationUid);
        }
    });
}

Form::~Form()
{
    disconnectFromProject();
}

void Form::componentComplete()
{
    QQuickItem::componentComplete();

    m_depth = -1;
    for (auto currForm = this; currForm != nullptr; currForm = parentForm(currForm->parent()))
    {
        m_depth++;
    }

    if (!m_projectUid.isEmpty())
    {
        connectToProject(m_projectUid, m_editSightingUid);
    }
}

Form* Form::parentForm(const QObject* obj)
{
    QQuickItem* visualParent = nullptr;

    auto p = (QObject*)obj;
    while (p)
    {
        auto form = qobject_cast<Form*>(p);
        if (form)
        {
            return form;
        }

        visualParent = qobject_cast<QQuickItem*>(p);
        if (visualParent)
        {
            break;
        }

        p = p->parent();
    }

    while (visualParent)
    {
        auto form = qobject_cast<Form*>(visualParent);
        if (form)
        {
            return form;
        }

        visualParent = visualParent->parentItem();
    }

    return nullptr;
}

bool Form::notifyProvider(const QVariantMap& message)
{
    return m_provider->notify(message);
}

QString Form::getPage(const QString& pageName) const
{
    if (pageName.isEmpty())
    {
        return QString();
    }

    return QUrl::fromLocalFile(m_projectManager->getFilePath(m_project->uid(), pageName)).toString();
}

void Form::connectToProject(const QString& projectUid, const QString& editSightingUid)
{
    auto app = App::instance();

    // Create a project.
    m_project = m_projectManager->load(projectUid);
    qFatalIf(!m_project, "Failed to load project: " + projectUid);

    // Initialize the provider.
    m_provider = app->createProviderPtr(m_project->provider(), this);

    // Rebuild the schema if there has been a change to the build or the form.
    // It is useful to detect changes so that developers can work on the underlying form file in the project.
    auto connector = app->createConnectorPtr(m_project->connector());
    auto formLastModified = QFileInfo(connector->getFormFilePath(m_project)).lastModified().toMSecsSinceEpoch();
    auto formFilename = m_provider->getSchemaFilename();
    auto formChanged = m_project->lastBuildString() != app->buildString() ||
                       m_project->formLastModified() != formLastModified ||
                       !QFile::exists(m_projectManager->getFilePath(projectUid, formFilename));
    if (formChanged)
    {
        m_project->set_lastBuildString(app->buildString());
        m_project->set_formLastModified(formLastModified);
        m_project->set_hashCache(QVariantMap());
        connector->reset(m_project);
        connector->buildSchema(m_project, m_projectManager->getFilePath(projectUid));
        connector->migrateProject(m_project);
    }

    // Connect to the provider.
    m_provider->connectToProject();

    // Pick up the language code from the project settings.
    m_languageCode = m_project->languageCode();
    m_sighting->set_languageCode(m_languageCode);

    // Load the schema.
    m_schemaHash = m_project->archiveFile(formFilename);
    m_schema = Schema::acquire(m_schemaHash);
    qFatalIf(!m_schema, "Failed to load schema: " + projectUid);
    m_sighting->set_schema(m_schema);
    m_sighting->set_schemaHash(m_schemaHash);

    // Create the renderer.
    m_renderer = app->createRendererPtr(m_schema->rendererName());

    // Populate the GPS time requirement: disable for simulation and iOS.
    #if defined(Q_OS_IOS)
        m_requireGPSTime = false;
    #else
        m_requireGPSTime = m_provider->requireGPSTime() && !app->settings()->simulateGPS();
    #endif
    m_timeManager->set_correctionEnabled(m_requireGPSTime);

    static bool once = true;
    if (once)
    {
        qDebug() << "requireGPSTime: " << m_requireGPSTime;
        once = false;
    }

    // Update globals.
    m_projectUid = projectUid;
    m_editSightingUid = editSightingUid;
    update_editing(!editSightingUid.isEmpty());

    // Load the state.
    loadState(formChanged);

    // Replay the streamers.
    if (!m_readonly)
    {
        replayLocationStreamers();
    }

    // Update wizard buttons.
    m_wizard->updateButtons();

    // Set the start page.
    update_startPage(m_project->startPage().isEmpty() ? m_provider->getStartPage() : getPage(m_project->startPage()));

    // Auto-initialize the wizard if in immersive mode.
    if (m_project->immersive())
    {
        if (m_wizard->recordUid() != m_sighting->rootRecordUid())
        {
            m_wizard->init(m_sighting->rootRecordUid());
        }

        if (editing())
        {
            m_sighting->set_pageStack(QVariantList());
        }

        update_startPage(editing() ? Wizard::indexPageUrl() : Wizard::pageUrl());
    }

    // Reset the page stack if things have changed.
    if (firstPageUrl() != m_startPage || m_provider->resetPageStackOnConnect())
    {
        setPageStack(QVariantList());
    }

    // Set connected and notify listeners.
    update_connected(true);

    // Set the user name requirement.
    update_requireUsername(m_provider->requireUsername());

    // Add runtime behavior.
    if (!m_readonly && !m_editing)
    {
        updateLocationStreamers();
        processAutoSubmit();
    }
}

void Form::disconnectFromProject()
{
    if (!m_connected)
    {
        return;
    }

    // Save the state.
    saveState();

    // Flush any edit state.
    if (m_editing)
    {
        m_database->deleteFormState(m_projectUid, m_stateSpace + "/edit");
    }

    // Send a beforeDisconnect event so that providers can tear down things which need the connected state to be true.
    emit beforeDisconnect();

    // Set connected and notify listeners: do this now before we tear anything down.
    update_connected(false);

    // Reset object.
    delete m_project;
    m_project = nullptr;

    m_trackFile.reset();

    m_globals.clear();

    m_provider->disconnectFromProject();
    m_provider.reset();
    m_renderer.reset();

    if (m_trackStreamer->running())
    {
        m_trackStreamer->stop();
    }

    if (m_pointStreamer->running())
    {
        m_pointStreamer->stop();
    }

    m_projectUid.clear();
    m_editSightingUid.clear();

    if (!m_schemaHash.isEmpty())
    {
        Schema::release(m_schemaHash);
        m_schemaHash.clear();
    }
    m_schema = nullptr;
}

std::unique_ptr<Sighting> Form::createSightingPtr(const QVariantMap& data) const
{
    auto sighting = new Sighting(data, m_stateSpace, m_schema, m_languageCode);
    sighting->set_schemaHash(m_schemaHash);
    return std::unique_ptr<Sighting>(sighting);
}

std::unique_ptr<Sighting> Form::createSightingPtr(const QString& sightingUid) const
{
    auto dbSighting = Database::DbData();

    if (sightingUid.isEmpty() || sightingUid == rootRecordUid())
    {
        dbSighting.data = m_sighting->save();
    }
    else if (!sightingUid.isEmpty())
    {
        m_database->loadData(m_project->uid(), sightingUid, &dbSighting);
    }

    return createSightingPtr(dbSighting.data);
}

void Form::resetSessionId()
{
    m_sessionId = QUuid::createUuid().toString(QUuid::Id128);
}

void Form::loadState(bool formChanged)
{
    auto formState = QVariantMap();

    // Load the state from the previous edit.
    m_database->loadFormState(m_project->uid(), m_stateSpace + (m_editing ? "/edit" : ""), &formState);
    m_sighting->load(formState.value("currSighting", QVariantMap()).toMap());

    // Pick up the schema, since we may have had an update and the state could be old.
    m_sighting->set_schema(m_schema);
    m_sighting->set_schemaHash(m_schemaHash);

    // If the edit mismatches, then start with the existing form state and load the edit sighting.
    if (m_editing && (!m_sighting->recordManager()->hasRecords() || m_sighting->rootRecordUid() != m_editSightingUid))
    {
        m_database->loadFormState(m_project->uid(), m_stateSpace, &formState);

        auto dbSighting = Database::DbData();
        m_database->loadData(m_project->uid(), m_editSightingUid, &dbSighting);
        m_sighting->load(dbSighting.data);
    }

    // Auto-create a root record as needed.
    if (!m_sighting->recordManager()->hasRecords())
    {
        m_sighting->recordManager()->newRecord("", "");
    }

    // Load the globals.
    m_globals = formState.value("globals", QVariantMap()).toMap();

    // Load the attachment tags.
    m_attachmentTags = formState.value("attachmentTags", QStringList()).toStringList();

    // Load the current session id (pick up the existing one if missing).
    m_sessionId = formState.value("sessionId", m_sessionId).toString();

    // Load the provider state.
    auto providerState = formState.value("provider", QVariantMap()).toMap();
    m_provider->loadState(providerState);

    // Load the field streamer state.
    m_fieldStreamerRecordUid = formState.value("fieldStreamerRecordUid").toString();
    m_fieldStreamerFieldUid = formState.value("fieldStreamerFieldUid").toString();
    m_fieldStreamer->loadState(formState.value("fieldStreamer").toMap());

    if (!m_editing && !m_readonly)
    {
        // Reload the streamers.
        m_trackStreamer->loadState(formState.value("trackStreamer", QVariantMap()).toMap());
        m_pointStreamer->loadState(formState.value("pointStreamer", QVariantMap()).toMap());

        // Configure sighting.
        m_sighting->set_username(App::instance()->settings()->username());
        m_sighting->set_sessionId(m_sessionId);
        m_sighting->set_immersive(m_project->immersive());
        m_sighting->set_globals(m_globals);
    }

    // Recalculate to bring back field flags that may not have been stored.
    m_sighting->recalculate(nullptr, false);

    // Reset the wizard on form change.
    if (formChanged)
    {
        if (!m_sighting->wizardRecordUid().isEmpty())
        {
            m_wizard->init(m_sighting->wizardRecordUid());
        }
    }
}

QStringList Form::trackFieldCandidates() const
{
    auto result = m_schema->buildFieldList("", [&](BaseField* field)
    {
        return field->hidden() && qobject_cast<FileField*>(field);
    });

    if (!result.isEmpty())
    {
        result.insert(0, "");
    }

    return result;
}

TrackFile* Form::trackFile()
{
    if (!m_trackFile)
    {
        m_trackFile = std::unique_ptr<TrackFile>(new TrackFile(App::instance()->tracksPath() + "/" + m_projectUid + ".db"));
    }

    return m_trackFile.get();
}

bool Form::snapTrack(Sighting* sighting)
{
    auto metadata = TrackFileMetadata();
    auto settings = App::instance()->settings();
    metadata.deviceId = settings->deviceId();
    metadata.username = settings->username();
    metadata.interval = m_trackStreamer->lastUpdateInterval() / 1000;
    metadata.distanceFilter = m_trackStreamer->lastDistanceFilter() / 1000;
    metadata.accuracyFilter = settings->locationAccuracyMeters();

    return sighting->snapTrack(trackFile(), metadata, m_project->trackFieldUid(), m_project->trackFileFormat());
}

bool Form::snapTrack()
{
    return snapTrack(m_sighting);
}

bool Form::snapTrackToNewSighting()
{
    if (!m_provider->canSnapTrackToNewSighting()) 
    {
        return false;
    }

    auto sighting = createSightingPtr(QVariantMap());
    sighting->recordManager()->newRecord("", "");
    sighting->set_trackOnly(true);
    sighting->recalculate();

    if (!snapTrack(sighting.get()))
    {
        return false;
    }

    saveSighting(sighting.get());
    setSightingFlag(sighting->rootRecordUid(), Database::DataFlag::DB_COMPLETED_FLAG, true);

    return true;
}

bool Form::canSubmitData() const
{
    return m_provider->canSubmitData();
}

void Form::submitData()
{
    if (!m_connected || m_readonly || m_editing)
    {
        qDebug() << "submitData failed: " << m_connected << m_readonly << m_editing;
        return;
    }

    m_project->set_lastSubmit(m_timeManager->currentDateTimeISO());
    m_provider->submitData();

    App::instance()->uploadQueue()->kick();
}

int Form::getPendingUploadCount() const
{
    return App::instance()->uploadQueue()->itemCount(m_projectUid);
}

void Form::processAutoSubmit()
{
    // Turn off the alarm.
    if (!m_submitAlarmId.isEmpty())
    {
        App::instance()->killAlarm(m_submitAlarmId);
        m_submitAlarmId.clear();
    }

    // Auto send is off.
    auto submitInterval = m_project->submitInterval();
    if (submitInterval == 0)
    {
        return;
    }

    // Initialize lastSubmit timestamp.
    auto now = m_timeManager->currentDateTimeISO();
    auto lastSubmit = m_project->lastSubmit();
    if (lastSubmit.isEmpty())
    {
        m_project->set_lastSubmit(now);
    }

    // Check for nothing to submit.
    if (!canSubmitData())
    {
        return;
    }

    // Check for timeout.
    auto delta = Utils::timestampDeltaMs(lastSubmit, now);
    if (delta >= (m_project->submitInterval() - 1) * 1000)
    {
        submitData();
        return;
    }

    // Timeout not yet reached: start the alarm.
    m_submitAlarmId = "auto-submit-alarm-" + m_projectUid + m_stateSpace;
    App::instance()->setAlarm(m_submitAlarmId, submitInterval);
}

void Form::saveState() const
{
    if (m_readonly)
    {
        return;
    }

    auto attachments = QStringList();
    auto streamers = QStringList();

    auto state = QVariantMap
    {
        { "globals", m_globals },
        { "sessionId", m_sessionId },
        { "attachmentTags", m_attachmentTags },
        { "currSighting", m_sighting->save(&attachments) },
        { "provider", m_provider->saveState(&attachments) },
        { "fieldStreamer", m_fieldStreamer->saveState() },
        { "fieldStreamerRecordUid", m_fieldStreamerRecordUid },
        { "fieldStreamerFieldUid", m_fieldStreamerFieldUid },
    };

    attachments.removeDuplicates();

    // Must be able to edit line/polygon fields.
    m_fieldStreamer->running() ? streamers.append("field") : void();

    if (!m_editing)
    {
        state["trackStreamer"] = m_trackStreamer->saveState();
        state["pointStreamer"] = m_pointStreamer->saveState();

        m_trackStreamer->running() ? streamers.append("track") : void();
        m_pointStreamer->running() ? streamers.append("point") : void();
    }

    m_database->saveFormState(m_project->uid(), m_stateSpace + (m_editing ? "/edit" : ""), state, attachments, streamers);
    m_project->set_hasStreamers(m_database->hasStreamers(m_project->uid()));
}

Element* Form::getElement(const QString& elementUid) const
{
    return m_schema->getElement(elementUid);
}

QString Form::getElementName(const QString& elementUid) const
{
    auto languageCode = m_languageCode != "system" ? m_languageCode : QLocale::system().name();

    return m_schema->getElementName(elementUid, languageCode);
}

QString Form::getElementIcon(const QString& elementUid, bool walkBack) const
{
    return resolveProjectUrl(m_schema->getElementIcon(elementUid, walkBack));
}

QColor Form::getElementColor(const QString& elementUid) const
{
    return getElement(elementUid)->color();
}

QVariant Form::getElementTag(const QString& elementUid, const QString& key) const
{
    return getElement(elementUid)->getTagValue(key);
}

QStringList Form::getElementFieldList(const QString& elementUid) const
{
    auto result = QStringList();

    for (auto leafElement = m_schema->getElement(elementUid);
         leafElement != m_schema->rootElement();
         leafElement = leafElement->parentElement())
    {
        result.append(leafElement->fieldUids());
    }

    return result;
}

QVariantMap Form::getElementTreeInfo(const QString& elementUid) const
{
    auto element = m_schema->getElement(elementUid);

    auto leafElement = element;
    while (leafElement->elementCount() == 1)
    {
        leafElement = leafElement->elementAt(0);
    }

    return QVariantMap
    {
        { "elementUid", elementUid },
        { "leafElementUid", leafElement->uid() },
        { "leafHasChildren", leafElement->hasChildren() },
    };
}

QString Form::rootRecordUid() const
{
    if (!m_connected)
    {
        return QString();
    }

    return m_sighting->rootRecordUid();
}

const QVariantMap* Form::globals() const
{
    return &m_globals;
}

Provider* Form::provider() const
{
    return m_provider.get();
}

Renderer* Form::renderer() const
{
    return m_renderer.get();
}

bool Form::supportLocationTrackField() const
{
    return m_provider->supportLocationTrackField();
}

bool Form::supportLocationTrackService() const
{
    return m_provider->supportLocationTrackService();
}

QVariant Form::providerAsVariant()
{
    return QVariant::fromValue<Provider*>(m_provider.get());
}

Record* Form::getRecord(const QString& recordUid) const
{
    return m_sighting->getRecord(recordUid.isEmpty() ? rootRecordUid() : recordUid);
}

void Form::saveSighting(Sighting* sighting, bool updateTime)
{
    if (m_readonly)
    {
        qDebug() << "saveSighting called on readonly sighting";
        return;
    }

    // Set the schema used.
    sighting->set_schemaHash(m_schemaHash);

    // Update the sessionId.
    if (sighting->sessionId().isEmpty())
    {
        sighting->set_sessionId(m_sessionId);
    }

    // Update the timestamps.
    auto now = m_timeManager->currentDateTimeISO();
    if (sighting->createTime().isEmpty())
    {
        sighting->set_createTime(now);
    }
    if (sighting->updateTime().isEmpty() || updateTime)
    {
        sighting->set_updateTime(now);
    }
    sighting->recalculate();

    // Cleanup any stray records.
    sighting->recordManager()->garbageCollect();

    // Archive any icons needed for rendering.
    auto icons = m_renderer->getSightingIcons(sighting);
    icons.append(m_project->icon());
    icons.removeAll("");
    auto iconHashes = m_project->archiveFiles(icons);
    sighting->set_iconHashes(iconHashes);

    // Commit the sighting.
    auto dbSighting = Database::DbData();
    dbSighting.timestamp = sighting->createTime();
    dbSighting.schemaHash = m_schemaHash;
    dbSighting.stateSpace = m_stateSpace;
    dbSighting.data = sighting->save(&dbSighting.attachments);

    dbSighting.attachments.removeDuplicates();

    auto location = sighting->locationPtr();
    if (location->isValid())
    {
        dbSighting.x = location->x();
        dbSighting.y = location->y();
    }

    // Flags.
    auto sightingUid = sighting->rootRecordUid();
    dbSighting.flags = Database::DataFlag::DB_SIGHTING_FLAG;
    dbSighting.flags |= isSightingTranscribing(sightingUid) ? Database::DataFlag::DB_TRANSCRIBING_FLAG : Database::DataFlag::DB_NO_FLAG;
    dbSighting.flags |= isSightingValid(sightingUid) ? m_database->getDataFlags(m_project->uid(), sightingUid) : Database::DataFlag::DB_NO_FLAG;
    dbSighting.flags |= m_renderer->getSightingCanEdit(sighting, dbSighting.flags) ? Database::DataFlag::DB_NO_FLAG : Database::DataFlag::DB_READONLY_FLAG;

    // Give the provider a chance to make any final changes.
    m_provider->finalizeSightingData(&dbSighting.data);

    // Commit to the db.
    m_database->saveData(m_project->uid(), sighting->rootRecordUid(), dbSighting);
    emit App::instance()->sightingSaved(m_project->uid(), sighting->rootRecordUid());

    // Update the completed flag. If already set, turn it off if the sighting is no longer valid.
    if (dbSighting.flags.testFlag(Database::DataFlag::DB_COMPLETED_FLAG))
    {
        setSightingFlag(sightingUid, Database::DataFlag::DB_COMPLETED_FLAG, sighting->getRecord(sightingUid)->isValid());
    }
}

void Form::saveSighting()
{
    if (m_readonly)
    {
        return;
    }

    if (m_editing)
    {
        /// A sighting was edited and subsequently saved.
        /// @category Actions
        setUsage("editSighting", true);
    }

    saveSighting(m_sighting);

    // Recalculate to ensure anything depending on the time works.
    m_sighting->recalculate();
}

void Form::loadSighting(const QString& sightingUid)
{
    qFatalIf(m_editing, "Load: complete the edit to revert to current");

    auto dbSighting = Database::DbData();
    m_database->loadData(m_project->uid(), sightingUid, &dbSighting);
    m_sighting->load(dbSighting.data);
    m_sighting->recalculate();

    emit App::instance()->sightingModified(m_projectUid, sightingUid);
}

void Form::editSighting(const QString& stateSpace, const QString& sightingUid) const
{
    saveState();
    m_provider->editSighting(stateSpace, sightingUid);
}

void Form::newSighting(bool copyFromCurrent)
{
    qFatalIf(m_editing, "Edit in progress");

    auto pageStack = m_sighting->pageStack();

    if (copyFromCurrent)
    {
        m_sighting->regenerateUids();
    }
    else
    {
        m_sighting->reset();
        m_sighting->recordManager()->newRecord("", "");
    }

    // TODO(justin): what happens if the pages contain references to records?
    m_sighting->set_pageStack(pageStack);

    m_sighting->set_sessionId(m_sessionId);

    m_sighting->recalculate();

    emit App::instance()->sightingModified(m_projectUid, m_sighting->rootRecordUid());
}

void Form::deleteSighting(const QString& sightingUid)
{
    qFatalIf(m_editing, "Edit in progress");
    qFatalIf(!isSightingValid(sightingUid), "deleteData: sightingUid not found");

    /// A sighting was deleted.
    /// @category Actions
    setUsage("deleteSighting", true);
    m_provider->deleteSighting(sightingUid);

    App::instance()->deleteSighting(m_projectUid, sightingUid, true);
}

void Form::prevSighting()
{
    qFatalIf(m_editing, "Edit in progress");

    QStringList sightingUids;
    m_database->getData(m_project->uid(), m_stateSpace, Database::DataFlag::DB_SIGHTING_FLAG, &sightingUids);
    auto currentIndex = sightingUids.indexOf(m_sighting->rootRecordUid());
    if ((currentIndex == 0) || (sightingUids.count() == 0))
    {
        return;
    }

    if (currentIndex == -1)
    {
        currentIndex = sightingUids.length();
    }

    loadSighting(sightingUids[currentIndex - 1]);
}

void Form::nextSighting()
{
    qFatalIf(m_editing, "Edit in progress");

    QStringList sightingUids;
    m_database->getData(m_project->uid(), m_stateSpace, Database::DataFlag::DB_SIGHTING_FLAG, &sightingUids);
    auto currentIndex = sightingUids.indexOf(m_sighting->rootRecordUid());
    if ((sightingUids.count() == 0) || (currentIndex == sightingUids.length() - 1))
    {
        return;
    }

    loadSighting(sightingUids[currentIndex + 1]);
}

void Form::newSessionId()
{
    qFatalIf(m_editing || m_readonly, "Edit in progress");

    resetSessionId();
    m_sighting->set_sessionId(m_sessionId);
    saveState();
}

void Form::snapCreateTime()
{
    m_sighting->set_createTime(m_timeManager->currentDateTimeISO());
}

void Form::snapUpdateTime()
{
    m_sighting->set_updateTime(m_timeManager->currentDateTimeISO());
}

bool Form::hasData() const
{
    return m_database->hasData(m_project->uid(), "*", Database::DataFlag::DB_SIGHTING_FLAG);
}

bool Form::isSightingValid(const QString& sightingUid) const
{
    return m_database->testData(m_project->uid(), sightingUid);
}

bool Form::isSightingTranscribing(const QString& sightingUid) const
{
    return m_database->testTranscribeItem(m_project->uid(), sightingUid);
}

bool Form::isSightingExported(const QString& sightingUid) const
{
    return m_database->getDataFlags(m_project->uid(), sightingUid).testFlag(Database::DataFlag::DB_EXPORTED_FLAG);
}

bool Form::isSightingUploaded(const QString& sightingUid) const
{
    return m_database->getDataFlags(m_project->uid(), sightingUid).testFlag(Database::DataFlag::DB_UPLOADED_FLAG);
}

bool Form::isSightingCompleted(const QString& sightingUid) const
{
    return isSightingValid(sightingUid) &&
           m_database->getDataFlags(m_project->uid(), sightingUid).testFlag(Database::DataFlag::DB_COMPLETED_FLAG);
}

void Form::setSightingFlag(const QString& sightingUid, Database::DataFlags flag, bool on)
{
    if (m_readonly)
    {
        return;
    }

    auto oldValue = m_database->getDataFlags(m_projectUid, sightingUid);
    m_database->setDataFlags(m_projectUid, sightingUid, flag, on);
    auto newValue = m_database->getDataFlags(m_projectUid, sightingUid);

    if (oldValue != newValue)
    {
        emit App::instance()->sightingFlagsChanged(m_projectUid, sightingUid);
    }
}

void Form::markSightingCompleted()
{
    if (m_readonly)
    {
        return;
    }

    auto sightingUid = m_sighting->rootRecordUid();
    auto valid = m_sighting->getRecord(sightingUid)->isValid();

    setSightingFlag(sightingUid, Database::DataFlag::DB_COMPLETED_FLAG, valid);

    // Process auto submit.
    processAutoSubmit();
}

void Form::deleteExportedSightings() const
{
    App::instance()->deleteSightingsByFlag(m_projectUid, m_stateSpace, Database::DataFlag::DB_EXPORTED_FLAG, m_project->disableArchive());
}

void Form::deleteUploadedSightings() const
{
    App::instance()->deleteSightingsByFlag(m_projectUid, m_stateSpace, Database::DataFlag::DB_UPLOADED_FLAG, m_project->disableArchive());
}

QVariant Form::getSetting(const QString& name, const QVariant& defaultValue) const
{
    return m_project->getSetting(name, defaultValue);
}

void Form::setSetting(const QString& name, const QVariant& value)
{
    m_project->setSetting(name, value);
}

void Form::setUsage(const QString& name, const QVariant& value) const
{
    if (!m_connected)
    {
        qDebug() << "Form::setUsage called while not connected; ignoring" << name;
        return;
    }

    m_provider->setUsage(name, value);
}

QVariant Form::getGlobal(const QString& name, const QVariant& defaultValue) const
{
    return m_globals.value(name, defaultValue);
}

void Form::setGlobal(const QString& name, QVariant value)
{
    if (value == m_globals.value(name))
    {
        return;
    }

    if (value.isValid())
    {
        m_globals.insert(name, Utils::variantFromJSValue(value));
    }
    else
    {
        m_globals.remove(name);
    }

    m_sighting->set_globals(m_globals);
    m_sighting->recalculate();
}

QVariant Form::getSightingVariable(const QString& name, const QVariant& defaultValue) const
{
    return m_sighting->getVariable(name, defaultValue);
}

void Form::setSightingVariable(const QString& name, QVariant value)
{
    m_sighting->setVariable(name, value);
    m_sighting->recalculate();
}

QVariant Form::getControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& defaultValue) const
{
    return m_sighting->getControlState(recordUid, fieldUid, name, defaultValue);
}

void Form::setControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& value)
{
    m_sighting->setControlState(recordUid, fieldUid, name, value);
}

void Form::resetControlState(const QString& recordUid, const QString& fieldUid, const QString& name)
{
    m_sighting->setControlState(recordUid, fieldUid, name, QVariant());
}

BaseField* Form::getField(const QString& fieldUid) const
{
    return m_schema->getField(fieldUid);
}

QString Form::getFieldType(const QString& fieldUid) const
{
    return m_schema->getFieldType(fieldUid);
}

QString Form::getFieldName(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->getFieldName(fieldUid);
}

QString Form::getFieldIcon(const QString& fieldUid) const
{
    return resolveProjectUrl(m_schema->getFieldIcon(fieldUid));
}

QString Form::getFieldHint(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->getFieldHint(fieldUid);
}

QString Form::getFieldHintIcon(const QString& fieldUid) const
{
    auto elementUid = m_schema->getField(fieldUid)->hintElementUid();
    if (elementUid.isEmpty())
    {
        return QString();
    }

    return getElementIcon(elementUid);
}

QString Form::getFieldHintLink(const QString& fieldUid) const
{
    auto link = m_schema->getField(fieldUid)->hintLink();
    if (link.isEmpty())
    {
        return QString();
    }

    if (link.startsWith("http://") || link.startsWith("https://"))
    {
        return QString(link);
    }

    return m_projectManager->getFileUrl(m_projectUid, link);
}

QString Form::getFieldConstraintMessage(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->getFieldConstraintMessage(fieldUid);
}

QString Form::getFieldRequiredMessage(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->getFieldRequiredMessage(fieldUid);
}

QString Form::getRecordFieldUid(const QString& recordUid) const
{
    return getRecord(recordUid)->recordFieldUid();
}

QString Form::getParentRecordUid(const QString& recordUid) const
{
    return getRecord(recordUid)->parentRecordUid();
}

QString Form::getRecordName(const QString& recordUid) const
{
    return getRecord(recordUid)->name();
}

QString Form::getRecordIcon(const QString& recordUid) const
{
    return resolveProjectUrl(getRecord(recordUid)->icon());
}

QVariantMap Form::getRecordData(const QString& recordUid) const
{
    return getRecord(recordUid)->save();
}

QString Form::getRecordSummary(const QString& recordUid) const
{
    return getRecord(recordUid)->summary();
}

QVariant Form::getFieldParameter(const QString& recordUid, const QString& fieldUid, const QString& key, const QVariant& defaultValue) const
{
    return m_sighting->getFieldParameter(recordUid, fieldUid, key, defaultValue);
}

QVariant Form::getFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& defaultValue) const
{
    return m_sighting->getFieldValue(recordUid, fieldUid, defaultValue);
}

QVariant Form::getFieldValue(const QString& fieldUid, const QVariant& defaultValue) const
{
    return getFieldValue(m_sighting->rootRecordUid(), fieldUid, defaultValue);
}

void Form::setFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& value)
{
    m_sighting->setFieldValue(recordUid, fieldUid, value);
}

void Form::setFieldValue(const QString& fieldUid, const QVariant& value)
{
    setFieldValue(m_sighting->rootRecordUid(), fieldUid, value);
}

void Form::setFieldValues(const QString& recordUid, const QVariantMap& values)
{
    m_sighting->setFieldValues(recordUid, values);
}

void Form::setFieldValues(const QVariantMap& values)
{
    setFieldValues(m_sighting->rootRecordUid(), values);
}

void Form::resetFieldValue(const QString& recordUid, const QString& fieldUid)
{
    return m_sighting->resetFieldValue(recordUid, fieldUid);
}

void Form::resetFieldValue(const QString& fieldUid)
{
    resetFieldValue(m_sighting->rootRecordUid(), fieldUid);
}

void Form::setFieldState(const QString& recordUid, const QString& fieldUid, int state)
{
    m_sighting->setFieldState(recordUid, fieldUid, (FieldState)state);
}

void Form::setFieldState(const QString& fieldUid, int state)
{
    setFieldState(m_sighting->rootRecordUid(), fieldUid, state);
}

void Form::resetRecord(const QString& recordUid)
{
    m_sighting->resetRecord(recordUid);
}

void Form::startFieldStreamer(const QString& recordUid, const QString& fieldUid, int updateInterval, double distanceFilter)
{
    auto field = getField(fieldUid);
    qFatalIf(!(qobject_cast<LineField*>(field) || qobject_cast<AreaField*>(field)), "Bad field type");

    m_fieldStreamerRecordUid = recordUid;
    m_fieldStreamerFieldUid = fieldUid;
    m_fieldStreamer->start(updateInterval, distanceFilter, 0, 0);

    saveState();
    App::instance()->garbageCollectStreamers();
}

void Form::stopFieldStreamer()
{
    m_fieldStreamerRecordUid.clear();
    m_fieldStreamerFieldUid.clear();

    m_fieldStreamer->stop();

    saveState();
    App::instance()->garbageCollectStreamers();
}

QString Form::getFieldDisplayValue(const QString& recordUid, const QString& fieldUid, const QString& defaultValue) const
{
    return getRecord(recordUid)->getFieldDisplayValue(fieldUid, defaultValue);
}

bool Form::getFieldRequired(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->testFieldFlag(fieldUid, FieldFlag::Required);
}

bool Form::getFieldVisible(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->getFieldVisible(fieldUid);
}

bool Form::getFieldTitleVisible(const QString& fieldUid) const
{
    return getField(fieldUid)->showName();
}

bool Form::getFieldValueValid(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->getFieldValid(fieldUid);
}

bool Form::getFieldValueCalculated(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->getFieldState(fieldUid) == FieldState::Calculated;
}

bool Form::getRecordValid(const QString& recordUid) const
{
    return getRecord(recordUid)->isValid();
}

QString Form::newRecord(const QString& parentRecordUid, const QString& recordFieldUid)
{
    auto recordUid = m_sighting->newRecord(parentRecordUid, recordFieldUid);

    m_sighting->getRecord(parentRecordUid)->setFieldState(recordFieldUid, FieldState::UserSet);

    emit recordCreated(recordUid);

    return recordUid;
}

void Form::deleteRecord(const QString& recordUid)
{
    auto parentRecordUid = getParentRecordUid(recordUid);
    auto recordFieldUid = getRecordFieldUid(recordUid);

    m_sighting->deleteRecord(recordUid);

    m_sighting->getRecord(parentRecordUid)->setFieldState(recordFieldUid, FieldState::UserSet);

    emit recordDeleted(recordUid);
}

bool Form::hasRecord(const QString& recordUid) const
{
    return m_sighting->recordManager()->hasRecord(recordUid);
}

bool Form::hasRecordChanged(const QString& recordUid) const
{
    return m_sighting->recordManager()->hasRecordChanged(recordUid);
}

QVariantList Form::getPageStack() const
{
    if (m_sighting->pageStack().isEmpty())
    {
        return QVariantList { QVariantMap {{ "pageUrl", startPage() }} };
    }
    else
    {
        return m_sighting->pageStack();
    }
}

void Form::setPageStack(const QVariantList& value)
{
    m_sighting->set_pageStack(value);
}

QString Form::firstPageUrl() const
{
    return getPageStack().constFirst().toMap().value("pageUrl").toString();
}

QString Form::lastPageUrl() const
{
    return getPageStack().constLast().toMap().value("pageUrl").toString();
}

QVariantMap Form::getSaveCommands(const QString& recordUid, const QString& fieldUid) const
{
    return QVariantMap
    {
        { "saveTargets", m_sighting->findSaveTargets(recordUid, fieldUid) },
        { "snapLocation", m_sighting->findSnapLocation(recordUid, fieldUid) },
    };
}

bool Form::allowTrackCommands() const
{
    if (!m_connected || m_readonly || m_editing)
    {
        qDebug() << "Error: allowTrackCommands failed: " << m_connected << m_readonly << m_editing;
        return false;
    }

    if (isSightingCompleted(rootRecordUid()))
    {
        qDebug() << "Error: cannot start the track for completed sightings";
        return false;
    }

    if (!supportLocationTrackField() && !supportLocationTrackService())
    {
        emit App::instance()->showError(tr("Missing track field or service"));
        return false;
    }

    return true;
}

bool Form::allowUserTrackChange() const
{
    if (!m_connected)
    {
        return false;
    }

    // If there are track commands, then the track is driven via the form.
    auto trackCommands = m_sighting->getFieldParameter(rootRecordUid(), "", "save.track").toList();
    if (!trackCommands.isEmpty())
    {
        return false;
    }

    // No track commands, so let the user make changes.
    return (!m_project->locationServiceUrl().isEmpty() || !m_project->trackFieldUid().isEmpty()) &&
           (m_project->locationPointInterval() != 0 || m_project->locationTrackInterval() != 0);
}

void Form::applyTrackCommand()
{
    auto command = m_sighting->findTrackCommand();
    if (command.isEmpty())
    {
        return;
    }

    if (!allowTrackCommands())
    {
        qDebug() << "Error: applyTrackCommand not supported";
        return;
    }

    if (command.value("newSessionId").toBool())
    {
        // Do not use newSessionId(), because that will modify the current sighting.
        // This way, the next sighting will pick up a new sessionId.
        resetSessionId();
    }

    // No change specified.
    auto updateIntervalSeconds = command.value("updateIntervalSeconds", -1).toInt();
    auto distanceFilterMeters = command.value("distanceFilterMeters", 0).toInt();

    // Determine changes.
    auto trackChange = (updateIntervalSeconds != -1) && ((m_trackStreamer->updateInterval() != updateIntervalSeconds) || (m_trackStreamer->distanceFilter() != distanceFilterMeters));
    auto trackStart = trackChange && !m_trackStreamer->running() && (updateIntervalSeconds != 0);
    auto trackStop = trackChange && m_trackStreamer->running() && (updateIntervalSeconds == 0);
    auto trackSnap = trackStop || command.value("snapTrack").toBool();

    // Snap.
    if (trackSnap)
    {
        snapTrack(m_sighting);
    }

    // Alter the track state.
    if (trackChange)
    {
        setTrackStreamer(updateIntervalSeconds, distanceFilterMeters);
    }

    // If the track just started, it should contain the location of the first sighting.
    if (trackStart)
    {
        auto sightingLocation = m_sighting->locationPtr();
        if (sightingLocation->isValid())
        {
            m_trackStreamer->pushUpdate(QUuid::createUuid().toString(QUuid::Id128), sightingLocation.get());
        }
    }

    // Cleanup streamers.
    saveState();
    App::instance()->garbageCollectStreamers();
}

void Form::setTrackStreamer(int updateIntervalSeconds, int distanceFilterMeters)
{
    if (!allowTrackCommands())
    {
        qDebug() << "Error: setTrackStreamer not supported";
        return;
    }

    if (updateIntervalSeconds > 0)
    {
        m_trackStreamer->start(updateIntervalSeconds * 1000, distanceFilterMeters, 0, 0);
        return;
    }

    if (updateIntervalSeconds == 0)
    {
        m_trackStreamer->stop();
        return;
    }
}

void Form::startTrackStreamer(int updateIntervalSeconds, int distanceFilterMeters)
{
    // Used by Online titlePage.qml.
    setTrackStreamer(updateIntervalSeconds, distanceFilterMeters);
    saveState();
    App::instance()->garbageCollectStreamers();
}

void Form::stopTrackStreamer()
{
    // Used by Online titlePage.qml.
    setTrackStreamer(0, 0);
    saveState();
    App::instance()->garbageCollectStreamers();
}

void Form::pushTrackLocation(const QVariantMap& locationMap)
{
    auto location = Location(locationMap);
    m_trackStreamer->pushUpdate(QUuid::createUuid().toString(QUuid::Id128), &location);
}

void Form::pushPage(const QString& pageUrl, const QVariantMap& params, int transition)
{
    auto finalPageUrl = pageUrl;
    if (!finalPageUrl.contains('/'))
    {
        finalPageUrl = m_projectManager->getFileUrl(m_projectUid, pageUrl);
    }

    auto paramsJson = QJsonObject::fromVariantMap(params);
    auto paramsSafe = paramsJson.toVariantMap();
    auto known = finalPageUrl.contains("ImagePage.qml");
    if (!known && paramsSafe != params)
    {
        // BUGBUG: Using this to debug a saveState bug.
        qDebug() << "Error: Parameters cannot be serialized:" << params;
        App::instance()->telemetry()->aiSendEvent("crash", QVariantMap {{ "stateSpace", m_stateSpace }, { "pageUrl", pageUrl }, { "keys", params.keys().toList().join(";") }});
    }

    auto pageStack = getPageStack();
    pageStack.append(QVariantMap {{ "pageUrl", finalPageUrl }, { "params", paramsSafe } });
    setPageStack(pageStack);

    if (!m_readonly)
    {
        saveState();
    }

    dumpPages("pushPage");

    emit pagePush(finalPageUrl, params, transition);
}

void Form::pushFormPage(const QVariantMap& params, int transition)
{
    constexpr char FORM_PAGE[] = "qrc:/imports/CyberTracker/FormView.qml";
    pushPage(FORM_PAGE, params, transition);
}

void Form::pushWizardPage(const QString& recordUid, const QVariantMap& rules, int transition)
{
    // Overwrite any existing wizard state if the wizard record is different.
    if (!m_sighting->wizardRecordUid().isEmpty() && m_sighting->wizardRecordUid() != recordUid)
    {
        m_wizard->reset();
    }

    // Set the record and rules.
    m_sighting->set_wizardRules(rules);
    m_sighting->set_wizardRecordUid(recordUid);

    // If the stack is empty, go to the first record.
    if (m_sighting->wizardPageStack().isEmpty())
    {
        m_wizard->first(recordUid, Wizard::TRANSITION_NO_REBUILD);
    }

    // Push the page.
    pushPage(Wizard::pageUrl(), QVariantMap(), transition);
}

void Form::pushWizardIndexPage(const QString& recordUid, const QVariantMap& rules, int transition)
{
    // Overwrite any existing wizard state if the wizard record is different.
    if (!m_sighting->wizardRecordUid().isEmpty() && m_sighting->wizardRecordUid() != recordUid)
    {
        m_wizard->reset();
    }

    // Set the record and rules.
    m_sighting->set_wizardRecordUid(recordUid);
    m_sighting->set_wizardRules(rules);

    // Push the page.
    m_wizard->index(recordUid, m_wizard->lastPageFieldUid(), transition);
}

void Form::popPage(int transition)
{
    auto pageStack = getPageStack();

    if (pageStack.count() == 1 && (m_depth == 0) && m_project->kioskMode() && App::instance()->kioskMode())
    {
        emit triggerKioskPopup();
        return;
    }

    pageStack.removeLast();
    setPageStack(pageStack);

    if (!m_readonly)
    {
        saveState();
    }

    dumpPages("popPage");

    // If the stack is empty, pop to the prior stack.
    if (pageStack.isEmpty())
    {
        // Remove the form loader page from the previous stack.
        if (m_depth > 0)
        {
            auto form = parentForm(this->parent());
            auto pageStack = form->getPageStack();
            pageStack.removeLast();
            form->setPageStack(pageStack);

            if (!form->readonly())
            {
                form->saveState();
            }
        }

        disconnectFromProject();
        emit pagesPopToParent(transition);
    }
    else
    {
        emit pagePop(transition);
    }
}

void Form::popPageBack(int transition)
{
    popPage(transition);

    emit pagePopBack();
}

void Form::popPages(int count)
{
    if (count == -1)
    {
        popPagesToStart();
        return;
    }

    for (auto i = 0; i < count; i++)
    {
        popPage(0);
    }
}

void Form::popPagesToStart()
{
    while (getPageStack().count() > 1)
    {
        popPage(0);
    }
}

void Form::popPagesToParent()
{
    popPagesToStart();
    popPage(0);
}

void Form::replaceLastPage(const QString& pageUrl, const QVariantMap& params, int transition)
{
    auto pageStack = getPageStack();
    pageStack.removeLast();
    pageStack.append(QVariantMap {{ "pageUrl", pageUrl }, { "params", params } });
    setPageStack(pageStack);

    if (!m_readonly)
    {
        saveState();
    }

    dumpPages("replaceLastPage");

    emit pageReplaceLast(pageUrl, params, transition);
}

void Form::loadPages()
{
    dumpPages("loadPages");

    emit pagesLoad(getPageStack());
}

void Form::dumpPages(const QString& description)
{
    if (!m_tracePageChanges)
    {
        return;
    }

    qDebug() << "============================================================================";
    qDebug() << description;
    qDebug() << "----------------------------------------------------------------------------";
    qDebug() << "projectUid: " << m_projectUid << " stateSpace: " << m_stateSpace << " depth: " << m_depth;

    auto pageStack = getPageStack();
    for (auto it = pageStack.constBegin(); it != pageStack.constEnd(); it++)
    {
        auto pageMap = it->toMap();
        qDebug() << "Url: " << pageMap.value("pageUrl") << " Params: " << pageMap.value("params").toMap();
    }

    qDebug() << "----------------------------------------------------------------------------";
}

QVariant Form::evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid, const QVariantMap& variables) const
{
    return m_sighting->evaluate(expression, contextRecordUid, contextFieldUid, &variables);
}

void Form::setImmersive(bool value)
{
    if (m_project->immersive() == value)
    {
        return;
    }

    if (m_readonly || m_editing)
    {
        qDebug() << "Immersive mode cannot be changed while readonly or editing";
        return;
    }

    // Avoid data loss by saving
    if (!m_sighting->wizardRecordUid().isEmpty() && !m_wizard->autoSave())
    {
        saveSighting();
    }

    // Reset so next form load will start in the right mode.
    m_sighting->reset();
    m_sighting->recordManager()->newRecord("", "");
    m_sighting->set_pageStack(QVariantList());
    m_sighting->set_immersive(value);
    m_sighting->recalculate();
    saveState();

    // Set the project value.
    m_project->set_immersive(value);
}

QVariantMap Form::exportToCSV(bool cleanup)
{
    // Create a new trackOnly sighting if the trackFile has data.
    snapTrackToNewSighting();

    // Export data.
    auto success = false;
    auto app = App::instance();
    auto now = m_timeManager->currentDateTime();
    auto exportFileInfo = ExportFileInfo { m_project->uid(), m_project->provider(), m_project->title(), now.toString("yyyy-MM-dd hh:mm:ss") };

    auto updateTimes = [&](const QString& timestamp)
    {
        if (exportFileInfo.startTime.isEmpty() || timestamp < exportFileInfo.startTime)
        {
            exportFileInfo.startTime = timestamp;
        }

        if (exportFileInfo.stopTime.isEmpty() || timestamp > exportFileInfo.stopTime)
        {
            exportFileInfo.stopTime = timestamp;
        }
    };

    auto tempPath = QDir::cleanPath(app->tempPath() + "/snapDataToCSV");
    QDir(tempPath).removeRecursively();
    qFatalIf(!Utils::ensurePath(tempPath), "Failed to create output folder");

    // Export sighting CSV.
    auto sightingHeaderRow = QStringList { "deviceId", "id", "timestamp", "latitude", "longitude", "altitude", "accuracy" };
    QtCSV::StringData sightingRows;

    app->database()->enumData(m_projectUid, m_stateSpace, Database::DataFlag::DB_SIGHTING_FLAG /* ON */, Database::DataFlag::DB_EXPORTED_FLAG /* OFF */,
        [&](const QString& /*projectUid*/, const QString& sightingUid, const Database::DbData& dbSighting)
    {
        auto sighting = createSightingPtr(dbSighting.data);

        auto attachments = sighting->recordManager()->attachments();
        for (auto it = attachments.constBegin(); it != attachments.constEnd(); it++)
        {
            auto filename = *it;
            if (!QFile::copy(app->getMediaFilePath(filename), tempPath + "/" + filename))
            {
                qDebug() << "Failed to copy media file: " << filename;
            }
        }

        if (sighting->trackOnly())
        {
            return;
        }

        sighting->recalculate();

        updateTimes(sighting->createTime());

        auto dataRow = QStringList { sighting->deviceId(), sightingUid, sighting->createTime() };
        auto location = sighting->locationPtr();
        dataRow.append(QString::number(location->y(), 'g', 10));
        dataRow.append(QString::number(location->x(), 'g', 10));
        dataRow.append(QString::number(location->z(), 'g', 10));
        dataRow.append(QString::number(location->accuracy(), 'f', 2));

        auto record = sighting->recordManager()->getRecord(sightingUid);
        record->enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
        {
            if (!fieldValue.isVisible())
            {
                return;
            }

            if (exportFileInfo.sightingCount == 0)
            {
                sightingHeaderRow.append(fieldValue.exportUid());
            }

            auto value = fieldValue.xmlValue();

            if (!value.isEmpty())
            {
                value.replace("\n\r", " ");
                value.replace('\n', ' ');
                value.remove('\r');
            }

            dataRow.append(value.trimmed());
        });

        sightingRows.addRow(dataRow);
        exportFileInfo.sightingCount++;
    });

    if (exportFileInfo.sightingCount > 0)
    {
        success = QtCSV::Writer::write(tempPath + "/sighting.csv", sightingRows, ",", "\"", QtCSV::Writer::WriteMode::REWRITE, sightingHeaderRow);
        if (!success)
        {
            return ApiResult::ErrorMap(tr("CSV sighting export failed"));
        }
    }

    // Export location CSV.
    QtCSV::StringData locationRows;

    // Tracks.
    app->database()->enumData(m_projectUid, m_stateSpace, Database::DataFlag::DB_SIGHTING_FLAG /* ON */, Database::DataFlag::DB_EXPORTED_FLAG /* OFF */,
        [&](const QString& /*projectUid*/, const QString& /*sightingUid*/, const Database::DbData& dbSighting)
    {
        auto sighting = createSightingPtr(dbSighting.data);
        if (sighting->trackFile().isEmpty())
        {
            return;
        }

        auto trackFile = TrackFile(app->getMediaFilePath(sighting->trackFile()));
        auto trackInfo = trackFile.getInfo();
        trackFile.enumLocations([&](const QString& locationUid, Location* location)
            {
                updateTimes(location->timestamp());

                locationRows.addRow(QStringList {
                    trackInfo.metadata.deviceId,
                    locationUid,
                    location->timestamp(),
                    QString::number(location->y(), 'g', 10),
                    QString::number(location->x(), 'g', 10),
                    QString::number(location->z(), 'g', 10),
                    QString::number(location->accuracy(), 'f', 2),
                    QString::number(location->speed()),
                    QString::number(location->counter())
                });

                exportFileInfo.locationCount++;
            });
    });

    if (exportFileInfo.locationCount > 0)
    {
        auto locationHeaderRow = QStringList { "deviceId", "id", "timestamp", "latitude", "longitude", "altitude", "accuracy", "speed", "counter" };
        success = QtCSV::Writer::write(tempPath + "/location.csv", locationRows, ",", "\"", QtCSV::Writer::WriteMode::REWRITE, locationHeaderRow);
        if (!success)
        {
            return ApiResult::ErrorMap(tr("CSV location export failed"));
        }
    }

    // Nothing to export.
    if (exportFileInfo.sightingCount == 0 && exportFileInfo.locationCount == 0)
    {
        QDir(tempPath).removeRecursively();
        return ApiResult::ExpectedMap(tr("No new data"));
    }

    // Build a zip.
    auto exportFilePath = QString("%1/%2-export-%3.zip").arg(app->exportPath(), m_provider->name(), now.toString("yyyyMMdd-hhmmss"));
    success = Utils::zipCompressDir(exportFilePath, tempPath);
    if (!success)
    {
        return ApiResult::ErrorMap(tr("Failed to compress file"));
    }

    // Success.
    app->exportFile()->registerFile(exportFilePath, exportFileInfo.toMap());
    m_database->setDataFlagsAll(m_projectUid, Database::DataFlag::DB_EXPORTED_FLAG);

    auditAdd("Register export CSV");

    // Cleanup if requested.
    if (cleanup)
    {
        app->deleteSightingsByFlag(m_projectUid, m_stateSpace, Database::DataFlag::DB_EXPORTED_FLAG, m_project->disableArchive());
    }

    return ApiResult::SuccessMap();
}

QString Form::resolveProjectUrl(const QString& projectFilename) const
{
    if (projectFilename.isEmpty())
    {
        return QString();
    }

    auto filePath = m_projectManager->getFilePath(m_project->uid(), projectFilename);
    if (QFile::exists(filePath))
    {
        return QUrl::fromLocalFile(filePath).toString();
    }
    else
    {
        return projectFilename;
    }
}

void Form::auditAdd(const QString& description) const
{
    if (m_project)
    {
        Audit::add(m_project->title(), description);
    }
}

void Form::updateLocationStreamers()
{
    if (!m_connected || m_editing || m_readonly)
    {
        return;
    }

    // Disable the track if it is not properly set up.
    // This handles the case where it was active, but the user did something like changed the service or track field while it was running.
    auto supportTrackService = m_provider->supportLocationTrackService();
    auto supportTrackField = m_provider->supportLocationTrackField();
    if (!supportTrackService && !supportTrackField)
    {
        return;
    }

    // If there are track commands, then ignore Project settings.
    auto hasTrackCommands = !m_sighting->getFieldParameter(rootRecordUid(), "", "save.track").toList().isEmpty();
    if (hasTrackCommands)
    {
        return;
    }

    // Ensure that the service has a URL OR the field is specified.
    auto disable = !m_project->locationStreamingEnabled();
    disable = disable || (supportTrackService && m_project->locationServiceUrl().isEmpty()); // Service.
    disable = disable || (supportTrackField && m_project->trackFieldUid().isEmpty());        // Field.

    if (disable)
    {
        m_pointStreamer->stop();
        m_trackStreamer->stop();
        snapTrackToNewSighting();
        saveState();
        App::instance()->garbageCollectStreamers();
        return;
    }

    // If the user activated streaming manually, then the project might not contain the right permissions.
    // This will restart the project and thereby ask for the permissions as needed.
    if (m_project->locationStreamingEnabled() && !m_project->requireBackgroundLocation())
    {
        m_projectManager->modify(m_projectUid, [&](Project* project)
        {
            project->set_requireLocation(true);
            project->set_requireBackgroundLocation(true);
        });

        emit App::instance()->changeToProject(m_projectUid);
        return;
    }

    // Activate or deactivate Point streamer.
    if (supportLocationTrackService() && !m_project->locationServiceUrl().isEmpty() && m_project->locationPointInterval() > 0)
    {
        /// Location updates are sent at interval (in seconds).
        /// @category Connect
        setUsage("locationPointInterval", m_project->locationPointInterval());
        m_pointStreamer->start(m_project->locationPointInterval() * 1000);
    }
    else
    {
        m_pointStreamer->stop();
    }

    // Activate or deactivate Track streamer.
    if (m_project->locationTrackInterval() > 0)
    {
        /// Location track points are recorded at interval (in seconds).
        /// @category Connect
        setUsage("locationTrackInterval", m_project->locationTrackInterval());
        /// Location track points are recorded at distance threshold (in metres).
        /// @category Connect
        setUsage("locationTrackDistance", m_project->locationTrackDistance());

        // This does nothing if the streamer has not changed, i.e. will not create a new track.
        m_trackStreamer->start(m_project->locationTrackInterval() * 1000, m_project->locationTrackDistance());
    }
    else
    {
        m_trackStreamer->stop();
        snapTrackToNewSighting();
    }

    // Save the form state and update the streamers.
    saveState();
    App::instance()->garbageCollectStreamers();
}

void Form::replayLocationStreamers()
{
    qFatalIf(m_readonly, "Not while readonly");

    m_database->transaction([&]() -> bool
    {
        auto saveChanges = false;

        // Replay the track: note that editing does not make sense.
        if (!m_editing && m_trackStreamer->running())
        {
            m_trackStreamer->processLog();
            if (!m_trackStreamer->isQueueEmpty())
            {
                saveChanges = true;
                auto trackFile = TrackFile(App::instance()->tracksPath() + "/" + m_projectUid + ".db", this);
                trackFile.addLocationBatch([&]()
                {
                    while (!m_trackStreamer->isQueueEmpty())
                    {
                        m_trackStreamer->enumQueue([&](const QString& locationUid, Location* location)
                        {
                            trackFile.addLocation(locationUid, location);
                        });

                        m_trackStreamer->clearQueue();
                        m_trackStreamer->processLog();
                    }
                });
            }
        }

        // Replay the field streamer: note that this can be done from the edit state.
        if (!m_fieldStreamerRecordUid.isEmpty() && !m_fieldStreamerFieldUid.isEmpty() && m_fieldStreamer->running())
        {
            auto points = getFieldValue(m_fieldStreamerRecordUid, m_fieldStreamerFieldUid).toList();

            m_fieldStreamer->processLog();
            while (!m_fieldStreamer->isQueueEmpty())
            {
                saveChanges = true;
                m_fieldStreamer->enumQueue([&](const QString& /*locationUid*/, Location* location)
                {
                    points.append(QVariant(QVariantList { location->x(), location->y(), location->z(), location->accuracy() }));
                });

                m_fieldStreamer->clearQueue();
                m_fieldStreamer->processLog();
            }

            setFieldValue(m_fieldStreamerRecordUid, m_fieldStreamerFieldUid, points);
        }

        // Commit changes as needed.
        if (saveChanges)
        {
            saveState();
        }

        return true;
    });
}

QString Form::buildSpeechPrompt(const QString& recordUid, const QStringList& fieldUids) const
{
    auto result = QStringList();
    auto record = m_sighting->getRecord(recordUid);
    if (record == nullptr)
    {
        qDebug() << "Bad recordUid: " << recordUid;
        return QString();
    }

    for (auto fieldIt = fieldUids.constBegin(); fieldIt != fieldUids.constEnd(); fieldIt++)
    {
        auto fieldUid = record->findFieldByTail(*fieldIt);
        if (fieldUid.isEmpty())
        {
            qDebug() << "Bad fieldUid: " << *fieldIt;
            continue;
        }

        auto field = getField(fieldUid);

        // Simple fields only.
        if (!qobject_cast<StringField*>(field) && !qobject_cast<CheckField*>(field) && !qobject_cast<NumberField*>(field))
        {
            continue;
        }

        if (!field->listElementUid().isEmpty()) // List items.
        {
            m_schema->enumElementChildren(field->listElementUid(), true, [&](Element* element)
            {
                result.append(getElementName(element->uid()).toLower());

                if (!element->aliases().isEmpty())
                {
                    result.append(element->aliases());
                }
            });
        }
        else // Simple fields.
        {
            result.append(getFieldName(recordUid, fieldUid).toLower());

            if (!field->nameElementUid().isEmpty())
            {
                auto aliases = getElement(field->nameElementUid())->aliases();
                if (!aliases.isEmpty())
                {
                    result.append(aliases);
                }
            }
        }
    }

    return result.join(" ");
}

QString Form::buildSpeechSchema(const QString& recordUid, const QStringList& fieldUids) const
{
    auto record = m_sighting->getRecord(recordUid);
    if (record == nullptr)
    {
        qDebug() << "Bad recordUid: " << recordUid;
        return QString();
    }

    auto propertiesMap = QVariantMap{{ "_RAW_", QVariantMap {{ "type", "string" }}}};
    auto requiredList = QStringList();

    for (auto i = 0; i < fieldUids.count(); i++)
    {
        auto fieldUid = record->findFieldByTail(fieldUids[i]);
        if (fieldUid.isEmpty())
        {
            qDebug() << "Bad fieldUid: " << fieldUids[i];
            continue;
        }

        auto fieldName = getFieldName(recordUid, fieldUid);

        auto field = getField(fieldUid);

        // Simple fields only.
        if (!qobject_cast<StringField*>(field) && !qobject_cast<CheckField*>(field) && !qobject_cast<NumberField*>(field))
        {
            continue;
        }

        if (record->getFieldFlags(fieldUid).testFlag(FieldFlag::Required))
        {
            // TODO(justin): Allow forms to be incrementally filled in.
            //requiredList.append(fieldName);
        }

         // List items.
        if (!field->listElementUid().isEmpty())
        {
            auto items = QStringList();
            m_schema->enumElementChildren(field->listElementUid(), true, [&](Element* element)
            {
                if (!field->listOther() && element->other())
                {
                    return;
                }
                items.append(getElementName(element->uid()));
            });

            if (qobject_cast<CheckField*>(field))
            {
                auto itemsDef = QVariantMap {{ "type", "string" }, { "enum", items }};
                propertiesMap.insert(fieldName, QVariantMap {{ "type", "array" }, { "items", itemsDef }});
            }
            else
            {
                propertiesMap.insert(fieldName, QVariantMap {{ "type", "string" }, { "enum", items }});
            }
            continue;
        }

        // Number fields.
        auto numberField = qobject_cast<NumberField*>(field);
        if (numberField)
        {
            if (numberField->decimals() <= 0)
            {
                propertiesMap.insert(fieldName, QVariantMap {{ "type", "integer" }});
            }
            else
            {
                propertiesMap.insert(fieldName, QVariantMap {{ "type", "number" }});
            }

            continue;
        }

        // String fields.
        auto stringField = qobject_cast<StringField*>(field);
        if (stringField)
        {
            propertiesMap.insert(fieldName, QVariantMap {{ "type", "string" }});
            continue;
        }

        // Check fields.
        auto checkField = qobject_cast<CheckField*>(field);
        if (checkField)
        {
            propertiesMap.insert(fieldName, QVariantMap {{ "type", "string" }, { "enum", QStringList { fieldName } }});
            continue;
        }
    }

    auto result = QVariantMap
    {
        { "type", "object" },
        { "properties", propertiesMap },
        { "required", requiredList },
        { "additionalProperties", false }
    };

    return Utils::variantMapToString(result);
}

void Form::requestTranscribe(const QString& filename, const QString& prompt, const QString& schema, const QString& fillRecordUid, const QStringList& fillFieldUids) const
{
    auto item = Database::DbTranscribeItem();
    item.projectUid = m_project->uid();
    item.stateSpace = m_stateSpace;
    item.dataUid = m_sighting->rootRecordUid();
    item.filename = filename;
    item.prompt = prompt;
    item.schema = schema;
    item.fillRecordUid = fillRecordUid;
    item.fillFieldUids = fillFieldUids;

    m_transcribeQueue->request(item);
}

void Form::processTranscribeResult(const Database::DbTranscribeItem& transcribeItem)
{
    if (!m_connected || m_readonly)
    {
        return;
    }

    if (transcribeItem.stateSpace != m_stateSpace || transcribeItem.dataUid != m_sighting->rootRecordUid())
    {
        return;
    }

    // Fill.
    if (!transcribeItem.fillRecordUid.isEmpty())
    {
        auto recordUid = transcribeItem.fillRecordUid;
        auto record = m_sighting->getRecord(recordUid);
        if (record) // It may have been deleted.
        {
            record->recordField()->enumFields([&](BaseField* field)
            {
                auto fieldUid = field->uid();
                auto fieldName = getFieldName(recordUid, fieldUid);
                auto fieldValue = transcribeItem.result.value(fieldName);
                if (fieldValue.isValid())
                {
                    if (field->listElementUid().isEmpty())
                    {
                        if (qobject_cast<CheckField*>(field))
                        {
                            if (fieldName == fieldValue)
                            {
                                setFieldValue(recordUid, fieldUid, true);
                            }
                        }
                        else
                        {
                            setFieldValue(recordUid, fieldUid, fieldValue);
                        }
                    }
                    else
                    {
                        if (qobject_cast<CheckField*>(field))
                        {
                            auto fieldValueList = fieldValue.toList();
                            auto checkListValue = getFieldValue(recordUid, fieldUid).toMap();
                            m_schema->enumElementChildren(field->listElementUid(), true, [&](Element* element)
                            {
                                auto elementName = getElementName(element->uid());
                                if (fieldValueList.contains(elementName))
                                {
                                    checkListValue[element->uid()] = true;
                                }
                            });

                            setFieldValue(recordUid, fieldUid, checkListValue);
                        }
                        else
                        {
                            m_schema->enumElementChildren(field->listElementUid(), true, [&](Element* element)
                            {
                                auto elementName = getElementName(element->uid());
                                if (fieldValue == elementName)
                                {
                                    setFieldValue(recordUid, fieldUid, element->uid());
                                }
                            });
                        }
                    }
                }
            });
        }
    }

    // Audio field.
    auto audioRecordUid = QString();
    auto audioFieldUid = QString();
    if (m_sighting->findAudioField(transcribeItem.filename, &audioRecordUid, &audioFieldUid))
    {
        auto audioField = qobject_cast<AudioField*>(getField(audioFieldUid));

        // Put the result into the audio field.
        auto audioValue = getFieldValue(audioRecordUid, audioFieldUid).toMap();
        audioValue.insert(transcribeItem.result);
        setFieldValue(audioRecordUid, audioFieldUid, audioValue);

        // Transcribe to field.
        auto transcribeTo = audioField->transcribeTo();
        if (!transcribeTo.isEmpty())
        {
            auto transcribeToFieldUid = m_sighting->getRecord(audioRecordUid)->findFieldByTail(transcribeTo);
            if (!transcribeToFieldUid.isEmpty())
            {
                setFieldValue(audioRecordUid, transcribeToFieldUid, transcribeItem.result.value("_RAW_").toString());
            }
            else
            {
                qDebug() << "transcribeTo field not found: " << transcribeToFieldUid;
            }
        }

        // Reset after fill.
        if (audioField->resetAfterFill())
        {
            resetFieldValue(audioRecordUid, audioFieldUid);
        }
    }

    // Save state.
    saveState();

    // Consume.
    m_transcribeQueue->consume(transcribeItem.filename);
}

} // namespace CyberTracker
