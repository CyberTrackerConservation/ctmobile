#pragma once
#include "pch.h"
#include "Connector.h"

constexpr char NATIVE_CONNECTOR[] = "Native";

class NativeConnector : public Connector
{
    Q_OBJECT

public:
    NativeConnector(QObject* parent = nullptr);

    bool canSharePackage(Project* project) const override;
    bool canShareAuth(Project* project) const override;
    QVariantMap getShareData(Project* project, bool auth) const override;

    ApiResult bootstrap(const QVariantMap& params) override;
    ApiResult hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project) override;
    bool canUpdate(Project *project) override;
    ApiResult update(Project* project) override;
};
