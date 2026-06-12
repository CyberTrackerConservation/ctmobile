#include "Location.h"
#include "App.h"
#include "tinygps/TinyGPSPlus.h"

Q_IMPORT_PLUGIN(LocationFactory)

using namespace CyberTracker;

namespace {

    static constexpr int LOG_STREAMER_QUEUE_QUOTA = 8192;

    TimeManager* timeManager()
    {
        return App::instance()->timeManager();
    }

    bool simulateGPS()
    {
        return App::instance()->settings()->simulateGPS();
    }

    double accuracyFilter()
    {
        return App::instance()->settings()->locationAccuracyMeters();
    }

    int speedFilter()
    {
        auto result = App::instance()->settings()->locationOutlierMaxSpeedKph();
        return result != 0 ? result : 1500; // Max speed by default.
    }

    LocationLogger* locationLogger()
    {
        return App::instance()->locationLogger();
    }

    auto PLATFORM_SOURCE_NAME =
    #if defined(Q_OS_WIN)
        "winrt"
    #elif defined(Q_OS_ANDROID)
        "android"
    #elif defined(Q_OS_IOS) || defined(Q_OS_MACOS)
        "corelocation"
    #else
        "nmea"
    #endif
        ;

    struct LoggerLine
    {
        char uid[16];
        double x = std::numeric_limits<double>::quiet_NaN();
        double y = std::numeric_limits<double>::quiet_NaN();
        double z = std::numeric_limits<double>::quiet_NaN();
        qint64 t = 0;
        double accuracy = std::numeric_limits<double>::quiet_NaN();
        double speed = std::numeric_limits<double>::quiet_NaN();
        double direction = std::numeric_limits<double>::quiet_NaN();
        qint32 satelliteCount = 0;
        qint32 flags = 0;

        enum Flags
        {
            LOCATION_NO_FLAG         = 0,
            LOCATION_STATIONARY_FLAG = 1
        };
    };
}

namespace CyberTracker {

//=================================================================================================
// LocationFactory

QGeoPositionInfoSource* LocationFactory::positionInfoSource(QObject* parent, const QVariantMap& /*parameters*/)
{
    return new PositionInfoSource(parent);
}

QGeoSatelliteInfoSource* LocationFactory::satelliteInfoSource(QObject* parent, const QVariantMap& /*parameters*/)
{
    return new SatelliteInfoSource(parent);
}

QGeoAreaMonitorSource* LocationFactory::areaMonitor(QObject* /*parent*/, const QVariantMap& /*parameters*/)
{
    return nullptr;
}

//=================================================================================================
// SimulateNmeaServer

namespace {
SimulateNmeaServer* s_simulateNmeaServer = nullptr;
static int s_simulateIntervalMultiplier = 1;
static bool s_simulateFromTrackLog = false;
}

SimulateNmeaServer::SimulateNmeaServer(QObject* parent) : QObject(parent)
{
    s_simulateNmeaServer = this;

    connect(&m_timer, &QTimer::timeout, this, &SimulateNmeaServer::onTimer);
}

SimulateNmeaServer::~SimulateNmeaServer()
{
    stop();

    s_simulateNmeaServer = nullptr;
}

bool SimulateNmeaServer::start(const QString& filePath)
{
    m_positions.clear();
    m_lineCounter = 0;
    m_lineStep = 1;

    auto fileName = QFileInfo(filePath).fileName();
    s_simulateFromTrackLog = fileName.startsWith("TrackLog") && fileName.endsWith(".bin");
    if (s_simulateFromTrackLog)
    {
        auto reader = LocationReader(filePath);
        reader.enumLines(0, [&](int /*index*/, const QString& /*uid*/, qint64 /*t*/, Location* location)
        {
            auto positionInfo = location->toGeoPositionInfo();
            if (positionInfo.isValid())
            {
                // Hack: stick the stationary flag in the VerticalSpeed attribute. Useful for testing.
                positionInfo.setAttribute(QGeoPositionInfo::VerticalSpeed, location->stationary() ? LoggerLine::LOCATION_STATIONARY_FLAG : 0);
                m_positions.append(positionInfo);
            }
        });
    }
    else
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "GPS - Failed to open nmea file";
            return false;
        }

        QString lastLine;
        while (!file.atEnd())
        {
            auto line = QString(file.readLine()).simplified() + "\r\n";
            if (line.startsWith("$GPGGA"))
            {
                if (!lastLine.isEmpty())
                {
                    auto positionInfo = Location(lastLine.toStdString().c_str(), lastLine.length()).toGeoPositionInfo();
                    if (positionInfo.isValid())
                    {
                        positionInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
                        m_positions.append(positionInfo);
                    }
                    lastLine.clear();
                }
            }

            lastLine += line;
        }

        if (!lastLine.isEmpty())
        {
            auto positionInfo = Location(lastLine.toStdString().c_str(), lastLine.length()).toGeoPositionInfo();
            if (positionInfo.isValid())
            {
                positionInfo.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
                m_positions.append(positionInfo);
            }
        }
    }

    m_timer.start(1000 / s_simulateIntervalMultiplier);

    return true;
}

void SimulateNmeaServer::stop()
{
    m_timer.stop();
    m_positions.clear();
    m_lineCounter = 0;
    m_lineStep = 1;
}

void SimulateNmeaServer::onTimer()
{
    if (m_positions.isEmpty())
    {
        return;
    }

    // Get the position and advance the counter.
    auto positionInfo = m_positions[m_lineCounter];
    m_lineCounter += m_lineStep;

    if (m_lineCounter == -1 || m_lineCounter == m_positions.length())
    {
        m_lineStep = -m_lineStep;
        m_lineCounter += m_lineStep;
    }

    // Update the timestamp.
    positionInfo.setTimestamp(QDateTime::currentDateTime());

    // Compute direction and speed.
    if (m_currentPositionInfo.isValid())
    {
        auto bearing = m_currentPositionInfo.coordinate().azimuthTo(positionInfo.coordinate());
        positionInfo.setAttribute(QGeoPositionInfo::Direction, bearing);

        auto distance = m_currentPositionInfo.coordinate().distanceTo(positionInfo.coordinate());
        auto timeDiff = m_currentPositionInfo.timestamp().msecsTo(positionInfo.timestamp());
        if (timeDiff > 0)
        {
            auto speed = (distance * 1000) / (timeDiff * s_simulateIntervalMultiplier);
            positionInfo.setAttribute(QGeoPositionInfo::GroundSpeed, speed);
        }
    }

    m_currentPositionInfo = positionInfo;
}

