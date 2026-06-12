#pragma once
#include "pch.h"
#include "Settings.h"
#include "Telemetry.h"
#include "AppLink.h"
#include "Project.h"
#include "HasUpdate.h"
#include "Database.h"
#include "UploadQueue.h"
#include "TranscribeQueue.h"
#include "Connector.h"
#include "Form.h"
#include "ProviderInterface.h"
#include "Renderer.h"
#include "TimeManager.h"
#include "Location.h"
#include "Goto.h"
#include "Compass.h"
#include "Satellite.h"
#include "OfflineMap.h"
#include "Vault.h"
#include "UtilsPlatform.h"
#include "Audit.h"
#include "BackupFile.h"
#include "ExportFile.h"
#include "Place.h"

namespace CyberTracker {

//#define SCREENSHOT_MODE
#if defined(SCREENSHOT_MODE)
constexpr int SCREENSHOT_WIDTH = 2048;
constexpr int SCREENSHOT_HEIGHT = 2732;
constexpr int SCREENSHOT_SCALE = 4;
#endif

typedef Connector* (*ConnectorFactory)(QObject* parent);
typedef Provider* (*ProviderFactory)(Form* form);
typedef Renderer* (*RendererFactory)(QObject* parent);

class App: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (bool, shutdown)
    QML_READONLY_AUTO_PROPERTY (QString, commandLine)

    QML_READONLY_AUTO_PROPERTY (QString, buildString)
    QML_READONLY_AUTO_PROPERTY (int, dpi)
    QML_READONLY_AUTO_PROPERTY (bool, desktopOS)
    QML_READONLY_AUTO_PROPERTY (bool, debugBuild)
    QML_READONLY_AUTO_PROPERTY (QString, fixedFontFamily)

    QML_READONLY_AUTO_PROPERTY (QVariantList, languages)
    QML_READONLY_AUTO_PROPERTY (QString, languageName)

    QML_READONLY_AUTO_PROPERTY (QVariantMap, config)

    QML_READONLY_AUTO_PROPERTY (QGuiApplication*, guiApplication)
    QML_READONLY_AUTO_PROPERTY (QQmlApplicationEngine*, qmlEngine)
    QML_READONLY_AUTO_PROPERTY (PlatformUtils*, platformUtils)
    QML_READONLY_AUTO_PROPERTY (QNetworkAccessManager*, networkAccessManager)
    QML_READONLY_AUTO_PROPERTY (Settings*, settings)
    QML_READONLY_AUTO_PROPERTY (Vault*, vault)
    QML_READONLY_AUTO_PROPERTY (Telemetry*, telemetry)
    QML_READONLY_AUTO_PROPERTY (Database*, database)
    QML_READONLY_AUTO_PROPERTY (UploadQueue*, uploadQueue)
    QML_READONLY_AUTO_PROPERTY (TranscribeQueue*, transcribeQueue)
    QML_READONLY_AUTO_PROPERTY (AppLink*, appLink)
    QML_READONLY_AUTO_PROPERTY (ProjectManager*, projectManager)
    QML_READONLY_AUTO_PROPERTY (HasUpdate*, hasUpdate)
    QML_READONLY_AUTO_PROPERTY (OfflineMapManager*, offlineMapManager)
    QML_READONLY_AUTO_PROPERTY (TimeManager*, timeManager)
    QML_READONLY_AUTO_PROPERTY (LocationLogger*, locationLogger)
    QML_READONLY_AUTO_PROPERTY (Audit*, audit)
    QML_READONLY_AUTO_PROPERTY (BackupFile*, backupFile)
    QML_READONLY_AUTO_PROPERTY (ExportFile*, exportFile)
    QML_READONLY_AUTO_PROPERTY (QString, iniPath)
    QML_READONLY_AUTO_PROPERTY (QString, iniUrl)
    QML_READONLY_AUTO_PROPERTY (QString, tempPath)
    QML_READONLY_AUTO_PROPERTY (QString, tempUrl)
    QML_READONLY_AUTO_PROPERTY (QString, workPath)
    QML_READONLY_AUTO_PROPERTY (QString, trackLogFilePath)
    QML_READONLY_AUTO_PROPERTY (QString, tracksPath)
    QML_READONLY_AUTO_PROPERTY (QString, mapMarkerCachePath)
    QML_READONLY_AUTO_PROPERTY (QString, gotoPath)
    QML_READONLY_AUTO_PROPERTY (QString, exportPath)
    QML_READONLY_AUTO_PROPERTY (QString, exportUrl)
    QML_READONLY_AUTO_PROPERTY (QString, backupFilesPath)
    QML_READONLY_AUTO_PROPERTY (QString, backupPath)
    QML_READONLY_AUTO_PROPERTY (QString, backupCTXPath)
    QML_READONLY_AUTO_PROPERTY (QString, backupTrackLogPath)
    QML_READONLY_AUTO_PROPERTY (QString, bugReportPath)
    QML_READONLY_AUTO_PROPERTY (QString, logFilePath)
    QML_READONLY_AUTO_PROPERTY (QString, mediaPath)
    QML_READONLY_AUTO_PROPERTY (QString, archivePath)
    QML_READONLY_AUTO_PROPERTY (QString, sendFilePath)
    QML_READONLY_AUTO_PROPERTY (GotoManager*, gotoManager)
    QML_READONLY_AUTO_PROPERTY (Compass*, compass)
    QML_READONLY_AUTO_PROPERTY (SatelliteManager*, satelliteManager)
    QML_READONLY_AUTO_PROPERTY (Place*, place)

