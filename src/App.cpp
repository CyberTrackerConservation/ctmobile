#include "pch.h"
#include "App.h"
#include "geomag/geomag.h"
#include "UtilsPlatform.h"
#include "UtilsMap.h"
#include "TrackFile.h"

#if defined(Q_OS_IOS)
#include <QDesktopServices>
#endif
#if defined(Q_OS_MACOS)
#include <mach/mach.h>
#endif

namespace CyberTracker {

namespace {
static App* s_instance;
constexpr char MEDIA_JSON_FILENAME[] = "Media.json";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(Q_OS_ANDROID)

extern "C"
{
    JNIEXPORT void JNICALL Java_org_cybertracker_mobile_MainActivity_batteryStateChange(JNIEnv*, jobject, jint level, jboolean charging)
    {
        if (s_instance)
        {
            s_instance->setBatteryState(level, charging);
        }
    }

    JNIEXPORT void JNICALL Java_org_cybertracker_mobile_MainActivity_logMessage(JNIEnv* env, jobject, jstring message)
    {
        qDebug() << env->GetStringUTFChars(message, 0);
    }

    JNIEXPORT void JNICALL Java_org_cybertracker_mobile_MainActivity_alarmFired(JNIEnv*, jobject)
    {
        if (s_instance)
        {
            s_instance->processAlarms();
        }
    }

    JNIEXPORT jboolean JNICALL Java_org_cybertracker_mobile_MainActivity_fileExists(JNIEnv* env, jobject, jstring filePath)
    {
        return static_cast<jboolean>(QFile::exists(env->GetStringUTFChars(filePath, 0)));
    }

    JNIEXPORT void JNICALL Java_org_cybertracker_mobile_MainActivity_fileReceived(JNIEnv* env, jobject, jstring filePathUrl)
    {
        if (s_instance)
        {
            s_instance->setCommandLine(QString(env->GetStringUTFChars(filePathUrl, 0)));
        }
    }

    JNIEXPORT void JNICALL Java_org_cybertracker_mobile_MainActivity_imagePickComplete(JNIEnv* env, jobject, jstring fileUrl)
    {
        if (s_instance)
        {
            emit s_instance->imagePickComplete(QString(env->GetStringUTFChars(fileUrl, 0)));
        }
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// App

App::App(const QString& rootPath, const QVariantMap& config, QObject* parent): QObject(parent), m_config(config), m_telemetry(nullptr), m_timeManager(nullptr)
{
    qFatalIf(s_instance != nullptr, "App already initialized");
    s_instance = this;

    m_shutdown = false;
    m_desktopOS = OS_DESKTOP;

#if defined(QT_DEBUG)
    m_debugBuild = true;
#else
    m_debugBuild = false;
#endif

    m_rootPath = rootPath;
#if OS_DESKTOP
    m_cachePath = m_rootPath + "/Cache";
#else
    m_cachePath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::CacheLocation);
#endif

    qFatalIf(!Utils::ensurePath(m_rootPath), "Failed to create root");
    qFatalIf(!Utils::ensurePath(m_cachePath), "Failed to create cache");

    // Log file.
    m_logFilePath = m_rootPath + "/Log.txt";

    // Settings.
    m_settings = new Settings(m_rootPath + "/Settings.json", m_config["title"].toString(), this);

    // Vault: not initialized for service.
    m_vault = nullptr;

    // TimeManager.
    m_timeManager = new TimeManager(this);
    auto now = Utils::currentDateTime();
    qDebug() << "Time device: " << Utils::encodeTimestamp(now) << now.timeZone().id();
    qDebug() << "Time manager: " << m_timeManager->currentDateTimeISO() << m_timeManager->timeZoneId();

    // Build string.
    m_buildString = QString(CT_VERSION);
    qFatalIf(m_buildString.isEmpty(), "No version string");
    qDebug() << "Version: " << m_buildString;

    // Temp path.
    m_tempPath = QDir::cleanPath(m_cachePath + "/Temp");
    qFatalIf(!Utils::ensurePath(m_tempPath), "Failed to create temp path");
    m_tempUrl = ::QUrl::fromLocalFile(m_tempPath).toString();

    // Work path.
    m_workPath = QDir::cleanPath(m_rootPath + "/Work");
    qFatalIf(!Utils::ensurePath(m_workPath), "Failed to create work path");

    // Backup path.
    m_backupPath = QDir::cleanPath(m_rootPath + "/Backup");
    Utils::enforceFolderQuota(m_backupPath, 64);
    qFatalIf(!Utils::ensurePath(m_backupPath), "Failed to create backup path");

    // BackupCTX path.
    m_backupCTXPath = QDir::cleanPath(m_rootPath + "/BackupCTX");
    Utils::enforceFolderQuota(m_backupCTXPath, 1024);
    qFatalIf(!Utils::ensurePath(m_backupCTXPath), "Failed to create backupCTX path");

    // BackupTrackLog path.
    m_backupTrackLogPath = QDir::cleanPath(m_rootPath + "/BackupTrackLog");
    Utils::enforceFolderQuota(m_backupTrackLogPath, 64);
    qFatalIf(!Utils::ensurePath(m_backupTrackLogPath), "Failed to create backupTrackLog path");

    // TrackLog file.
    m_trackLogFilePath = QDir::cleanPath(m_rootPath + "/TrackLog.bin");
    QFile(m_trackLogFilePath).open(QIODevice::NewOnly);
    qFatalIf(!QFile::exists(m_trackLogFilePath), "Failed to create track log file: " + m_trackLogFilePath);

    // Tracks path.
    m_tracksPath = QDir::cleanPath(m_rootPath + "/Tracks");
    qFatalIf(!Utils::ensurePath(m_tracksPath), "Failed to create tracks path");

    // BackupFiles path.
    m_backupFilesPath = QDir::cleanPath(m_rootPath + "/BackupFiles");
    qFatalIf(!Utils::ensurePath(m_backupFilesPath), "Failed to create backupFiles path");
}

App::App(const QString& rootPath, const QVariantMap& config, QGuiApplication* guiApplication, QQmlApplicationEngine* qmlEngine): App(rootPath, config, guiApplication)
{
    connect(qmlEngine, &QQmlApplicationEngine::destroyed, this, [&]{ delete s_instance; });

    connect(guiApplication, &QGuiApplication::aboutToQuit, this, [&]()
    {
        qDebug() << "Application shutdown";
        m_shutdown = true;

        emit aboutToQuit();

        // Shutdown async work.
        m_hasUpdate->stop();
        m_uploadQueue->stop();
        m_transcribeQueue->stop();

        // Abort httpWaits.
        Utils::httpShutdown();

        // Terminate the whole process.
    #if defined(Q_OS_WIN)
        TerminateProcess(GetCurrentProcess(), static_cast<UINT>(0));
    #elif defined(Q_OS_MACOS)
        std::_Exit(0);
    #endif
    });

    m_fixedFontFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();

    m_guiApplication = guiApplication;
    m_qmlEngine = qmlEngine;

    setBatteryState(100, false);

    m_dpi = (int)(qApp->screens()[0]->logicalDotsPerInch());

    // Vault.
    m_vault = new Vault(m_rootPath + "/Vault.json", this);

    // Load languages.
    // Generated using "translator -l languages.json"
    m_languages = Utils::variantListFromJsonFile(":/config/languages.json");

    // Cleanup logs to keep them moderately sized.
    if (QFile::exists(m_logFilePath) && QFile(m_logFilePath).size() > 1024 * 1024 * 8)
    {
        auto currLogFilePath = m_logFilePath;
        auto prevLogFilePath = m_rootPath + "/Log.old.txt";
        QFile::remove(prevLogFilePath);
        QFile::rename(currLogFilePath, prevLogFilePath);
    }

    // Init platform utils.
    m_platformUtils = new PlatformUtils(this);
    m_platformUtils->init();

    connect(m_platformUtils, &PlatformUtils::mediaSaved, this, [&](const QString& filename, const QString& contentUri)
    {
        auto replicateFilePath = m_rootPath + "/" + MEDIA_JSON_FILENAME;
        auto replicateJson = Utils::jsonDocumentFromFile(replicateFilePath).object();
        replicateJson.insert(filename.toLower(), contentUri);
        Utils::writeJsonToFile(replicateFilePath, QJsonDocument(replicateJson).toJson());
    });

    // INI path.
    m_iniPath = QDir::cleanPath(m_rootPath + "/Settings.ini");
    m_iniUrl = ::QUrl::fromLocalFile(m_iniPath).toString();

    // Map markers cache: created when rendered.
    m_mapMarkerCachePath = QDir::cleanPath(m_cachePath + "/MapMarkers");

    // Goto path.
    m_gotoPath = QDir::cleanPath(m_rootPath + "/Goto");
    qFatalIf(!Utils::ensurePath(m_gotoPath), "Failed to create goto path");

    // Export path.
    m_exportPath = QDir::cleanPath(m_rootPath + "/ExportData");
    qFatalIf(!Utils::ensurePath(m_exportPath), "Failed to create exportData path");
    m_exportUrl = ::QUrl::fromLocalFile(m_exportPath).toString();

    // Bug report path.
    m_bugReportPath = QDir::cleanPath(m_rootPath + "/BugReports");
    qFatalIf(!Utils::ensurePath(m_bugReportPath), "Failed to create BugReports path");

    // Media path.
    m_mediaPath = QDir::cleanPath(m_rootPath + "/Media");
    qFatalIf(!Utils::ensurePath(m_mediaPath), "Failed to create Media path");

    // Files path.
    m_archivePath = QDir::cleanPath(m_rootPath + "/Archive");
    qFatalIf(!Utils::ensurePath(m_archivePath), "Failed to create Archive path");

    // Share path.
    m_sendFilePath = QDir::cleanPath(m_rootPath + "/SendFile");
    qFatalIf(!Utils::ensurePath(m_sendFilePath), "Failed to create Share path");

    // NetworkAccessManager.
    m_networkAccessManager = m_qmlEngine->networkAccessManager();
    m_networkAccessManager->setTransferTimeout(60000); // 60s timeout.

    // Backup the database if the build changes.
    if (m_settings->buildString() != m_buildString)
    {
        m_settings->set_buildString(m_buildString);
        backupDatabase("New build");
    }

    // Initialize projection to decimal degrees.
    if (m_settings->projection().isEmpty())
    {
        m_settings->set_projection(QVariantMap {{ "i", 4326 }, { "d", "tr:Decimal degrees" }});
    }

    // Settings listeners.
    connect(m_settings, &Settings::languageCodeChanged, this, &App::languageCodeChanged);
    connect(m_settings, &Settings::simulateGPSChanged, this, &App::simulateGPSChanged);
    connect(m_settings, &Settings::kioskModeProjectUidChanged, this, &App::kioskModeProjectUidChanged);
    connect(m_settings, &Settings::locationAccuracyMetersChanged, this, &App::locationAccuracyChanged);
    connect(m_settings, &Settings::darkThemeChanged, this, &App::darkThemeChanged);
    locationAccuracyChanged();
    darkThemeChanged();

    // Database.
    m_database = new Database(QDir::cleanPath(m_rootPath + "/Projects.db"), this);

    // BackupFile.
    m_backupFile = new BackupFile(this);

    // ExportFile.
    m_exportFile = new ExportFile(this);

    // AppLink.
    m_appLink = new AppLink(this);

    // ProjectManager.
    m_projectManager = new ProjectManager(QDir::cleanPath(m_rootPath + "/Projects"), this);

    // UpdateManager.
    m_hasUpdate = new HasUpdate(this);

    // OfflineMapMananager.
    m_offlineMapManager = new OfflineMapManager(QDir::cleanPath(m_rootPath + "/OfflineMaps"), this);

    // Place.
    m_place = new Place(this);

    // GotoManager.
    m_gotoManager = new GotoManager(this);

    // Compass.
    m_compass = new Compass(this);

    // SatelliteManager.
    m_satelliteManager = new SatelliteManager(this);

    // LocationLogger.
    m_locationLogger = new LocationLogger(m_trackLogFilePath, this);

    // DeviceId: cleanup from the time I made it a string guid.
    if (m_settings->deviceId().isEmpty())
    {
        auto deviceId = QString();
        #if defined(Q_OS_ANDROID)
            deviceId = QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("getDeviceUid", "()Ljava/lang/String;").toString();
        #endif
        m_settings->set_deviceId(deviceId.isEmpty() ? QUuid::createUuid().toString(QUuid::Id128) : deviceId);
    }

    // Last location.
    m_lastLocation = new Location(this);
    m_lastLocationText = "??";
    m_lastLocationRecent = false;
    m_lastLocationRecentTimer.setInterval(5000);
    m_lastLocationRecentTimer.setSingleShot(true);
    m_lastLocationAccurate = false;
    connect(&m_lastLocationRecentTimer, &QTimer::timeout, this, [&]()
    {
        update_lastLocationRecent(false);
    });

    m_positionInfoSourceName = "ct_gps";

    // Setup alarm timer.
    m_alarmTimer.setSingleShot(true);
    m_alarmTimer.setTimerType(Qt::VeryCoarseTimer);
    connect(&m_alarmTimer, &QTimer::timeout, this, &App::processAlarms);

    // Google credentials.
    m_googleCredentials = m_config["googleCredentials"].toMap()[desktopOS() ? "desktop" : "mobile"].toMap();
    Utils::googleSetCredentials(m_googleCredentials);

    // Telemetry.
    m_telemetry = new Telemetry(m_rootPath + "/Telemetry.json", this);
    m_telemetry->set_gaSecret(m_config["gaSecret"].toString());
    m_telemetry->set_gaMeasurementId(m_config["gaMeasurementId"].toString());
    m_telemetry->set_aiKey(m_config["aiKey"].toString());

    // UploadQueue.
    m_uploadQueue = new UploadQueue(this);

    // Transcribe.
    m_transcribeQueue = new TranscribeQueue(this);
    connect(m_transcribeQueue, &TranscribeQueue::itemCompleted, this, &App::transcribeItemCompleted);

    // Connect to start.
    connect(guiApplication, &QGuiApplication::applicationStateChanged, this, [&](Qt::ApplicationState applicationState)
    {
        if (applicationState != Qt::ApplicationState::ApplicationActive)
        {
            return;
        }

        #if defined(Q_OS_ANDROID)
        static bool s_once = false;
        if (!s_once)
        {
            s_once = true;
            QJniObject jniTempPath = QJniObject::fromString(m_tempPath);
            QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("qtReady", "(Ljava/lang/String;)V", jniTempPath.object<jstring>());
        }
        #endif
    });

#if defined(Q_OS_ANDROID)
    m_commandLine = QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("getLaunchUri", "()Ljava/lang/String;").toString();
    if (!m_commandLine.isEmpty())
    {
        qDebug() << "Command line specified: " << m_commandLine;
    }
#endif

#if defined(Q_OS_IOS)
    QDesktopServices::setUrlHandler("https", this, "launchUrl");
    QDesktopServices::setUrlHandler("file", this, "launchUrl");
#endif

    // Detect device cache clear.
    if (!QFile::exists(m_cachePath + "/cache.json"))
    {
        clearDeviceCache();
    }

    // Audit.
    m_audit = new Audit(this);
}

App::~App()
{
    s_instance = nullptr;
}

App* App::instance()
{
    return s_instance;
}

void App::startUI()
{
    migrateDatabase();

    languageCodeChanged();
    simulateGPSChanged();
    kioskModeProjectUidChanged();
    m_hasUpdate->start();
    m_uploadQueue->start();
    m_transcribeQueue->start();

    maybeRequestReview();
}

void App::maybeRequestReview()
{
    // Bump the launch counter every launch.
    const int launchCount = m_settings->getSetting("launchCount", 0).toInt() + 1;
    m_settings->setSetting("launchCount", launchCount);

    // Only ever ask once.
    if (m_settings->getSetting("reviewRequested", false).toBool())
    {
        return;
    }

    // Threshold comes from config; 0 (or absent) disables the prompt.
    const int required = m_config.value("reviewLaunchCount", 0).toInt();
    if (required <= 0)
    {
        return;
    }

    if (launchCount >= required && m_projectManager->count() >= 1)
    {
        m_settings->setSetting("reviewRequested", true);
        m_platformUtils->requestReview();
    }
}

void App::setCommandLine(const QString& commandLine)
{
    m_commandLine = commandLine;
    emit commandLineChanged();
}

QString App::getLocationServiceLockFilePath() const
{
    return m_cachePath + "/TrackLog.svc";
}

bool App::getLocationServiceRunning() const
{
    return !QLockFile(getLocationServiceLockFilePath()).tryLock();
}

void App::launchUrl(const QUrl& url)
{
    setCommandLine(url.toString());
}

void App::migrateDatabase()
{
    auto schemaHashes = QMap<QString, QString>();

    m_database->enumLegacyData([&](const QString& projectUid, const QString& stateSpace, const QString& uid, Database::DataFlags flags, const QVariantMap& data, const QStringList& attachments)
    {
        if (!m_projectManager->exists(projectUid))
        {
            return;
        }

        auto dataKey = projectUid + '.' + stateSpace;

        auto dbData = Database::DbData();
        dbData.data = data;
        dbData.attachments = attachments;
        dbData.flags = flags;
        dbData.stateSpace = stateSpace;
        dbData.schemaHash = data.value("schemaHash").toString();

        // Migrate sightings.
        if (flags.testFlag(Database::DataFlag::DB_SIGHTING_FLAG))
        {
            dbData.timestamp = data.value("updateTime").toString();

            if (!dbData.schemaHash.isEmpty())
            {
                // Already has hash.
                schemaHashes.insert(dataKey, dbData.schemaHash);
            }

            m_database->saveData(projectUid, uid, dbData);

            if (dbData.schemaHash.isEmpty() && projectUid != ProjectManager::PROJECT_ARCHIVE)
            {
                auto form = createFormPtr(projectUid, stateSpace);
                auto schemaFilePath = m_projectManager->getFilePath(projectUid, form->provider()->getSchemaFilename());
                if (!QFile::exists(schemaFilePath))
                {
                    qDebug() << "Failed to find a schema for: " << projectUid << uid;
                    return;
                }

                dbData.schemaHash = copyToArchive(schemaFilePath);

                form->loadSighting(uid);
                auto icons = form->renderer()->getSightingIcons(form->sighting());
                icons.append(form->project()->icon());
                auto iconHashes = form->project()->archiveFiles(icons);
                dbData.data.insert("iconHashes", iconHashes);

                auto location = form->sighting()->locationPtr();
                if (location)
                {
                    dbData.x = location->x();
                    dbData.y = location->y();
                }

                m_database->saveData(projectUid, uid, dbData);

                schemaHashes.insert(dataKey, dbData.schemaHash);
            }

            return;
        }
    });

    m_database->dropLegacyData();

    // Delete data with no schema: this is pure sanity cleanup.
    auto deleteItems = QList<QPair<QString, QString>>();
    m_database->enumData("*", "*", Database::DataFlag::DB_SIGHTING_FLAG,
        [&](const QString& projectUid, const QString& sightingUid, const Database::DbData& dbData)
        {
            auto schemaFilePath = getArchiveFilePath(dbData.schemaHash);
            if (!QFile::exists(schemaFilePath))
            {
                deleteItems.append(QPair<QString, QString>(projectUid, sightingUid));
            }
        });

    for (auto deleteIt = deleteItems.constBegin(); deleteIt != deleteItems.constEnd(); deleteIt++)
    {
        m_database->deleteData(deleteIt->first, deleteIt->second);
    }

    // Cleanup streamer state: this corrects an earlier design when the classic streamer was separate.
    // Move tokens to vault from project settings.
    auto projects = m_projectManager->buildListPtr();
    for (auto projectIt = projects.constBegin(); projectIt != projects.constEnd(); projectIt++)
    {
        auto project = *projectIt;
        auto projectUid = project->uid();
        m_database->deleteFormState(projectUid, "/classic");
        project->set_hasStreamers(m_database->hasStreamers(projectUid));

        // Migrate connector changes.
        auto connector = createConnectorPtr(project->connector());
        if (connector)
        {
            connector->migrateProject(project.get());
        }

        // Move tokens to the vault.
        auto accessToken = project->getSetting("accessToken").toString();
        if (!accessToken.isEmpty())
        {
            m_vault->setAccessToken(projectUid, accessToken);
            project->setSetting("accessToken", QVariant());
        }

        auto refreshToken = project->getSetting("refreshToken").toString();
        if (!refreshToken.isEmpty())
        {
            m_vault->setRefreshToken(projectUid, refreshToken);
            project->setSetting("refreshToken", QVariant());
        }
    }

    m_database->compact();

    // Sync with downloads.
    m_exportFile->syncWithDownloads();
}

void App::registerConnector(const QString& name, ConnectorFactory connectorFactory)
{
    qFatalIf(m_connectors.contains(name), "Connector already registered: " + name);
    m_connectors[name] = connectorFactory;
}

Connector* App::createConnector(const QString& name, QObject* parent)
{
    if (name.isEmpty())
    {
        qDebug() << "Error - connector not specified";
        return nullptr;
    }

    auto allowConnector = m_config.value("allowConnector").toString();
    if (!allowConnector.isEmpty() && name != allowConnector)
    {
        qDebug() << "Error - unsupported connector";
        return nullptr;
    }

    qFatalIf(!m_connectors.contains(name), "Connector not found: " + name);
    return m_connectors[name](parent);
}

std::unique_ptr<Connector> App::createConnectorPtr(const QString& name)
{
    return std::unique_ptr<Connector>(createConnector(name, nullptr));
}

void App::registerProvider(const QString& name, ProviderFactory providerFactory)
{
    qFatalIf(m_providers.contains(name), "Provider already registered: " + name);
    m_providers[name] = providerFactory;
}

Provider* App::createProvider(const QString& name, Form* form)
{
    if (name.isEmpty())
    {
        qDebug() << "Error - Provider not specified";
        return nullptr;
    }

    qFatalIf(!m_providers.contains(name), "Provider not found: " + name);
    return m_providers[name](form);
}

std::unique_ptr<Provider> App::createProviderPtr(const QString& name, Form* form)
{
    return std::unique_ptr<Provider>(createProvider(name, form));
}

void App::registerRenderer(const QString& name, RendererFactory rendererFactory)
{
    qFatalIf(m_renderers.contains(name), "Renderer already registered: " + name);
    m_renderers[name] = rendererFactory;
}

Renderer* App::createRenderer(const QString& name, QObject* parent)
{
    if (name.isEmpty())
    {
        return new Renderer(parent);
    }

    qFatalIf(!m_renderers.contains(name), "Renderer not found: " + name);
    return m_renderers[name](parent);
}

std::unique_ptr<Renderer> App::createRendererPtr(const QString& name)
{
    return std::unique_ptr<Renderer>(createRenderer(name, nullptr));
}

std::unique_ptr<Database> App::createDatabasePtr()
{
    return std::unique_ptr<Database>(new Database(m_database->filePath(), nullptr));
}

Form* App::createForm(const QString& projectUid, const QString& stateSpace, bool readonly, QObject* parent)
{
    auto form = new Form(parent);
    form->set_readonly(readonly);
    form->set_stateSpace(stateSpace);
    form->connectToProject(projectUid);

    if (!form->connected())
    {
        delete form;
        form = nullptr;
    }

    return form;
}

std::unique_ptr<Form> App::createFormPtr(const QString& projectUid, const QString& stateSpace, bool readonly)
{
    return std::unique_ptr<Form>(createForm(projectUid, stateSpace, readonly));
}

void App::deleteSightingsByFlag(const QString& projectUid, const QString& stateSpace, Database::DataFlags matchFlag, bool disableArchive)
{
    // Check if any data contains the flag.
    if (!m_database->hasData(projectUid, stateSpace, matchFlag))
    {
        return;
    }

    // Archive or delete data.
    if (projectUid != ProjectManager::PROJECT_ARCHIVE && m_settings->archiveSupport() && m_settings->archiveData() && !disableArchive)
    {
        m_database->archiveData(ProjectManager::PROJECT_ARCHIVE, projectUid, stateSpace, Database::DataFlag::DB_SIGHTING_FLAG | matchFlag);

        emit sightingsChanged(ProjectManager::PROJECT_ARCHIVE);
    }
    else
    {
        m_database->deleteData(projectUid, stateSpace, Database::DataFlag::DB_SIGHTING_FLAG | matchFlag);

        // Clean up stray media.
        garbageCollectMedia();
    }

    // Send notification.
    emit sightingsChanged(projectUid);
}

void App::deleteSighting(const QString& projectUid, const QString& sightingUid, bool disableArchive)
{
    if (!m_database->testData(projectUid, sightingUid))
    {
        qDebug() << "Failed to remove sighting: " << sightingUid;
        return;
    }

    if (projectUid != ProjectManager::PROJECT_ARCHIVE && m_settings->archiveSupport() && m_settings->archiveData() && !disableArchive)
    {
        m_database->archiveData(ProjectManager::PROJECT_ARCHIVE, projectUid, sightingUid);
        emit sightingSaved(ProjectManager::PROJECT_ARCHIVE, sightingUid);
    }
    else
    {
        m_database->deleteData(projectUid, sightingUid);
    }

    emit sightingDeleted(projectUid, sightingUid);
}

void App::setSightingNote(const QString& projectUid, const QString& sightingUid, const QString& note)
{
    Sighting::setNote(projectUid, sightingUid, note);

    emit sightingModified(projectUid, sightingUid);
}

QString App::getArchiveFilePath(const QString& hashFilename) const
{
    return QString("%1/%2").arg(m_archivePath, hashFilename);
}

QString App::resolveArchiveUrl(const QString& hashFilename) const
{
    if (hashFilename.contains(':'))
    {
        return hashFilename;
    }

    if (hashFilename.isEmpty())
    {
        return QString();
    }

    auto filePath = getArchiveFilePath(hashFilename);
    if (!QFile::exists(filePath))
    {
        qDebug() << "Request for missing archive file: " << hashFilename;
        return QString();
    }

    return ::QUrl::fromLocalFile(filePath).toString();
}

bool App::copyToArchive(const QString& filePath, const QString& hashFilename) const
{
    auto fileInfo = QFileInfo(filePath);
    qFatalIf(!fileInfo.exists(), "File not found: " + filePath);

    auto hashFilePath = getArchiveFilePath(hashFilename);
    auto hashFileInfo = QFileInfo(hashFilePath);

    if (hashFileInfo.exists() && fileInfo.size() == hashFileInfo.size())
    {
        return true;
    }

    if (!QFile::copy(filePath, hashFilePath))
    {
        qFatalIf(true, "Failed to commit to file store: " + filePath);
        return false;
    }

    return true;
}

QString App::copyToArchive(const QString& filePath) const
{
    auto hashFilename = Utils::computeFileHash(filePath) + "." + QFileInfo(filePath).suffix();

    return copyToArchive(filePath, hashFilename) ? hashFilename : "";
}

void App::garbageCollectArchive()
{
    auto filesInUse = QHash<QString, bool>();

    // Add currently loaded schemas.
    auto cached = Schema::cached();
    for (auto cachedIt = cached.constBegin(); cachedIt != cached.constEnd(); cachedIt++)
    {
        filesInUse.insert(*cachedIt, true);
    }

    // Build a list of files in use.
    m_database->enumData("*", "*", Database::DataFlag::DB_SIGHTING_FLAG,
        [&](const QString& /*projectUid*/, const QString& /*sightingUid*/, const Database::DbData& dbSighting)
        {
            filesInUse.insert(dbSighting.schemaHash.toLower(), true);

            auto iconHashes = dbSighting.data.value("iconHashes").toMap();
            for (auto iconHashIt = iconHashes.constKeyValueBegin(); iconHashIt != iconHashes.constKeyValueEnd(); iconHashIt++)
            {
                filesInUse.insert(iconHashIt->second.toString().toLower(), true);
            }
        });

    // Enumerate the archived files and remove those not referenced.
    auto archiveFiles = QDir(m_archivePath).entryInfoList(QDir::Files);
    for (auto it = archiveFiles.constBegin(); it != archiveFiles.constEnd(); it++)
    {
        if (filesInUse.contains(it->fileName().toLower()))
        {
            continue;
        }

        qDebug() << "GC archive: " << it->fileName();
        QFile::remove(it->filePath());
    }
}

QVariantMap App::getLogin(const QString& key, const QString& defaultServer) const
{
    auto logins = m_settings->logins();
    auto login = logins.value(key).toMap();

    // Lookup legacy item.
    if (login.isEmpty())
    {
        login = m_settings->getSetting(key + "Login", QVariantMap()).toMap();
    }

    // Ensure good defaults.
    login["server"] = login.value("server", defaultServer);
    login["serverIndex"] = login.value("serverIndex", -1);
    login["username"] = login.value("username", "");
    login["password"] = login.value("password", "");

    return login;
}

void App::setLogin(const QString& key, const QVariantMap& value)
{
    auto finalValue = value;

    if (!m_debugBuild && !value.value("cachePassword").toBool())
    {
        finalValue.remove("password");
    }

    auto logins = m_settings->logins();
    logins[key] = finalValue;
    m_settings->set_logins(logins);

    // Remove legacy item.
    m_settings->setSetting(key + "Login", QVariant());
}

QString App::getLocationText(double x, double y, QVariant /*z*/) const
{
    return Utils::getLocationText(m_settings->projection(), x, y);
}

QVariantMap App::projectLocation(double x, double y) const
{
    return Utils::projectLocation(m_settings->projection(), x, y);
}

QVariantMap App::unprojectLocation(double a, double b) const
{
    return Utils::unprojectLocation(m_settings->projection(), a, b);
}

void App::positionUpdated(const QGeoPositionInfo& update)
{
    auto location = new Location(update, this);

    // Compute magnetic declination.
    float magneticDeclination = 0.0;
    getMagneticDeclination(location->y(), location->x(), location->z(), &magneticDeclination);
    m_settings->set_magneticDeclination(magneticDeclination);

    // Update the compass based on the GPS heading if simulation is active.
    if (!std::isnan(location->direction()))
    {
        m_compass->set_simulatedAzimuth(location->direction() - magneticDeclination);
    }

    // Recalculate goto data.
    m_gotoManager->recalculate(location);

    // Keep lastLocationRecent up to date.
    update_lastLocationRecent(true);
    m_lastLocationRecentTimer.start();

    // Keep lastLocationAccurate up to date.
    update_lastLocationAccurate(location->isAccurate());

    // Change and signal.
    delete m_lastLocation;
    update_lastLocation(location);
    update_lastLocationText(getLocationText(location->x(), location->y()));
}

void App::languageCodeChanged()
{
    auto code = m_settings->languageCode();

    // Redirect to approximations.
    auto redirects = QMap<QString, QString>
    {
        { "system", QLocale::system().name() },
        { "prs", "fa" },
    };

    code = redirects.contains(code) ? redirects[code] : code;

    m_locale = QLocale(code);
    if (m_locale == QLocale::c())
    {
        qDebug() << "Using system locale: " << code;
        m_locale = QLocale::system();
    }

    // Load the language file.
    if (m_translator.load(":/i18n/ct_" + code))
    {
        qApp->installTranslator(&m_translator);
        m_qmlEngine->retranslate();
    }
    else
    {
        qDebug() << "ERROR: Failed to load translation for: " << code;
    }

    // Update the language name.
    update_languageName(tr("Unknown"));
    for (auto it = m_languages.constBegin(); it != m_languages.constEnd(); it++)
    {
        auto map = it->toMap();
        if (code.startsWith(map["code"].toString(), Qt::CaseInsensitive))
        {
            update_languageName(map["name"].toString());
            break;
        }
    }

    // Refresh the battery text.
    setBatteryState(m_batteryLevel, m_batteryCharging);

    // Aliases.
    auto projectAlias = m_config.value("projectAlias", "form").toString();
    if (projectAlias == "project")
    {
        update_alias_Project(tr("Project"));
        update_alias_Projects(tr("Projects"));
        update_alias_project(tr("project"));
        update_alias_projects(tr("projects"));
    }
    else if (projectAlias == "form")
    {
        update_alias_Project(tr("Form"));
        update_alias_Projects(tr("Forms"));
        update_alias_project(tr("form"));
        update_alias_projects(tr("forms"));
    }
    else if (projectAlias == "survey")
    {
        update_alias_Project(tr("Survey"));
        update_alias_Projects(tr("Surveys"));
        update_alias_project(tr("survey"));
        update_alias_projects(tr("surveys"));
    }
}

void App::simulateGPSChanged()
{
    auto enabled = m_settings->simulateGPS();

    if (enabled)
    {
        auto filePath = m_settings->simulateGPSFilename();
        if (filePath.isEmpty())
        {
            filePath = ":/app/gabon.nmea";
        }
        else
        {
            filePath.prepend("/");
            filePath.prepend(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).constFirst());
        }

        m_simulateNmeaServer.stop();
        m_simulateNmeaServer.start(filePath);
    }
    else
    {
        m_simulateNmeaServer.stop();
    }

    m_compass->set_simulated(enabled);
}

void App::kioskModeProjectUidChanged()
{
    auto kioskMode = false;

    if (Utils::isAndroid() && m_config.value("androidKiosk", false).toBool())
    {
        kioskMode = !m_settings->kioskModeProjectUid().isEmpty();
        qDebug() << "kioskMode: " << kioskMode;
        #if defined(Q_OS_ANDROID)
            QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("setKioskMode", "(Z)V", static_cast<jboolean>(kioskMode));
        #endif
    }

    update_kioskMode(kioskMode);
}

void App::locationAccuracyChanged()
{
    update_locationAccuracyText(QString("%1 > %2").arg(tr("Accuracy"), getDistanceText(m_settings->locationAccuracyMeters())));
}

void App::darkThemeChanged()
{
#if defined(Q_OS_ANDROID)
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("setDarkTheme", "(Z)V", static_cast<jboolean>(m_settings->darkTheme()));
#endif
}

void App::transcribeItemCompleted(const Database::DbTranscribeItem& transcribeItem)
{
    if (m_database->testData(transcribeItem.projectUid, transcribeItem.dataUid))
    {
        auto form = std::unique_ptr<Form>(new Form(nullptr));
        form->set_stateSpace(transcribeItem.stateSpace);
        form->connectToProject(transcribeItem.projectUid, transcribeItem.dataUid);
        form->processTranscribeResult(transcribeItem);
        form->saveSighting(form->sighting(), false);
    }
}

QString App::createBugReport(const QString& description)
{
    auto reportName = "BugReport";
    qDebug() << "Create bug report: " << reportName;

    auto reportPath = m_bugReportPath;
#if defined(Q_OS_ANDROID)
    reportPath = Utils::androidGetExternalFilesDir() + "/bugReports";
#elif defined(Q_OS_IOS)
    reportPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DocumentsLocation);
#endif