QGeoPositionInfo SimulateNmeaServer::currentPositionInfo() const
{
    return m_currentPositionInfo;
}

//=================================================================================================
// SimulatePositionInfoSource

class SimulatePositionInfoSource : public QGeoPositionInfoSource
{
public:
    explicit SimulatePositionInfoSource(QObject* parent = nullptr): QGeoPositionInfoSource(parent)
    {
        m_timer.setInterval(1000 / s_simulateIntervalMultiplier);
        connect(&m_timer, &QTimer::timeout, this, &SimulatePositionInfoSource::onTimer);
    }

    ~SimulatePositionInfoSource() override
    {
    }

    QGeoPositionInfo lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const override
    {
        return m_lastPosition;
    }

    void setPreferredPositioningMethods(QGeoPositionInfoSource::PositioningMethods /*methods*/) override
    {
    }

    PositioningMethods supportedPositioningMethods() const override
    {
        return QGeoPositionInfoSource::SatellitePositioningMethods;
    }

    void setUpdateInterval(int msec) override
    {
        m_timer.setInterval(msec / s_simulateIntervalMultiplier);
    }

    int minimumUpdateInterval() const override
    {
        return 1000 / s_simulateIntervalMultiplier;
    }

    QGeoPositionInfoSource::Error error() const override
    {
        return QGeoPositionInfoSource::NoError;
    }

public slots:
    void startUpdates() override
    {
        m_timer.start();
    }

    void stopUpdates() override
    {
        m_timer.stop();
    }

    void requestUpdate(int timeout = 0) override
    {
        QTimer::singleShot(timeout, this, &SimulatePositionInfoSource::onTimer);
    }

    void onTimer()
    {
        m_lastPosition = s_simulateNmeaServer->currentPositionInfo();
        emit positionUpdated(m_lastPosition);
    }

private:
    QTimer m_timer;
    QGeoPositionInfo m_lastPosition;
};

//=================================================================================================
// LoggerPositionInfoSource

class LoggerPositionInfoSource : public QGeoPositionInfoSource
{
public:
    explicit LoggerPositionInfoSource(QObject* parent = nullptr): QGeoPositionInfoSource(parent)
    {
        m_reader = new LocationReader(App::instance()->trackLogFilePath());
        m_timer.setInterval(1000);
        connect(&m_timer, &QTimer::timeout, this, &LoggerPositionInfoSource::onTimer);
    }

    ~LoggerPositionInfoSource() override
    {
        delete m_reader;
        m_reader = nullptr;
    }

    QGeoPositionInfo lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const override
    {
        return m_lastPosition;
    }

    void setPreferredPositioningMethods(QGeoPositionInfoSource::PositioningMethods /*methods*/) override
    {
    }

    PositioningMethods supportedPositioningMethods() const override
    {
        return QGeoPositionInfoSource::SatellitePositioningMethods;
    }

    void setUpdateInterval(int msec) override
    {
        m_timer.setInterval(msec);
    }

    int minimumUpdateInterval() const override
    {
        return 1000;
    }

    QGeoPositionInfoSource::Error error() const override
    {
        return QGeoPositionInfoSource::NoError;
    }

public slots:
    void startUpdates() override
    {
        m_lastLine = -1;
        m_timer.start();
    }

    void stopUpdates() override
    {
        m_timer.stop();
    }

    void requestUpdate(int timeout = 0) override
    {
        QTimer::singleShot(timeout, this, &LoggerPositionInfoSource::onTimer);
    }

    void onTimer()
    {
        auto lastLine = m_reader->lineCount() - 1;
        if (lastLine < 0)
        {
            // No lines yet.
            return;
        }

        if (lastLine == m_lastLine)
        {
            // Do not emit the last line multiple times.
            return;
        }

        m_lastLine = lastLine;

        m_reader->enumLines(lastLine, [&](int /*index*/, const QString& /*uid*/, qint64 /*t*/, Location* location)
        {
            m_lastPosition = location->toGeoPositionInfo();
            emit positionUpdated(m_lastPosition);
        });
    }

private:
    QTimer m_timer;
    QGeoPositionInfo m_lastPosition;
    LocationReader* m_reader = nullptr;
    int m_lastLine = -1;
};

//=================================================================================================
// PositionInfoSource

