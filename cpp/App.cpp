#include <pch.h>
#include "App.h"
#include <jlcompress.h>
#include <geomag.h>
#include "UtilsShare.h"

namespace {
static App* s_instance;
static ShareUtils* s_shareUtils;
static QtMessageHandler s_lastMessageHandler;
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
}
#endif
#if defined(Q_OS_IOS)
#include <QDesktopServices>
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void logHandler(QtMsgType type, const QMessageLogContext &context, const QString& msg)
{
    s_lastMessageHandler(type, context, msg);

    emit App::instance()->consoleMessage(msg);

    QFile outFile(s_instance->logFilePath());
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);

    auto timestamp = '[' + (s_instance->timeManager() ? s_instance->timeManager()->currentDateTimeISO() : "??") + ']';

    QTextStream ts(&outFile);
    auto txt = QString();
    switch (type)
    {
    case QtInfoMsg:
        txt = QString("Info: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << timestamp << txt << "\t" << QString(msg) << Qt::endl;
        break;
    case QtDebugMsg:
        txt = QString("Debug: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << timestamp << txt << "\t" << QString(msg) << Qt::endl;
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << timestamp << txt << "\t" << QString(msg) << Qt::endl;
        break;
    case QtCriticalMsg:
        txt = QString("Critical: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << timestamp << txt << "\t" << QString(msg) << Qt::endl;
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1 %2 %3:").arg(context.file).arg(context.line).arg(context.function);
        ts << timestamp << txt << "\t" << QString(msg) << Qt::endl;

        if (s_instance)
        {
            s_instance->telemetry()->aiSendEvent("exception", QVariantMap {{ "file", context.file }, { "line", context.line }, { "function", context.function }, { "message", msg }});
            QEventLoop eventLoop;
            QTimer::singleShot(5000, s_instance, [&]()
            {
                eventLoop.quit();
            });
            eventLoop.exec();
        }

        abort();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// App

App::App(QObject*)
{
}

App::App(QGuiApplication* guiApplication, QQmlApplicationEngine* qmlEngine, const QVariantMap& config): QObject(guiApplication), m_timeManager(nullptr)
{
    qFatalIf(s_instance != nullptr, "App already initialized");
    s_instance = this;
    m_sessionId = QUuid::createUuid().toString(QUuid::Id128);

#if defined(QT_DEBUG)
    m_debugBuild = true;
#else
    m_debugBuild = false;
#endif

    m_buildString = QString(CT_VERSION);
    qFatalIf(m_buildString.isEmpty(), "No version string");

    m_mobileOS = OS_MOBILE;
    m_desktopOS = OS_DESKTOP;

    m_fixedFontFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();

    m_guiApplication = guiApplication;
    m_qmlEngine = qmlEngine;

    setBatteryState(100, false);

    m_config = config;
    m_dpi = (int)(qApp->screens()[0]->logicalDotsPerInch());

    // Load languages.
    // Generated using "translator -l languages.json"
    m_languages = Utils::variantListFromJsonFile(":/languages.json");

    // Set up the root path.
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    m_rootPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation);
    m_cachePath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::CacheLocation);
#else
    m_rootPath = QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DesktopLocation) + "/CTProjects";
    m_cachePath = m_rootPath + "/Cache";
#endif
    qFatalIf(!Utils::ensurePath(m_rootPath), "Failed to create root");
    qFatalIf(!Utils::ensurePath(m_cachePath), "Failed to create cache");

    // Bug reports.
    m_logFilePath = m_rootPath + "/Log.txt";

    // Cleanup logs to keep them moderately sized.
    if (QFile::exists(m_logFilePath) && QFile(m_logFilePath).size() > 1024 * 1024 * 8)
    {
        auto currLogFilePath = m_logFilePath;
        auto prevLogFilePath = m_rootPath + "/Log.old.txt";
        QFile::remove(prevLogFilePath);
        QFile::rename(currLogFilePath, prevLogFilePath);
    }

    // Install a custom message handler.
    s_lastMessageHandler = qInstallMessageHandler(logHandler);

    qDebug() << "Version: " << m_buildString;

    // INI path.
    m_iniPath = QDir::cleanPath(m_rootPath + "/Settings.ini");

    // Temp path.
    m_tempPath = QDir::cleanPath(m_cachePath + "/Temp");
    qFatalIf(!Utils::ensurePath(m_tempPath), "Failed to create temp path");
    m_tempUrl = QUrl::fromLocalFile(m_tempPath);

    // Work path.
    m_workPath = QDir::cleanPath(m_rootPath + "/Work");
    qFatalIf(!Utils::ensurePath(m_workPath), "Failed to create work path");

    // Map tile cache: created by MBTilesReader.
    m_mapTileCachePath = QDir::cleanPath(m_cachePath + "/MapTiles");
    m_mbTilesReader = new MBTilesReader(this);
    m_mbTilesReader->set_cachePath(m_mapTileCachePath);

    // Map markers cache: created when rendered.
    m_mapMarkerCachePath = QDir::cleanPath(m_cachePath + "/MapMarkers");

    // Goto path.
    m_gotoPath = QDir::cleanPath(m_rootPath + "/Goto");
    qFatalIf(!Utils::ensurePath(m_gotoPath), "Failed to create goto path");

    // Export path.
    m_exportPath = QDir::cleanPath(m_rootPath + "/ExportData");
    qFatalIf(!Utils::ensurePath(m_exportPath), "Failed to create exportData path");
    m_exportUrl = QUrl::fromLocalFile(m_exportPath);

    // Backup path.
    m_backupPath = QDir::cleanPath(m_rootPath + "/Backup");
    Utils::enforceFolderQuota(m_backupPath, 64);
    qFatalIf(!Utils::ensurePath(m_backupPath), "Failed to create backup path");

    // Bug report path.
    m_bugReportPath = QDir::cleanPath(m_rootPath + "/BugReports");
    qFatalIf(!Utils::ensurePath(m_bugReportPath), "Failed to create BugReports path");

    // Media path.
    m_mediaPath = QDir::cleanPath(m_rootPath + "/Media");
    qFatalIf(!Utils::ensurePath(m_mediaPath), "Failed to create Media path");

    // Share path.
    m_sendFilePath = QDir::cleanPath(m_rootPath + "/SendFile");
    qFatalIf(!Utils::ensurePath(m_sendFilePath), "Failed to create Share path");

    // NetworkAccessManager.
    m_networkAccessManager = m_qmlEngine->networkAccessManager();
    m_networkAccessManager->setTransferTimeout(60000); // 60s timeout.

    // Settings.
    m_settings = new Settings(m_rootPath + "/Settings.json", m_config["title"].toString(), this);

    // Backup the database if the build changes.
    if (m_settings->buildString() != m_buildString)
    {
        m_settings->set_buildString(m_buildString);
        backupDatabase("New build");
    }

    // Settings listeners.
    connect(m_settings, &Settings::languageCodeChanged, this, &App::languageCodeChanged);
    connect(m_settings, &Settings::simulateGPSChanged, this, &App::simulateGPSChanged);
    connect(m_settings, &Settings::kioskModeProjectUidChanged, this, &App::kioskModeProjectUidChanged);
    connect(m_settings, &Settings::locationAccuracyMetersChanged, this, &App::locationAccuracyChanged);
    locationAccuracyChanged();

    // TimeManager.
    m_timeManager = new TimeManager(this);

    // Database.
    m_database = new Database(QDir::cleanPath(m_rootPath + "/Projects.db"), this);

    // AppLink.
    m_appLink = new AppLink(this);

    // ProjectManager.
    m_projectManager = new ProjectManager(QDir::cleanPath(m_rootPath + "/Projects"), this);

    // OfflineMapMananager.
    m_offlineMapManager = new OfflineMapManager(QDir::cleanPath(m_rootPath + "/OfflineMaps"), this);

    // TaskManager.
    m_taskManager = new TaskManager(this);

    // GotoManager.
    m_gotoManager = new GotoManager(this);

    // Compass.
    m_compass = new Compass(this);

    // SatelliteManager.
    m_satelliteManager = new SatelliteManager(this);

    // DeviceId: cleanup from the time I made it a string guid.
    if (m_settings->deviceId().isEmpty())
    {
        auto deviceId = QString();
        #if defined(Q_OS_ANDROID)
            deviceId = QtAndroid::androidActivity().callObjectMethod("getDeviceId", "()Ljava/lang/String;").toString();
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
    m_telemetry = new Telemetry(this);
    m_telemetry->set_gaSecret(m_config["gaSecret"].toString());
    m_telemetry->set_gaMeasurementId(m_config["gaMeasurementId"].toString());
    m_telemetry->set_aiKey(m_config["aiKey"].toString());

    // Startup.
    languageCodeChanged();
    simulateGPSChanged();
    kioskModeProjectUidChanged();
    m_projectManager->startUpdateScanner();

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
            QAndroidJniObject jniTempPath = QAndroidJniObject::fromString(m_tempPath);
            QtAndroid::androidActivity().callMethod<void>("qtReady", "(Ljava/lang/String;)V", jniTempPath.object<jstring>());
        }
        #endif
    });