    qFatalIf(reportPath.isEmpty(), "Bad bug report path");
    qFatalIf(!Utils::ensurePath(reportPath), "Failed to create report path");
    reportPath = QDir::cleanPath(reportPath + "/" + reportName + ".zip");
    qFatalIf(QFile::exists(reportPath) && !QFile::remove(reportPath), "Failed to delete old report");

    auto tempPath = QDir::cleanPath(m_tempPath + "/" + reportName);
    QDir(tempPath).removeRecursively();
    qFatalIf(!Utils::ensurePath(tempPath), "Failed to create output folder");

    // Copy files to the temp folder.
    QFile::copy(m_rootPath + "/Projects.db", tempPath + "/Projects.db");
    QFile::copy(m_rootPath + "/Settings.json", tempPath + "/Settings.json");
    QFile::copy(m_rootPath + "/Settings.ini", tempPath + "/Settings.ini");
    QFile::copy(m_rootPath + "/Media.json", tempPath + "/Media.json");
    QFile::copy(m_rootPath + "/Log.txt", tempPath + "/Log_" + m_buildString + ".txt");
    QFile::copy(m_rootPath + "/Log.old.txt", tempPath + "/Log.old.txt");
    QFile::copy(m_trackLogFilePath, tempPath + "/TrackLog.bin");
    Utils::copyPath(m_exportPath, tempPath + "/ExportData");
    Utils::copyPath(m_rootPath + "/Projects", tempPath + "/Projects", QStringList { "map.mbtiles" }); // Exclude map.mbtiles.
    Utils::copyPath(m_backupPath, tempPath + "/Backup");
    Utils::copyPath(m_archivePath, tempPath + "/Archive");
    Utils::copyPath(m_workPath, tempPath + "/Work");
    Utils::copyPath(m_mediaPath, tempPath + "/Media");
    Utils::copyPath(m_backupCTXPath, tempPath + "/BackupCTX");
    Utils::copyPath(m_backupTrackLogPath, tempPath + "/TrackLogs");

