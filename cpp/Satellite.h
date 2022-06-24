#pragma once
#include "pch.h"

enum class SatelliteType
{
    Unknown = 0,
    Navstar,
    Sbas,
    Glonass,
    Qzss,
    Beidou,
    Galileo
};

struct Satellite
{
    int id = 0;
    qint64 timestamp = 0;
    SatelliteType type = SatelliteType::Unknown;
    bool used = false;
    float elevation = 0;
    float strength = 0;
    float azimuth = 0;
    int index = 0;
};

class SatelliteManager: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY(int, signalStrength)

public:
    SatelliteManager(QObject* parent = nullptr);
    ~SatelliteManager();

    bool active();
    void start();
    void stop();

    int signalStrength();
    QList<Satellite> snapSatellites();

signals:
    void satellitesChanged();

private slots:
    void satellitesInViewUpdated(const QList<QGeoSatelliteInfo>& infos);
    void satellitesInUseUpdated(const QList<QGeoSatelliteInfo>& infos);

private:
    QMap<int, Satellite> m_satelliteMap;
    int m_refCount = 0;
    QGeoSatelliteInfoSource* m_source = nullptr;

    SatelliteType getTypeFromId(int id);
    void update();
};
