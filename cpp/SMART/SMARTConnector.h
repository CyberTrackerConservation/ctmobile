#pragma once
#include "pch.h"
#include "Connector.h"

constexpr char SMART_CONNECTOR[] = "SMART";

class SMARTConnector : public Connector 
{
    Q_OBJECT

public:
    SMARTConnector(QObject *parent = nullptr);

    bool canExportData(Project* project) const override;

    bool canSharePackage(Project* project) const override;
    bool canShareAuth(Project* project) const override;
    QVariantMap getShareData(Project* project, bool auth) const override;

    bool canLogin(Project* project) const override;

    ApiResult bootstrap(const QVariantMap& params) override;
    ApiResult hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project) override;
    bool canUpdate(Project* project) const override;
    ApiResult update(Project* project) override;

    void reset(Project* project) override;

private:
    QString m_smart7Required;
    bool getZipMetadata(const QString& zipFilePath, QVariantMap* metadataOut, int* smartVersionOut = nullptr);

    ApiResult bootstrapOffline(const QString& zipFilePath);
    ApiResult bootstrapConnect(const QVariantMap& params);

    ApiResult updateOffline(Project* project);
    ApiResult updateConnect(Project* project);
};
