#pragma once
#include "pch.h"
#include "Connector.h"

namespace CyberTracker {

class SMARTConnector : public Connector 
{
    Q_OBJECT

public:
    static constexpr char CONNECTOR_NAME[] = "SMART";

public:
    SMARTConnector(QObject *parent = nullptr);
    ~SMARTConnector() override;

    void migrateProject(Project* project) const override;

    bool canExportData(Project* project) const override;

    bool canSharePackage(Project* project) const override;
    bool canShareAuth(Project* project) const override;
    QVariantMap getShareData(Project* project, bool auth) const override;

    bool canLogin(Project* project) const override;

    ApiResult hasUpdate(Project* project) override;
    ApiResult hasAnnouncements(Project* project, QVariantList* announcementsOut) override;
    ApiResult uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* state, const CommitState& commitState) override;

    QString getFormFilePath(Project* project) const override;
    ApiResult bootstrap(const QVariantMap& params) const override;
    bool canUpdate(Project* project) const override;
    ApiResult update(Project* project) const override;
    ApiResult buildSchema(Project* project, const QString& path) const override;

    void reset(Project* project) const override;

private:
    QString m_smart7Required;
    bool getZipMetadata(const QString& zipFilePath, QVariantMap* metadataOut, int* smartVersionOut = nullptr, int* minMobileBuildOut = nullptr) const;

    ApiResult bootstrapOffline(const QString& zipFilePath) const;
    ApiResult bootstrapConnect(const QVariantMap& params) const;

    ApiResult updateOffline(Project* project) const;
    ApiResult updateConnect(Project* project) const;
};

} // namespace CyberTracker