PositionInfoSource::PositionInfoSource(QObject* parent) : QGeoPositionInfoSource(parent)
{
    m_simulate = simulateGPS();

    if (m_simulate)
    {
        m_source = new SimulatePositionInfoSource(this);
    }
    else if (::locationLogger()->running())
    {
        m_source = new LoggerPositionInfoSource(this);
    }
    else
    {
        qDebug() << "GPS - PositionInfo sources: " << QGeoPositionInfoSource::availableSources();

        m_source = QGeoPositionInfoSource::createSource(PLATFORM_SOURCE_NAME, this);

        if (!m_source)
        {
            qDebug() << "GPS - Position source not found";
            return;
        }

        m_source->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
    }

    // Get last known position timestamp to avoid it being re-used.
    auto lastPosition = m_source->lastKnownPosition(true);
    m_lastKnownTimestamp = lastPosition.isValid() ? lastPosition.timestamp().toMSecsSinceEpoch() : 0;

    // Handle position updates.
    connect(m_source, &QGeoPositionInfoSource::positionUpdated, this, [&](const QGeoPositionInfo& _update)
    {
        // Standard validation.
        if (!_update.isValid() || !_update.timestamp().isValid() || !_update.coordinate().isValid())
        {
            qDebug() << "GPS - positionUpdated: update not valid 1";
            return;
        }

        // Sanity validation.
        auto x = _update.coordinate().longitude();
        auto y = _update.coordinate().latitude();
        auto t = _update.timestamp().toMSecsSinceEpoch();
        if (std::isnan(x) || std::isnan(y) || t < 0)
        {
            qDebug() << "GPS - positionUpdated: update not valid 2";
            return;
        }

        if (x < -180.0 || x > 180.0 || y < -90.0 || y > 90.0)
        {
            qDebug() << "GPS - positionUpdated: update not valid 3";
            return;
        }

        // Skip old readings.
        if (t <= m_lastKnownTimestamp)
        {
            qDebug() << "GPS - positionUpdated: update is old";
            return;
        }

        auto update = _update;

        // Hack to recover bad timestamp on some broken devices.
        if (update.timestamp().date().year() < 2022)
        {
            qDebug() << "GPS - Bad GPS timestamp: " << update.timestamp();
            update.setTimestamp(timeManager()->currentDateTime());
            t = update.timestamp().toMSecsSinceEpoch();
        }
        // Compute time error.
        else
        {
            auto s = update.hasAttribute(QGeoPositionInfo::GroundSpeed) ? update.attribute(QGeoPositionInfo::GroundSpeed) : std::numeric_limits<double>::quiet_NaN();
            timeManager()->computeTimeError(x, y, s, t);
            update.setTimestamp(timeManager()->currentDateTime());
        }

        // Skip readings if conditions are not met.
        if (m_lastUpdate.isValid())
        {
            // Outlier detection if the speed is too fast.
            auto speedFilter = ::speedFilter();
            if (speedFilter != 0)
            {
                auto speedKph = update.hasAttribute(QGeoPositionInfo::GroundSpeed) ? update.attribute(QGeoPositionInfo::GroundSpeed) * 3.6 : 0; // Convert to kph.
                if (speedKph > speedFilter)
                {
                    if (!::simulateGPS())
                    {
                        qDebug() << "GPS - Outlier detected - speed " << speedKph << " is more than " << speedFilter;
                        emit App::instance()->showError(tr("GPS outlier detected"));
                        return;
                    }
                }
            }

            // Time filter.
            auto timeSinceLast = t - m_lastTimestamp + 1000;
            if (timeSinceLast > 0 && timeSinceLast < m_source->updateInterval())
            {
                // On iOS, the update interval seems to always be 1 second. Suspect that this is because the Qt iOS GPS interface is a singleton.
            #if !defined(Q_OS_IOS)
                qDebug() << "GPS - Time filter used - timeSinceLast = " << timeSinceLast << " updateInterval = " << m_source->updateInterval();
            #endif
                return;
            }
        }

        // Success.
        m_lastUpdate = update;
        m_lastTimestamp = t;

        // Tell the app first, so it can compute declination, etc.
        App::instance()->positionUpdated(update);

        // Publish update to listeners.
        emit positionUpdated(update);
    });

    connect(m_source, &QGeoPositionInfoSource::errorOccurred, this, &PositionInfoSource::errorOccurred);
}

PositionInfoSource::~PositionInfoSource()
{
    if (!m_source)
    {
        return;
    }

    if (!m_started)
    {
        return;
    }

    disconnect(m_source, &QGeoPositionInfoSource::positionUpdated, 0, 0);
}

void PositionInfoSource::startUpdates()
{
    if (!m_source)
    {
        return;
    }

    if (m_started)
    {
        return;
    }

    m_lastUpdate = QGeoPositionInfo();
    m_lastTimestamp = 0;

    m_started = true;
    m_source->startUpdates();
}

void PositionInfoSource::stopUpdates()
{
    if (!m_source)
    {
        return;
    }

    if (!m_started)
    {
        return;
    }

    m_source->stopUpdates();
    m_started = false;
}

void PositionInfoSource::requestUpdate(int timeout)
{
    if (!m_source)
    {
        return;
    }

    m_source->requestUpdate(timeout);
}

QGeoPositionInfo PositionInfoSource::lastKnownPosition(bool fromSatellitePositioningMethodsOnly) const
{
    if (!m_source)
    {
        return QGeoPositionInfo();
    }

    return m_source->lastKnownPosition(fromSatellitePositioningMethodsOnly);
}

void PositionInfoSource::setPreferredPositioningMethods(QGeoPositionInfoSource::PositioningMethods methods)
{
    QGeoPositionInfoSource::setPreferredPositioningMethods(methods);

    if (!m_source)
    {
        return;
    }

    m_source->setPreferredPositioningMethods(methods);
}

QGeoPositionInfoSource::PositioningMethods PositionInfoSource::supportedPositioningMethods() const
{
    if (!m_source)
    {
        return QGeoPositionInfoSource::PositioningMethods();
    }

    return m_source->supportedPositioningMethods();
}

QGeoPositionInfo PositionInfoSource::lastUpdate() const
{
    return m_lastUpdate;
}

qint64 PositionInfoSource::lastTimestamp() const
{
    return m_lastTimestamp;
}

void PositionInfoSource::setUpdateInterval(int msec)
{
    QGeoPositionInfoSource::setUpdateInterval(msec);

    if (!m_source)
    {
        return;
    }

    m_source->setUpdateInterval(msec);
}

int PositionInfoSource::minimumUpdateInterval() const
{
    if (!m_source)
    {
        return 1000;
    }

    return m_source->minimumUpdateInterval();
}

QGeoPositionInfoSource::Error PositionInfoSource::error() const
{
    if (!m_source)
    {
        return NoError;
    }

    return m_source->error();
}

//=================================================================================================
// SatelliteInfoSource

SatelliteInfoSource::SatelliteInfoSource(QObject* parent): QGeoSatelliteInfoSource(parent)
{
    // Simulation.
    m_simulate = simulateGPS();
    if (m_simulate)
    {
        m_simulateTimer.setInterval(1000);
        connect(&m_simulateTimer, &QTimer::timeout, this, &SatelliteInfoSource::simulateData);
        return;
    }

    // Regular source.
    qDebug() << "GPS - SatelliteInfo sources: " << QGeoSatelliteInfoSource::availableSources();

    m_source = QGeoSatelliteInfoSource::createSource(PLATFORM_SOURCE_NAME, this);

    if (!m_source)
    {
        qDebug() << "GPS - Satellite source not found";
        return;
    }

    connect(m_source, &QGeoSatelliteInfoSource::satellitesInViewUpdated, this, &SatelliteInfoSource::satellitesInViewUpdated);
    connect(m_source, &QGeoSatelliteInfoSource::satellitesInUseUpdated, this, &SatelliteInfoSource::satellitesInUseUpdated);
    connect(m_source, &QGeoSatelliteInfoSource::errorOccurred, this, &SatelliteInfoSource::errorOccurred);
}

SatelliteInfoSource::~SatelliteInfoSource()
{
    SatelliteInfoSource::stopUpdates();
}

