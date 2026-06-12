#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "Form.h"

namespace CyberTracker {

class ODKProvider : public Provider
{
    Q_OBJECT
    QML_READONLY_AUTO_PROPERTY(QString, parserError)

public:
    static constexpr char PROVIDER_NAME[] = "ODK";

public:
    ODKProvider(Form* form);

    bool connectToProject() override;

    bool requireUsername() const override;

    bool supportLocationTrackField() const override;

    QString getStartPage() const override;

    bool finalizePackage(const QString& packageFilesPath) const override;

    bool canSubmitData() const override;
    void submitData() override;
};

} // namespace CyberTracker