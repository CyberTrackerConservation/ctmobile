#pragma once
#include "pch.h"
#include "ProviderInterface.h"

namespace CyberTracker {

class NativeProvider : public Provider
{
    Q_OBJECT
    QML_READONLY_AUTO_PROPERTY(QString, parserError)

public:
    static constexpr char PROVIDER_NAME[] = "Native";

public:
    NativeProvider(Form* form);

    bool connectToProject() override;

    QString getStartPage() const override;
    bool finalizePackage(const QString& packageFilesPath) const override;

    bool supportLocationTrackField() const override;

    bool canSubmitData() const override;
    void submitData() override;
};

} // namespace CyberTracker