#include "pch.h"
#include "Form.h"
#include "App.h"

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
    m_pointStreamer = new LocationStreamer(this);
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
    connect(m_trackStreamer, &LocationStreamer::positionUpdated, this, [&](const QGeoPositionInfo& /*update*/)
    {
        if (m_connected)
        {
            auto locationUid = QString();
            auto locationMap = App::instance()->lastLocation();
            locationMap["c"] = m_trackStreamer->counter();

            if (m_saveTrack)
            {
                locationUid = QUuid::createUuid().toString(QUuid::Id128);
                m_database->saveSighting(m_project->uid(), m_stateSpace, locationUid, Sighting::DB_LOCATION_FLAG, locationMap, QStringList());
            }

            emit locationTrack(locationMap, locationUid);
        }
    });

    // Emit a point.
    connect(m_pointStreamer, &LocationStreamer::positionUpdated, this, [&](const QGeoPositionInfo& /*update*/)
    {
        if (m_connected)
        {
            emit locationPoint(App::instance()->lastLocation());
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
    m_provider->connectToProject(m_project->lastBuildString() != app->buildString());
    m_project->set_lastBuildString(app->buildString());

    // Pick up the language code from the project settings.
    m_languageCode = m_project->languageCode();

    // Load the Elements.
    m_provider->getElements(m_elementManager);

    // Load the Fields.
    m_provider->getFields(m_fieldManager);

    // Populate the GPS time requirement: disable for simulation and iOS.
    #if defined(Q_OS_IOS)
        m_useGPSTime = false;
    #else
        m_useGPSTime = m_provider->getUseGPSTime() && !app->settings()->simulateGPS();
    #endif
    m_timeManager->set_correctionEnabled(m_useGPSTime);

    static bool once = true;
    if (once)
    {
        qDebug() << "useGPSTime: " << m_useGPSTime;
        once = false;
    }

    // Update globals.
    m_projectUid = projectUid;
    m_editSightingUid = editSightingUid;
    update_editing(!editSightingUid.isEmpty());

    // Load the state.
    loadState();

    // Ensure calculations are fresh.
    m_sighting->recalculate();

    // Update wizard buttons.
    m_wizard->updateButtons();

    // Set the start page.
    update_startPage(m_project->startPage().isEmpty() ? m_provider->getStartPage() : getPage(m_project->startPage()));

    // Set connected and notify listeners.
    update_connected(true);

    // Set the user name requirement.
    update_requireUsername(m_provider->requireUsername());

    // Resume any paused tasks.
    if (!m_readonly && !m_editing)
    {
        app->taskManager()->resumePausedTasks(m_project->uid());
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

void Form::loadState()
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
    }
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

QVariantList Form::buildSightingView(const QString& sightingUid)
{
    return m_provider->buildSightingView(createSightingPtr(sightingUid).get());
}

QVariantMap Form::buildSightingMapLayer(const QString& layerUid, const std::function<QVariantMap(const Sighting* sighting)>& getSymbol)
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
        auto locationData = sighting->location();

        if (Utils::locationValid(locationData))
        {
            auto x = locationData.value("x").toDouble();
            auto y = locationData.value("y").toDouble();
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

QVariantMap Form::buildTrackMapLayer(const QString& layerUid, const QVariantMap& symbol, bool filterToLastSegment)
{
    auto extentRect = QRectF();
    auto point = QVariantList();
    point.reserve(2);   // X and Y.
    point.append(QVariant());
    point.append(QVariant());

    auto partList = QVariantList();
    auto part = QVariantList();

    auto locationUids = QStringList();
    m_database->getSightings(m_project->uid(), m_stateSpace, Sighting::DB_LOCATION_FLAG, &locationUids);

    auto startIndex = filterToLastSegment ? locationUids.length() - 2 : 0;

    for (auto i = startIndex; i < locationUids.length(); i++)
    {
        auto locationUid = locationUids[i];
        auto locationMap = QVariantMap();
        m_database->loadSighting(m_project->uid(), locationUid, &locationMap);

        if (!Utils::locationValid(locationMap))
        {
            continue;
        }

        auto counter = locationMap["c"].toInt();

        if ((counter == 0) && (part.count() > 0))
        {
            if (part.count() > 1)
            {
                partList.append(QVariant::fromValue(part));
            }
            part.clear();
        }

        auto x = locationMap.value("x").toDouble();
        auto y = locationMap.value("y").toDouble();
        extentRect = extentRect.united(Utils::pointToRect(x, y));
        point[0] = x;
        point[1] = y;
        part.append(QVariant::fromValue(point));
    }

    if (part.count() > 1)
    {
        partList.append(QVariant::fromValue(part));
    }

    auto spatialReference = QVariantMap();
    spatialReference["wkid"] = 4326;

    auto geometry = QVariantMap();
    geometry["spatialReference"] = spatialReference;
    geometry["paths"] = partList;

    return QVariantMap
    {
        { "id", layerUid },
        { "type", "form" },
        { "name", layerUid },
        { "symbol", symbol },
        { "geometryType", "Polyline" },
        { "geometry", geometry },
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
        return;
    }

    // Update the timestamps.
    auto now = m_timeManager->currentDateTimeISO();
    if (sighting->createTime().isEmpty())
    {
        sighting->snapCreateTime(now);
    }
    sighting->snapUpdateTime(now);

    // Cleanup any stray records.
    sighting->recordManager()->garbageCollect();

    // Commit the sighting.
    auto attachments = QStringList();
    auto sightingData = sighting->save(&attachments);

    // If the track streamer is running, emit a location for the sighting if available.
    if (!m_editing && m_trackStreamer->running() && m_trackStreamer->updateInterval() != 0)
    {
        auto locationMap = sighting->location();
        if (Utils::locationValid(locationMap))
        {
            m_saveTrack = autoSaveTrack;    // Picked up by positionUpdated to determine whether to write a location.

            // Note we cannot just emit a locationSaved, because that would bypass the location streamer,
            // which tracks things like distance covered.
            m_trackStreamer->pushUpdate(locationMap);

            m_saveTrack = true;             // Restore to default.
        }
    }

    // Give the provider a chance to make any final changes.
    m_provider->finalizeSighting(sightingData);

    // Update the completed flag if it is already set.
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

void Form::markSightingCompleted()
{
    if (m_readonly)
    {
        return;
    }

    auto sightingUid = m_sighting->rootRecordUid();
    auto valid = m_sighting->getRecord(sightingUid)->isValid();
    m_database->setSightingFlags(m_projectUid, sightingUid, Sighting::DB_COMPLETED_FLAG, valid);
}

void Form::removeExportedSightings()
{
    App::instance()->removeSightingsByFlag(m_projectUid, m_stateSpace, Sighting::DB_EXPORTED_FLAG);
}

void Form::removeUploadedSightings()
{
    App::instance()->removeSightingsByFlag(m_projectUid, m_stateSpace, Sighting::DB_UPLOADED_FLAG);
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

QVariant Form::getFieldParameter(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& defaultValue) const
{
    auto field = fieldUid.isEmpty() ? getRecord(recordUid)->recordField() : m_fieldManager->getField(fieldUid);
    auto rootField = m_fieldManager->rootField();

    while (field)
    {
        auto value = field->getParameter(name);
        if (value.isValid())
        {
            return value;
        }

        if (field == rootField)
        {
            break;
        }

        field = field->parentField();
    }

    return defaultValue;
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

QString Form::getFieldDisplayValue(const QString& recordUid, const QString& fieldUid, const QString& defaultValue) const
{
    return getRecord(recordUid)->getFieldDisplayValue(fieldUid, defaultValue);
}

void Form::resetFieldValue(const QString& recordUid, const QString& fieldUid)
{
    return m_sighting->resetFieldValue(recordUid, fieldUid);
}

void Form::resetRecord(const QString& recordUid)
{
    m_sighting->resetRecord(recordUid);
}

void Form::resetFieldValue(const QString& fieldUid)
{
    resetFieldValue(m_sighting->rootRecordUid(), fieldUid);
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

    emit recordCreated(recordUid);

    return recordUid;
}

void Form::deleteRecord(const QString& recordUid)
{
    m_sighting->deleteRecord(recordUid);

    emit recordDeleted(recordUid);
}

bool Form::hasRecord(const QString& recordUid) const
{
    return m_sighting->recordManager()->hasRecord(recordUid);
}

QVariantList Form::getPageStack()
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

void Form::pushWizardPage(const QString& recordUid, const QStringList& fieldUids, int transition)
{
    if (m_sighting->wizardRecordUid() != recordUid || m_sighting->wizardPageStack().isEmpty())
    {
        m_wizard->reset();
        m_sighting->set_wizardRecordUid(recordUid);
        m_sighting->set_wizardFieldUids(fieldUids);
        m_wizard->first(recordUid, transition);
    }
    else
    {
        auto nextPageParams = m_sighting->wizardPageStack().constLast().toMap();
        pushPage(m_wizard->pageUrl(), nextPageParams, transition);
    }
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

void Form::popPages(int count)
{
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
