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

    QUrl getStartPage() override;
};
