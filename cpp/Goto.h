#pragma once
#include "pch.h"
#include "Location.h"

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

    void recalculate(Location* location);

signals:
    void gotoTargetChanged(const QString& title);

private:
    void loadState();
    void saveState();
};
