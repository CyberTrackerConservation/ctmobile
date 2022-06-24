#pragma once
#include "pch.h"
#include "Database.h"
#include "AppLink.h"

class ProjectManager;

class Project: public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (QString, uid)
    QML_WRITABLE_AUTO_PROPERTY (int, version)
    QML_WRITABLE_AUTO_PROPERTY (QString, icon)
    QML_WRITABLE_AUTO_PROPERTY (QString, iconDark)
    QML_WRITABLE_AUTO_PROPERTY (QString, title)
    QML_WRITABLE_AUTO_PROPERTY (QString, subtitle)
    QML_WRITABLE_AUTO_PROPERTY (QString, connector)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, connectorParams)
    QML_WRITABLE_AUTO_PROPERTY (QString, provider)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, providerParams)
    QML_WRITABLE_AUTO_PROPERTY (QVariantList, maps)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, colors)
    QML_WRITABLE_AUTO_PROPERTY (bool, kioskMode)
    QML_WRITABLE_AUTO_PROPERTY (QString, kioskModePin)
    QML_WRITABLE_AUTO_PROPERTY (QStringList, androidPermissions)
    QML_WRITABLE_AUTO_PROPERTY (bool, androidDisableBatterySaver)
    QML_WRITABLE_AUTO_PROPERTY (bool, updateOnLaunch)
    QML_WRITABLE_AUTO_PROPERTY (QString, startPage)
    QML_WRITABLE_AUTO_PROPERTY (QVariantMap, telemetry)
    QML_WRITABLE_AUTO_PROPERTY (bool, defaultWizardMode)

    QML_SETTING (QString, lastBuildString, QString())
    QML_SETTING (QString, lastUpdate, QString())
    QML_SETTING (QVariantList, languages, QVariantList())
    QML_SETTING (int, languageIndex, -1)
    QML_SETTING (QVariantMap, activeProjectLayers, QVariantMap()) // active map layers
    QML_SETTING (bool, wizardMode, m_defaultWizardMode)
    QML_SETTING (QVariantMap, providerState, QVariantMap())

    QML_SETTING (QString, reportedBy, QString())

    QML_SETTING (QString, username, QString())
    QML_SETTING (QString, accessToken, QString())
    QML_SETTING (QString, refreshToken, QString())

    QML_SETTING (QString, webUpdateUrl, QString())
    QML_SETTING (QVariantMap, webUpdateMetadata, QVariantMap())

    QML_SETTING (bool, hasUpdate, false)

    Q_PROPERTY (QUrl loginIcon READ loginIcon CONSTANT)
    Q_PROPERTY (bool loggedIn READ loggedIn NOTIFY loggedInChanged)

public:
    Project(QObject* parent = nullptr);

    void saveToQmlFile(const QString& filePath);

    QVariant getSetting(const QString& name, const QVariant& defaultValue = QVariant()) const;
    void setSetting(const QString& name, const QVariant& value = QVariant());

    QString languageCode() const;

    QVariantMap save(bool keepAuth) const;
    void load(const QVariantMap& data, bool loadSettings);

    QUrl loginIcon() const;
    bool loggedIn();

    Q_INVOKABLE bool login(const QString& server, const QString& username, const QString& password);
    Q_INVOKABLE void logout();

signals:
    void loggedInChanged(bool loggedIn);

private:
    ProjectManager* m_projectManager = nullptr;
};

class UpdateScanTask: public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit UpdateScanTask(QObject* parent, const QString& projectUid);

private:
    QString m_projectUid;
    virtual void run() override;

signals:
    void completed(const QString& projectUid, bool success, bool hasUpdate);
};

class ProjectManager: public QObject
{
    Q_OBJECT
    QML_READONLY_AUTO_PROPERTY (QString, projectsPath)
    QML_READONLY_AUTO_PROPERTY (QString, lastNewProjectUid)

public:
    explicit ProjectManager(QObject* parent = nullptr);
    explicit ProjectManager(const QString& projectsPath, QObject* parent = nullptr);

    void startUpdateScanner();

    void modify(const QString& projectUid, const std::function<void(Project*)>& callback);
    QList<std::shared_ptr<Project>> buildList() const;
    std::unique_ptr<Project> loadPtr(const QString& projectUid) const;

    Q_INVOKABLE QString getFilePath(const QString& projectUid, const QString& filename = "") const;
    Q_INVOKABLE QUrl getFileUrl(const QString& projectUid, const QString& filename = "") const;
    Q_INVOKABLE QString getUpdateFolder(const QString& projectUid) const;

    Q_INVOKABLE QVariantMap bootstrap(const QString& connectorName, const QVariantMap& params, bool showToasts = true);
    Q_INVOKABLE bool canUpdate(const QString& projectUid) const;
    Q_INVOKABLE QVariantMap update(const QString& projectUid, bool showToasts = true);
    Q_INVOKABLE QVariantMap updateAll(const QString& connectorName = "", const QString& projectUid = "");

    Q_INVOKABLE Project* load(const QString& projectUid) const;
    Q_INVOKABLE bool init(const QString& projectUid, const QString& provider, const QVariantMap& providerParams, const QString& connector, const QVariantMap& connectorParams);
    Q_INVOKABLE bool exists(const QString& projectUid) const;

    Q_INVOKABLE void reset(const QString& projectUid);
    Q_INVOKABLE void remove(const QString& projectUid);

    Q_INVOKABLE int count() const;

    Q_INVOKABLE bool canShareLink(const QString& projectUid) const;
    Q_INVOKABLE bool canShareAuth(const QString& projectUid) const;
    Q_INVOKABLE QString getShareUrl(const QString& projectUid, bool auth) const;
    Q_INVOKABLE QString createQRCode(const QString& projectUid, bool auth, int size = 400) const;

    Q_INVOKABLE bool canExportData(const QString& projectUid) const;

    Q_INVOKABLE bool canSharePackage(const QString& projectUid) const;
    Q_INVOKABLE QString createPackage(const QString& projectUid, bool includeProject = true, bool keepAuth = true, bool includeData = true) const;
    Q_INVOKABLE QVariantMap installPackage(const QString& filePath);

    Q_INVOKABLE QVariantMap webInstall(const QString& webUpdateUrl);

signals:
    void projectSettingChanged(const QString& projectUid, const QString& name, const QVariant& value);
    void projectsChanged();
    void projectLoggedInChanged(const QString& projectUid, bool loggedIn);
    void projectTriggerLogin(const QString& projectUid);
    void projectUpdateScanComplete();

private:
    Database* m_database;
    AppLink* m_appLink;

    QMap<QString, bool> m_updateScanTasks;
    QTimer m_updateScanTimer;
    int m_blockUpdateScanCounter = 0;
    void pauseUpdateScan();
    void resumeUpdateScan();

    QVariantMap getShareData(const QString& projectUid, bool auth) const;
    void hasUpdateAsync(const QString& projectUid);
    void hasUpdatesAsync();
};
