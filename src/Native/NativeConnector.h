#pragma once
#include "pch.h"
#include "Connector.h"

namespace CyberTracker {

class NativeConnector : public Connector
{
    Q_OBJECT

public:
    static constexpr char CONNECTOR_NAME[] = "Native";

public:
    NativeConnector(QObject* parent = nullptr);
    ~NativeConnector() override;

    bool canSharePackage(Project* project) const override;
    bool canShareAuth(Project* project) const override;
    QVariantMap getShareData(Project* project, bool auth) const override;

    bool canLogin(Project* project) const override;

    ApiResult hasUpdate(Project* project) override;
    ApiResult uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* state, const CommitState& commitState) override;

    QString getFormFilePath(Project* project) const override;
    ApiResult bootstrap(const QVariantMap& params) const override;
    bool canUpdate(Project *project) const override;
    ApiResult update(Project* project) const override;

    ApiResult buildSchema(Project* project, const QString& path) const override;

    void reset(Project* project) const override;

private:
    ApiResult installPackageZip(Project* project, const QString& zipFilePath, const QString& webUpdateUrl, const QVariantMap& metadata, const QString& eTag) const;
};

} // namespace CyberTracker