    if (QDir(Utils::classicRoot()).exists())
    {
        Utils::copyPath(Utils::classicRoot(), tempPath + "/Classic");
    }

    // Write description file.
    {
        QFile file(tempPath + "/Report.txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            auto data = QString();
            data += Utils::encodeTimestamp(Utils::currentDateTime()).toLatin1();
            data += "\r\nBuild " + m_buildString + "\r\n";
            file.write(data.toLatin1());
            file.write(description.toLatin1());
        }
    }

    auto result = QString();

    // Zip the file.
    if (Utils::zipCompressDir(reportPath, tempPath))
    {
        result = reportPath;
        Utils::mediaScan(reportPath);
    }

    QDir(tempPath).removeRecursively();

    return result;
}

void App::backupDatabase(const QString& reason)
{
    qDebug() << "Backup database: " << reason;
    // Generate a name based on time.
    auto outputPath = QString("%1/Projects_%2.db").arg(
        m_backupPath,
        QDateTime::currentDateTime().toString("yyyy-MM-ddThh_mm_ss"));

    QFile::copy(m_rootPath + "/Projects.db", outputPath);

    // Keep at most 64 backup files.
    Utils::enforceFolderQuota(m_backupPath, 64);
}

void App::backupCTX(const QString& filePath) const
{
    auto file = QFile(filePath);
    if (!file.copy(m_backupCTXPath + "/" + QFileInfo(filePath).fileName()))
    {
        qDebug() << "Error: failed to backup CTX file: " << file.errorString();
    }
}

void App::backupTrackLog() const
{
    auto file = QFile(m_trackLogFilePath);
    if (!file.copy(m_backupTrackLogPath + "/" + Utils::makeUniqueFilename("TrackLog", QDateTime::currentDateTime(), ".bin")))
    {
        qDebug() << "Error: failed to backup tracklog file: " << file.errorString();
    }
}

void App::initProgress(const QString& projectUid, const QString& operation, const QVector<int>& phases)
{
    m_progressProjectUid = projectUid;
    m_progressOperation = operation;
    m_progressPhases = phases;
    m_progressStates.clear();
    m_progressStates.resize(phases.count());

    sendProgress(0, 0);
}

void App::sendProgress(int value, int phase)
{
    qFatalIf(phase >= m_progressPhases.count(), "Bad phase number");
    qFatalIf(value > m_progressPhases[phase], "Bad phase value");

    m_progressStates[phase] = value;

    auto total = 0;
    auto state = 0;
    for (int i = 0; i < m_progressPhases.count(); i++)
    {
        total += m_progressPhases[i];
        state += m_progressStates[i];
    }

    emit progressEvent(m_progressProjectUid, m_progressOperation, state, total);
    QGuiApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void App::requestPermission(const QPermission& permission, const QVariant& callback, const QString& rationale)
{
#if QT_CONFIG(permissions)
    switch (qApp->checkPermission(permission))
    {
    case Qt::PermissionStatus::Undetermined:
        {
            qApp->requestPermission(permission, [this, callback, rationale](const QPermission& permission)
            {
                auto granted = permission.status() == Qt::PermissionStatus::Granted;
                if (granted)
                {
                    emit permissionGranted(callback);
                }
                else
                {
                    emit showError(rationale);
                }
            });
        }
        break;

    case Qt::PermissionStatus::Denied:
        emit showError(rationale);
        break;

    case Qt::PermissionStatus::Granted:
        emit permissionGranted(callback);
        break;
    }
#else
    emit permissionGranted(callback);
#endif
}

void App::requestPermissionCamera(const QVariant& callback)
{
    requestPermission(QCameraPermission(), callback, tr("Using the camera requires permission. Please allow the request in order to proceed."));
}

void App::requestPermissionMicrophone(const QVariant& callback)
{
    requestPermission(QMicrophonePermission(), callback, tr("Using the microphone requires permission. Please allow the request in order to proceed."));
}

void App::requestPermissionLocation(const QVariant& callback)
{
    if (m_settings->simulateGPS())
    {
        emit permissionGranted(callback);
        return;
    }

    auto permission = QLocationPermission();
    permission.setAccuracy(QLocationPermission::Precise);
    permission.setAvailability(QLocationPermission::WhenInUse);
    requestPermission(permission, callback, tr("Access to GPS is required for this feature to work. Please allow the request in order to proceed."));
}

void App::requestPermissionBackgroundLocation(const QVariant& callback)
{
    if (m_settings->simulateGPS())
    {
        emit permissionGranted(callback);
        return;
    }

    auto permission = QLocationPermission();
    permission.setAccuracy(QLocationPermission::Precise);
    permission.setAvailability(QLocationPermission::Always);
    if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted)
    {
        emit permissionGranted(callback);
        return;
    }

    auto rationale = tr("Background access to GPS is required. Please allow the request in order to proceed.");

#if defined(Q_OS_ANDROID)
    if (QNativeInterface::QAndroidApplication::sdkVersion() < 29)
    {
        emit permissionGranted(callback);
        return;
    }
    
    auto watcher = new QFutureWatcher<QtAndroidPrivate::PermissionResult>(this);
    watcher->setFuture(QtAndroidPrivate::requestPermission("android.permission.ACCESS_BACKGROUND_LOCATION"));
    connect(watcher, &QFutureWatcher<QtAndroidPrivate::PermissionResult>::finished, [this, watcher, callback, rationale]
    {
        if (watcher->result() == QtAndroidPrivate::PermissionResult::Authorized)
        {
            emit permissionGranted(callback);
        }
        else
        {
            emit showError(rationale);
        }

        watcher->deleteLater();
    });
#else
    requestPermission(permission, callback, rationale);
#endif
}

bool App::promptForLocation(const QString& projectUid) const
{
    if (m_settings->simulateGPS() || m_desktopOS)
    {
        return false;
    }

    auto permission = QLocationPermission();
    permission.setAccuracy(QLocationPermission::Precise);
    if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted)
    {
        return false;
    }