    QML_READONLY_AUTO_PROPERTY (Location*, lastLocation)
    QML_READONLY_AUTO_PROPERTY (QString, lastLocationText)
    QML_READONLY_AUTO_PROPERTY (bool, lastLocationRecent)
    QML_READONLY_AUTO_PROPERTY (bool, lastLocationAccurate)
    QML_READONLY_AUTO_PROPERTY (QString, locationAccuracyText)

    QML_READONLY_AUTO_PROPERTY (QLocale, locale)

    QML_READONLY_AUTO_PROPERTY (QString, positionInfoSourceName)

    QML_READONLY_AUTO_PROPERTY (bool, kioskMode)

    QML_READONLY_AUTO_PROPERTY (int, batteryLevel)
    QML_READONLY_AUTO_PROPERTY (bool, batteryCharging)
    QML_READONLY_AUTO_PROPERTY (QString, batteryIcon)
    QML_READONLY_AUTO_PROPERTY (QString, batteryText)

    QML_READONLY_AUTO_PROPERTY (QVariantMap, googleCredentials)

    QML_READONLY_AUTO_PROPERTY (QString, alias_Project)
    QML_READONLY_AUTO_PROPERTY (QString, alias_Projects)
    QML_READONLY_AUTO_PROPERTY (QString, alias_project)
    QML_READONLY_AUTO_PROPERTY (QString, alias_projects)

public:
    explicit App(const QString& rootPath, const QVariantMap& config, QObject* parent = nullptr);
    explicit App(const QString& rootPath, const QVariantMap& config, QGuiApplication* guiApplication, QQmlApplicationEngine* qmlEngine);
    ~App();

    static App* instance();

    void startUI();

    void setCommandLine(const QString& commandLine);

    QString getLocationServiceLockFilePath() const;
    bool getLocationServiceRunning() const;

    void migrateDatabase();

    void registerConnector(const QString& name, ConnectorFactory connectorFactory);
    std::unique_ptr<Connector> createConnectorPtr(const QString& name);
    Q_INVOKABLE Connector* createConnector(const QString& name, QObject* parent = nullptr);

    void registerProvider(const QString& name, ProviderFactory providerFactory);
    std::unique_ptr<Provider> createProviderPtr(const QString& name, Form* form);
    Q_INVOKABLE Provider* createProvider(const QString& name, Form* form);

    void registerRenderer(const QString& name, RendererFactory rendererFactory);
    std::unique_ptr<Renderer> createRendererPtr(const QString& name);
    Q_INVOKABLE Renderer* createRenderer(const QString& name, QObject* parent = nullptr);

    std::unique_ptr<Database> createDatabasePtr();

    std::unique_ptr<Form> createFormPtr(const QString& projectUid, const QString& stateSpace, bool readonly = true);
    Q_INVOKABLE Form* createForm(const QString& projectUid, const QString& stateSpace, bool readonly = true, QObject* parent = nullptr);
    Q_INVOKABLE void deleteSightingsByFlag(const QString& projectUid, const QString& stateSpace, Database::DataFlags matchFlag, bool disableArchive);
    Q_INVOKABLE void deleteSighting(const QString& projectUid, const QString& sightingUid, bool disableArchive);
    Q_INVOKABLE void setSightingNote(const QString& projectUid, const QString& sightingUid, const QString& note);

    QString getArchiveFilePath(const QString& filename) const;
    QString resolveArchiveUrl(const QString& hashFilename) const;
    bool copyToArchive(const QString& filePath, const QString& hashFilename) const;
    QString copyToArchive(const QString& filePath) const;
    void garbageCollectArchive();