void SatelliteInfoSource::simulateData()
{
    auto satellitesInView = QList<QGeoSatelliteInfo>();
    auto satellitesInUse = QList<QGeoSatelliteInfo>();

    auto random = QRandomGenerator::global();

    for (auto i = 0; i < 16; i++)
    {
        auto satellite = QGeoSatelliteInfo();
        satellite.setSatelliteIdentifier(1 + random->generateDouble() * 335);
        satellite.setSignalStrength(random->generateDouble() * 75);
        satellite.setSatelliteSystem(QGeoSatelliteInfo::GPS);
        satellite.setAttribute(QGeoSatelliteInfo::Azimuth, random->generateDouble() * 180);
        satellite.setAttribute(QGeoSatelliteInfo::Elevation, random->generateDouble() * 180);
        satellitesInView.append(satellite);

        if (satellite.signalStrength() > 30)
        {
            satellitesInUse.append(satellite);
        }
    }

    emit satellitesInViewUpdated(satellitesInView);
    emit satellitesInUseUpdated(satellitesInUse);
}

QGeoSatelliteInfoSource::Error SatelliteInfoSource::error() const
{
    if (!m_source)
    {
        return NoError;
    }

    return m_source->error();
}

int SatelliteInfoSource::minimumUpdateInterval() const
{
    return !m_source || m_simulate ? 1000 : m_source->minimumUpdateInterval();
}

void SatelliteInfoSource::setUpdateInterval(int msec)
{
    QGeoSatelliteInfoSource::setUpdateInterval(msec);

    if (m_simulate)
    {
        m_simulateTimer.setInterval(msec);
    }
    else if (m_source)
    {
        m_source->setUpdateInterval(msec);
    }
}

void SatelliteInfoSource::requestUpdate(int timeout)
{
    if (m_simulate)
    {
        QTimer::singleShot(timeout == 0 ? 1000 : timeout, this, &SatelliteInfoSource::simulateData);
    }
    else if (m_source)
    {
        m_source->requestUpdate(timeout);
    }
}

void SatelliteInfoSource::startUpdates()
{
    if (m_started)
    {
        return;
    }

    if (m_simulate)
    {
        m_simulateTimer.start();
    }
    else if (m_source)
    {
        m_source->startUpdates();
    }

    m_started = true;
}

void SatelliteInfoSource::stopUpdates()
{
    if (!m_started)
    {
        return;
    }

    if (m_simulate)
    {
        m_simulateTimer.stop();
    }
    else if (m_source)
    {
        m_source->stopUpdates();
    }

    m_started = false;
}

//=================================================================================================
// Location

Location::Location(QObject* parent): QObject(parent)
{
}

Location::Location(const Location& location, QObject* parent): QObject(parent)
{
    m_x = location.m_x;
    m_y = location.m_y;
    m_z = location.m_z;
    m_t = location.m_t;
    m_d = location.m_d;
    m_a = location.m_a;
    m_s = location.m_s;
    m_counter = location.m_counter;
    m_interval = location.m_interval;
    m_distanceFilter = location.m_distanceFilter;
    m_accuracyFilter = location.m_accuracyFilter;
}

Location::Location(const QVariantMap& data, QObject* parent): QObject(parent)
{
    m_x = data.value("x", m_x).toDouble();
    m_y = data.value("y", m_y).toDouble();
    m_z = data.value("z", m_z).toDouble();
    m_t = data.value("t", m_t).toLongLong();
    m_d = data.value("d", m_d).toDouble();
    m_a = data.value("a", m_a).toDouble();
    m_s = data.value("s", m_s).toDouble();
    m_counter = data.value("c", m_counter).toInt();
    m_interval = data.value("interval", m_interval).toInt();
    m_distanceFilter = data.value("distanceFilter", m_distanceFilter).toInt();
    m_accuracyFilter = data.value("accuracyFilter", m_accuracyFilter).toInt();

    // Older versions used a string timestamp.
    if (data.contains("ts"))
    {
        set_timestamp(data.value("ts").toString());
    }
}

Location::Location(const QGeoPositionInfo& positionInfo, QObject* parent): QObject(parent)
{
    auto f = [&](QGeoPositionInfo::Attribute attribute) -> double
    {
        return positionInfo.hasAttribute(attribute) ? positionInfo.attribute(attribute) : std::numeric_limits<double>::quiet_NaN();
    };

    m_x = positionInfo.coordinate().longitude();
    m_y = positionInfo.coordinate().latitude();
    m_z = positionInfo.coordinate().altitude();
    m_t = positionInfo.timestamp().toMSecsSinceEpoch();
    m_s = f(QGeoPositionInfo::GroundSpeed);
    m_d = f(QGeoPositionInfo::Direction);
    m_a = f(QGeoPositionInfo::HorizontalAccuracy);

    m_accuracyFilter = ::accuracyFilter();
}

Location::Location(const char* nmeaBuffer, int size, QObject* parent): QObject(parent)
{
    auto gps = TinyGPSPlus();

    auto currData = nmeaBuffer;
    for (auto i = 0; i < size; i++, currData++)
    {
        gps.encode(*currData);
    }

    if (gps.location.isValid())
    {
        m_x = gps.location.lng();
        m_y = gps.location.lat();
        m_z = gps.altitude.isValid() ? gps.altitude.meters() : 0;
        m_t = timeManager()->currentDateTime().toMSecsSinceEpoch();
        m_s = gps.speed.isValid() ? gps.speed.kmph() : std::numeric_limits<double>::quiet_NaN();
        m_d = gps.course.isValid() ? gps.course.deg() : std::numeric_limits<double>::quiet_NaN();
        m_a = 1.0;
    }

    m_accuracyFilter = ::accuracyFilter();
}

bool Location::isValid() const
{
    return m_t != 0;
}

bool Location::isAccurate() const
{
    return !isValid() || std::isnan(m_a) ? false : m_a <= ::accuracyFilter();
}

QVariant Location::toPoint() const
{
    return QVariantList { m_x, m_y, m_z, m_a };
}

QVariantMap Location::toMap() const
{
    return QVariantMap
    {
        { "x", m_x },
        { "y", m_y },
        { "z", m_z },
        { "t", m_t },
        { "d", m_d },
        { "a", m_a },
        { "s", m_s },
        { "ts", timestamp() },
        { "c", m_counter },
        { "interval", m_interval },
        { "distanceFilter", m_distanceFilter },
        { "accuracyFilter", m_accuracyFilter },
    };
}

