#pragma once
#include "pch.h"
#include "Connector.h"

namespace CyberTracker {

class ClassicConnector : public Connector
{
    Q_OBJECT

public:
    static constexpr char CONNECTOR_NAME[] = "Classic";

    static constexpr char UPLOAD_WEBHOOK_ONLY[] = "WEBHOOK_ONLY";
    static constexpr char UPLOAD_AZURE[] = "AZURE";
    static constexpr char UPLOAD_FTP[] = "FTP";
    static constexpr char UPLOAD_HTTP[] = "HTTP";
    static constexpr char UPLOAD_ESRI[] = "ESRI";

public:
    ClassicConnector(QObject *parent = nullptr);
    ~ClassicConnector() override;

    void migrateProject(Project* project) const override;

    bool canExportData(Project* project) const override;

    bool canSharePackage(Project* project) const override;
    bool canShareAuth(Project* project) const override;
    QVariantMap getShareData(Project* project, bool auth) const override;

    bool canLogin(Project* project) const override;
    bool loggedIn(Project* project) const override;
    bool login(Project* project, const QString& server, const QString& username, const QString& password) const override;

    bool isESRI(Project* project) const;
    void bootstrapEsriLogin(Project* project) const;

    ApiResult hasUpdate(Project* project) override;
    ApiResult refreshAccessToken(Project* project, QString* oldRefreshTokenOut, QString* newAccessTokenOut, QString* newRefreshTokenOut) override;
    ApiResult uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* state, const CommitState& commitState) override;

    QString getFormFilePath(Project* project) const override;
    ApiResult bootstrap(const QVariantMap& params) const override;
    bool canUpdate(Project* project) const override;
    ApiResult update(Project* project) const override;
    ApiResult buildSchema(Project* project, const QString& path) const override;

    void reset(Project* project) const override;
    void remove(Project* project) const override;

    void deleteState(Project* project) const override;
    void deleteData(Project* project) const override;

    static bool getCTSMetadata(const QString& ctsFile, QVariantMap* metadataOut);
    static ApiResult copyAppFiles(Project* project, const QString& targetPath);
    static ApiResult installAppFiles(Project* project, const QString& sourcePath);
};

} // namespace CyberTracker
