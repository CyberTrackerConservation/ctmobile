#pragma once
#include "pch.h"
#include "ProviderInterface.h"

constexpr char GOOGLE_PROVIDER[] = "Google";

class GoogleProvider : public Provider
{
    Q_OBJECT
    QML_READONLY_AUTO_PROPERTY(QString, parserError)
    QML_READONLY_AUTO_PROPERTY(QVariantMap, settings)

public:
    GoogleProvider(QObject *parent = nullptr);

    bool connectToProject(bool newBuild, bool* formChangedOut) override;

    bool requireUsername() const override;

    QUrl getStartPage() override;
    void getElements(ElementManager* elementManager) override;
    void getFields(FieldManager* fieldManager) override;
    
    QUrlQuery buildSighting(Sighting* sighting);
    void submitData() override;

private:
    void parseForm(const QString& formFilePath, const QString& targetFolder);
};
