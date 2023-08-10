#pragma once
#include "pch.h"
#include "Settings.h"
#include "Telemetry.h"
#include "AppLink.h"
#include "Project.h"
#include "Database.h"
#include "Connector.h"
#include "Form.h"
#include "ProviderInterface.h"
#include "MBTilesReader.h"
#include "TimeManager.h"
#include "Location.h"
#include "Goto.h"
#include "Compass.h"
#include "Satellite.h"
#include "OfflineMap.h"

//#define SCREENSHOT_MODE
#if defined(SCREENSHOT_MODE)
constexpr int SCREENSHOT_WIDTH = 2048;
constexpr int SCREENSHOT_HEIGHT = 2732;
constexpr int SCREENSHOT_SCALE = 4;
#endif

typedef Connector* (*ConnectorFactory)(QObject* parent);
typedef Provider* (*ProviderFactory)(QObject* parent);

class App: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, commandLine)
    QML_READONLY_AUTO_PROPERTY (QString, sessionId)

    QML_READONLY_AUTO_PROPERTY (QString, buildString)
    QML_READONLY_AUTO_PROPERTY (int, dpi)
    QML_READONLY_AUTO_PROPERTY (bool, desktopOS)
    QML_READONLY_AUTO_PROPERTY (bool, mobileOS)
    QML_READONLY_AUTO_PROPERTY (bool, debugBuild)
    QML_READONLY_AUTO_PROPERTY (QString, fixedFontFamily)

    QML_READONLY_AUTO_PROPERTY (QVariantList, languages)

    QML_READONLY_AUTO_PROPERTY (QVariantMap, config)

    QML_READONLY_AUTO_PROPERTY (QGuiApplication*, guiApplication)
    QML_READONLY_AUTO_PROPERTY (QQmlApplicationEngine*, qmlEngine)
    QML_READONLY_AUTO_PROPERTY (QNetworkAccessManager*, networkAccessManager)
    QML_READONLY_AUTO_PROPERTY (Settings*, settings)
    QML_READONLY_AUTO_PROPERTY (Telemetry*, telemetry)
    QML_READONLY_AUTO_PROPERTY (Database*, database)
    QML_READONLY_AUTO_PROPERTY (AppLink*, appLink)
    QML_READONLY_AUTO_PROPERTY (ProjectManager*, projectManager)
    QML_READONLY_AUTO_PROPERTY (OfflineMapManager*, offlineMapManager)
    QML_READONLY_AUTO_PROPERTY (TimeManager*, timeManager)
    QML_READONLY_AUTO_PROPERTY (TaskManager*, taskManager)
    QML_READONLY_AUTO_PROPERTY (QString, iniPath)
    QML_READONLY_AUTO_PROPERTY (QString, tempPath)
    QML_READONLY_AUTO_PROPERTY (QUrl, tempUrl)
    QML_READONLY_AUTO_PROPERTY (QString, workPath)
    QML_READONLY_AUTO_PROPERTY (QString, mapTileCachePath)
    QML_READONLY_AUTO_PROPERTY (QString, mapMarkerCachePath)
    QML_READONLY_AUTO_PROPERTY (MBTilesReader*, mbTilesReader)
    QML_READONLY_AUTO_PROPERTY (QString, gotoPath)
    QML_READONLY_AUTO_PROPERTY (QString, exportPath)
    QML_READONLY_AUTO_PROPERTY (QUrl, exportUrl)
    QML_READONLY_AUTO_PROPERTY (QString, backupPath)
    QML_READONLY_AUTO_PROPERTY (QString, bugReportPath)
    QML_READONLY_AUTO_PROPERTY (QString, logFilePath)
    QML_READONLY_AUTO_PROPERTY (QString, mediaPath)
    QML_READONLY_AUTO_PROPERTY (QString, sendFilePath)
    QML_READONLY_AUTO_PROPERTY (GotoManager*, gotoManager)
    QML_READONLY_AUTO_PROPERTY (Compass*, compass)
    QML_READONLY_AUTO_PROPERTY (SatelliteManager*, satelliteManager)

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
    App(QObject* parent = nullptr);
    App(QGuiApplication* guiApplication, QQmlApplicationEngine* qmlEngine, const QVariantMap& config);
    ~App();

    static App* instance();

    void setCommandLine(const QString& commandLine);

    void registerConnector(const QString& name, ConnectorFactory connectorFactory);
    std::unique_ptr<Connector> createConnectorPtr(const QString& name);
    Q_INVOKABLE Connector* createConnector(const QString& name, QObject* parent = nullptr);

    void registerProvider(const QString& name, ProviderFactory connectorFactory);
    std::unique_ptr<Provider> createProviderPtr(const QString& name, QObject* parentForm);
    Q_INVOKABLE Provider* createProvider(const QString& name, QObject* parentForm);

    std::unique_ptr<Database> createDatabasePtr();

    std::unique_ptr<Form> createFormPtr(const QString& projectUid, const QString& stateSpace, bool readonly = true);
    Q_INVOKABLE Form* createForm(const QString& projectUid, const QString& stateSpace, bool readonly = true, QObject* parent = nullptr);
    void removeSightingsByFlag(const QString& projectUid, const QString& stateSpace, uint matchFlag);

    QObject* acquireQml(const QString& filePath, QString* keyOut);
    void releaseQml(const QString& key);
    void garbageCollectQmlCache();

    QString lookupLanguageCode(const QString& languageName);

    void positionUpdated(const QGeoPositionInfo& update);

    void setAlarm(const QString& alarmId, int seconds);
    void killAlarm(const QString& alarmId);
    void processAlarms();

    Q_INVOKABLE double scaleByFontSize(double value) const;

    Q_INVOKABLE QVariantMap getLogin(const QString& key, const QString& defaultServer = QString()) const;
    Q_INVOKABLE void setLogin(const QString& key, const QVariantMap& value);

    Q_INVOKABLE QString getLocationText(double x, double y, QVariant z = QVariant());

    Q_INVOKABLE void requestDisableBatterySaver();

    Q_INVOKABLE QString createBugReport(const QString& description);
    Q_INVOKABLE void backupDatabase(const QString& reason);

    Q_INVOKABLE void initProgress(const QString& projectUid, const QString& operation, const QVector<int>& phases);
    Q_INVOKABLE void sendProgress(int value, int phase = 0);

    Q_INVOKABLE bool hasPermission(const QString& permission) const;
    Q_INVOKABLE bool requestPermissionCamera();
    Q_INVOKABLE bool requestPermissionRecordAudio();
    Q_INVOKABLE bool requestPermissionLocation();
    Q_INVOKABLE bool requestPermissionBackgroundLocation();
    Q_INVOKABLE bool requestPermissions(const QString& projectUid);
    Q_INVOKABLE bool promptForLocation(const QString& projectUid = QString()) const;
    Q_INVOKABLE bool promptForBackgroundLocation(const QString& projectUid) const;
    Q_INVOKABLE bool promptForBlackviewMessage(const QString& projectUid) const;

    Q_INVOKABLE bool getActive();

    Q_INVOKABLE void setBatteryState(int level, bool charging);

    Q_INVOKABLE QString formatDate(const QDate& date) const;
    Q_INVOKABLE QString formatTime(const QTime& time) const;
    Q_INVOKABLE QString formatDateTime(qint64 mSecsSinceEpoch) const;
    Q_INVOKABLE QString formatDateTime(const QDateTime& dateTime) const;

    Q_INVOKABLE QString downloadFile(const QString& url, const QString& username = QString(), const QString& password = QString(), const QString& token = QString(), const QString& tokenType = "Bearer");

    Q_INVOKABLE QString moveToMedia(const QString& filePathUrl);
    Q_INVOKABLE QString copyToMedia(const QString& filePathUrl);
    Q_INVOKABLE bool removeMedia(const QString& filename) const;
    Q_INVOKABLE QString getMediaFilePath(const QString& filename) const;
    Q_INVOKABLE QUrl getMediaFileUrl(const QString& filename) const;
    Q_INVOKABLE QString getMediaMimeType(const QString& filename) const;
    Q_INVOKABLE void garbageCollectMedia() const;

    Q_INVOKABLE void copyToClipboard(const QString& text);
    Q_INVOKABLE void snapScreenshotToClipboard();
    Q_INVOKABLE void snapScreenshotToFile(const QString& filePathUrl);

    Q_INVOKABLE void runLinkOnClipboard();
    Q_INVOKABLE QVariantMap runCommandLine(const QString& customCommandLine = QString());
    Q_INVOKABLE void runQRCode(const QString& tag);
    Q_INVOKABLE QVariantMap installPackage(const QString& filePathUrl, bool showToasts = true);

    Q_INVOKABLE void clearDeviceCache();
    Q_INVOKABLE void clearComponentCache();

    Q_INVOKABLE QString getDirectionText(double directionDegrees) const;
    Q_INVOKABLE QString getDistanceText(double distanceMeters) const;
    Q_INVOKABLE QString getSpeedText(double speedMetersPerSecond) const;
    Q_INVOKABLE QString getAreaText(double areaMeters) const;
    Q_INVOKABLE QString getTimeText(int timeSeconds) const;

    Q_INVOKABLE QString deviceImei() const;
    Q_INVOKABLE bool wifiConnected() const;
    Q_INVOKABLE bool pingUrl(const QString& url) const;
    Q_INVOKABLE bool networkAvailable() const;

    Q_INVOKABLE bool uploadAllowed();
    Q_INVOKABLE bool updateAllowed();

    Q_INVOKABLE void share(const QUrl& url, const QString& text);
    Q_INVOKABLE void sendFile(const QString& filePath, const QString& title);

    Q_INVOKABLE void registerExportFile(const QString& filePath, const QVariantMap& data);
    Q_INVOKABLE void removeExportFile(const QString& filePath);
    Q_INVOKABLE QVariantList buildExportFilesModel(const QString& projectUid = QString(), const QString& filter = QString());

    Q_INVOKABLE QString renderSvgToPng(const QString& svgUrl, int width, int height) const;

    Q_INVOKABLE QVariantMap esriCreateLocationService(const QString& username, const QString& serviceName, const QString& serviceDescription, const QString& token) const;

    QString renderMapMarker(const QString& url, const QColor& color, int size) const;
    QString renderMapCallout(const QString& url, int size) const;
    Q_INVOKABLE QVariantMap createMapCalloutSymbol(const QString& filePath) const;
    Q_INVOKABLE QVariantMap createMapPointSymbol(const QString& url, const QColor& color) const;
    Q_INVOKABLE QVariantMap createMapLineSymbol(const QColor& color) const;

