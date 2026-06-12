#pragma once
#include "pch.h"
#include "Database.h"
#include "AppLink.h"

namespace CyberTracker {

class ProjectManager;

class Project: public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (QString, uid)
    QML_WRITABLE_AUTO_PROPERTY (int, version)
    QML_WRITABLE_AUTO_PROPERTY (QString, icon)
    QML_WRITABLE_AUTO_PROPERTY (QString, iconDark)
    QML_WRITABLE_AUTO_PROPERTY (QString, name)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, names)
    QML_WRITABLE_AUTO_PROPERTY (QString, subtitle)
    QML_WRITABLE_AUTO_PROPERTY (QString, connector)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, connectorParams)
    QML_WRITABLE_AUTO_PROPERTY (QString, provider)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, providerParams)
    QML_WRITABLE_AUTO_PROPERTY (QVariantList, maps)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, colors)
    QML_WRITABLE_AUTO_PROPERTY (bool, kioskMode)
    QML_WRITABLE_AUTO_PROPERTY (QString, kioskModePin)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, androidPermissions)    // Legacy
    QML_WRITABLE_AUTO_PROPERTY (bool, androidBackgroundLocation)    // Legacy
    QML_WRITABLE_AUTO_PROPERTY (bool, androidDisableBatterySaver)   // Legacy
    QML_WRITABLE_AUTO_PROPERTY (bool, requireLocation)
    QML_WRITABLE_AUTO_PROPERTY (bool, requireBackgroundLocation)
    QML_WRITABLE_AUTO_PROPERTY (bool, requireMicrophone)
    QML_WRITABLE_AUTO_PROPERTY (bool, requireCamera)
    QML_WRITABLE_AUTO_PROPERTY (bool, updateOnLaunch)
    QML_WRITABLE_AUTO_PROPERTY (QString, startPage)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, telemetry)
    QML_WRITABLE_AUTO_PROPERTY (bool, defaultWizardMode)
    QML_WRITABLE_AUTO_PROPERTY (bool, defaultImmersive)
    QML_WRITABLE_AUTO_PROPERTY (bool, defaultSimpleCamera)
    QML_WRITABLE_AUTO_PROPERTY (int, defaultLocationPointInterval)
    QML_WRITABLE_AUTO_PROPERTY (int, defaultLocationTrackInterval)
    QML_WRITABLE_AUTO_PROPERTY (int, defaultLocationTrackDistance)
    QML_WRITABLE_AUTO_PROPERTY (QString, defaultLocationServiceUrl)
    QML_WRITABLE_AUTO_PROPERTY (QString, defaultTrackFieldUid)
    QML_WRITABLE_AUTO_PROPERTY (QString, defaultTrackFileFormat)
    QML_WRITABLE_AUTO_PROPERTY (int, defaultSubmitInterval)
    QML_WRITABLE_AUTO_PROPERTY (QString, offlineMapUrl)
    QML_WRITABLE_AUTO_PROPERTY (bool, disableArchive)

    QML_SETTING (QString, lastBuildString, QString())
    QML_SETTING (QString, lastUpdate, QString())
    QML_SETTING (QString, lastSubmit, QString())
    QML_SETTING (qint64, formLastModified, 0)
    QML_SETTING (QVariantList, languages, QVariantList())
    QML_SETTING (int, languageIndex, -1)
    QML_SETTING (bool, wizardMode, m_defaultWizardMode)
    QML_SETTING (bool, wizardAutosave, true)
    QML_SETTING (bool, immersive, m_defaultImmersive)
    QML_SETTING (bool, simpleCamera, m_defaultSimpleCamera)
    QML_SETTING (QVariantMap, providerState, QVariantMap())

    QML_SETTING (QString, reportedBy, QString())
    QML_SETTING (QString, reportedByDefault, QString())

    QML_SETTING (QString, username, QString())

    QML_SETTING (bool, locationStreamingEnabled, false)
    QML_SETTING (int, locationPointInterval, m_defaultLocationPointInterval)
    QML_SETTING (int, locationTrackInterval, m_defaultLocationTrackInterval)
    QML_SETTING (int, locationTrackDistance, m_defaultLocationTrackDistance)
    QML_SETTING (QString, locationServiceUrl, m_defaultLocationServiceUrl)
    QML_SETTING (QString, trackFieldUid, m_defaultTrackFieldUid)
    QML_SETTING (QString, trackFileFormat, m_defaultTrackFileFormat)

    QML_SETTING (QVariantMap, esriLocationServiceState, QVariantMap())

    QML_SETTING (int, submitInterval, m_defaultSubmitInterval)

    QML_SETTING (QString, webUpdateUrl, QString())
    QML_SETTING (QVariantMap, webUpdateMetadata, QVariantMap())
    QML_SETTING (QString, webUpdateETag, QString())

    QML_SETTING (bool, hasUpdate, false)
    QML_SETTING (bool, hasStreamers, false)
    QML_SETTING (QVariantList, announcements, QVariantList())

    QML_SETTING (QVariantMap, hashCache, QVariantMap())

    QML_SETTING (bool, hasMigrated, false)

    Q_PROPERTY (QString displayIcon READ displayIcon CONSTANT)

    Q_PROPERTY (QString loginIcon READ loginIcon CONSTANT)
    Q_PROPERTY (bool canLogin READ canLogin CONSTANT)
    Q_PROPERTY (bool loggedIn READ loggedIn NOTIFY loggedInChanged)

    Q_PROPERTY (QString title READ title NOTIFY titleChanged)

    Q_PROPERTY (QString accessToken READ accessToken WRITE set_accessToken CONSTANT)
    Q_PROPERTY (QString uploadToken READ uploadToken WRITE set_uploadToken CONSTANT)
    Q_PROPERTY (QString refreshToken READ refreshToken WRITE set_refreshToken CONSTANT)