    if (projectUid.isEmpty())
    {
        return true;
    }

    auto project = m_projectManager->loadPtr(projectUid);
    if (project->requireLocation() || project->requireBackgroundLocation())
    {
        return true;
    }

    return false;
}

bool App::promptForBackgroundLocation(const QString& projectUid) const
{
    if (m_settings->simulateGPS() || m_desktopOS)
    {
        return false;
    }

    auto permission = QLocationPermission();
    permission.setAccuracy(QLocationPermission::Precise);
    permission.setAvailability(QLocationPermission::Always);
    if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted)
    {
        return false;
    }

    if (projectUid.isEmpty())
    {
        return true;
    }

    auto project = m_projectManager->loadPtr(projectUid);
    if (project->requireBackgroundLocation())
    {
    #if defined(Q_OS_IOS)
        return false;
    #else
        return true;
    #endif
    }

    return false;
}

bool App::promptForCamera(const QString& projectUid) const
{
    auto permission = QCameraPermission();
    if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted)
    {
        return false;
    }

    if (projectUid.isEmpty())
    {
        return true;
    }

    auto project = m_projectManager->loadPtr(projectUid);
    if (project->requireCamera())
    {
        return true;
    }

    return false;
}

bool App::promptForMicrophone(const QString& projectUid) const
{
    auto permission = QMicrophonePermission();
    if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted)
    {
        return false;
    }

    if (projectUid.isEmpty())
    {
        return true;
    }

    auto project = m_projectManager->loadPtr(projectUid);
    if (project->requireMicrophone())
    {
        return true;
    }

    return false;
}