signals:
    void consoleMessage(const QString& message);

    void providerEvent(const QString& providerName, const QString& projectUid, const QString& name, const QVariant& value);
    void progressEvent(const QString& projectUid, const QString& operationName, int index, int count);
    void showMessageBox(const QString& title, const QString& message1, const QString& message2 = QString(), const QString& message3 = QString());
    void backPressed();
    void popPageStack();
    void photoTaken(const QString& filename);
    void barcodeScan(const QString& barcode);
    void alarmFired(const QString& alarmId);
    void showToast(const QString& message);
    void showError(const QString& message);
    
    void sightingsChanged(const QString& projectUid, const QString& stateSpace = QString());
    void sightingSaved(const QString& projectUid, const QString& sightingUid, const QString& stateSpace = QString());
    void sightingRemoved(const QString& projectUid, const QString& sightingUid, const QString& stateSpace = QString());
    void sightingModified(const QString& projectUid, const QString& sightingUid, const QString& stateSpace = QString());
    void sightingFlagsChanged(const QString& projectUid, const QString& sightingUid, const QString& stateSpace = QString());

    void zoomToMapLayer(const QVariantMap& layer);

    void exportFilesChanged();

private slots:
    void launchUrl(const QUrl& url);
    void languageCodeChanged();
    void simulateGPSChanged();
    void kioskModeProjectUidChanged();
    void locationAccuracyChanged();

private:
    QString m_rootPath;
    QString m_cachePath;    
    QMap<QString, ConnectorFactory> m_connectors;
    QMap<QString, ProviderFactory> m_providers;
    QMap<QString, std::pair<QObject*, int>> m_qmlCache;

    SimulateNmeaServer m_simulateNmeaServer;

    QTranslator m_translator;

    bool m_requestedDisableBatterySaver = false;

    QString m_progressProjectUid;
    QString m_progressOperation;
    QVector<int> m_progressPhases;
    QVector<int> m_progressStates;

    QMap<QString, QString> m_languageLookup;

    QTimer m_alarmTimer;
    QMap<QString, qint64> m_alarms;

    int m_lastLocationCounter = 0;
    QTimer m_lastLocationRecentTimer;

    bool requestPermission(const QString& permission, const QString& rationale);
};