QGeoPositionInfo Location::toGeoPositionInfo() const
{
    auto result = QGeoPositionInfo();
    auto coord = QGeoCoordinate();
    coord.setLatitude(m_y);
    coord.setLongitude(m_x);

    if (!std::isnan(m_z))
    {
        coord.setAltitude(m_z);
    }

    result.setCoordinate(coord);

    auto time = Utils::decodeTimestamp(timestamp()).toUTC();
    result.setTimestamp(time);

    if (!std::isnan(m_a))
    {
        result.setAttribute(QGeoPositionInfo::HorizontalAccuracy, m_a);
    }

    if (!std::isnan(m_s))
    {
        result.setAttribute(QGeoPositionInfo::GroundSpeed, m_s);
    }

    if (!std::isnan(m_d))
    {
        result.setAttribute(QGeoPositionInfo::Direction, m_d);
    }

    return result;
}

double Location::distanceTo(double x, double y) const
{
    return QGeoCoordinate(m_y, m_x).distanceTo(QGeoCoordinate(y, x));
}

double Location::headingTo(double x, double y) const
{
    return Utils::headingInDegrees(m_y, m_x, y, x);
}

double Location::x() const
{
    return m_x;
}

void Location::set_x(double value)
{
    m_x = value;
}

double Location::y() const
{
    return m_y;
}

void Location::set_y(double value)
{
    m_y = value;
}

double Location::z() const
{
    return m_z;
}

void Location::set_z(double value)
{
    m_z = value;
}

qint64 Location::t() const
{
    return m_t;
}

void Location::set_t(qint64 value)
{
    m_t = value;
}

double Location::direction() const
{
    return m_d;
}

void Location::set_direction(double value)
{
    m_d = value;
}

double Location::accuracy() const
{
    return m_a;
}

void Location::set_accuracy(double value)
{
    m_a = value;
}

double Location::speed() const
{
    return m_s;
}

void Location::set_speed(double value)
{
    m_s = value;
}

bool Location::stationary() const
{
    return m_stationary;
}

void Location::set_stationary(bool value)
{
    m_stationary = value;
}

QString Location::timestamp() const
{
    return Utils::encodeTimestamp(QDateTime::fromMSecsSinceEpoch(m_t, Utils::detectTimeZone(m_x, m_y)));
}

void Location::set_timestamp(const QString& isoDateTime)
{
    m_t = Utils::decodeTimestampMSecs(isoDateTime);
}

int Location::counter() const
{
    return m_counter;
}

void Location::set_counter(int value)
{
    m_counter = value;
}

int Location::interval() const
{
    return m_interval;
}

void Location::set_interval(int value)
{
    m_interval = value;
}

int Location::distanceFilter() const
{
    return m_distanceFilter;
}

void Location::set_distanceFilter(int value)
{
    m_distanceFilter = value;
}

int Location::accuracyFilter() const
{
    return m_accuracyFilter;
}

void Location::set_accuracyFilter(int value)
{
    m_accuracyFilter = value;
}

//=================================================================================================
// Location log file.

//=================================================================================================
// LocationReader

LocationReader::LocationReader(const QString& filePath)
{
    QFile(filePath).open(QIODevice::NewOnly);
    qFatalIf(!QFile::exists(filePath), "Failed to create log file: " + filePath);

    m_reader.setFileName(filePath);
    qFatalIf(!m_reader.open(QIODevice::ReadOnly), "Failed to open log file for reading: " + filePath);
}

int LocationReader::lineCount() const
{
    return static_cast<uint>(m_reader.size() / sizeof(LoggerLine));
}

void LocationReader::enumLines(int startIndex, std::function<void(int index, const QString& uid, qint64 t, Location* location)> callback)
{
    if (!m_reader.seek(startIndex * sizeof(LoggerLine)))
    {
        qDebug() << "Bad line index: " << startIndex << lineCount();
        return;
    }

    auto lastIndex = lineCount();
    auto line = LoggerLine();
    Location location;

    for (auto index = startIndex; index < lastIndex; index++)
    {
        if (m_reader.read((char *)&line, sizeof(line)) != sizeof(line))
        {
            qDebug() << "Bad line read index: " << index << lineCount();
            return;
        }

        location.set_x(line.x);
        location.set_y(line.y);
        location.set_z(line.z);
        location.set_t(line.t);
        location.set_accuracy(line.accuracy);
        location.set_speed(line.speed);
        location.set_direction(line.direction);
        location.set_stationary((line.flags & LoggerLine::LOCATION_STATIONARY_FLAG) != 0);

        callback(index, QByteArray(line.uid, 16).toHex(), line.t, &location);
    }
}

//=================================================================================================
// LocationWriter

namespace {
static auto s_locationWriter = (LocationWriter*)nullptr;
static auto s_locationStaleT = (qint64)0;
}
#if defined(Q_OS_ANDROID)
extern "C"
{
JNIEXPORT void JNICALL Java_org_cybertracker_mobile_LocationUpdateService_logMessage(JNIEnv* env, jobject, jstring message)
{
    qDebug() << env->GetStringUTFChars(message, 0);
}

JNIEXPORT void JNICALL Java_org_cybertracker_mobile_LocationUpdateService_locationUpdate(JNIEnv*, jobject, jdouble x, jdouble y, jdouble z, jlong t, jdouble accuracy, jdouble speed, jdouble direction, jint satelliteCount, jboolean stationary)
{
    if (s_locationWriter && (t > s_locationStaleT))
    {
        s_locationStaleT = t;
        s_locationWriter->append(QUuid::createUuid().toString(QUuid::Id128), x, y, z, t, accuracy, speed, direction, satelliteCount, stationary);
    }
}
}
#endif

LocationWriter::LocationWriter(const QString& filePath)
{
    qFatalIf(s_locationWriter != nullptr, "LocationWriter already created");
    s_locationWriter = this;

    QFile(filePath).open(QIODevice::NewOnly);
    qFatalIf(!QFile::exists(filePath), "Failed to create log file: " + filePath);

    m_writer.setFileName(filePath);
    qFatalIf(!m_writer.open(QIODevice::Append), "Failed to open log file for write: " + m_writer.errorString());
}

LocationWriter::~LocationWriter()
{
    s_locationWriter = nullptr;
}

void LocationWriter::reset()
{
    m_writer.resize(0);
    m_writer.flush();
}