    void positionUpdated(const QGeoPositionInfo& update);

    void setAlarm(const QString& alarmId, int seconds);
    void killAlarm(const QString& alarmId);
    void processAlarms();

    void backupCTX(const QString& filePath) const;
    void backupTrackLog() const;

    Q_INVOKABLE double scaleByFontSize(double value) const;

    Q_INVOKABLE QVariantMap getLogin(const QString& key, const QString& defaultServer = QString()) const;
    Q_INVOKABLE void setLogin(const QString& key, const QVariantMap& value);

    Q_INVOKABLE QString getLocationText(double x, double y, QVariant z = QVariant()) const;
    Q_INVOKABLE QVariantMap projectLocation(double x, double y) const;
    Q_INVOKABLE QVariantMap unprojectLocation(double a, double b) const;

    Q_INVOKABLE QString createBugReport(const QString& description);
    Q_INVOKABLE void backupDatabase(const QString& reason);

    Q_INVOKABLE void initProgress(const QString& projectUid, const QString& operation, const QVector<int>& phases);
    Q_INVOKABLE void sendProgress(int value, int phase = 0);

    Q_INVOKABLE void requestPermissionCamera(const QVariant& callback);
    Q_INVOKABLE void requestPermissionMicrophone(const QVariant& callback);
    Q_INVOKABLE void requestPermissionLocation(const QVariant& callback);
    Q_INVOKABLE void requestPermissionBackgroundLocation(const QVariant& callback);

    Q_INVOKABLE bool promptForLocation(const QString& projectUid = QString()) const;
    Q_INVOKABLE bool promptForBackgroundLocation(const QString& projectUid) const;
    Q_INVOKABLE bool promptForCamera(const QString& projectUid = QString()) const;
    Q_INVOKABLE bool promptForMicrophone(const QString& projectUid = QString()) const;

    Q_INVOKABLE bool getActive();

    Q_INVOKABLE void setBatteryState(int level, bool charging);

    Q_INVOKABLE QString formatDate(const QDate& date) const;
    Q_INVOKABLE QString formatTime(const QTime& time) const;
    Q_INVOKABLE QString formatDateTime(qint64 mSecsSinceEpoch) const;
    Q_INVOKABLE QString formatDateTime(const QDateTime& dateTime) const;

    Q_INVOKABLE QString downloadFile(const QString& url, const QString& username = QString(), const QString& password = QString(), const QString& token = QString(), const QString& tokenType = "Bearer");

    Q_INVOKABLE QString copyContent(const QString& contentUri, const QString& targetPath);

    Q_INVOKABLE QString copyToDownload(const QString& filePath, const QString& folder);

    Q_INVOKABLE QString getMediaFilename(const QString& fileSuffix) const;
    Q_INVOKABLE QString moveToMedia(const QString& filePath) const;
    Q_INVOKABLE QString copyToMedia(const QString& filePath) const;
    Q_INVOKABLE bool replicateMedia(const QString& filename);
    Q_INVOKABLE bool removeMedia(const QString& filename);
    Q_INVOKABLE QString convertMediaToJpeg(const QString& filename) const;
    Q_INVOKABLE QString getMediaFilePath(const QString& filename) const;
    Q_INVOKABLE QString getMediaFileUrl(const QString& filename) const;
    Q_INVOKABLE QString getMediaMimeType(const QString& filename) const;
    Q_INVOKABLE void garbageCollectMedia() const;

    Q_INVOKABLE void garbageCollectStreamers();

    Q_INVOKABLE void copyToClipboard(const QString& text);
    Q_INVOKABLE void snapScreenshotToClipboard();
    Q_INVOKABLE void snapScreenshotToFile(const QString& filePathUrl);

    Q_INVOKABLE void runLinkOnClipboard();
    Q_INVOKABLE QVariantMap runCommandLine(const QString& customCommandLine = QString());
    Q_INVOKABLE void runQRCode(const QString& tag);
    Q_INVOKABLE QVariantMap installPackage(const QString& filePathUrl, bool showToasts = true);
    Q_INVOKABLE QString createQRCodeFile(const QString& data, int size = 400) const;

    Q_INVOKABLE void clearDeviceCache();
    Q_INVOKABLE void clearComponentCache();

    Q_INVOKABLE QString getDirectionText(double directionDegrees) const;
    Q_INVOKABLE QString getDistanceText(double distanceMeters) const;
    Q_INVOKABLE QString getAltitudeText(double altitudeMeters) const;
    Q_INVOKABLE QString getSpeedText(double speedMetersPerSecond) const;
    Q_INVOKABLE QString getAreaText(double areaMeters) const;
    Q_INVOKABLE QString getTimeText(int timeSeconds) const;

