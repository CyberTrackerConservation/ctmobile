#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "Form.h"

namespace CyberTracker {

class EsriProvider : public Provider
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY(QString, parserError)
    QML_READONLY_AUTO_PROPERTY(QString, locationServiceUrl)
    QML_READONLY_AUTO_PROPERTY(QString, locationServicePage)

public:
    static constexpr char PROVIDER_NAME[] = "Esri";

public:
    EsriProvider(Form* form);

    bool connectToProject() override;

    bool requireUsername() const override;

    bool supportLocationTrackField() const override;
    bool supportLocationTrackService() const override;

    QString getStartPage() const override;

    bool finalizePackage(const QString& packageFilesPath) const override;

    void submitData() override;

private:
    bool ensureServiceState();
    QVariantList buildSightingPayload(Sighting* sighting, QVariantMap* attachmentsMapOut);
    QVariantMap buildLocationPayload(const QString& uid, Location* location) const;
    bool sendSighting(Sighting* sighting);
    void sendLastLocation(Location* location);
    void sendTrackFile(Sighting* sighting);
};

} // namespace CyberTracker