public:
    Project(QObject* parent = nullptr);
    ~Project() override;

    void saveToQmlFile(const QString& filePath);

    QVariant getSetting(const QString& name, const QVariant& defaultValue = QVariant()) const;
    void setSetting(const QString& name, const QVariant& value = QVariant());

    QVariant getConnectorParam(const QString& name, const QVariant& defaultValue = QVariant()) const;
    void setConnectorParam(const QString& name, const QVariant& value);

    QString accessToken() const;
    void set_accessToken(const QString& token);
    QString uploadToken() const;
    void set_uploadToken(const QString& token);
    QString refreshToken() const;
    void set_refreshToken(const QString& token);

    QString languageCode() const;

    QVariantMap save(bool keepSettings, bool keepAuth) const;
    void load(const QVariantMap& data, bool loadSettings);
    void reload();

    QString archiveFile(const QString& filename);
    QVariantMap archiveFiles(const QStringList& files);

    QString displayIcon() const;

    QString loginIcon() const;
    bool canLogin();
    bool loggedIn();

    QString title() const;

    QVariantMap summary() const;

    Q_INVOKABLE bool login(const QString& server, const QString& username, const QString& password);
    Q_INVOKABLE void login2(const QString& username);
    Q_INVOKABLE void logout();

signals:
    void loggedInChanged(bool loggedIn);
    void titleChanged();

private:
    ProjectManager* m_projectManager = nullptr;
};

class ProjectManager: public QObject
{
    Q_OBJECT
    QML_READONLY_AUTO_PROPERTY (QString, projectsPath)
    QML_READONLY_AUTO_PROPERTY (QString, lastNewProjectUid)

    friend class Project;

public:
    static constexpr char PROJECT_ARCHIVE[] = "Archive";

public:
    explicit ProjectManager(QObject* parent = nullptr);
    explicit ProjectManager(const QString& projectsPath, QObject* parent = nullptr);

    void modify(const QString& projectUid, const std::function<void(Project*)>& callback);

    void enumProjects(const std::function<void(const QString& projectUid)>& callback) const;
    QStringList buildList() const;
    QList<std::shared_ptr<Project>> buildListPtr() const;
    std::unique_ptr<Project> loadPtr(const QString& projectUid) const;

    Q_INVOKABLE QString getFilePath(const QString& projectUid, const QString& filename = "") const;
    Q_INVOKABLE QString getFileUrl(const QString& projectUid, const QString& filename = "") const;
    Q_INVOKABLE QString getUpdateFolder(const QString& projectUid) const;

    Q_INVOKABLE QVariantMap bootstrap(const QString& connectorName, const QVariantMap& params, bool showToasts = true);
    Q_INVOKABLE bool canLogin(const QString& projectUid) const;
    Q_INVOKABLE bool canUpdate(const QString& projectUid) const;
    Q_INVOKABLE QVariantMap update(const QString& projectUid, bool showToasts = true);
    Q_INVOKABLE QVariantMap updateAll(const QString& connectorName = "", const QString& projectUid = "");

    Q_INVOKABLE Project* load(const QString& projectUid) const;
    Q_INVOKABLE bool init(const QString& projectUid, const QString& provider, const QVariantMap& providerParams, const QString& connector, const QVariantMap& connectorParams);
    Q_INVOKABLE bool exists(const QString& projectUid) const;

    Q_INVOKABLE void reset(const QString& projectUid, bool keepData = false);
    Q_INVOKABLE void resetArchive();
    Q_INVOKABLE void remove(const QString& projectUid);

    Q_INVOKABLE void moveProject(const QString& projectUid, int delta);
    Q_INVOKABLE int count() const;

    Q_INVOKABLE bool canShareLink(const QString& projectUid) const;
    Q_INVOKABLE bool canShareAuth(const QString& projectUid) const;
    Q_INVOKABLE QString getShareUrl(const QString& projectUid, bool auth) const;
    Q_INVOKABLE QString createQRCode(const QString& projectUid, bool auth, int size = 400) const;

    Q_INVOKABLE bool canExportData(const QString& projectUid) const;

    Q_INVOKABLE bool canSharePackage(const QString& projectUid) const;
    Q_INVOKABLE QString createPackage(const QString& filename, const QString& projectUid, bool includeProject = true, bool keepAuth = true, bool includeData = true) const;
    Q_INVOKABLE QString createArchivePackage(const QString& filename, const QStringList& sightingUids) const;
    Q_INVOKABLE QString createArchivePackage() const;

    bool canInstallPackage(const QString& filePath) const;
    ApiResult installPackage(const QString& filePath, QString* projectUidOut = nullptr);

    Q_INVOKABLE QVariantMap webInstall(const QString& webUpdateUrl);

signals:
    void projectSettingChanged(const QString& projectUid, const QString& name, const QVariant& value);
    void projectsChanged();
    void projectLoggedInChanged(const QString& projectUid, bool loggedIn);
    void projectTriggerLogin(const QString& projectUid);
    void projectInstalled(const QString& projectUid, const QVariantMap& result);
    void projectUpdated(const QString& projectUid, const QVariantMap& result);
    void projectDeleted(const QString& projectTitle);
    void archiveReset();

private:
    Database* m_database = nullptr;
    AppLink* m_appLink = nullptr;

    mutable QRecursiveMutex m_cacheLock;
    mutable QHash<QString, QVariantMap> m_cache;

    void buildCache() const;
    void setCacheEntry(const QString& projectUid, const QVariantMap& data);

    QVariantMap getShareData(const QString& projectUid, bool auth) const;
};

} // namespace CyberTracker
