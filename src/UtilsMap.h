#pragma once
#include "pch.h"

namespace Esri {
namespace ArcGISRuntime {
class Layer;
class GraphicsOverlay;
class Symbol;
class HeatmapRenderer;
} // namespace ArcGISRuntime
} // namespace Esri

namespace CyberTracker {
namespace Utils {

Esri::ArcGISRuntime::Layer* createMBTilesLayer(const QString& filePath, QObject* parent = nullptr);
Esri::ArcGISRuntime::Layer* createECWLayer(const QString& filePath, QObject* parent = nullptr);
Esri::ArcGISRuntime::Layer* createGeoJsonLayer(const QString& filePath, QObject* parent = nullptr);
void appendPointsToOverlay(Esri::ArcGISRuntime::GraphicsOverlay* overlay, const QVariantMap& layer);
void appendLinesToOverlay(Esri::ArcGISRuntime::GraphicsOverlay* overlay, const QVariantMap& layer);
void appendLineToOverlay(Esri::ArcGISRuntime::GraphicsOverlay* overlay, double x1, double y1, double x2, double y2, double width, const QColor& color);
void appendPointToOverlay(Esri::ArcGISRuntime::GraphicsOverlay* overlay, double x, double y, int size, const QString& url);

void processProjections();

QString getLocationText(const QVariantMap& projection, double x, double y);
QVariantMap projectLocation(const QVariantMap& projection, double x, double y);
QVariantMap unprojectLocation(const QVariantMap& projection, double a, double b);

Esri::ArcGISRuntime::Symbol* createPictureMarkerSymbol(const QString& svg, int size, QObject* parent = nullptr);
Esri::ArcGISRuntime::Symbol* createTrackArrowSymbol(int lineWidth, int arrowSize, const QColor& color, QObject* parent = nullptr);
Esri::ArcGISRuntime::HeatmapRenderer* createHeatmapRenderer(int blurRadius, double minIntensity, double maxIntensity, QObject* parent = nullptr);

}

} // namespace CyberTracker