bool App::getActive()
{
    return m_guiApplication->applicationState() == Qt::ApplicationActive;
}

void App::setBatteryState(int level, bool charging)
{
    update_batteryLevel(level);
    update_batteryCharging(charging);

    auto icon = QString("qrc:/icons/battery_");
    if (charging)
    {
        icon += "charging_";
    }

    // Round up.
    level = std::min(100, level + 5);

    // Update icon.
    icon += QString::number((level / 10) * 10);
    icon += ".svg";
    update_batteryIcon(icon);

    // Update text.
    auto text = QString("%1 - %2%3").arg(tr("Battery level"), QString::number(level), "%");
    update_batteryText(text);
}

QString App::formatDate(const QDate& date) const
{
    return m_locale.toString(date, m_settings->dateFormat());
}

QString App::formatTime(const QTime& time) const
{
    return m_locale.toString(time, m_settings->timeFormat());
}

QString App::formatDateTime(qint64 mSecsSinceEpoch) const
{
    return formatDateTime(Utils::decodeTimestamp(m_timeManager->formatDateTime(mSecsSinceEpoch)));
}

QString App::formatDateTime(const QDateTime& dateTime) const
{
    return m_locale.toString(dateTime, m_settings->dateTimeFormat());
}

QString App::downloadFile(const QString& url, const QString& username, const QString& password, const QString& token, const QString& tokenType)
{
    QNetworkRequest request;
    QEventLoop eventLoop;
    bool success = false;

    // Setup the target file.
    auto result = m_tempPath + "/" + QUuid::createUuid().toString(QUuid::Id128);
    QFile::remove(result);
    qFatalIf(QFile::exists(result), "Target cannot be deleted");
    QFile file(result);
    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "Failed to create temp zip file: " << result;
        return QString();
    }

    // Auth.
    if (!username.isEmpty() && !password.isEmpty())
    {
        auto auth = "Basic " + (username + ":" + password).toLocal8Bit().toBase64();
        request.setRawHeader("Authorization", auth);
    }

    if (!token.isEmpty())
    {
        request.setRawHeader("Authorization", QString(tokenType + " " + token).toUtf8());
    }

    // Start.
    initProgress("App", "Download", { 100 });

    auto finalUrl = Utils::redirectOnlineDriveUrl(url);

    for (;;)
    {
        request.setUrl(finalUrl);
        auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->get(request));

        connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);

        connect(reply.get(), &QNetworkReply::downloadProgress, this, [&](qint64 bytesReceived, qint64 bytesTotal)
        {
            if (bytesTotal != 0)
            {
                sendProgress(static_cast<int>((bytesReceived * 100) / bytesTotal), 0);
            }
        });

        connect(reply.get(), &QNetworkReply::readyRead, this, [&]()
        {
            file.write(reply->readAll());
        });

        eventLoop.exec();

        auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirectUrl.isValid())
        {
            finalUrl = redirectUrl.toString();
            continue;
        }

        success = reply->isFinished() && reply->error() == QNetworkReply::NoError;
        if (!success)
        {
            qDebug() << "Error downloading file: " << reply->errorString();
        }

        break;
    }

    file.close();

    if (!success)
    {
        QFile::remove(result);
        return QString();
    }

    // Pick up a good suffix.
    auto suffix = QFileInfo(finalUrl).suffix();
    if (suffix.isEmpty() || suffix.length() > 4)
    {
        suffix = Utils::detectFileSuffix(result);
    }

    if (!suffix.isEmpty())
    {
        auto newName = result + "." + suffix;
        if (QFile::rename(result, newName))
        {
            result = newName;
        }
    }

    // Success.
    return result;
}

QString App::copyContent(const QString& contentUri, const QString& targetPath)
{
    auto result = m_platformUtils->copyContent(contentUri, targetPath);
    if (result.isEmpty())
    {
        emit showError(tr("Failed to copy file"));
    }

    return result;
}

QString App::copyToDownload(const QString& filePath, const QString& folder)
{
    return m_platformUtils->copyToDownload(filePath, folder);
}

QString App::getMediaFilename(const QString& fileSuffix) const
{
    auto prefixes = QMap<QString, QString>
    {
        { "wav",  "AUDIO" },
        { "mp3",  "AUDIO" },
        { "png",  "PHOTO" },
        { "jpeg", "PHOTO" },
        { "jpg",  "PHOTO" },
        { "heic", "PHOTO" },
    };

    return Utils::makeUniqueFilename(prefixes.value(fileSuffix.toLower(), "FILE") + "_", m_timeManager->currentDateTime(), "." + fileSuffix);
}

QString App::moveToMedia(const QString& filePath) const
{
    if (!QFile::exists(filePath))
    {
        qDebug() << "File not found " << filePath;
        return "";
    }

    auto mediaFilename = getMediaFilename(QFileInfo(filePath).suffix());
    auto mediaFilePath = QString("%1/%2").arg(m_mediaPath, mediaFilename);

    if (!QFile::rename(filePath, mediaFilePath))
    {
        if (!QFile::copy(filePath, mediaFilePath))
        {
            return "";
        }
    }

    return mediaFilename;
}

QString App::copyToMedia(const QString& filePath) const
{
    if (!QFile::exists(filePath))
    {
        qDebug() << "File not found " << filePath;
        return "";
    }

    auto mediaFilename = getMediaFilename(QFileInfo(filePath).suffix());
    auto mediaFilePath = QString("%1/%2").arg(m_mediaPath, mediaFilename);

    if (!QFile::copy(filePath, mediaFilePath))
    {
        qDebug() << "Failed to copy " << filePath << " to " << mediaFilePath;
        return "";
    }

    return mediaFilename;
}