void LocationWriter::append(const QString& uid, double x, double y, double z, qint64 t, double accuracy, double speed, double direction, int satelliteCount, bool stationary)
{
    auto line = LoggerLine();

    auto uidByteArray = QByteArray::fromHex(uid.toUtf8());
    memcpy(line.uid, uidByteArray.constData(), 16);

    line.x = x;
    line.y = y;
    line.z = z;
    line.t = t;
    line.speed = speed;
    line.direction = direction;
    line.accuracy = accuracy;
    line.satelliteCount = satelliteCount;
    line.flags = stationary ? LoggerLine::LOCATION_STATIONARY_FLAG : 0;

    // Sanity validation.
    if (std::isnan(line.x) || std::isnan(line.y) || line.t <= 0)
    {
        return;
    }

    // Write log reading.
    if (m_writer.write((const char *)&line, sizeof(line)) != sizeof(line))
    {
        qDebug() << "Error: failed to write to log:" << m_writer.errorString();
    }

    m_writer.flush();
}

//=================================================================================================
// LocationStreamer

LocationStreamer::LocationStreamer(QObject* parent): QObject(parent), m_reader(App::instance()->locationLogger()->filePath())
{
    m_simulate = simulateGPS();
    m_running = false;
    m_updateInterval = 0;
    m_distanceFilter = 0.0;
    m_distanceCovered = 0;
    m_lastUpdateX = m_lastUpdateY = std::numeric_limits<double>::quiet_NaN();
    m_lastUpdateT = 0;
    m_lastUpdateLine = 0;
    m_lastUpdateInterval = 0;
    m_lastDistanceFilter = 0;
    m_counter = 0;

    m_interestUid = Utils::createUuid();

    connect(&m_timer, &QTimer::timeout, this, &LocationStreamer::timeout);
}

LocationStreamer::LocationStreamer(const QString& filename, int updateInterval, double distanceFilter, double distanceCovered, int counter, QObject* parent): QObject(parent), m_reader(filename)
{
    // This is a test hook for replaying log files.
    m_simulate = false;
    m_running = true;
    m_updateInterval = updateInterval;
    m_distanceFilter = distanceFilter;
    m_distanceCovered = distanceCovered;
    m_lastUpdateX = m_lastUpdateY = std::numeric_limits<double>::quiet_NaN();
    m_lastUpdateT = 0;
    m_lastUpdateLine = 0;
    m_lastUpdateInterval = 0;
    m_lastDistanceFilter = 0;
    m_counter = counter;

    m_interestUid = Utils::createUuid();
}

LocationStreamer::~LocationStreamer()
{
    resetSource();
}

void LocationStreamer::startSource()
{
    if (m_updateInterval == 0 && m_distanceFilter == 0)
    {
        return;
    }

    m_timer.setInterval(m_simulate ? 1000 / s_simulateIntervalMultiplier : 1000); // Initial timeout.
    m_timer.start();

    locationLogger()->takeInterest(m_interestUid);
}

void LocationStreamer::resetSource()
{
    if (!m_timer.isActive())
    {
        return;
    }

    m_timer.stop();

    locationLogger()->loseInterest(m_interestUid);
}

void LocationStreamer::loadState(const QVariantMap& map)
{
    resetSource();

    m_running = map.value("running", false).toBool();
    m_updateInterval = map.value("updateInterval", 0).toInt();
    m_distanceFilter = map.value("distanceFilter", 0.0).toDouble();
    m_distanceCovered = map.value("distanceCovered", 0.0).toDouble();
    m_lastUpdateInterval = map.value("lastUpdateInterval", 0).toInt();
    m_lastDistanceFilter = map.value("lastDistanceFilter", 0).toInt();
    m_counter = map.value("counter", 0).toInt();

    m_lastUpdateX = map.value("lastUpdateX", std::numeric_limits<double>::quiet_NaN()).toDouble();
    m_lastUpdateY = map.value("lastUpdateY", std::numeric_limits<double>::quiet_NaN()).toDouble();
    m_lastUpdateT = map.value("lastUpdateT", 0).toLongLong();
    m_lastUpdateLine = map.value("lastUpdateLine", 0).toInt();

    if (m_running)
    {
        startSource();
    }

    updateRateText();
}

QVariantMap LocationStreamer::saveState() const
{
    return QVariantMap
    {
        { "running", m_running },
        { "updateInterval", m_updateInterval },
        { "distanceFilter", m_distanceFilter },
        { "distanceCovered", m_distanceCovered },
        { "lastUpdateInterval", m_lastUpdateInterval },
        { "lastDistanceFilter", m_lastDistanceFilter },
        { "counter", m_counter },
        { "lastUpdateX", m_lastUpdateX },
        { "lastUpdateY", m_lastUpdateY },
        { "lastUpdateT", m_lastUpdateT },
        { "lastUpdateLine", m_lastUpdateLine },
    };
}

void LocationStreamer::pushUpdate(const QString& locationUid, Location* location)
{
    appendToQueue(locationUid, location);

    if (!m_queue.isEmpty())
    {
        emit locationsAvailable(m_queue.length());

        clearQueue();
    }
}

void LocationStreamer::appendToQueue(const QString& locationUid, Location* location)
{
    if (!m_running)
    {
        return;
    }

    // Remove duplicates.
    if (location->t() == m_lastUpdateT)
    {
        return;
    }

    // Keep points in order to address races with field values and timers.
    if (m_lastUpdateT != 0 && (location->t() < m_lastUpdateT))
    {
        qDebug() << "Attempt to push update that is too old: " << location->t() << m_lastUpdateT;
        return;
    }

    if (!std::isnan(m_lastUpdateX) && !std::isnan(m_lastUpdateY))
    {
        update_distanceCovered(m_distanceCovered + location->distanceTo(m_lastUpdateX, m_lastUpdateY));
    }

    m_lastUpdateX = location->x();
    m_lastUpdateY = location->y();
    m_lastUpdateT = location->t();

    auto queueLocation = new Location(*location, nullptr);
    queueLocation->set_counter(m_counter);
    queueLocation->set_interval(m_updateInterval / 1000);
    queueLocation->set_distanceFilter(m_distanceFilter);
    queueLocation->set_accuracyFilter(accuracyFilter());

    m_queue.append(QPair<QString, Location*>(locationUid, queueLocation));

    update_counter(m_counter + 1);
}

bool LocationStreamer::isQueueEmpty() const
{
    return m_queue.isEmpty();
}

