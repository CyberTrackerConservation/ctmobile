#pragma once
#include "pch.h"
#include "Location.h"

namespace CyberTracker {

struct TrackFileMetadata
{
    QString deviceId;
    QString username;
    int interval = 0;
    int distanceFilter = 0;
    int accuracyFilter = 0;
};

struct TrackFileInfo
{
    TrackFileMetadata metadata;
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

    void reset();

    QString moveToMedia();

    void compact();

    QString summary() const;
    QVariantList getPoints() const;

    TrackFileInfo getInfo() const;
    void setInfo(const TrackFileInfo& info);

    TrackFileInfo rebuildInfo(const TrackFileMetadata& metadata);

    void addLocation(const QString& uid, Location* location);
    bool addLocationBatch(std::function<void()> callback);

    static void enumLocations(const QString& filePath, std::function<void(qint64 t, double x, double y, int counter)> callback);

    void enumLocations(const std::function<void(const QString& uid, Location* location)>& callback) const;
    void enumLocations(std::function<void(double x, double y, double z)> callback) const;

    bool hasLocations() const;

    bool exportKml(const QString& filePath) const;
    bool exportKmz(const QString& filePath) const;
    bool exportGeoJson(const QString& filePath) const;
    QVariantList createEsriFeatures(const QString& globalId, const QString& fullName) const;

    QString exportFile(const QString& format, bool compress);

private:
    QString m_connectionName;
    QSqlQuery* m_batchQuery = nullptr;

    void init();
    void close();
    static void migrateTimestamp(QSqlDatabase& db);
    QVariantMap infoToMap(const TrackFileInfo& info) const;
    TrackFileInfo mapToInfo(const QVariantMap& map) const;
};

} // namespace CyberTracker