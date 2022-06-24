#pragma once
#include "pch.h"

class MBTilesReader : public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (QString, cachePath)

public:
    MBTilesReader(QObject* parent = nullptr);
    ~MBTilesReader();

    Q_INVOKABLE void reset();

    Q_INVOKABLE QString getQml(const QString& filePath);
    Q_INVOKABLE QUrl getTile(const QString& filePath, int level, int row, int column);
    Q_INVOKABLE void close(const QString& filePath);

private:
    QMap<QString, QSqlDatabase> m_dbs;
    QMap<QString, QString> m_cachePaths;

    struct LodBounds
    {
        int colMin, colMax, rowMin, rowMax;
    };

    QMap<QString, QMap<int, LodBounds>> m_lodBounds;

    int m_gcCounter = 0;
    void garbageCollect();
};