    Q_INVOKABLE QString deviceImei() const;
    Q_INVOKABLE bool wifiConnected() const;
    Q_INVOKABLE bool pingUrl(const QString& url) const;
    Q_INVOKABLE bool networkAvailable() const;

    Q_INVOKABLE bool uploadAllowed();
    Q_INVOKABLE bool updateAllowed();

    Q_INVOKABLE void share(const QString& url, const QString& text);
    Q_INVOKABLE void sendFile(const QString& filePath, const QString& title);

    Q_INVOKABLE QString renderSvgToPng(const QString& svgUrl, int width, int height) const;

    Q_INVOKABLE QVariantMap esriCreateLocationService(const QString& username, const QString& serviceName, const QString& serviceDescription, const QString& token) const;

    QString renderMapMarker(const QString& url, const QColor& color, int size, bool cache = true) const;
    QString renderMapCallout(const QString& url, int size, bool cache = true) const;
    Q_INVOKABLE QVariantMap createMapCalloutSymbol(const QString& filePath) const;
    Q_INVOKABLE QVariantMap createMapPointSymbol(const QString& url, const QColor& color) const;
    Q_INVOKABLE QVariantMap createMapLineSymbol(const QColor& color) const;

    Q_INVOKABLE void showSystemAlert(const QString& contentTitle, const QString& contentText, const QString& bigText) const;
    Q_INVOKABLE void hideSystemAlert() const;

    Q_INVOKABLE QString translateText(const QString& text) const;

    Q_INVOKABLE QVariantList trackFileToPoints(const QString& filename) const;

    Q_INVOKABLE QObject* parentStackView(const QObject* obj) const;

    Q_INVOKABLE void backToHomeScreen() const;

    Q_INVOKABLE void startOnlineTest();

signals:
    void aboutToQuit();

    void permissionGranted(const QVariant& callback);

    void providerEvent(const QString& providerName, const QString& projectUid, const QString& name, const QVariant& value);
    void progressEvent(const QString& projectUid, const QString& operationName, int index, int count);
    void showMessageBox(const QString& title, const QString& message1, const QString& message2 = QString(), const QString& message3 = QString());
    void backPressed();
    void popPageStack();
    void photoTaken(const QString& filename);
    void barcodeScan(const QString& barcode);
    void locationAcquired(double x, double y, double altitude, double accuracy, const QString& timestamp, const QVariantMap& contextData);
    void alarmFired(const QString& alarmId);
    void showToast(const QString& message);
    void showError(const QString& message);

    void changeToProject(const QString& projectUid);
    
    void sightingsChanged(const QString& projectUid);
    void sightingSaved(const QString& projectUid, const QString& sightingUid);
    void sightingDeleted(const QString& projectUid, const QString& sightingUid);
    void sightingModified(const QString& projectUid, const QString& sightingUid);
    void sightingFlagsChanged(const QString& projectUid, const QString& sightingUid);

    void locationSaved(const QString& projectUid, const QString& locationUid, CyberTracker::Location* location);

    void zoomToMapLayer(const QVariantMap& layer);

    void imagePickComplete(const QString& fileUrl);

    void onlineTestComplete(bool online);

    void auditAdd(const QString& projectTitle, const QString& description);

private slots:
    void launchUrl(const QUrl& url);
    void languageCodeChanged();
    void simulateGPSChanged();
    void kioskModeProjectUidChanged();
    void locationAccuracyChanged();
    void darkThemeChanged();
    void transcribeItemCompleted(const CyberTracker::Database::DbTranscribeItem& transcribeItem);

private:
    QString m_rootPath;
    QString m_cachePath;
    QMap<QString, ConnectorFactory> m_connectors;
    QMap<QString, ProviderFactory> m_providers;
    QMap<QString, RendererFactory> m_renderers;
    QMap<QString, std::pair<QObject*, int>> m_qmlCache;

    SimulateNmeaServer m_simulateNmeaServer;

    QTranslator m_translator;

    QString m_progressProjectUid;
    QString m_progressOperation;
    QVector<int> m_progressPhases;
    QVector<int> m_progressStates;

    QTimer m_alarmTimer;
    QMap<QString, qint64> m_alarms;

    int m_lastLocationCounter = 0;
    QTimer m_lastLocationRecentTimer;

    //QList<Form*> m_forms;

    void requestPermission(const QPermission& permission, const QVariant& callback, const QString& rationale);
    void maybeRequestReview();
};

} // namespace CyberTracker
