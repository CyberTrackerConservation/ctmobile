#pragma once
#include "pch.h"
#include "ProviderInterface.h"
#include "Form.h"

constexpr char ODK_PROVIDER[] = "ODK";

class ODKProvider : public Provider
{
    Q_OBJECT
    QML_READONLY_AUTO_PROPERTY(QString, parserError)
    QML_READONLY_AUTO_PROPERTY(QVariantMap, settings)

public:
    ODKProvider(QObject *parent = nullptr);

    bool connectToProject(bool newBuild, bool* formChangedOut) override;

    bool requireUsername() const override;

    QUrl getStartPage() override;
    void getElements(ElementManager* elementManager) override;
    void getFields(FieldManager* fieldManager) override;

    bool notify(const QVariantMap& message) override;
    
    bool finalizePackage(const QString& packageFilesPath) const override;

    bool canSubmitData() const override;
    void submitData() override;
};