void LocationStreamer::enumQueue(const std::function<void(const QString& locationUid, Location* location)>& callback) const
{
    if (!m_running)
    {
        return;
    }

    for (auto queueIt = m_queue.constBegin(); queueIt != m_queue.constEnd(); queueIt++)
    {
        callback(queueIt->first, queueIt->second);
    }
}

void LocationStreamer::clearQueue()
{
    for (auto queueIt = m_queue.constBegin(); queueIt != m_queue.constEnd(); queueIt++)
    {
        delete queueIt->second;
    }

    m_queue.clear();
}

void LocationStreamer::timeout()
{
    // Adjust interval to final state.
    m_timer.setInterval(m_simulate ? m_updateInterval / s_simulateIntervalMultiplier : m_updateInterval);

    // Process the log file.
    processLog();

    // Call the queue.
    while (!m_queue.isEmpty())
    {
        emit locationsAvailable(m_queue.length());
        clearQueue();
        processLog();
    }
}

void LocationStreamer::processLog()
{
    // Enumerate readings.
    auto distanceCovered = (double)0.0;
    auto accuracyFilter = ::accuracyFilter();
    auto speedFilter = ::speedFilter();

    auto quota = LOG_STREAMER_QUEUE_QUOTA;

    m_reader.enumLines(m_lastUpdateLine, [&](int index, const QString& uid, qint64 t, Location* location)
    {
        // Keep going until the quota is done.
        if (quota == 0)
        {
            return;
        }

        // Discard outliers.
        if (!m_simulate || s_simulateFromTrackLog)
        {
            // Accuracy filter.
            if (accuracyFilter != 0 && location->accuracy() > accuracyFilter)
            {
                qDebug() << "SKIP: does not meet accuracy requirement: " << location->accuracy();
                return;
            }

            // Speed filter.
            if (speedFilter != 0 && location->speed() > speedFilter)
            {
                qDebug() << "SKIP: does not meet speed requirement: " << location->speed();
                return;
            }

            // Stationary filter.
            if (location->stationary())
            {
                // Too noisey to output.
                return;
            }
        }

        // Distance filter.
        if (m_distanceFilter != 0)
        {
            if (std::isnan(m_lastUpdateX) || std::isnan(m_lastUpdateY))
            {
                m_lastUpdateX = location->x();
                m_lastUpdateY = location->y();
            }
            else
            {
                distanceCovered += location->distanceTo(m_lastUpdateX, m_lastUpdateY);
                if (distanceCovered >= m_distanceFilter)
                {
                    appendToQueue(uid, location);
                    distanceCovered = 0;
                    m_lastUpdateLine = index;
                    quota--;
                }
            }

            return;
        }

        // Interval filter.
        m_lastUpdateLine = index;
        auto updateInterval = m_simulate ? m_updateInterval / s_simulateIntervalMultiplier : m_updateInterval;
        if (t - m_lastUpdateT >= updateInterval)
        {
            appendToQueue(uid, location);
            m_lastUpdateT = t;
            quota--;
            return;
        }
    });
}

void LocationStreamer::start(int updateInterval, double distanceFilter, double distanceCovered, int counter)
{
    auto cleanup = qScopeGuard([&] { updateRateText(); });

    if (m_distanceFilter != 0.0)
    {
        updateInterval = 1000;
    }

    if (updateInterval == 0)
    {
        stop();
        return;
    }

    auto wasRunning = m_running;

    if (m_running && updateInterval == m_updateInterval && distanceFilter == m_distanceFilter)
    {
        return;
    }

    resetSource();

    update_updateInterval(updateInterval);
    update_distanceFilter(distanceFilter);
    update_distanceCovered(distanceCovered);
    update_lastUpdateInterval(updateInterval);
    update_lastDistanceFilter(distanceFilter);
    update_counter(counter);
    update_running(true);

    if (!wasRunning)
    {
        m_lastUpdateX = m_lastUpdateY = std::numeric_limits<double>::quiet_NaN();
        m_lastUpdateT = 0;
        m_lastUpdateLine = m_reader.lineCount();
    }

    startSource();

    if (!wasRunning)
    {
        emit started();
    }
}

void LocationStreamer::stop()
{
    auto cleanup = qScopeGuard([&] { updateRateText(); });

    auto wasRunning = m_running;

    resetSource();

    update_running(false);
    update_updateInterval(0);
    update_distanceFilter(0.0);
    m_counter = 0;

    if (wasRunning)
    {
        emit stopped();
    }
}

void LocationStreamer::updateRateText()
{
    auto rateText = QString();

    if (!m_running)
    {
        rateText = tr("Off");
    }
    else if (m_distanceFilter > 0)
    {
        rateText = App::instance()->getDistanceText(m_distanceFilter);
    }
    else
    {
        rateText = App::instance()->getTimeText(qRound((double)m_updateInterval / 1000));
    }

    update_rateText(rateText);

    if (m_rateLabel.isEmpty())
    {
        update_rateFullText(rateText);
    }
    else
    {
        update_rateFullText(QString("%1 - %2").arg(m_rateLabel, rateText));
    }
}

//=================================================================================================
// LocationLogger

LocationLogger::LocationLogger(const QString& filePath, QObject* parent): QObject(parent), m_filePath(filePath), m_reader(filePath)
{
    m_running = false;

    m_stateTimer.setInterval(5000);
    connect(this, &LocationLogger::runningChanged, this, &LocationLogger::updateState);
    connect(&m_stateTimer, &QTimer::timeout, this, &LocationLogger::updateState);

    connect(&m_serviceSyncTimer, &QTimer::timeout, this, &LocationLogger::serviceSync);

    updateState();
}

LocationLogger::~LocationLogger()
{
    delete m_writer;
    m_writer = nullptr;
}

bool LocationLogger::hasInterest(const QString& interestUid) const
{
    return m_interest.value(interestUid, false);
}

void LocationLogger::takeInterest(const QString& interestUid)
{
    qFatalIf(m_interest.contains(interestUid), "Already has interest: " + interestUid);
    m_interest.insert(interestUid, true);

    if (m_interest.count() == 1)
    {
        start();
    }
}

void LocationLogger::loseInterest(const QString& interestUid)
{
    qFatalIf(!m_interest.contains(interestUid), "Interest not found: " + interestUid);
    m_interest.remove(interestUid);

    if (m_interest.isEmpty())
    {
        stop();
    }
}

