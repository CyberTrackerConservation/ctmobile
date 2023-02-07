#pragma once
#include "pch.h"
#include "Connector.h"

constexpr char CLASSIC_CONNECTOR[] = "Classic";

class ClassicConnector : public Connector
{
    Q_OBJECT

public:
    ClassicConnector(QObject *parent = nullptr);

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
    void remove(Project* project) override;

    static bool getCTSMetadata(const QString& ctsFile, QVariantMap* metadataOut);
    static ApiResult copyAppFiles(Project* project, const QString& targetPath);
    static ApiResult installAppFiles(Project* project, const QString& sourcePath);
};
