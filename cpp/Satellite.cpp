#include "Satellite.h"
#include "App.h"
#include <QDateTime>

SatelliteManager::SatelliteManager(QObject* parent): QObject(parent)
{
    m_signalStrength = 0;
}

SatelliteManager::~SatelliteManager()
{
    while (m_refCount > 0)
    {
        stop();
    }
}

bool SatelliteManager::active()
{
    return m_refCount > 0;
}

void SatelliteManager::start()
{
    m_refCount++;
    if (m_refCount == 1)
    {
        m_source = new SatelliteInfoSource(this);

        if (m_source)
        {
            connect(m_source, &QGeoSatelliteInfoSource::satellitesInViewUpdated, this, &SatelliteManager::satellitesInViewUpdated);
            connect(m_source, &QGeoSatelliteInfoSource::satellitesInUseUpdated, this, &SatelliteManager::satellitesInUseUpdated);

            m_source->setUpdateInterval(1000);
            m_source->startUpdates();
        }
    }
}

void SatelliteManager::stop()
{
    m_refCount--;
    if (m_refCount == 0)
    {
        if (m_source)
        {
            m_source->stopUpdates();

            disconnect(m_source, &QGeoSatelliteInfoSource::satellitesInViewUpdated, 0, 0);
            disconnect(m_source, &QGeoSatelliteInfoSource::satellitesInUseUpdated, 0, 0);

            delete m_source;
            m_source = nullptr;
        }

        m_satelliteMap.clear();

        m_signalStrength = 0;
    }
}

int SatelliteManager::signalStrength()
{
    return m_signalStrength;
}

QList<Satellite> SatelliteManager::snapSatellites()
{
    auto result = QList<Satellite>();

    for (auto id: m_satelliteMap.keys())
    {
        result.append(m_satelliteMap[id]);
    }

    // Re-order according to how the GPS provided them.
    std::sort(result.begin(), result.end(),
        [&](Satellite& s1, Satellite& s2) -> bool
        {
            return s1.index < s2.index;
        });

    return result;
}

SatelliteType SatelliteManager::getTypeFromId(int id)
{
    if (id >= 1 && id <= 32)
    {
        return SatelliteType::Navstar;
    }

    if (id == 33)
    {
        return SatelliteType::Sbas;
    }

    if (id == 39)
    {
        return SatelliteType::Sbas;
    }

    if (id >= 40 && id <= 41)
    {
        return SatelliteType::Sbas;
    }

    if (id == 46)
    {
        return SatelliteType::Sbas;
    }

    if (id == 48)
    {
        return SatelliteType::Sbas;
    }

    if (id == 49)
    {
        return SatelliteType::Sbas;
    }

    if (id == 51)
    {
        return SatelliteType::Sbas;
    }

    if (id >= 65 && id <= 96)
    {
        return SatelliteType::Glonass;
    }

    if (id >= 193 && id <= 200)
    {
        return SatelliteType::Qzss;
    }

    if (id >= 201 && id <= 235)
    {
        return SatelliteType::Beidou;
    }

    if (id >= 301 && id <= 336)
    {
        return SatelliteType::Galileo;
    }

    return SatelliteType::Unknown;
}

void SatelliteManager::satellitesInViewUpdated(const QList<QGeoSatelliteInfo>& infos)
{
    m_source->setUpdateInterval(3000);

    if (App::instance()->settings()->simulateGPS())
    {
        m_satelliteMap.clear();
    }

    auto timestamp = QDateTime::currentSecsSinceEpoch();
    auto index = 0;
    for (auto info: infos)
    {
        auto id = info.satelliteIdentifier();

        auto satellite = m_satelliteMap[id];
        satellite.id = id;
        satellite.timestamp = timestamp;
        satellite.elevation = info.attribute(QGeoSatelliteInfo::Elevation);
        satellite.strength = info.signalStrength();
        satellite.azimuth = info.attribute(QGeoSatelliteInfo::Azimuth);
        satellite.index = index++;

        if (satellite.type == SatelliteType::Unknown)
        {
            satellite.type = getTypeFromId(id);
        }

        m_satelliteMap[id] = satellite;
    }
}

void SatelliteManager::satellitesInUseUpdated(const QList<QGeoSatelliteInfo>& infos)
{
    for (auto id: m_satelliteMap.keys())
    {
        m_satelliteMap[id].used = false;
    }

    for (auto info: infos)
    {
        auto id = info.satelliteIdentifier();
        m_satelliteMap[id].used = true;
    }

    update();
}

void SatelliteManager::update()
{
    auto currTime = QDateTime::currentSecsSinceEpoch();
    auto sumStrength = 0;
    auto sumCount = 0;

    for (auto id: m_satelliteMap.keys())
    {
        auto lastTime = m_satelliteMap[id].timestamp;
        auto timeDelta = currTime - lastTime;
        if (timeDelta < 6)
        {
            sumStrength += m_satelliteMap[id].strength;
            sumCount++;
        }
        else if (timeDelta < 30)
        {
            m_satelliteMap[id].strength = 0;
        }
        else
        {
            m_satelliteMap.remove(id);
        }
    }

    auto meanStrength = sumCount == 0 ? 0 : sumStrength / sumCount;
    if (m_signalStrength != meanStrength)
    {
        update_signalStrength(meanStrength);
    }

    emit satellitesChanged();
}
