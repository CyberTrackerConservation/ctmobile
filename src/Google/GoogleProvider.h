#pragma once
#include "pch.h"
#include "ProviderInterface.h"

namespace CyberTracker {

class GoogleProvider : public Provider
{
    Q_OBJECT
    QML_READONLY_AUTO_PROPERTY(QString, parserError)

public:
    static constexpr char PROVIDER_NAME[] = "Google";

public:
    GoogleProvider(Form* form);

    bool connectToProject() override;

    bool requireUsername() const override;

    QString getStartPage() const override;
    
    QUrlQuery buildSighting(Sighting* sighting) const;
    void submitData() override;

private:
    void parseForm(const QString& formFilePath, const QString& targetFolder);
};

} // namespace CyberTracker