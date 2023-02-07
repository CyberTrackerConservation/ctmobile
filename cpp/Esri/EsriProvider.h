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

    bool connectToProject(bool newBuild, bool* formChangedOut) override;

    bool requireUsername() const override;

    bool supportLocationPoint() const override;
    bool supportLocationTrack() const override;

    QUrl getStartPage() override;
    void getElements(ElementManager* elementManager) override;
    void getFields(FieldManager* fieldManager) override;

    bool notify(const QVariantMap& message) override;

    bool finalizePackage(const QString& packageFilesPath) const override;

    void submitData() override;

private:
    void setFieldTags(const QString& serviceInfoFilePath, const QString& fieldsFilePath);
    QVariantList buildSightingPayload(Sighting* sighting, QVariantMap* attachmentsMapOut);
    QVariantMap buildLocationPayload(const QString& uid, const QVariantMap& location) const;
    bool sendSighting(Sighting* sighting);
    void sendLocations(const QStringList& locationUids, const QVariantList& payloadItems);
    void sendLastLocation(Location* location);
    void sendTrackFile(Sighting* sighting);
};
