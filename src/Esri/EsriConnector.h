#pragma once
#include "pch.h"
#include "Connector.h"

namespace CyberTracker {

class EsriConnector : public Connector
{
    Q_OBJECT

public:
    static constexpr char CONNECTOR_NAME[] = "Esri";
    static constexpr char UPLOAD_LAST_LOCATION[] = "LAST_LOCATION";
    static constexpr char UPLOAD_TRACK[] = "TRACK";
    static constexpr char UPLOAD_DATA[] = "DATA";

public:
    EsriConnector(QObject *parent = nullptr);
    ~EsriConnector() override;

    bool canSharePackage(Project* project) const override;
    bool canShareAuth(Project* project) const override;
    QVariantMap getShareData(Project* project, bool auth) const override;

    bool canLogin(Project* project) const override;
    bool loggedIn(Project* project) const override;
    bool login(Project* project, const QString& server, const QString& username, const QString& password) const override;
    void logout(Project* project) const override;

    ApiResult refreshAccessToken(Project* project, QString* oldRefreshTokenOut, QString* newAccessTokenOut, QString* newRefreshTokenOut) override;
    ApiResult hasUpdate(Project* project) override;
    ApiResult uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* state, const CommitState& commitState) override;

    QString getFormFilePath(Project* project) const override;
    ApiResult bootstrap(const QVariantMap& params) const override;
    bool canUpdate(Project *project) const override;
    ApiResult update(Project* project) const override;
    ApiResult buildSchema(Project* project, const QString& path) const override;

    void reset(Project* project) const override;
};

} // namespace CyberTracker