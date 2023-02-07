#pragma once
#include "pch.h"
#include "ProviderInterface.h"

constexpr char CLASSIC_PROVIDER[] = "Classic";

class ClassicProvider : public Provider
{
    Q_OBJECT

public:
    ClassicProvider(QObject *parent = nullptr);

    static int version();

    bool connectToProject(bool newBuild, bool* formChangedOut) override;

    QUrl getStartPage() override;
    void getElements(ElementManager* elementManager) override;
    void getFields(FieldManager* fieldManager) override;
};
