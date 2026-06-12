#pragma once
#include "pch.h"
#include "Database.h"
#include "Project.h"

namespace CyberTracker {

class Connector: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, name)
    QML_READONLY_AUTO_PROPERTY (QString, icon)

public:
    Connector(QObject* parent = nullptr);
    virtual ~Connector();

    void setNetworkAccessManager(QNetworkAccessManager* networkAccessManager);

    virtual void migrateProject(Project* project) const;

    virtual QString getFormFilePath(Project* project) const;

    virtual bool canExportData(Project* project) const;

    virtual bool canSharePackage(Project* project) const;
    virtual bool canShareAuth(Project* project) const;
    virtual QVariantMap getShareData(Project* project, bool auth) const;

    virtual bool canLogin(Project* project) const;
    virtual bool loggedIn(Project* project) const;
    virtual bool login(Project* project, const QString& server, const QString& username, const QString& password) const;
    virtual void logout(Project* project) const;

    virtual ApiResult refreshAccessToken(Project* project, QString* oldRefreshTokenOut, QString* newAccessTokenOut, QString* newRefreshTokenOut);
    virtual ApiResult hasUpdate(Project* project);
    virtual ApiResult hasAnnouncements(Project* project, QVariantList* announcementsOut);

    typedef std::function<void()> CommitState;
    virtual ApiResult uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* state, const CommitState& commitState);

    virtual ApiResult bootstrap(const QVariantMap& params) const;
    virtual bool canUpdate(Project* project) const;
    virtual ApiResult update(Project* project) const;
    virtual ApiResult buildSchema(Project* project, const QString& path) const;

    virtual void reset(Project* project) const;
    virtual void remove(Project* project) const;

    virtual void deleteState(Project* project) const;
    virtual void deleteData(Project* project) const;

public:
    static ApiResult ExpectedError(const QString& errorString);
    static ApiResult Failure(const QString& errorString);
    static ApiResult Success(const QString& errorString = QString());
    static ApiResult Unauthorized();
    static ApiResult NotImplemented();
    static ApiResult NotSupported();
    static ApiResult AlreadyUpToDate();
    static ApiResult AlreadyConnected();
    static ApiResult UpdateAvailable();
    static ApiResult Failure(const Utils::HttpResponse& response);
    static ApiResult ResultFromResponse(const Utils::HttpResponse& response);

protected:
    Database* m_database = nullptr;
    ProjectManager* m_projectManager = nullptr;
    QNetworkAccessManager* m_networkAccessManager = nullptr;

    ApiResult bootstrapUpdate(const QString& projectUid) const;
    ApiResult uploadDataODK(Project* project, const QString& url, const QByteArray& payload, const QStringList& attachments) const;
};

} // namespace CyberTracker