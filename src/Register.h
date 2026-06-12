#pragma once
#include "pch.h"

namespace CyberTracker {

void registerEngine(const QString& rootPath, QGuiApplication* guiApplication, QQmlApplicationEngine* qmlEngine, const QVariantMap& config);
void registerConnectors(QQmlEngine* engine);
void registerProviders(QQmlEngine* engine);
void registerRenderers(QQmlEngine* engine);

} // namespace CyberTracker