bool App::replicateMedia(const QString& filename)
{
    auto mediaFilePath = QString("%1/%2").arg(m_mediaPath, filename);
    QFile mediaFile(mediaFilePath);
    if (!mediaFile.exists())
    {
        qDebug() << "Media file missing: " << mediaFilePath;
        return false;
    }

    if (!m_platformUtils->saveMedia(mediaFilePath, filename, getMediaMimeType(filename), m_config["title"].toString()))
    {
        qDebug() << "Failed to replicate media: " << mediaFilePath;
        return false;
    }

    return true;
}

bool App::removeMedia(const QString& filename)
{
    if (filename.isEmpty())
    {
        return true;
    }

    auto mediaFilePath = QString("%1/%2").arg(m_mediaPath, filename);
    auto result = QFile::remove(mediaFilePath);
    if (QFile::exists(mediaFilePath) && !result)
    {
        qDebug() << "Failed to remove media: " << mediaFilePath;
        return false;
    }

    // Cleanup media that was replicated.
    auto replicateFilePath = m_rootPath + "/" + MEDIA_JSON_FILENAME;
    auto replicateJson = Utils::jsonDocumentFromFile(replicateFilePath).object();
    auto contentUri = replicateJson.value(filename.toLower()).toString();
    if (!contentUri.isEmpty())
    {
        replicateJson.remove(filename.toLower());
        Utils::writeJsonToFile(replicateFilePath, QJsonDocument(replicateJson).toJson());
        m_platformUtils->deleteMedia(contentUri);
    }

    return result;
}

QString App::convertMediaToJpeg(const QString& filename) const
{
    return QFileInfo(Utils::convertImageToJpeg(getMediaFilePath(filename))).fileName();
}

QString App::getMediaFilePath(const QString& filename) const
{
    if (filename.isEmpty())
    {
        return QString();
    }

    qFatalIf(filename.contains('/'), "Filename should not have a path");

    return m_mediaPath + "/" + filename;
}

QString App::getMediaFileUrl(const QString& filename) const
{
    if (filename.isEmpty())
    {
        return QString();
    }

    return ::QUrl::fromLocalFile(getMediaFilePath(filename)).toString();
}

QString App::getMediaMimeType(const QString& filename) const
{
    auto mimeType = Utils::mimeDatabase()->mimeTypeForFile(getMediaFilePath(filename));
    return mimeType.isValid() ? mimeType.name().toLatin1() : QString();
}

void App::garbageCollectMedia() const
{
    auto attachments = m_database->getAttachments();

    // Insert attachments into a map.
    auto map = QMap<QString, bool>();
    for (auto it = attachments.constBegin(); it != attachments.constEnd(); it++)
    {
        map.insert(it->toLower(), true);
    }

    // Read replicated store.
    auto replicateFilePath = m_rootPath + "/" + MEDIA_JSON_FILENAME;
    auto replicateJson = Utils::jsonDocumentFromFile(replicateFilePath).object();

    // Enumerate the media files and remove those not referenced.
    auto mediaFiles = QDir(m_mediaPath).entryInfoList(QDir::Files);
    for (auto it = mediaFiles.constBegin(); it != mediaFiles.constEnd(); it++)
    {
        if (map.contains(it->fileName().toLower()))
        {
            continue;
        }

        qDebug() << "GC media: " << it->fileName();
        QFile::remove(it->filePath());

        replicateJson.remove(it->fileName().toLower());
    }

    Utils::writeJsonToFile(replicateFilePath, QJsonDocument(replicateJson).toJson());
}

void App::garbageCollectStreamers()
{
    constexpr char SYSTEM_STREAMER[] = "system";

    auto interest = !m_database->getStreamers().isEmpty();

    if (!interest && m_locationLogger->hasInterest(SYSTEM_STREAMER))
    {
        m_locationLogger->loseInterest(SYSTEM_STREAMER);
    }
    else if (interest && !m_locationLogger->hasInterest(SYSTEM_STREAMER))
    {
        m_locationLogger->takeInterest(SYSTEM_STREAMER);
    }

    m_locationLogger->requestServiceSync();
}

QString App::deviceImei() const
{
#if defined(Q_OS_ANDROID)
    return QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("getImei", "()Ljava/lang/String;").toString();
#else
    return "";
#endif
}

bool App::wifiConnected() const
{
#if defined(Q_OS_ANDROID)
    return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("getWifiConnected", "()Z");
#else
    // TODO support for ios
    return true;
#endif
}

bool App::pingUrl(const QString& url) const
{
    return Utils::pingUrl(m_networkAccessManager, url);
}

bool App::networkAvailable() const
{
    return Utils::networkAvailable(m_networkAccessManager);
}

bool App::uploadAllowed()
{
    return m_settings->uploadRequiresWiFi() ? wifiConnected() : true;
}

bool App::updateAllowed()
{
    return uploadAllowed();
}

void App::copyToClipboard(const QString& text)
{
#if OS_DESKTOP
    auto topLevelObject = m_qmlEngine->rootObjects().value(0);
    auto window = qobject_cast<QQuickWindow *>(topLevelObject);
    if (window)
    {
        QGuiApplication::clipboard()->setText(text);
    }
#else
    Q_UNUSED(text)
#endif
}

void App::snapScreenshotToClipboard()
{
#if OS_DESKTOP
    auto topLevelObject = m_qmlEngine->rootObjects().value(0);
    auto window = qobject_cast<QQuickWindow *>(topLevelObject);
    if (window)
    {
    #ifdef SCREENSHOT_MODE
        static auto s_snapshotSizes = QList<QSize>{ QSize(1242, 2688), QSize(1242, 2208), QSize(2048, 2732) };
        static auto s_snapshotIndex = 0;
        static auto s_mainIndex = 0;

        auto path = m_tempPath + "/Snapshots";
        qFatalIf(!Utils::ensurePath(path), "Bad path");

        auto size = s_snapshotSizes[s_snapshotIndex];
        window->setMaximumSize(QSize(size.width() * 2, size.height() * 2));
        window->resize(size.width() / SCREENSHOT_SCALE, size.height() / SCREENSHOT_SCALE);

        QTimer::singleShot(1000, this, [size, path, this]()
        {
            auto topLevelObject = m_qmlEngine->rootObjects().value(0);
            auto window = qobject_cast<QQuickWindow *>(topLevelObject);
            auto image = window->grabWindow();
            auto image2 = image.scaled(size, Qt::IgnoreAspectRatio, Qt::FastTransformation);

            auto filename = QString("%1_%2_%3.png").arg(QString::number(s_mainIndex), QString::number(size.width()), QString::number(size.height()));
            image2.save(path + "/" + filename, "PNG");

            s_snapshotIndex++;
            if (s_snapshotIndex == s_snapshotSizes.count())
            {
                s_snapshotIndex = 0;
                s_mainIndex++;
                window->setMaximumSize(QSize(480, 640));
                window->resize(480, 640 / 2);
                return;
            }

            QTimer::singleShot(1000, this, [this]() { snapScreenshotToClipboard(); });
        });
    #else
        QGuiApplication::clipboard()->setImage(window->grabWindow());
    #endif
    }
#endif
}

void App::snapScreenshotToFile(const QString& filePathUrl)
{
    auto filePath = Utils::urlToLocalFile(filePathUrl);

    auto topLevelObject = m_qmlEngine->rootObjects().value(0);
    auto window = qobject_cast<QQuickWindow *>(topLevelObject);
    if (window)
    {
    #ifdef SCREENSHOT_MODE
        window->setMaximumSize(QSize(SCREENSHOT_WIDTH * 2, SCREENSHOT_HEIGHT * 2));
        window->resize(SCREENSHOT_WIDTH / SCREENSHOT_SCALE, SCREENSHOT_HEIGHT / SCREENSHOT_SCALE);
        QTimer::singleShot(100, [this, filePath]() {
            auto topLevelObject = m_qmlEngine->rootObjects().value(0);
            auto window = qobject_cast<QQuickWindow *>(topLevelObject);
            auto image = window->grabWindow();
            image.save(filePath, "PNG");
            window->setMaximumSize(QSize(480, 640));
            window->resize(480, 640 / 2);
        });
    #else
        auto image = window->grabWindow();
        image.save(filePath, "PNG");
    #endif
    }
}

void App::runLinkOnClipboard()
{
#if OS_DESKTOP
    auto text = QGuiApplication::clipboard()->text();

    auto prefixes = m_config["appLinkUrls"].toStringList();
    auto isAppLink = false;
    for (auto it = prefixes.constBegin(); it != prefixes.constEnd(); it++)
    {
        if (text.startsWith(*it, Qt::CaseInsensitive))
        {
            isAppLink = true;
            break;
        }
    }

    if (isAppLink || text.endsWith(".zip", Qt::CaseInsensitive))
    {
        setCommandLine(text);
    }
#endif
}

void App::runQRCode(const QString& tag)
{
    setCommandLine(tag);
}

