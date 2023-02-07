#pragma once
#include "pch.h"

struct TrackFileInfo
{
    QString deviceId;
    QString username;
    int count = 0;
    QRectF extent;
    double speedMin = 10000;
    double speedMax = 0;
    double speedAvg = 0;
    double accuracyMin = 100000;
    double accuracyMax = 0;
    double accuracyAvg = 0;
    QString timeStart;
    QString timeStop;
    int timeTotal = 0;
    double speedTotal = 0;
    double accuracyTotal = 0;
    double distance = 0;
};

class TrackFile: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, filePath)

public:
    explicit TrackFile(QObject* parent = nullptr);
    explicit TrackFile(const QString& filePath, QObject* parent = nullptr);
    virtual ~TrackFile();

    void compact();

    TrackFileInfo getInfo() const;
    void setInfo(const TrackFileInfo& info);

    void batchAddInit(const QString& deviceId, const QString& username);
    void batchAddLocation(const QString& uid, const QVariantMap& locationMap);
    int batchAddDone();

    void enumLocations(const std::function<void(const QString& uid, const QVariantMap& locationMap)>& callback) const;

    bool exportKml(const QString& filePath) const;
    bool exportKmz(const QString& filePath) const;
    bool exportGeoJson(const QString& filePath) const;
    QVariantList createEsriFeatures(const QString& globalId, const QString& fullName) const;

    QString exportFile(const QString& format, bool compress);

private:
    QSqlDatabase m_db;
    QString m_connectionName;
    double m_batchLastX = 0;
    double m_batchLastY = 0;
    TrackFileInfo m_batchInfo;

    void verifyThread() const;
    QVariantMap infoToMap(const TrackFileInfo& info) const;
    TrackFileInfo mapToInfo(const QVariantMap& map) const;
};
