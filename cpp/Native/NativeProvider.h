#pragma once
#include "pch.h"
#include "ProviderInterface.h"

constexpr char NATIVE_PROVIDER[] = "Native";

class NativeProvider : public Provider
{
    Q_OBJECT

public:
    NativeProvider(QObject *parent = nullptr);

    QUrl getStartPage() override;
    void getElements(ElementManager* elementManager) override;
    void getFields(FieldManager* fieldManager) override;
};