QVariantMap App::runCommandLine(const QString& customCommandLine)
{
    auto commandLine = customCommandLine.isEmpty() ? m_commandLine : customCommandLine;
    auto applinkUrl = commandLine.contains("applink");
    qDebug() << "Command line: " << (applinkUrl ? "applink" : commandLine);

    // Success if nothing to do.
    if (commandLine.isEmpty())
    {
        return ApiResult::SuccessMap();
    }

    // Install a package.
    if (commandLine.endsWith(".zip", Qt::CaseInsensitive))
    {
        // Remove file scheme.
        auto filePathUrl = commandLine;
        auto tempFilePath = QString();

        // Download if needed.
        if (filePathUrl.startsWith("https://", Qt::CaseInsensitive))
        {
            auto tempFilePath = App::instance()->downloadFile(filePathUrl);
            if (tempFilePath.isEmpty())
            {
                return ApiResult::ErrorMap(tr("Download failed"));
            }

            filePathUrl = ::QUrl::fromLocalFile(tempFilePath).toString();
        }

        // Install package.
        auto result = installPackage(filePathUrl, true);

        // Cleanup source if in temp.
        if (!tempFilePath.isEmpty())
        {
            QFile::remove(tempFilePath);
        }

        result["launch"] = true;

        return result;
    }

    // Applinks.
    auto prefixes = m_config["appLinkUrls"].toStringList();

    // Sort by longest first since we will be removing the beginning part.
    std::sort(prefixes.begin(), prefixes.end(), [](const QString &s1, const QString &s2) -> bool
    {
        return s2.size() < s1.size();
    });

    auto isAppLink = false;
    for (auto it = prefixes.constBegin(); it != prefixes.constEnd(); it++)
    {
        if (commandLine.startsWith(*it, Qt::CaseInsensitive))
        {
            commandLine = QUrl::fromPercentEncoding(commandLine.toUtf8());
            commandLine.remove(0, it->length());
            isAppLink = true;
            break;
        }
    }

    if (!isAppLink)
    {
    #if defined(Q_OS_IOS)
        ::QDesktopServices::unsetUrlHandler("https");
        ::QDesktopServices::openUrl(QUrl(m_commandLine));
        ::QDesktopServices::setUrlHandler("https", this, "launchUrl");
    #endif
        return ApiResult::SuccessMap();
    }

    if (commandLine.isEmpty() || commandLine == "/")
    {
        if (desktopOS())
        {
            return ApiResult::ErrorMap(tr("Empty link"));
        }

        return ApiResult::SuccessMap();
    }

    // Sometimes browsers add an extra slash.
    if (commandLine.startsWith("/?"))
    {
        // Remove "/".
        commandLine.remove(0, 1);
    }

    if (commandLine.startsWith('?', Qt::CaseInsensitive))
    {
        // Remove "?".
        commandLine.remove(0, 1);

        // Allow "x=" to overcome WhatsApp breaking up the URL.
        if (commandLine.startsWith("x="))
        {
            commandLine.remove(0, 2);
        }

        auto params = m_appLink->decode(commandLine);

        // Bad command line.
        if (params.isEmpty())
        {
            return ApiResult::ErrorMap(tr("Bad link"));
        }

        auto launch = params.value("launch").toBool();

        // Web install.
        if (params.contains("webUpdateUrl"))
        {
            auto result = m_projectManager->webInstall(params["webUpdateUrl"].toString());
            result["launch"] = launch;

            return result;
        }

        // Web hook.
        if (params.contains("webhook"))
        {
            auto result = ApiResult::SuccessMap();
            result["webhook"] = true;
            result["params"] = params;

            return result;
        }

        // Set Goto.
        if (params.contains("x") && params.contains("y"))
        {
            auto x = params.value("x").toDouble();
            auto y = params.value("y").toDouble();
            auto t = params.value("t", tr("Target")).toString();
            auto i = params.value("i").toString();

            if (x < -180.0 || x > 180.0 || y < -90.0 || y > 90.0 || t.length() > 128)
            {
                return ApiResult::ErrorMap(tr("Bad goto target"));
            }

            auto iconFilePath = QString();
            if (!i.isEmpty())
            {
                auto url = i.startsWith("https://") ? i : "https://wildlifeicons.org/icons/" + i;
                iconFilePath = App::downloadFile(url);
                if (iconFilePath.isEmpty())
                {
                    return ApiResult::ErrorMap(tr("Icon download failed"));
                }

                if (!iconFilePath.endsWith(".svg", Qt::CaseInsensitive))
                {
                    return ApiResult::ErrorMap(tr("Icon must be svg"));
                }
            }

            m_gotoManager->setTarget(t, QVariantList { QVariant(QVariantList { x, y }) }, 0, iconFilePath);
            m_settings->set_mapMoreDataVisible(true);
            m_settings->set_mapMoreDataViewIndex(2);
            m_settings->set_mapPanMode(2);

            auto result = ApiResult::SuccessMap();
            result["setGoto"] = true;
            return result;
        }

        // Connect using the passed in parameters.
        auto connectorName = params.value("connector").toString();
        auto connector = m_connectors.contains(connectorName) ? createConnectorPtr(connectorName) : std::unique_ptr<Connector>();

        if (params.value("auth").toBool())
        {
            auto result = ApiResult::SuccessMap();
            result["auth"] = true;
            params["icon"] = connector ? connector->icon() : "";
            result["params"] = params;
            result["launch"] = launch;

            return result;
        }

        if (connector)
        {
            auto result = connector->bootstrap(params).toMap();
            result["launch"] = launch;
            return result;
        }
    }

    return ApiResult::ErrorMap(tr("Unknown command"));
}

QVariantMap App::installPackage(const QString& filePathUrl, bool showToasts)
{
    auto filePath = Utils::urlToLocalFile(filePathUrl);
    auto result = ApiResult();

    // Install offline map.
    if (m_offlineMapManager->canInstallPackage(filePath))
    {
        result = m_offlineMapManager->installPackage(filePath);
    }
    // Install project.
    else if (m_projectManager->canInstallPackage(filePath))
    {
        result = m_projectManager->installPackage(filePath);
    }
    // Not installable.
    else
    {
        result = ApiResult::Error(tr("Not an installable package"));
    }

    // Emit results.
    if (showToasts)
    {
        if (result.success)
        {
            emit showToast(tr("Install successful"));
        }
        else if (result.expected)
        {
            emit showToast(result.errorString);
        }
        else
        {
            emit showError(result.errorString);
        }
    }

    return result.toMap();
}

QString App::createQRCodeFile(const QString& data, int size) const
{
    auto result = sendFilePath() + "/QRCode.png";
    auto image = Utils::renderQRCode(data, size, size);
    image.save(result, "PNG");
    return result;
}

void App::clearDeviceCache()
{
    qFatalIf(m_cachePath.isEmpty(), "Not initialized yet");

    QDir(m_cachePath).removeRecursively();
    qFatalIf(!Utils::ensurePath(m_cachePath), "Failed to create cache");
    qFatalIf(!Utils::ensurePath(m_tempPath), "Failed to create temp path");

    Utils::writeJsonToFile(m_cachePath + "/cache.json" , "{}");
    m_settings->set_autoLaunchProjectUid("");
}

void App::clearComponentCache()
{
    m_qmlEngine->clearComponentCache();
}

QString App::getDirectionText(double directionDegrees) const
{
    if (std::isnan(directionDegrees))
    {
        return "?";
    }

    return QString("%1%2").arg(QString::number((int)directionDegrees), "°");
}

QString App::getDistanceText(double distanceMeters) const
{
    auto result = QString();

    if (std::isnan(distanceMeters))
    {
        return "?";
    }

    if (m_settings->metricUnits())
    {
        if (distanceMeters < 1000)
        {
            result = QString("%1 m").arg(m_locale.toString(distanceMeters, 'f', 0));
        }
        else
        {
            auto distanceKm = distanceMeters / 1000;
            result = QString("%1 km").arg(m_locale.toString(distanceKm, 'f', 2));
        }
    }
    else
    {
        auto distanceFt = distanceMeters * 3.28084;
        if (distanceFt < 1000)
        {
            result = QString("%1 ft").arg(m_locale.toString(distanceFt, 'f', 0));
        }
        else
        {
            auto distanceMi = distanceMeters / 1609.344;
            result += QString("%1 mi").arg(m_locale.toString(distanceMi, 'f', 3));
        }
    }

    return result;
}

QString App::getAltitudeText(double altitudeMeters) const
{
    auto result = QString();

    if (std::isnan(altitudeMeters))
    {
        return "?";
    }

    if (m_settings->metricUnits())
    {
        result = QString("%1 m").arg(m_locale.toString(altitudeMeters, 'f', 0));
    }
    else
    {
        auto altitudeFeet = altitudeMeters * 3.28084;
        result = QString("%1 ft").arg(m_locale.toString(altitudeFeet, 'f', 0));
    }

    return result;
}

QString App::getSpeedText(double speedMetersPerSecond) const
{
    auto metricUnits = m_settings->metricUnits();

    if (std::isnan(speedMetersPerSecond))
    {
        return "?";
    }

    return QString("%1 %2")
        .arg(m_locale.toString(qRound(speedMetersPerSecond * (metricUnits ? 3.6 : 2.23694))), metricUnits ? "kph" : "mph");
}

QString App::getAreaText(double areaMeters) const
{
    auto metricUnits = m_settings->metricUnits();

    if (std::isnan(areaMeters))
    {
        return "?";
    }

    if (metricUnits)
    {
        auto areaHa = areaMeters / 10000;

        if (areaMeters < 1000) // meters.
        {
            return QString("%1 %2%3").arg(m_locale.toString(areaMeters, 'f', 1), "m", QChar(0x00B2));
        }
        else if (areaHa < 1000) // hectares.
        {
            return QString("%1 %2").arg(m_locale.toString(areaHa, 'f', 1), "ha");
        }
        else // km2.
        {
            auto areaKm = areaMeters / 1000000;
            return QString("%1 km%2").arg(m_locale.toString(areaKm, 'f', 1), QChar(0x00B2));
        }
    }
    else
    {
        auto areaFt = areaMeters * 10.7639;
        auto areaAc = areaMeters / 4046.85642;

        if (areaFt < 43560) // square feet.
        {
            return QString("%1 %2%3").arg(m_locale.toString(areaFt, 'f', 1), "ft", QChar(0x00B2));
        }
        else if (areaAc < 640) // acres.
        {
            return QString("%1 %2").arg(m_locale.toString(areaAc, 'f', 1), "ac");
        }
        else // mi2
        {
            auto areaMi = areaMeters / 2.5900E+6;
            return QString("%1 mi%2").arg(m_locale.toString(areaMi, 'f', 1), QChar(0x00B2));
        }
    }
}

QString App::getTimeText(int timeSeconds) const
{
    auto arg1 = 0;
    auto arg2 = QString();

    if (timeSeconds < 60)
    {
        arg1 = timeSeconds;
        arg2 = arg1 == 1 ? tr("second") : tr("seconds");
    }
    else if (timeSeconds < 60 * 60 || timeSeconds % 60 != 0)
    {
        arg1 = timeSeconds / 60;
        arg2 = arg1 == 1 ? tr("minute") : tr("minutes");
    }
    else
    {
        arg1 = timeSeconds / (60 * 60);
        arg2 = arg1 == 1 ? tr("hour") : tr("hours");
    }

    return QString("%1 %2").arg(arg1).arg(arg2);
}

