#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "Form.h"

constexpr char ESRI_PROVIDER[] = "Esri";

class EsriProvider : public Provider
{
    Q_OBJECT
    QML_READONLY_AUTO_PROPERTY(QString, parserError)
    QML_READONLY_AUTO_PROPERTY(QVariantMap, settings)

public:
    EsriProvider(QObject *parent = nullptr);

    bool connectToProject(bool newBuild) override;

    bool requireUsername() const override;

    QUrl getStartPage() override;
    void getElements(ElementManager* elementManager) override;
    void getFields(FieldManager* fieldManager) override;

    bool notify(const QVariantMap& message) override;

    bool finalizePackage(const QString& packageFilesPath) const override;

    Q_INVOKABLE void submitData();

private:
    void setFieldTags(const QString& serviceInfoFilePath, const QString& fieldsFilePath);
    QVariantList buildSightingPayload(Form* form, const QString& sightingUid, QVariantMap* attachmentsMapOut);
    bool sendSighting(Form* form, const QString& sightingUid);
};
