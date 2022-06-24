#pragma once
#include "pch.h"

class GotoManager: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, gotoTitle)
    QML_READONLY_AUTO_PROPERTY (QVariantList, gotoPath)
    QML_READONLY_AUTO_PROPERTY (int, gotoPathIndex)
    QML_READONLY_AUTO_PROPERTY (QString, gotoTitleText)
    QML_READONLY_AUTO_PROPERTY (QVariantMap, gotoLocation)
    QML_READONLY_AUTO_PROPERTY (int, gotoHeading)
    QML_READONLY_AUTO_PROPERTY (QString, gotoLocationText)
    QML_READONLY_AUTO_PROPERTY (QString, gotoDistanceText)
    QML_READONLY_AUTO_PROPERTY (QString, gotoHeadingText)

public:
    GotoManager(QObject* parent = nullptr);
    ~GotoManager();

    Q_INVOKABLE void setTarget(const QString& title, const QVariantList& path, int pathIndex);
    Q_INVOKABLE QVariantList findPath(int id);

    Q_INVOKABLE bool bootstrap(const QString& zipFilePath);

    QVariantList& getLayers();
    void removeLayer(const QString& id);

    void recalculate(const QVariantMap& lastLocation);

    void install(const QVariantMap& layer);

signals:
    void gotoTargetChanged(const QString& title);

private:
    void loadState();
    void saveState();

    QVariantList m_cache;
    QVariantList m_paths;
    double m_symbolScale = 1;
    QVariantMap transform(const QVariantMap& source);
    QVariantMap makePointSymbol(const QVariantMap& style);
    QVariantMap buildLayerPoints(const QVariantMap& source, QRectF* extentOut);
    QVariantMap makeLineSymbol(const QVariantMap& style);
    QVariantList buildLayerPolylines(const QVariantMap& source, QRectF* extentOut);
    bool getZipContents(const QString& zipFile, QVariantMap& contents);
};
