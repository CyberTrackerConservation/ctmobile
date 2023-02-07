#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "Form.h"

constexpr char EARTH_RANGER_PROVIDER[] = "EarthRanger";

class EarthRangerProvider : public Provider
{
    Q_OBJECT

public:
    EarthRangerProvider(QObject *parent = nullptr);

    bool connectToProject(bool newBuild, bool* formChangedOut) override;

    QUrl getStartPage() override;
    void getElements(ElementManager* elementManager) override;
    void getFields(FieldManager* fieldManager) override;

    QString getFieldName(const QString& fieldUid) const override;
    bool getFieldTitleVisible(const QString& fieldUid) const override;
    QString getSightingSummaryText(Sighting* sighting) const override;
    QUrl getSightingSummaryIcon(Sighting* sighting) const override;
    QUrl getSightingStatusIcon(Sighting* sighting, int flags) const override;

    QVariantList buildMapDataLayers() const override;

    bool notify(const QVariantMap& message) override;

    bool finalizePackage(const QString& packageFilesPath) const override;

    Q_INVOKABLE QString banner() const;

private:
    std::atomic_int m_runningUid;

    void parseReportedBy();
    void parseEventTypes();
    bool parseField(const QString& reportUid, RecordField* recordField, const QJsonObject& jsonField, QString* keyOut);
    QJsonObject resolveForm(const QJsonValue& form, const QJsonObject& props);
    void parseReport(Element* reportElement);
    bool sendSighting(Form* form, const QString& sightingUid);
};
