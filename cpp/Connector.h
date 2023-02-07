#pragma once
#include "pch.h"
#include "Database.h"
#include "Project.h"

class Connector: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, name)
    QML_READONLY_AUTO_PROPERTY (QString, icon)

public:
    Connector(QObject* parent);

    virtual bool canExportData(Project* project) const;

    virtual bool canSharePackage(Project* project) const;
    virtual bool canShareAuth(Project* project) const;
    virtual QVariantMap getShareData(Project* project, bool auth) const;

    virtual bool canLogin(Project* project) const;
    virtual bool loggedIn(Project* project) const;
    virtual bool login(Project* project, const QString& server, const QString& username, const QString& password);
    virtual void logout(Project* project);

    virtual ApiResult bootstrap(const QVariantMap& params);
    virtual bool refreshAccessToken(QNetworkAccessManager* networkAccessManager, Project* project);
    virtual ApiResult hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project);
    virtual bool canUpdate(Project* project) const;
    virtual ApiResult update(Project* project);

    virtual void reset(Project* project);
    virtual void remove(Project* project);

protected:
    Database* m_database = nullptr;
    ProjectManager* m_projectManager = nullptr;

    static ApiResult ExpectedError(const QString& errorString);
    static ApiResult Failure(const QString& errorString);
    static ApiResult Success(const QString& errorString = QString());
    static ApiResult AlreadyUpToDate();
    static ApiResult AlreadyConnected();
    static ApiResult UpdateAvailable();

    ApiResult bootstrapUpdate(const QString& projectUid);
};

