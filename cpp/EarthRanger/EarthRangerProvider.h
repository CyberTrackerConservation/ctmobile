#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "Form.h"

constexpr char EARTH_RANGER_PROVIDER[] = "EarthRanger";

class EarthRangerProvider : public Provider
{
    Q_OBJECT
    Q_PROPERTY (QString reportedBy READ reportedBy WRITE setReportedBy NOTIFY reportedByChanged)

public:
    EarthRangerProvider(QObject *parent = nullptr);

    bool connectToProject(bool newBuild) override;

    QUrl getStartPage() override;
    void getElements(ElementManager* elementManager) override;
    void getFields(FieldManager* fieldManager) override;

    QString getFieldName(const QString& fieldUid) override;
    bool getFieldTitleVisible(const QString& fieldUid) override;

    bool notify(const QVariantMap& message) override;

    bool finalizePackage(const QString& packageFilesPath) const override;

signals:
    void reportedByChanged();

private:
    std::atomic_int m_runningUid;

    QString reportedBy();
    void setReportedBy(const QString& value);

    void parseReportedBy();
    void parseEventTypes();
    bool parseField(const QString& reportUid, RecordField* recordField, const QJsonObject& jsonField, QString* keyOut);
    QJsonObject resolveForm(const QJsonValue& form, const QJsonObject& props);
    void parseReport(Element* reportElement);
    bool sendSighting(Form* form, const QString& sightingUid);
};