bool LocationLogger::requestServiceSync()
{
    if (simulateGPS())
    {
        return false;
    }

#if defined(Q_OS_ANDROID)
    m_serviceSyncTimer.setInterval(100);
    m_serviceSyncTimer.start();
    return true;
#else
    return false;
#endif
}

void LocationLogger::serviceSync()
{
#if defined(Q_OS_ANDROID)
    auto currState = App::instance()->getLocationServiceRunning();
    auto nextState = m_running;

    if (currState != nextState)
    {
        if (nextState)
        {
            qDebug() << "Starting location service";
            QJniObject jsTitle = QJniObject::fromString(tr("Reading location"));
            QJniObject jsText = QJniObject::fromString(tr(""));
            QJniObject jsBigText = QJniObject::fromString(tr(""));
            QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("startLocationUpdates", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V", jsTitle.object<jstring>(), jsText.object<jstring>(), jsBigText.object<jstring>());
        }
        else
        {
            qDebug() << "Stopping location service";
            QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("stopLocationUpdates", "()V");
        }
    }

    m_serviceSyncTimer.setInterval(3000);
    m_serviceSyncTimer.start();
#endif
}

bool LocationLogger::start()
{
    delete m_writer;
    m_writer = nullptr;

    if (!requestServiceSync())
    {
        m_writer = new LocationWriter(m_filePath);

        delete m_source;
        m_source = nullptr;
        m_source = new PositionInfoSource(this);
        connect(m_source, &PositionInfoSource::positionUpdated, this, &LocationLogger::positionUpdated);

        m_source->setUpdateInterval(1000);
        m_source->startUpdates();
    }

    update_running(true);

    qDebug() << "LocationLogger started: " << timeManager()->currentDateTimeISO();

    return true;
}

void LocationLogger::stop()
{
    if (!requestServiceSync())
    {
        m_source->stopUpdates();
        delete m_source;
        m_source = nullptr;
    }

    delete m_writer;
    m_writer = nullptr;

    App::instance()->backupTrackLog();
    LocationWriter(m_filePath).reset();

    update_running(false);

    qDebug() << "LocationLogger stopped: " << timeManager()->currentDateTimeISO();
}

void LocationLogger::positionUpdated(const QGeoPositionInfo& update)
{
    // Standard validation.
    if (!update.isValid() || !update.timestamp().isValid() || !update.coordinate().isValid())
    {
        return;
    }

    auto x = update.coordinate().longitude();
    auto y = update.coordinate().latitude();
    auto z = update.coordinate().altitude();
    auto t = update.timestamp().toMSecsSinceEpoch();
    auto accuracy = update.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) ? update.attribute(QGeoPositionInfo::HorizontalAccuracy) : std::numeric_limits<double>::quiet_NaN();
    auto speed = update.hasAttribute(QGeoPositionInfo::GroundSpeed) ? update.attribute(QGeoPositionInfo::GroundSpeed) : std::numeric_limits<double>::quiet_NaN();
    auto direction = update.hasAttribute(QGeoPositionInfo::Direction) ? update.attribute(QGeoPositionInfo::Direction) : std::numeric_limits<double>::quiet_NaN();
    auto satelliteCount = 0; // Not implemented.
    auto flags = simulateGPS() && update.hasAttribute(QGeoPositionInfo::VerticalSpeed) ? static_cast<int>(update.attribute(QGeoPositionInfo::VerticalSpeed)) : 0;

    // Sanity validation.
    if (std::isnan(x) || std::isnan(y) || t <= 0)
    {
        return;
    }

    // Ignore stale data.
    if (t <= s_locationStaleT)
    {
        return;
    }

    s_locationStaleT = t;

    // Append location.
    m_writer->append(QUuid::createUuid().toString(QUuid::Id128), x, y, z, t, accuracy, speed, direction, satelliteCount, flags);
}

void LocationLogger::updateState()
{
    auto darkTheme = App::instance()->settings()->darkTheme();

    // Off.
    if (!m_running)
    {
        m_stateTimer.stop();
        update_state(State::StateOff);
        update_stateIcon("qrc:/icons/gps_off.svg");
        update_stateColor(darkTheme ? Qt::white : Qt::black);
        return;
    }

    // Update timer: start fast and then go to 5 seconds.
    if (!m_stateTimer.isActive())
    {
        m_stateTimer.start();
    }

    // Empty or no new data.
    auto lastLineCount = m_reader.lineCount() - 1;
    if (lastLineCount < 0 || lastLineCount == m_lastLineCount)
    {
        update_state(State::StateNoData);
        update_stateIcon("qrc:/icons/gps_unknown.svg");
        update_stateColor(darkTheme ? Qt::yellow : Qt::darkYellow);
        return;
    }

    m_lastLineCount = lastLineCount;

    // Get the last reading.
    auto lastAccuracy = (double)0;
    auto lastSpeed = (double)0;
    auto lastPositionInfo = QGeoPositionInfo();

    m_reader.enumLines(lastLineCount, [&](int /*index*/, const QString& /*uid*/, qint64 /*t*/, Location* location)
    {
        lastAccuracy = location->accuracy();
        lastSpeed = location->speed();
        lastPositionInfo = location->toGeoPositionInfo();
    });

    // Check if accurate.
    auto lastFixIsAccurate = std::isnan(lastAccuracy) ? false : lastAccuracy <= ::accuracyFilter();
    if (!lastFixIsAccurate)
    {
        update_state(State::StateLowAccuracy);
        update_stateIcon("qrc:/icons/gps_unknown.svg");
        update_stateColor(darkTheme ? Qt::red : Qt::darkRed);
        return;
    }

    // Check if outlier.
    auto lastFixIsOutlier = std::isnan(lastSpeed) ? false : lastSpeed > ::speedFilter();
    if (lastFixIsOutlier)
    {
        update_state(State::StateOutlier);
        update_stateIcon("qrc:/icons/gps_unknown.svg");
        update_stateColor(darkTheme ? Qt::red : Qt::darkRed);
        return;
    }

    // Fix is good.
    update_state(State::StateFix);
    update_stateColor(darkTheme ? Qt::green : Qt::darkGreen);
    update_stateIcon("qrc:/icons/gps_fixed.svg");

    emit App::instance()->positionUpdated(lastPositionInfo);
}

} // namespace CyberTracker
