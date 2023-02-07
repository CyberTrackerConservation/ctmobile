#include "pch.h"
#include "Form.h"
#include "App.h"
#include "TrackFile.h"
#include "qtcsv/stringdata.h"
#include "qtcsv/csvwriter.h"
#include <jlcompress.h>

Form::Form(QObject* /*parent*/)
{
    m_connected = false;
    m_editing = false;
    m_stateSpace = "";
    m_readonly = false;
    m_languageCode = QLocale::system().name();
    m_depth = -1;
    m_tracePageChanges = false;
    m_requireUsername = false;

    m_database = App::instance()->database();
    m_timeManager = App::instance()->timeManager();
    m_projectManager = App::instance()->projectManager();
    m_elementManager = new ElementManager(this);
    m_fieldManager = new FieldManager(this);
    m_trackStreamer = new LocationStreamer(this);
    m_trackStreamer->set_rateLabel(tr("Track"));
    m_pointStreamer = new LocationStreamer(this);
    m_pointStreamer->set_rateLabel(tr("Location"));
    m_sighting = new Sighting(QVariantMap {}, this);
    m_wizard = new Wizard(this);

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

    // Update requireUsername.
    connect(App::instance()->settings(), &Settings::usernameChanged, this, [&]()
    {
        if (m_connected)
        {
            update_requireUsername(m_provider->requireUsername());
        }
    });

    // Forward global Project events.
    connect(m_projectManager, &ProjectManager::projectSettingChanged, this, [&](const QString& projectUid, const QString& name, const QVariant& value)
    {
        if (m_connected && m_projectUid == projectUid)
        {
            update_languageCode(m_project->languageCode());

            emit projectSettingChanged(name, value);
        }
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
    connect(m_trackStreamer, &LocationStreamer::locationUpdated, this, [&](Location* update)
    {
        if (m_connected)
        {
            auto locationUid = QUuid::createUuid().toString(QUuid::Id128);

            m_database->saveSighting(m_project->uid(), m_stateSpace, locationUid, Sighting::DB_LOCATION_FLAG, update->toMap(), QStringList());

            emit locationTrack(update, locationUid);
        }
    });

    // Emit a point.
    connect(m_pointStreamer, &LocationStreamer::locationUpdated, this, [&](Location* update)
    {
        if (m_connected)
        {
            emit locationPoint(update);
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
    auto matchToThis = [](Form* form, const QString& projectUid, const QString& stateSpace) -> bool
    {
        return form->connected() && form->projectUid() == projectUid && form->stateSpace() == stateSpace;
    };

    connect(App::instance(), &App::sightingsChanged, this, [&, this](const QString& projectUid, const QString& stateSpace)
    {
        if (matchToThis(this, projectUid, stateSpace))
        {
            emit sightingsChanged();
        }
    });

    connect(App::instance(), &App::sightingSaved, this, [&, this](const QString& projectUid, const QString& sightingUid, const QString& stateSpace)
    {
        if (matchToThis(this, projectUid, stateSpace))
        {
            emit sightingSaved(sightingUid);
        }
    });

    connect(App::instance(), &App::sightingRemoved, this, [&, this](const QString& projectUid, const QString& sightingUid, const QString& stateSpace)
    {
        if (matchToThis(this, projectUid, stateSpace))
        {
            emit sightingRemoved(sightingUid);
        }
    });

    connect(App::instance(), &App::sightingModified, this, [&, this](const QString& projectUid, const QString& sightingUid, const QString& stateSpace)
    {
        if (matchToThis(this, projectUid, stateSpace))
        {
            emit sightingModified(sightingUid);
        }
    });

    connect(App::instance(), &App::sightingFlagsChanged, this, [&, this](const QString& projectUid, const QString& sightingUid, const QString& stateSpace)
    {
        if (matchToThis(this, projectUid, stateSpace))
        {
            emit sightingFlagsChanged(sightingUid);
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

QUrl Form::getPage(const QString& pageName) const
{
    if (pageName.isEmpty())
    {
        return QUrl();
    }

    return QUrl::fromLocalFile(m_projectManager->getFilePath(m_project->uid(), pageName));
}

void Form::connectToProject(const QString& projectUid, const QString& editSightingUid)
{
    auto app = App::instance();

    // Create a project.
    m_project = m_projectManager->load(projectUid);
    qFatalIf(!m_project, "Failed to load project: " + projectUid);

    // Initialize the provider.
    m_provider = app->createProviderPtr(m_project->provider(), this);

    // Connect to the provider and tell it if this is a new build.
    auto formChanged = false;
    m_provider->connectToProject(m_project->lastBuildString() != app->buildString(), &formChanged);
    m_project->set_lastBuildString(app->buildString());

    // Pick up the language code from the project settings.
    m_languageCode = m_project->languageCode();

    // Load the Elements.
    m_provider->getElements(m_elementManager);

    // Load the Fields.
    m_provider->getFields(m_fieldManager);

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
    if (firstPageUrl() != m_startPage.toString())
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
        app->taskManager()->resumePausedTasks(m_project->uid());
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

    // Set connected and notify listeners: do this now before we tear anything down.
    update_connected(false);

    // Reset object.
    delete m_project;
    m_project = nullptr;

    m_globals.clear();

    m_provider->disconnectFromProject();
    m_provider.reset();

    if (!m_readonly && !m_editing)
    {
        if (m_trackStreamer->running())
        {
            m_trackStreamer->stop();
        }

        if (m_pointStreamer->running())
        {
            m_pointStreamer->stop();
        }
    }

    m_projectUid.clear();
    m_editSightingUid.clear();
}

std::unique_ptr<Sighting> Form::createSightingPtr(const QVariantMap& data) const
{
    return std::unique_ptr<Sighting>(new Sighting(data, (QObject*)this));
}

std::unique_ptr<Sighting> Form::createSightingPtr(const QString& sightingUid) const
{
    auto data = QVariantMap();

    if (sightingUid.isEmpty() || sightingUid == rootRecordUid())
    {
        data = m_sighting->save();
    }
    else if (!sightingUid.isEmpty())
    {
        m_database->loadSighting(m_project->uid(), sightingUid, &data);
    }

    return createSightingPtr(data);
}

void Form::loadState(bool formChanged)
{
    auto formState = QVariantMap();

    // Load the state from the previous edit.
    m_database->loadFormState(m_project->uid(), m_stateSpace + (m_editing ? "/edit" : ""), &formState);
    m_sighting->load(formState.value("currSighting", QVariantMap()).toMap());

    // If the edit mismatches, then start with the existing form state and load the edit sighting.
    if (m_editing && (!m_sighting->recordManager()->hasRecords() || m_sighting->rootRecordUid() != m_editSightingUid))
    {
        m_database->loadFormState(m_project->uid(), m_stateSpace, &formState);

        auto data = QVariantMap();
        m_database->loadSighting(m_project->uid(), m_editSightingUid, &data);
        m_sighting->load(data);
    }

    // Auto-create a root record as needed.
    if (!m_sighting->recordManager()->hasRecords())
    {
        m_sighting->recordManager()->newRecord("", "");
    }

    // Load the globals.
    m_globals = formState.value("globals", QVariantMap()).toMap();

    // Load the provider state.
    auto providerState = formState.value("provider", QVariantMap()).toMap();
    m_provider->load(providerState);

    // Restart the tracks as needed.
    if (!m_editing && !m_readonly)
    {
        m_trackStreamer->loadState(formState.value("trackStreamer", QVariantMap()).toMap());
        m_pointStreamer->loadState(formState.value("pointStreamer", QVariantMap()).toMap());

        // Pick up project location autosend settings.
        if (m_project->sendLocationInterval() != 0)
        {
            m_pointStreamer->start(m_project->sendLocationInterval() * 1000);
        }
    }

    // Recalculate to bring back field flags that may not have been stored.
    m_sighting->recalculate();

    // Reset the wizard on form change.
    if (formChanged)
    {
        if (!m_sighting->wizardRecordUid().isEmpty())
        {
            m_wizard->init(m_sighting->wizardRecordUid());
        }
    }
}

bool Form::snapTrack()
{
    if (!m_connected || m_readonly || m_editing)
    {
        qDebug() << "snapTrack failed: " << m_connected << m_readonly << m_editing;
        return false;
    }

    // Build a track file of all unsnapped locations.
    auto trackFilename = "track.db";
    auto trackFilePath = App::instance()->tempPath() + "/" + trackFilename;
    QFile::remove(trackFilePath);

    auto trackFile = std::unique_ptr<TrackFile>(new TrackFile(trackFilePath));

    trackFile->batchAddInit(App::instance()->settings()->deviceId(), App::instance()->settings()->username());
    m_database->enumSightings(m_projectUid, m_stateSpace, Sighting::DB_LOCATION_FLAG /* ON */, Sighting::DB_SNAPPED_FLAG /* OFF */,
        [&](const QString& uid, uint /*flags*/, const QVariantMap& data, const QStringList& /*attachments*/)
        {
            trackFile->batchAddLocation(uid, data);
        });

    auto counter = trackFile->batchAddDone();
    trackFile.reset();

    // No points found.
    if (counter == 0)
    {
        qDebug() << "snapTrack: no points";
        QFile::remove(trackFilePath);
        return true;
    }

    // trackFileField was specified, so export to the requested format and fill it in.
    auto trackFileFormat = QString();
    auto trackFileFieldUid = m_sighting->getTrackFileFieldUid(&trackFileFormat);
    if (!trackFileFieldUid.isEmpty())
    {
        TrackFile trackFile(trackFilePath);
        auto exportedFilePath = trackFile.exportFile(trackFileFormat, true);
        if (!exportedFilePath.isEmpty())
        {
            auto mediaFilename = App::instance()->moveToMedia(QUrl::fromLocalFile(exportedFilePath).toString());
            m_sighting->setRootFieldValue(trackFileFieldUid, mediaFilename);
        }
        else
        {
            qDebug() << "snapTrack: failed to snap track to format: " << trackFileFormat;
        }
    }

    // Move the file to the media path and add it to the sighting.
    auto mediaFilename = App::instance()->moveToMedia(QUrl::fromLocalFile(trackFilePath).toString());
    m_sighting->set_trackFile(mediaFilename);

    // Save the sighting state.
    saveState();

    // Mark the locations as snapped.
    m_database->setSightingFlags(m_projectUid, m_stateSpace, Sighting::DB_LOCATION_FLAG /* ON */, Sighting::DB_COMPLETED_FLAG /* OFF */, Sighting::DB_SNAPPED_FLAG);

    // Success.
    return true;
}

bool Form::canSubmitData() const
{
    auto uids = QStringList();
    m_database->getSightings(m_projectUid, m_stateSpace, Sighting::DB_SIGHTING_FLAG | Sighting::DB_COMPLETED_FLAG /* ON */, Sighting::DB_SUBMITTED_FLAG /* OFF */, &uids);

    return !uids.isEmpty() && m_provider->canSubmitData();
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
}

int Form::getPendingUploadCount() const
{
    auto uids = QStringList();
    m_database->getSightings(m_projectUid, m_stateSpace, Sighting::DB_SUBMITTED_FLAG /* ON */, Sighting::DB_UPLOADED_FLAG /* OFF */, &uids);

    return uids.length();
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

void Form::saveState()
{
    if (m_readonly)
    {
        return;
    }

    auto state = QVariantMap();
    state["globals"] = m_globals;
    auto attachments = QStringList();
    state["currSighting"] = m_sighting->save(&attachments);

    auto providerMap = QVariantMap();
    m_provider->save(providerMap);
    state["provider"] = providerMap;

    if (!m_editing)
    {
        state["trackStreamer"] = m_trackStreamer->saveState();
        state["pointStreamer"] = m_pointStreamer->saveState();
    }

    m_database->saveFormState(m_project->uid(), m_stateSpace + (m_editing ? "/edit" : ""), state, attachments);
}

Element* Form::getElement(const QString& elementUid) const
{
    qFatalIf(!m_connected, "Form not connected when retrieving Element");

    auto element = m_elementManager->getElement(elementUid);
    qFatalIf(element == nullptr, "Element not found: " + elementUid);

    return element;
}

QString Form::getElementName(const QString& elementUid) const
{
    auto languageCode = m_languageCode != "system" ? m_languageCode : QLocale::system().name();

    return m_elementManager->getElementName(elementUid, languageCode);
}

QUrl Form::getElementIcon(const QString& elementUid, bool walkBack) const
{
    return m_elementManager->getElementIcon(elementUid, walkBack);
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

    for (auto leafElement = m_elementManager->getElement(elementUid);
         leafElement != m_elementManager->rootElement();
         leafElement = leafElement->parentElement())
    {
        result.append(leafElement->fieldUids());
    }

    return result;
}

QVariantList Form::buildSightingView(const QString& sightingUid) const
{
    return m_provider->buildSightingView(createSightingPtr(sightingUid).get());
}

QVariantMap Form::buildSightingMapLayer(const QString& layerUid, const std::function<QVariantMap(Sighting* sighting)>& getSymbol) const
{
    auto spatialReference = QVariantMap();
    spatialReference["wkid"] = 4326;

    auto pointList = QVariantList();
    auto point = QVariantMap();
    point["spatialReference"] = spatialReference;

    auto sightingUids = QStringList();
    m_database->getSightings(m_project->uid(), m_stateSpace, Sighting::DB_SIGHTING_FLAG, &sightingUids);
    pointList.reserve(sightingUids.size());

    auto extentRect = QRectF();

    for (auto it = sightingUids.constBegin(); it != sightingUids.constEnd(); it++)
    {
        auto sighting = createSightingPtr(*it);
        auto location = sighting->locationPtr();

        if (location->isValid())
        {
            auto x = location->x();
            auto y = location->y();
            extentRect = extentRect.united(Utils::pointToRect(x, y));
            point["x"] = x;
            point["y"] = y;
            point["createTime"] = sighting->createTime();
            point["stateSpace"] = m_stateSpace;
            point["sightingUid"] = sighting->rootRecordUid();
            point["symbol"] = getSymbol(sighting.get());
            pointList.append(QVariant::fromValue(point));
        }
    }

    return QVariantMap
    {
        { "id", layerUid },
        { "type", "form" },
        { "name", layerUid },
        { "geometryType", "Points" },
        { "geometry", pointList },
        { "extent", QVariantMap
            {
                { "spatialReference", QVariantMap {{ "wkid", 4326 }} },
                { "xmin", extentRect.left() },
                { "ymin", extentRect.top() },
                { "xmax", extentRect.right() },
                { "ymax", extentRect.bottom() },
            }
        },
    };
}

QVariantMap Form::buildTrackMapLayer(const QString& layerUid, const QVariantMap& symbol) const
{
    auto extentRect = QRectF();
    auto point = QVariantList();
    point.reserve(2);   // X and Y.
    point.append(QVariant());
    point.append(QVariant());

    auto partList = QVariantList();
    auto part = QVariantList();
    auto lastCounter = std::numeric_limits<int>::max();
    auto hasData = false;

    m_database->enumSightings(m_projectUid, m_stateSpace, Sighting::DB_LOCATION_FLAG, [&](const QString& /*uid*/, int /*flags*/, const QVariantMap& data, const QStringList& /*attachments*/)
    {
        hasData = true;
        auto counter = data["c"].toInt();

        if ((counter < lastCounter) && (part.count() > 0))
        {
            if (part.count() > 1)
            {
                partList.append(QVariant::fromValue(part));
            }
            part.clear();
        }

        lastCounter = counter;

        auto x = data.value("x").toDouble();
        auto y = data.value("y").toDouble();
        extentRect = extentRect.united(Utils::pointToRect(x, y));
        point[0] = x;
        point[1] = y;
        part.append(QVariant::fromValue(point));
    });

    if (part.count() > 1)
    {
        partList.append(QVariant::fromValue(part));
    }

    return QVariantMap
    {
        { "id", layerUid },
        { "type", "form" },
        { "name", layerUid },
        { "symbol", symbol },
        { "geometryType", "Polyline" },
        { "geometry", QVariantMap
            {
                { "spatialReference", QVariantMap {{ "wkid", 4326 }} },
                { "paths", partList }
            } 
        },
        { "extent", QVariantMap
            {
                { "spatialReference", QVariantMap {{ "wkid", 4326 }} },
                { "xmin", extentRect.left() },
                { "ymin", extentRect.top() },
                { "xmax", extentRect.right() },
                { "ymax", extentRect.bottom() },
            }
        },
        { "lastPoint", hasData ? QVariant::fromValue(point) : QVariant() },
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

bool Form::supportLocationPoint() const
{
    return m_provider->supportLocationPoint();
}

bool Form::supportLocationTrack() const
{
    return m_provider->supportLocationTrack();
}

bool Form::supportSightingEdit() const
{
    return m_provider->supportSightingEdit();
}

bool Form::supportSightingDelete() const
{
    return m_provider->supportSightingDelete();
}

QVariant Form::providerAsVariant()
{
    return QVariant::fromValue<Provider*>(m_provider.get());
}

Record* Form::getRecord(const QString& recordUid) const
{
    return m_sighting->getRecord(recordUid.isEmpty() ? rootRecordUid() : recordUid);
}

void Form::saveSighting(Sighting* sighting, bool autoSaveTrack)
{
    if (m_readonly)
    {
        qDebug() << "saveSighting called on readonly sighting";
        return;
    }

    // Update the timestamps.
    auto now = m_timeManager->currentDateTimeISO();
    if (sighting->createTime().isEmpty())
    {
        sighting->snapCreateTime(now);
    }
    sighting->snapUpdateTime(now);
    sighting->recalculate();

    // Cleanup any stray records.
    sighting->recordManager()->garbageCollect();

    // Commit the sighting.
    auto attachments = QStringList();
    auto sightingData = sighting->save(&attachments);

    // If the track streamer is running, emit a location for the sighting if available.
    if (!m_editing && autoSaveTrack && m_trackStreamer->running() && m_trackStreamer->updateInterval() != 0)
    {
        auto location = sighting->locationPtr();
        if (location->isValid())
        {
            // Pick up the timestamp from the sighting, not the location.
            location->set_timestamp(sighting->createTime());

            // Note we cannot just emit a locationSaved, because that would bypass the location streamer,
            // which tracks things like distance covered.
            m_trackStreamer->pushUpdate(location.get());
        }
    }

    // Give the provider a chance to make any final changes.
    m_provider->finalizeSighting(sightingData);

    // Update the completed flag. If already set, turn it off if the sighting is no longer valid.
    auto sightingUid = sighting->rootRecordUid();
    if (isSightingCompleted(sightingUid))
    {
        markSightingCompleted();
    }

    // Commit to the db.
    m_database->saveSighting(m_project->uid(), m_stateSpace, sighting->rootRecordUid(), Sighting::DB_SIGHTING_FLAG, sightingData, attachments);
    emit App::instance()->sightingSaved(m_project->uid(), sighting->rootRecordUid(), m_stateSpace);
}

void Form::saveSighting()
{
    if (m_readonly)
    {
        return;
    }

    saveSighting(m_sighting);

    // Recalculate to ensure anything depending on the time works.
    m_sighting->recalculate();
}

void Form::loadSighting(const QString& sightingUid)
{
    qFatalIf(m_editing, "Load: complete the edit to revert to current");

    auto sightingData = QVariantMap();
    m_database->loadSighting(m_project->uid(), sightingUid, &sightingData);
    m_sighting->load(sightingData);
    m_sighting->recalculate();

    emit App::instance()->sightingModified(m_projectUid, sightingUid, m_stateSpace);
}

bool Form::canEditSighting(const QString& sightingUid) const
{
    uint flags = m_database->getSightingFlags(m_projectUid, sightingUid);
    if (flags & Sighting::DB_READONLY_FLAG)
    {
        return false;
    }

    return m_provider->canEditSighting(createSightingPtr(sightingUid).get(), flags);
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

    m_sighting->recalculate();

    emit App::instance()->sightingModified(m_projectUid, m_sighting->rootRecordUid(), m_stateSpace);
}

void Form::removeSighting(const QString& sightingUid)
{
    qFatalIf(m_editing, "Edit in progress");

    m_database->deleteSighting(m_project->uid(), sightingUid);

    emit App::instance()->sightingRemoved(m_projectUid, sightingUid, m_stateSpace);
}

void Form::prevSighting()
{
    qFatalIf(m_editing, "Edit in progress");

    QStringList sightingUids;
    m_database->getSightings(m_project->uid(), m_stateSpace, Sighting::DB_SIGHTING_FLAG, &sightingUids);
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
    m_database->getSightings(m_project->uid(), m_stateSpace, Sighting::DB_SIGHTING_FLAG, &sightingUids);
    auto currentIndex = sightingUids.indexOf(m_sighting->rootRecordUid());
    if ((sightingUids.count() == 0) || (currentIndex == sightingUids.length() - 1))
    {
        return;
    }

    loadSighting(sightingUids[currentIndex + 1]);
}

void Form::snapCreateTime()
{
    m_sighting->snapCreateTime();
}

void Form::snapUpdateTime()
{
    m_sighting->snapUpdateTime();
}

bool Form::hasData() const
{
    return m_database->hasSightings(m_project->uid(), "*", Sighting::DB_SIGHTING_FLAG | Sighting::DB_LOCATION_FLAG);
}

bool Form::isSightingValid(const QString& sightingUid) const
{
    return m_database->testSighting(m_project->uid(), sightingUid);
}

bool Form::isSightingExported(const QString& sightingUid) const
{
    return m_database->getSightingFlags(m_project->uid(), sightingUid) & Sighting::DB_EXPORTED_FLAG;
}

bool Form::isSightingUploaded(const QString& sightingUid) const
{
    return m_database->getSightingFlags(m_project->uid(), sightingUid) & Sighting::DB_UPLOADED_FLAG;
}

bool Form::isSightingCompleted(const QString& sightingUid) const
{
    return isSightingValid(sightingUid) &&
           m_database->getSightingFlags(m_project->uid(), sightingUid) & Sighting::DB_COMPLETED_FLAG;
}

void Form::setSightingFlag(const QString& sightingUid, uint flag, bool on)
{
    if (m_readonly)
    {
        return;
    }

    auto oldValue = m_database->getSightingFlags(m_projectUid, sightingUid);
    m_database->setSightingFlags(m_projectUid, sightingUid, flag, on);
    auto newValue = m_database->getSightingFlags(m_projectUid, sightingUid);

    if (oldValue != newValue)
    {
        emit App::instance()->sightingFlagsChanged(m_projectUid, sightingUid, m_stateSpace);
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

    setSightingFlag(sightingUid, Sighting::DB_COMPLETED_FLAG, valid);

    // Process auto submit.
    processAutoSubmit();
}

void Form::removeExportedSightings()
{
    App::instance()->removeSightingsByFlag(m_projectUid, m_stateSpace, Sighting::DB_EXPORTED_FLAG);
}

void Form::removeUploadedSightings()
{
    App::instance()->removeSightingsByFlag(m_projectUid, m_stateSpace, Sighting::DB_UPLOADED_FLAG);
}

QString Form::getSightingSummaryText(Sighting* sighting) const
{
    return m_provider->getSightingSummaryText(sighting);
}

QUrl Form::getSightingSummaryIcon(Sighting* sighting) const
{
    return m_provider->getSightingSummaryIcon(sighting);
}

QUrl Form::getSightingStatusIcon(Sighting* sighting, int flags) const
{
    return m_provider->getSightingStatusIcon(sighting, flags);
}

QVariant Form::getSetting(const QString& name, const QVariant& defaultValue) const
{
    return m_project->getSetting(name, defaultValue);
}

void Form::setSetting(const QString& name, const QVariant& value)
{
    m_project->setSetting(name, value);
}

QVariant Form::getGlobal(const QString& name, const QVariant& defaultValue) const
{
    return m_globals.value(name, defaultValue);
}

void Form::setGlobal(const QString& name, QVariant value)
{
    m_globals[name] = value;
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

BaseField* Form::getField(const QString& fieldUid) const
{
    return m_fieldManager->getField(fieldUid);
}

QString Form::getFieldType(const QString& fieldUid) const
{
    return m_fieldManager->getFieldType(fieldUid);
}

QString Form::getFieldName(const QString& recordUid, const QString& fieldUid) const
{
    auto result = m_provider->getFieldName(fieldUid);
    if (result.isEmpty())
    {
        result = getRecord(recordUid)->getFieldName(fieldUid);
    }

    return result;
}

QUrl Form::getFieldIcon(const QString& fieldUid) const
{
    auto result = m_provider->getFieldIcon(fieldUid);
    if (result.isEmpty())
    {
        result = getElementIcon(m_fieldManager->getField(fieldUid)->nameElementUid(), true);
    }

    return result;
}

QString Form::getFieldHint(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->getFieldHint(fieldUid);
}

QUrl Form::getFieldHintIcon(const QString& fieldUid) const
{
    auto elementUid = m_fieldManager->getField(fieldUid)->hintElementUid();
    if (elementUid.isEmpty())
    {
        return QString();
    }

    return m_elementManager->getElementIcon(elementUid);
}

QUrl Form::getFieldHintLink(const QString& fieldUid) const
{
    auto link = m_fieldManager->getField(fieldUid)->hintLink();
    if (link.isEmpty())
    {
        return QUrl();
    }

    if (link.startsWith("http://") || link.startsWith("https://"))
    {
        return QUrl(link);
    }

    return m_projectManager->getFileUrl(m_projectUid, link);
}

QString Form::getFieldConstraintMessage(const QString& recordUid, const QString& fieldUid) const
{
    return getRecord(recordUid)->getFieldConstraintMessage(fieldUid);
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
    return getRecord(recordUid)->icon();
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
    return m_provider->getFieldTitleVisible(fieldUid);
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
        return QVariantList { QVariantMap {{ "pageUrl", startPage().toString() }} };
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

void Form::applyTrackCommand()
{
    if (m_editing)
    {
        return;
    }

    if (isSightingCompleted(rootRecordUid()))
    {
        return;
    }

    auto trackSetting = m_sighting->findTrackSetting();
    if (trackSetting.isEmpty())
    {
        return;
    }

    if (!supportLocationTrack())
    {
        emit App::instance()->showError(tr("Missing track field or service"));
        return;
    }

    if (trackSetting.value("snapTrack").toBool())
    {
        snapTrack();
    }

    auto updateIntervalSeconds = trackSetting.value("updateIntervalSeconds", -1).toInt();
    if (updateIntervalSeconds == -1)
    {
        return;
    }

    auto distanceFilterMeters = trackSetting.value("distanceFilterMeters", 0).toInt();

    if (updateIntervalSeconds > 0)
    {
        m_trackStreamer->start(updateIntervalSeconds * 1000, distanceFilterMeters);
        return;
    }

    if (updateIntervalSeconds == 0)
    {
        m_trackStreamer->stop();
    }
}

void Form::pushPage(const QString& pageUrl, const QVariantMap& params, int transition)
{
    auto finalPageUrl = pageUrl;
    if (!finalPageUrl.contains('/'))
    {
        finalPageUrl = m_projectManager->getFileUrl(m_projectUid, pageUrl).toString();
    }

    auto pageStack = getPageStack();
    pageStack.append(QVariantMap {{ "pageUrl", finalPageUrl }, { "params", params } });
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
    constexpr char FORM_PAGE[] = "qrc:/imports/CyberTracker.1/FormView.qml";
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
    m_sighting->recalculate();
    saveState();

    // Set the project value.
    m_project->set_immersive(value);
}

QVariantMap Form::exportToCSV(bool cleanup)
{
    auto success = false;
    auto app = App::instance();
    auto now = m_timeManager->currentDateTime();
    auto exportFileInfo = ExportFileInfo { m_project->uid(), m_project->provider(), m_project->title(), now.toString("yyyy-MM-dd hh:mm:ss") };

    auto tempPath = QDir::cleanPath(app->tempPath() + "/snapDataToCSV");
    QDir(tempPath).removeRecursively();
    qFatalIf(!Utils::ensurePath(tempPath), "Failed to create output folder");

    // Export sighting CSV.
    auto sightingHeaderRow = QStringList { "deviceId", "id", "timestamp", "latitude", "longitude", "altitude", "accuracy" };

    QtCSV::StringData sightingRows;
    app->database()->enumSightings(m_projectUid, m_stateSpace, Sighting::DB_SIGHTING_FLAG /* ON */, Sighting::DB_EXPORTED_FLAG /* OFF */, [&](const QString& sightingUid, uint /*flags*/, const QVariantMap& data, const QStringList& /*attachments*/)
    {
        auto sighting = createSightingPtr(data);
        sighting->recalculate();

        if (exportFileInfo.startTime.isEmpty())
        {
            exportFileInfo.startTime = sighting->createTime();
        }
        exportFileInfo.stopTime = sighting->createTime();

        auto dataRow = QStringList { sighting->deviceId(), sightingUid, sighting->createTime() };
        auto location = sighting->locationPtr();
        dataRow.append(QString::number(location->y()));
        dataRow.append(QString::number(location->x()));
        dataRow.append(QString::number(location->z()));
        dataRow.append(QString::number(location->accuracy()));

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

            value = value.trimmed();

            // Since Classic does not reconstruct element lists, we have to look up Elements.
            if (!value.isEmpty())
            {
                auto element = m_elementManager->getElement(value);
                if (element != nullptr)
                {
                    if (!element->exportUid().isEmpty())
                    {
                        value = element->exportUid();
                    }
                    else
                    {
                        value = element->name();
                    }
                }
            }

            dataRow.append(value);
        });

        auto attachments = sighting->recordManager()->attachments();
        for (auto it = attachments.constBegin(); it != attachments.constEnd(); it++)
        {
            auto filename = *it;
            if (!QFile::copy(app->getMediaFilePath(filename), tempPath + "/" + filename))
            {
                qDebug() << "Failed to copy media file: " << filename;
            }
        }

        sightingRows.addRow(dataRow);
        exportFileInfo.sightingCount++;
    });

    if (exportFileInfo.sightingCount > 0)
    {
        success = QtCSV::Writer::write(tempPath + "/sighting.csv", sightingRows, ",", "\"", QtCSV::Writer::REWRITE, sightingHeaderRow);
        if (!success)
        {
            return ApiResult::ErrorMap(tr("CSV sighting export failed"));
        }
    }

    // Export location CSV.
    QtCSV::StringData locationRows;

    auto lastStopTime = Utils::decodeTimestampMSecs(exportFileInfo.stopTime);

    app->database()->enumSightings(m_projectUid, m_stateSpace, Sighting::DB_LOCATION_FLAG /* ON */, Sighting::DB_EXPORTED_FLAG /* OFF */, [&](const QString& locationUid, uint /*flags*/, const QVariantMap& data, const QStringList& /*attachments*/)
    {
        Location location(data);

        if (exportFileInfo.startTime.isEmpty())
        {
            exportFileInfo.startTime = location.timestamp();
        }

        if (Utils::decodeTimestampMSecs(location.timestamp()) > lastStopTime)
        {
            exportFileInfo.stopTime = location.timestamp();
        }

        locationRows.addRow(QStringList {
            location.deviceId(),
            locationUid,
            location.timestamp(),
            QString::number(location.y()),
            QString::number(location.x()),
            QString::number(location.z()),
            QString::number(location.accuracy()),
            QString::number(location.speed()),
            QString::number(location.counter()),
        });

        exportFileInfo.locationCount++;
    });

    if (exportFileInfo.locationCount > 0)
    {
        auto locationHeaderRow = QStringList { "deviceId", "id", "timestamp", "latitude", "longitude", "altitude", "accuracy", "speed", "counter" };
        success = QtCSV::Writer::write(tempPath + "/location.csv", locationRows, ",", "\"", QtCSV::Writer::REWRITE, locationHeaderRow);
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
    success = JlCompress::compressDir(exportFilePath, tempPath, true);
    if (!success)
    {
        return ApiResult::ErrorMap(tr("Failed to compress file"));
    }

    // Success.
    app->registerExportFile(exportFilePath, exportFileInfo.toMap());
    m_database->setSightingFlagsAll(m_projectUid, Sighting::DB_EXPORTED_FLAG);

    // Cleanup if requested.
    if (cleanup)
    {
        m_database->deleteSightings(m_projectUid, m_stateSpace, Sighting::DB_EXPORTED_FLAG);
    }

    return ApiResult::SuccessMap();
}
