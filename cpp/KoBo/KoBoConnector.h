#pragma once
#include "pch.h"
#include "Connector.h"

constexpr char KOBO_CONNECTOR[] = "KoBo";

class KoBoConnector : public Connector
{
    Q_OBJECT

public:
    KoBoConnector(QObject *parent = nullptr);

    bool canSharePackage(Project* project) const override;
    bool canShareAuth(Project* project) const override;
    QVariantMap getShareData(Project* project, bool auth) const override;

    bool canLogin(Project* project) const override;
    bool loggedIn(Project* project) const override;
    bool login(Project* project, const QString& server, const QString& username, const QString& password) override;
    void logout(Project* project) override;

    ApiResult bootstrap(const QVariantMap& params) override;
    ApiResult hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project) override;
    bool canUpdate(Project *project) const override;
    ApiResult update(Project* project) override;

    void reset(Project* project) override;

private:
    QString getDeployedVersion(const QVariantMap& formMap);
};
