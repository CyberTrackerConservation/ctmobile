#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "Form.h"

namespace CyberTracker {

class EarthRangerProvider : public Provider
{
    Q_OBJECT

public:
    static constexpr char PROVIDER_NAME[] = "EarthRanger";

public:
    EarthRangerProvider(Form* form);

    bool connectToProject() override;

    QString getStartPage() const override;

    bool finalizePackage(const QString& packageFilesPath) const override;

    Q_INVOKABLE QString banner() const;

private:
    bool sendSighting(Form* form, const QString& sightingUid);
};

} // namespace CyberTracker