void App::setAlarm(const QString& alarmId, int seconds)
{
    m_alarms[alarmId] = QDateTime::currentSecsSinceEpoch() + seconds;
    processAlarms();
}

void App::killAlarm(const QString& alarmId)
{
    m_alarms.remove(alarmId);
    processAlarms();
}

void App::processAlarms()
{
    // Alarms are often set from within the previous alarmFired, so we have to ensure that processAlarms is not called
    static auto s_alarmMutex = false;
    if (s_alarmMutex)
    {
        return;
    }

    auto currentTime = QDateTime::currentSecsSinceEpoch();

    // Loop over alarms and fire them if they are expired.
    auto alarmIds = m_alarms.keys();
    for (auto it = alarmIds.constBegin(); it != alarmIds.constEnd(); it++)
    {
        auto alarmId = *it;
        if (m_alarms.value(alarmId) <= currentTime + 1)
        {
            m_alarms.remove(alarmId);
            s_alarmMutex = true;
            emit alarmFired(alarmId);
            s_alarmMutex = false;
        }
    }

    // Compute the next alarm time.
    qint64 nextAlarm = std::numeric_limits<qint64>::max();
    for (auto it = m_alarms.constKeyValueBegin(); it != m_alarms.constKeyValueEnd(); it++)
    {
        nextAlarm = std::min(nextAlarm, it->second - currentTime);
    }

    if (nextAlarm != std::numeric_limits<qint64>::max())
    {
    #if defined(Q_OS_ANDROID)
        QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("setAlarm", "(I)V", static_cast<jint>(nextAlarm));
    #else
        m_alarmTimer.start(nextAlarm * 1000);
    #endif
    }
    else
    {
    #if defined(Q_OS_ANDROID)
        QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("killAlarm", "()V");
    #else
        m_alarmTimer.stop();
    #endif
    }
}

double App::scaleByFontSize(double value) const
{
    return (value * settings()->fontSize()) / 100.0;
}

void App::share(const QString& url, const QString& text)
{
    m_platformUtils->share(url, text);
}

void App::sendFile(const QString& filePath, const QString& title)
{
    auto actualFilePath = Utils::decodeFilePath(filePath);
    qFatalIf(!QFile::exists(actualFilePath), "File does not exist: " + filePath + " " + title);

    auto mimeType = Utils::mimeDatabase()->mimeTypeForFile(actualFilePath).name();

    m_platformUtils->sendFile(actualFilePath, title, mimeType, 0);
}

QString App::renderSvgToPng(const QString& svgUrl, int width, int height) const
{
    QFileInfo fileInfo(svgUrl);

    auto targetFilePath = QString("%1/%2_%3_%4.png").arg(m_tempPath, fileInfo.baseName(), QString::number(width), QString::number(height));
    if (!QFile::exists(targetFilePath))
    {
        Utils::renderSvgToPngFile(svgUrl, targetFilePath, width, height);
    }

    return QUrl::fromLocalFile(targetFilePath).toString();
}

QVariantMap App::esriCreateLocationService(const QString& username, const QString& serviceName, const QString& serviceDescription, const QString& token) const
{
    QFile file1(":/Esri/CreateService.json");
    if (!file1.open(QFile::ReadOnly | QFile::Text))
    {
        return ApiResult::ErrorMap(tr("Failed to load service template"));
    }

    auto serviceDefinition = QString(file1.readAll());
    serviceDefinition.replace("{name}", Utils::replaceNonAlphanumeric(serviceName));
    serviceDefinition.replace("{description}", serviceDescription.isEmpty() ? "CyberTracker Location Tracking service" : serviceDescription);
    qFatalIf(QJsonDocument::fromJson(serviceDefinition.toLatin1()).object().isEmpty(), "Bad service JSON");

    QFile file2(":/Esri/CreateLayers.json");
    if (!file2.open(QFile::ReadOnly | QFile::Text))
    {
        return Utils::HttpResponse::Error("Failed to load layers template").toMap();
    }

    auto layersDefinition = QString(file2.readAll());
    qFatalIf(QJsonDocument::fromJson(layersDefinition.toLatin1()).object().isEmpty(), "Bad layers JSON");

    auto serviceUrl = QString();
    auto response = Utils::esriBuildService(m_networkAccessManager, username, serviceName, serviceDefinition, layersDefinition, token, &serviceUrl);
    if (!response.success)
    {
        return ApiResult::ErrorMap(response.errorString);
    }

    auto serviceState = QVariantMap();
    response = Utils::esriInitLocationTracking(m_networkAccessManager, serviceUrl, token, &serviceState);
    if (!response.success)
    {
        return ApiResult::ErrorMap(response.errorString);
    }

    auto result = ApiResult::SuccessMap();
    result.insert("serviceUrl", serviceUrl);
    result.insert("serviceState", serviceState);
    return result;
}

QString App::renderMapMarker(const QString& url, const QColor& color, int size, bool cache) const
{
    auto darkTheme = settings()->darkTheme();
    auto filePath = QString("%1/%2-%3-%4%5.png").arg(m_mapMarkerCachePath, QFileInfo(url).baseName(), QString::number(size), color.name(), darkTheme ? "d" : "");
    if (QFile::exists(filePath) && cache)
    {
        return filePath;
    }

    qFatalIf(!Utils::ensurePath(m_mapMarkerCachePath), "Failed to create map tile cache path");

    if (!Utils::renderMapMarker(url, filePath, color, darkTheme ? Qt::white : Qt::black, size, size))
    {
        qDebug() << "Failed to render svg: " << url;
        return "";
    }

    return filePath;
}

QString App::renderMapCallout(const QString& url, int size, bool cache) const
{
    auto darkTheme = settings()->darkTheme();

    auto filePath = QString("%1/%2-%3%4.png").arg(m_mapMarkerCachePath, QFileInfo(url).baseName()).arg(size).arg(darkTheme ? "d" : "");
    if (QFile::exists(filePath) && cache)
    {
        return filePath;
    }

    qFatalIf(!Utils::ensurePath(m_mapMarkerCachePath), "Failed to create map tile cache path");

    auto color = darkTheme ? Qt::black : Qt::white;
    auto outlineColor = darkTheme ? Qt::white : Qt::black;

    if (!Utils::renderMapCallout(url, filePath, color, outlineColor, size))
    {
        qDebug() << "Failed to render callout: " << url;
        return "";
    }

    return filePath;
}

QVariantMap App::createMapCalloutSymbol(const QString& filePath) const
{
    auto size = scaleByFontSize(32);

    return QVariantMap
    {
        { "symbolType", "PictureMarkerSymbol" },
        { "angle", 0.0 },
        { "type", "esriPMS" },
        { "url", renderMapCallout(filePath, size) },
        { "width", (int)size },
        { "height", (int)size },
        { "yoffset", size / 2 }
    };
}

QVariantMap App::createMapPointSymbol(const QString& url, const QColor& color) const
{
    auto size = scaleByFontSize(32);

    return QVariantMap
    {
        { "symbolType", "PictureMarkerSymbol" },
        { "angle", 0.0 },
        { "type", "esriPMS" },
        { "url", renderMapMarker(url, color, size) },
        { "width", (int)size },
        { "height", (int)size },
        { "yoffset", size / 2 }
    };
}

QVariantMap App::createMapLineSymbol(const QColor& color) const
{
    return QVariantMap
    {
        { "symbolType", "SimpleLineSymbol" },
        { "color", QVariantList { color.red(), color.green(), color.blue(), color.alpha() } },
        { "style", "esriSLSShortDot" },
        { "type", "esriSLS" },
        { "width", scaleByFontSize(1.75) }
    };
}

void App::showSystemAlert(const QString& contentTitle, const QString& contentText, const QString& bigText) const
{
#if defined(Q_OS_ANDROID)
    QJniObject jsContentTitle = QJniObject::fromString(contentTitle);
    QJniObject jsContentText = QJniObject::fromString(contentText);
    QJniObject jsBigText = QJniObject::fromString(bigText);
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("showAlert", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V", jsContentTitle.object<jstring>(), jsContentText.object<jstring>(), jsBigText.object<jstring>());
#else
    Q_UNUSED(contentTitle);
    Q_UNUSED(contentText);
    Q_UNUSED(bigText);
#endif
}

void App::hideSystemAlert() const
{
#if defined(Q_OS_ANDROID)
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("hideAlert", "()V");
#endif
}

QString App::translateText(const QString& text) const
{
    auto map = QMap<QString, QString> {
        { "tr:Decimal degrees", tr("Decimal degrees") },
    };

    if (map.contains(text))
    {
        return map[text];
    }

    return text;
}

QVariantList App::trackFileToPoints(const QString& filename) const
{
    auto filePath = getMediaFilePath(filename);
    if (!QFile::exists(filePath))
    {
        return QVariantList();
    }

    return TrackFile(filePath).getPoints();
}

QObject* App::parentStackView(const QObject* obj) const
{
    auto p = (QObject*)obj;
    while (p)
    {
        auto className = p->metaObject()->className();
        if (QString(className).startsWith("StackView"))
        {
            return p;
        }

        p = p->parent();
    }

    return nullptr;
}

void App::backToHomeScreen() const
{
#if defined(Q_OS_ANDROID)
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid())
    {
        // Sends the application to the background
        activity.callMethod<jboolean>("moveTaskToBack", "(Z)Z", true);
    }
#endif
}

void App::startOnlineTest()
{
    auto request = QNetworkRequest(QUrl("https://www.google.com/generate_204"));  // lightweight no-content URL
    request.setTransferTimeout(3000);

    auto reply = m_networkAccessManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [reply, this]()
    {
        auto status = reply->isFinished() ? reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() : 550;

        emit onlineTestComplete(reply->error() == QNetworkReply::NoError && status == 204);

        reply->deleteLater();
    });
}

} // namespace CyberTracker
