#pragma once
#include "pch.h"

void registerEngine(QGuiApplication* guiApplication, QQmlApplicationEngine* qmlEngine, const QVariantMap& config);
void registerConnectors(QQmlEngine* engine);
void registerProviders(QQmlEngine* engine);