#if defined(Q_OS_ANDROID)
    m_commandLine = QtAndroid::androidActivity().callObjectMethod("getLaunchUri", "()Ljava/lang/String;").toString();
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

    // Update language settings.
    languageCodeChanged();
}

App::~App()
{
    garbageCollectQmlCache();
    qInstallMessageHandler(nullptr);
    s_instance = nullptr;
}

App* App::instance()
{
    return s_instance;
}

void App::setCommandLine(const QString& commandLine)
{
    m_commandLine = commandLine;
    emit commandLineChanged();
}

void App::launchUrl(const QUrl& url)
{
    setCommandLine(url.toString());
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

Provider* App::createProvider(const QString& name, QObject* parentForm)
{
    qFatalIf(qobject_cast<Form*>(parentForm) == nullptr, "Parent should be a Form");

    if (name.isEmpty())
    {
        qDebug() << "Error - Provider not specified";
        return nullptr;
    }

    qFatalIf(!m_providers.contains(name), "Provider not found: " + name);
    return m_providers[name](parentForm);
}

std::unique_ptr<Provider> App::createProviderPtr(const QString& name, QObject* parentForm)
{
    return std::unique_ptr<Provider>(createProvider(name, parentForm));
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

void App::removeSightingsByFlag(const QString& projectUid, const QString& stateSpace, uint matchFlag)
{
    // Check if any data contains the flag.
    if (!m_database->hasSightings(projectUid, stateSpace, matchFlag))
    {
        return;
    }

    // Bulk delete locations.
    m_database->deleteSightings(projectUid, stateSpace, Sighting::DB_LOCATION_FLAG | matchFlag);

    // Bulk delete sightings.
    m_database->deleteSightings(projectUid, stateSpace, Sighting::DB_SIGHTING_FLAG | matchFlag);

    // Clean up stray media.
    garbageCollectMedia();

    // Send notification.
    emit sightingsChanged(projectUid, stateSpace);
}

QObject* App::acquireQml(const QString& filePath, QString* keyOut)
{
    QFileInfo fileInfo(filePath);
    auto key = filePath + '_' + QString::number(fileInfo.lastModified().toMSecsSinceEpoch());

    if (!m_qmlCache.contains(key))
    {
        clearComponentCache();
        m_qmlCache.insert(key, std::make_pair<QObject*, int>(Utils::loadQmlFromFile(m_qmlEngine, filePath), 0));
    }

    m_qmlCache[key].second++;
    *keyOut = key;

    return m_qmlCache[key].first;
}

void App::releaseQml(const QString& key)
{
    auto value = m_qmlCache[key];
    qFatalIf(value.second == 0, "Bad ref count on qml file cache");

    m_qmlCache[key].second--;

    if (m_qmlCache.count() > 8)
    {
        garbageCollectQmlCache();
    }
}

void App::garbageCollectQmlCache()
{
    auto complete = false;
    while (!complete)
    {
        complete = true;
        auto cacheKeys = m_qmlCache.keys();
        for (auto it = cacheKeys.constBegin(); it != cacheKeys.constEnd(); it++)
        {
            auto value = m_qmlCache.value(*it);
            if (value.second == 0)
            {
                complete = false;
                delete value.first;
                m_qmlCache.remove(*it);
                break;
            }
        }
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

QString App::getLocationText(double x, double y, QVariant z)
{
    auto result = QString();
    auto coordinate = QGeoCoordinate(y, x);

    switch (m_settings->coordinateFormat())
    {
    case 0:
        result = coordinate.toString(QGeoCoordinate::DegreesWithHemisphere);
        break;

    case 1:
        result = coordinate.toString(QGeoCoordinate::DegreesMinutesSecondsWithHemisphere);
        break;

    case 2:
        result = coordinate.toString(QGeoCoordinate::DegreesMinutesWithHemisphere);
        break;

    case 3:
        {
            auto ux = 0.0;
            auto uy = 0.0;
            auto zone = 0;
            auto north = false;

            Utils::latLonToUTMXY(y, x, &ux, &uy, &zone, &north);

            // Round to the nearest integer and render.
            ux = qFloor(qFabs(ux) + 0.5);
            uy = qFloor(qFabs(uy) + 0.5);
            result = QString("%1%2 - %3E %4").arg(zone).arg(north ? 'N' : 'S').arg(ux, 0, 'f', 0).arg(uy, 0, 'f', 0);
        }
        break;

    default:
        result = coordinate.toString(QGeoCoordinate::DegreesWithHemisphere);
    }

    if (z.isValid() && !std::isnan(z.toDouble()))
    {
        result = result + ", " + getDistanceText(z.toDouble());
    }

    return result;
}

void App::positionUpdated(const QGeoPositionInfo& update)
{
    auto location = new Location(update, this);

    // Compute magnetic declination.
    float magneticDeclination = 0.0;
    getMagneticDeclination(location->y(), location->x(), location->z(), &magneticDeclination);
    m_settings->set_magneticDeclination(magneticDeclination);

    // Compute UTM zone.
    auto ux = 0.0;
    auto uy = 0.0;
    auto zone = 0;
    auto north = false;
    Utils::latLonToUTMXY(location->y(), location->x(), &ux, &uy, &zone, &north);
    m_settings->set_utmZone(zone);
    m_settings->set_utmHemi(north ? "N" : "S");

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
            QtAndroid::androidActivity().callMethod<void>("setKioskMode", "(Z)V", static_cast<jboolean>(kioskMode));
        #endif
    }

    update_kioskMode(kioskMode);
}

void App::locationAccuracyChanged()
{
    update_locationAccuracyText(QString("%1 > %2").arg(tr("Accuracy"), getDistanceText(m_settings->locationAccuracyMeters())));
}

void App::requestDisableBatterySaver()
{
    if (m_requestedDisableBatterySaver)
    {
        return;
    }

#if defined(Q_OS_ANDROID)
    QtAndroid::androidActivity().callMethod<void>("requestDisableBatterySaver", "()V");
#endif

    m_requestedDisableBatterySaver = true;
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
    QFile::copy(m_logFilePath, tempPath + "/Log_" + m_buildString + ".txt");
    QFile::copy(m_rootPath + "/Log.old.txt", tempPath + "/Log.old.txt");
    Utils::copyPath(m_exportPath, tempPath + "/ExportData");
    Utils::copyPath(m_rootPath + "/Projects", tempPath + "/Projects", QStringList { "map.mbtiles" }); // Exclude map.mbtiles.
    Utils::copyPath(m_backupPath, tempPath + "/Backup");
    Utils::copyPath(m_workPath, tempPath + "/Work");

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
            data += Utils::encodeTimestamp(QDateTime::currentDateTime()).toLatin1();
            data += "\r\nBuild " + m_buildString + "\r\n";
            file.write(data.toLatin1());
            file.write(description.toLatin1());
        }
    }

    auto result = QString();

    // Zip the file.
    if (JlCompress::compressDir(reportPath, tempPath, true))
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
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

bool App::requestPermission(const QString& permission, const QString& rationale)
{
    auto result = true;

#if defined(Q_OS_ANDROID)
    if (QtAndroid::androidSdkVersion() < 23)
    {
        return true;
    }

    auto androidPermission = "android.permission." + permission;
    QtAndroid::PermissionResult request = QtAndroid::checkPermission(androidPermission);
    if (request == QtAndroid::PermissionResult::Denied)
    {
        QtAndroid::requestPermissionsSync(QStringList { androidPermission });
        request = QtAndroid::checkPermission(androidPermission);

        if (request == QtAndroid::PermissionResult::Denied)
        {
            qDebug() << "Permission denied: " << permission;
            emit showMessageBox(tr("Permission request"), rationale);
            result = false;
        }
   }
#else
    Q_UNUSED(permission)
    Q_UNUSED(rationale)
#endif

   return result;
}

bool App::requestPermissions(const QString& projectUid)
{
    auto result = true;

#if defined(Q_OS_ANDROID)
    auto project = m_projectManager->loadPtr(projectUid);

    // Check if we need to disable battery optimizations.
    if (project->androidDisableBatterySaver())
    {
        requestDisableBatterySaver();
    }

    auto permissions = project->androidPermissions();
    permissions.removeOne("CAMERA");
    permissions.removeOne("RECORD_AUDIO");

    // Build a list of permissions that have not been granted.
    auto androidPermissions = QStringList();
    for (auto permission: permissions)
    {
        auto androidPermission = "android.permission." + permission;
        QtAndroid::PermissionResult request = QtAndroid::checkPermission(androidPermission);
        if (request == QtAndroid::PermissionResult::Denied)
        {
            androidPermissions.append(androidPermission);
        }
    }

    // If some have not been granted yet, request them now.
    if (!androidPermissions.isEmpty())
    {
        QtAndroid::requestPermissionsSync(androidPermissions);

        // Ensure all have made it through.
        for (auto androidPermission: androidPermissions)
        {
            QtAndroid::PermissionResult request = QtAndroid::checkPermission(androidPermission);

            if (request == QtAndroid::PermissionResult::Denied)
            {
                qDebug() << "Permission denied: " << androidPermission;
                emit showMessageBox(tr("Permission request"), QString(tr("This %1 requires access to some features of the device. Please allow these requests in order to proceed.")).arg(alias_project()));
                result = false;
                break;
            }
        }
    }

    if (result && project->androidBackgroundLocation())
    {
        result = requestPermissionBackgroundLocation();
    }
#else
    Q_UNUSED(projectUid)
#endif

   return result;
}

bool App::hasPermission(const QString& permission) const
{
#if defined(Q_OS_ANDROID)
   auto androidPermission = "android.permission." + permission;
   QtAndroid::PermissionResult request = QtAndroid::checkPermission(androidPermission);
   if (request == QtAndroid::PermissionResult::Denied)
   {
        return false;
   }
#else
   Q_UNUSED(permission)
#endif

   return true;
}

bool App::requestPermissionCamera()
{
   return requestPermission("CAMERA", tr("Using the camera requires permission. Please allow the request in order to proceed."));
}

bool App::requestPermissionRecordAudio()
{
    return requestPermission("RECORD_AUDIO", tr("Recording audio requires permission. Please allow the request in order to proceed."));
}

bool App::requestPermissionLocation()
{
#if defined(Q_OS_ANDROID)
    auto message = tr("Access to GPS is required for this feature to work. Please allow the request in order to proceed.");
    return requestPermission("ACCESS_FINE_LOCATION", message) && requestPermission("ACCESS_COARSE_LOCATION", message);
#else
    return true;
#endif
}

bool App::requestPermissionBackgroundLocation()
{
#if defined(Q_OS_ANDROID)
    if (QtAndroid::androidSdkVersion() >= 29)
    {
        return requestPermission("ACCESS_BACKGROUND_LOCATION", tr("Background access to GPS is required. Please allow the request in order to proceed."));
    }
#endif

    return true;
}

bool App::promptForLocation(const QString& projectUid) const
{
#if defined(Q_OS_ANDROID)
    if (hasPermission("ACCESS_COARSE_LOCATION") && hasPermission("ACCESS_FINE_LOCATION"))
    {
        return false;
    }

    if (projectUid.isEmpty())
    {
        return true;
    }

    auto project = m_projectManager->loadPtr(projectUid);
    auto permissions = project->androidPermissions();
    if (permissions.contains("ACCESS_COARSE_LOCATION") || permissions.contains("ACCESS_FINE_LOCATION") || project->androidBackgroundLocation())
    {
        return true;
    }
#else
    Q_UNUSED(projectUid)
#endif

    return false;
}

bool App::promptForBackgroundLocation(const QString& projectUid) const
{
#if defined(Q_OS_ANDROID)
    if (QtAndroid::androidSdkVersion() < 29)
    {
        return false;
    }

    if (hasPermission("ACCESS_BACKGROUND_LOCATION"))
    {
        return false;
    }

    if (projectUid.isEmpty())
    {
        return true;
    }

    auto project = m_projectManager->loadPtr(projectUid);
    if (project->androidBackgroundLocation())
    {
        return true;
    }
#else
    Q_UNUSED(projectUid)
#endif

    return false;
}

bool App::promptForBlackviewMessage(const QString& projectUid) const
{
#if defined(Q_OS_ANDROID)
    if (QtAndroid::androidSdkVersion() < 31)
    {
        return false;
    }

    if (m_settings->blackviewMessageShown())
    {
        return false;
    }

    auto manufacturer = QtAndroid::androidActivity().callObjectMethod("getManufacturer", "()Ljava/lang/String;").toString();
    if (manufacturer != "Blackview")
    {
        return false;
    }

    if (projectUid.isEmpty())
    {
        return true;
    }

    auto project = m_projectManager->loadPtr(projectUid);
    if (project->androidBackgroundLocation())
    {
        return true;
    }

    return false;

#else
    Q_UNUSED(projectUid)
    return false;
#endif
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

        success = reply->error() == QNetworkReply::NoError;
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

QString App::moveToMedia(const QString& filePathUrl)
{
    auto filePath = Utils::urlToLocalFile(filePathUrl);

    QFileInfo fileInfo(filePath);
    if (fileInfo.exists())
    {
        auto mediaFilename = QUuid::createUuid().toString(QUuid::Id128) + "_" + fileInfo.fileName();
        if (QFile::rename(filePath, m_mediaPath + "/" + mediaFilename))
        {
            return mediaFilename;
        }
        else
        {
            qDebug() << "Failed to move " << filePath << " 1";
        }
    }
    else
    {
        qDebug() << "Failed to move " << filePath << " 2";
    }

    return "";
}

QString App::copyToMedia(const QString& filePathUrl)
{
    auto filePath = Utils::urlToLocalFile(filePathUrl);

    QFileInfo fileInfo(filePath);
    if (fileInfo.exists())
    {
        auto mediaFilename = QUuid::createUuid().toString(QUuid::Id128) + "." + Utils::mimeDatabase()->mimeTypeForFile(filePath).preferredSuffix();
        if (QFile::copy(filePath, m_mediaPath + "/" + mediaFilename))
        {
            return mediaFilename;
        }
        else
        {
            qDebug() << "Failed to copy " << filePath << " 1";
        }
    }
    else
    {
        qDebug() << "Failed to copy " << filePath << " 2";
    }

    return "";
}

bool App::removeMedia(const QString& filename) const
{
    if (filename.isEmpty())
    {
        return true;
    }

    auto result = QFile::remove(m_mediaPath + "/" + filename);

    if (!result)
    {
        qDebug() << "Failed to remove attachment: " << filename;
    }

    return result;
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

QUrl App::getMediaFileUrl(const QString& filename) const
{
    if (filename.isEmpty())
    {
        return QUrl();
    }

    return QUrl::fromLocalFile(getMediaFilePath(filename));
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
    }
}

QString App::deviceImei() const
{
#if defined(Q_OS_ANDROID)
    return QtAndroid::androidActivity().callObjectMethod("getImei", "()Ljava/lang/String;").toString();
#else
    return "";
#endif
}

bool App::wifiConnected() const
{
#if defined(Q_OS_ANDROID)
    return QtAndroid::androidActivity().callMethod<jboolean>("getWifiConnected", "()Z");
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
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    auto topLevelObject = m_qmlEngine->rootObjects().value(0);
    auto window = qobject_cast<QQuickWindow *>(topLevelObject);
    if (window)
    {
        QApplication::clipboard()->setText(text);
    }
#else
    Q_UNUSED(text)
#endif
}

void App::snapScreenshotToClipboard()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
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
        QApplication::clipboard()->setImage(window->grabWindow());
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
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    auto text = QApplication::clipboard()->text();

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

    qDebug() << "Command line: " << commandLine;

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

            filePathUrl = QUrl::fromLocalFile(tempFilePath).toString();
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
    std::sort(prefixes.begin(), prefixes.end(), [](const QString &s1, const QString &s2)
    {
        return s2.size() < s1.size();
    });

    auto isAppLink = false;
    for (auto it = prefixes.constBegin(); it != prefixes.constEnd(); it++)
    {
        if (commandLine.startsWith(*it, Qt::CaseInsensitive))
        {
            commandLine.remove(0, it->length());
            isAppLink = true;
            break;
        }
    }

    if (!isAppLink)
    {
    #if defined(Q_OS_IOS)
        QDesktopServices::unsetUrlHandler("https");
        QDesktopServices::openUrl(QUrl(m_commandLine));
        QDesktopServices::setUrlHandler("https", this, "launchUrl");
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

//    // For Google Auth.
//    if (commandLine.startsWith("/?auth="))
//    {
//        auto code = commandLine.remove(0, 7).trimmed();
//        if (code.isEmpty())
//        {
//            return ApiResult::ErrorMap(tr("Missing code"));
//        }

//        auto accessToken = QString();
//        auto refreshToken = QString();

//        auto response = Utils::googleAcquireOAuthTokenFromCode(m_networkAccessManager, code, &accessToken, &refreshToken);
//        if (!response.success)
//        {
//            return response.toApiResult();
//        }

//        m_settings->set_googleAccessToken(accessToken);
//        m_settings->set_googleRefreshToken(refreshToken);

//        auto result = ApiResult::SuccessMap();
//        result["googleConnect"] = true;
//        return result;
//    }

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

void App::clearDeviceCache()
{
    qFatalIf(m_cachePath.isEmpty(), "Not initialized yet");

    QDir(m_cachePath).removeRecursively();
    qFatalIf(!Utils::ensurePath(m_cachePath), "Failed to create cache");
    qFatalIf(!Utils::ensurePath(m_tempPath), "Failed to create temp path");

    Utils::writeJsonToFile(m_cachePath + "/cache.json" , "{}");
    m_settings->set_autoLaunchProjectUid("");
    m_settings->set_blackviewMessageShown(false);
}

void App::clearComponentCache()
{
    m_qmlEngine->clearComponentCache();
}

QString App::lookupLanguageCode(const QString& languageName)
{
    // Check if we need to build the lookup table.
    if (m_languageLookup.isEmpty())
    {
        for (auto it = m_languages.constBegin(); it != m_languages.constEnd(); it++)
        {
            auto map = it->toMap();
            auto code = map["code"].toString();

            m_languageLookup.insert(code, code);

            auto name = map["name"].toString().toLower();
            m_languageLookup.insert(name, code);

            auto nativeName = map["nativeName"].toString().toLower();
            m_languageLookup.insert(nativeName, code);
        }
    }

    return m_languageLookup.value(languageName.toLower());
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
        QtAndroid::androidActivity().callMethod<void>("setAlarm", "(I)V", static_cast<jint>(nextAlarm));
    #else
        m_alarmTimer.start(nextAlarm * 1000);
    #endif
    }
    else
    {
    #if defined(Q_OS_ANDROID)
        QtAndroid::androidActivity().callMethod<void>("killAlarm", "()V");
    #else
        m_alarmTimer.stop();
    #endif
    }
}

double App::scaleByFontSize(double value) const
{
    return (value * settings()->fontSize()) / 100.0;
}

void App::share(const QUrl& url, const QString& text)
{
    if (!s_shareUtils)
    {
        s_shareUtils = new ShareUtils(this);
    }

    s_shareUtils->share(url, text);
}

void App::sendFile(const QString& filePath, const QString& title)
{
    auto actualFilePath = Utils::decodeFilePath(filePath);
    qFatalIf(!QFile::exists(actualFilePath), "File does not exist");

    auto mimeType = Utils::mimeDatabase()->mimeTypeForFile(actualFilePath).name();

    if (!s_shareUtils)
    {
        s_shareUtils = new ShareUtils(this);
    }

    s_shareUtils->sendFile(actualFilePath, title, mimeType, 0);
}

void App::registerExportFile(const QString& filePath, const QVariantMap& data)
{
    auto packedFilePath = Utils::encodeFilePath(filePath);
    m_database->addExport(packedFilePath, data);
    Utils::mediaScan(filePath);

    emit exportFilesChanged();
}

void App::removeExportFile(const QString& filePath)
{
    auto data = QVariantMap();
    m_database->loadExport(filePath, &data);

    auto actualFilePath = Utils::decodeFilePath(filePath);
    if (data.isEmpty() || !QFile::exists(actualFilePath))
    {
        emit showError(tr("Export missing"));
        return;
    }

    m_database->removeExport(filePath);
    QFile::rename(actualFilePath, m_backupPath + "/" + QFileInfo(actualFilePath).fileName());
    Utils::enforceFolderQuota(m_backupPath, 64);

    emit showToast(QString(tr("%1 deleted").arg(data["name"].toString())));

    emit exportFilesChanged();
}

QVariantList App::buildExportFilesModel(const QString& projectUid, const QString& filter)
{
    auto result = QVariantList();

    auto exportFiles = QStringList();
    m_database->getExports(&exportFiles);

    for (auto it = exportFiles.constBegin(); it != exportFiles.constEnd(); it++)
    {
        auto filePath = *it;

        // Filter if not interested.
        auto fileInfo = QVariantMap();
        m_database->loadExport(filePath, &fileInfo);
        if (!projectUid.isEmpty() && projectUid != fileInfo.value("projectUid").toString())
        {
            continue;
        }

        if (!filter.isEmpty() && filter != fileInfo.value("filter").toString())
        {
            continue;
        }

        // Cleanup if file missing.
        auto actualFilePath = Utils::decodeFilePath(filePath);
        QFile file(actualFilePath);
        if (!file.exists())
        {
            m_database->removeExport(filePath);
            continue;
        }

        // Append path and size.
        fileInfo["path"] = filePath;
        auto size = file.size();
        auto sizeUnits = QString("b");
        if (size > 1024)
        {
            size /= 1024;
            sizeUnits = "kb";
        }

        if (size > 1024)
        {
            size /= 1024;
            sizeUnits = "mb";
        }

        auto sizeText = QString("%1 %2").arg(QString::number(size), sizeUnits);
        fileInfo["size"] = sizeText;

        auto startTime = Utils::decodeTimestamp(fileInfo["startTime"].toString());
        auto stopTime = Utils::decodeTimestamp(fileInfo["stopTime"].toString());

        fileInfo["model"] = QVariantList
        {
            QVariantMap {{ "name", alias_Project() }, { "value", fileInfo["projectTitle"] } },
            QVariantMap {{ "name", tr("Start date") }, { "value", formatDate(startTime.date()) } },
            QVariantMap {{ "name", tr("Start time") }, { "value", formatTime(startTime.time()) } },
            QVariantMap {{ "name", tr("Stop date") }, { "value", formatDate(stopTime.date()) } },
            QVariantMap {{ "name", tr("Stop time") }, { "value", formatTime(stopTime.time()) } },
            QVariantMap {{ "name", tr("Sightings") }, { "value", fileInfo["sightingCount"] } },
            QVariantMap {{ "name", tr("Locations") }, { "value", fileInfo["locationCount"] } },
        };

        // Return this.
        result.append(fileInfo);
    }

    return result;
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
    serviceDefinition.replace("{name}", QString(serviceName).replace(' ', '_'));
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
    auto response = Utils::esriBuildService(m_networkAccessManager, username, serviceDefinition, layersDefinition, token, &serviceUrl);
    if (!response.success)
    {
        return ApiResult::ErrorMap(response.errorString);
    }

    auto result = ApiResult::SuccessMap();
    result.insert("serviceUrl", serviceUrl);
    return result;
}

QString App::renderMapMarker(const QString& url, const QColor& color, int size) const
{
    auto darkTheme = settings()->darkTheme();
    auto filePath = QString("%1/%2-%3-%4%5.png").arg(m_mapMarkerCachePath, QFileInfo(url).baseName(), QString::number(size), color.name(), darkTheme ? "d" : "");
    if (QFile::exists(filePath))
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

QString App::renderMapCallout(const QString& url, int size) const
{
    auto darkTheme = settings()->darkTheme();

    auto filePath = QString("%1/%2-%3.png").arg(m_mapMarkerCachePath, QFileInfo(url).baseName()).arg(size).arg(darkTheme ? "d" : "");
    if (QFile::exists(filePath))
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
