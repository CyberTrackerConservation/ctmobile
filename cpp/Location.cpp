#include "Location.h"
#include "App.h"

#if defined(Q_OS_ANDROID)
#include "LocationAndroid.h"
#endif

Q_IMPORT_PLUGIN(LocationFactory)

namespace {

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

    QString deviceId()
    {
        return App::instance()->settings()->deviceId();            
    }
}

//=================================================================================================
// LocationFactory

bool s_bypassInternalLocationFactory = false;

QGeoPositionInfoSource* LocationFactory::positionInfoSource(QObject* parent)
{
    return s_bypassInternalLocationFactory ? nullptr : new PositionInfoSource(parent);
}

QGeoSatelliteInfoSource* LocationFactory::satelliteInfoSource(QObject* parent)
{
    return s_bypassInternalLocationFactory ? nullptr : new SatelliteInfoSource(parent);
}

QGeoAreaMonitorSource* LocationFactory::areaMonitor(QObject* /*parent*/)
{
    return nullptr;
}

//=================================================================================================
// SimulateNmeaServer

constexpr char SOCKET_ADDRESS[]= "127.0.0.1";
constexpr quint16 SOCKET_PORT = 19321;

SimulateNmeaServer::SimulateNmeaServer(QObject* parent) : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, &SimulateNmeaServer::onNewConnection);
    connect(&m_timer, &QTimer::timeout, this, &SimulateNmeaServer::onTimer);
}

SimulateNmeaServer::~SimulateNmeaServer()
{
    stop();
}

bool SimulateNmeaServer::start(const QString& nmeaFilePath)
{
    QFile file(nmeaFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "GPS - Failed to open nmea file";
        return false;
    }

    m_nmeaLines.clear();
    m_lineCounter = 0;

    QString lastLine;
    while (!file.atEnd())
    {
        auto line = QString(file.readLine()).simplified() + "\r\n";
        if (line.startsWith("$GPGGA"))
        {
            if (!lastLine.isEmpty())
            {
                m_nmeaLines.append(lastLine);
                lastLine.clear();
            }
        }

        lastLine += line;
    }

    if (!lastLine.isEmpty())
    {
        m_nmeaLines.append(lastLine);
    }

    if (!m_server.listen(QHostAddress(SOCKET_ADDRESS), SOCKET_PORT))
    {
        qDebug() << "GPS - Failed to start server";
        return false;
    }

    m_timer.start(1000);

    return true;
}

void SimulateNmeaServer::stop()
{
    for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); it++)
    {
        (*it)->close();
    }
    m_clients.clear();

    m_server.close();
    m_timer.stop();
    m_nmeaLines.clear();
    m_lineCounter = 0;
}

void SimulateNmeaServer::onTimer()
{
    if (m_clients.count() != 0)
    {
        for (auto it = m_clients.constBegin(); it != m_clients.constEnd(); it++)
        {
            auto socket = *it;
            if (socket->state() == QTcpSocket::ConnectedState)
            {
                auto line = m_nmeaLines[m_lineCounter].toLatin1();
                socket->write(line);
                socket->flush();
            }
        }

        m_lineCounter = m_lineCounter == m_nmeaLines.count() - 1 ? 0 : m_lineCounter + 1;
    }
}

void SimulateNmeaServer::onNewConnection()
{
    QTcpSocket* socket = m_server.nextPendingConnection();
    if (socket == nullptr)
    {
        return;
    }

    m_clients.append(socket);
    connect(socket, &QTcpSocket::disconnected, this, &SimulateNmeaServer::onClientDisconnected);
    connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);

    if (m_clients.count() == 1)
    {
        m_timer.start(1000);
    }
}

void SimulateNmeaServer::onClientDisconnected()
{
    if (m_clients.count() == 1)
    {
        m_timer.stop();
    }

    QTcpSocket* clientSocket = qobject_cast<QTcpSocket *>(QObject::sender());
    m_clients.removeOne(clientSocket);
}

//=================================================================================================
// PositionInfoSource

std::atomic<int> PositionInfoSource::s_platformRefCount;

PositionInfoSource::PositionInfoSource(QObject* parent) : QGeoPositionInfoSource(parent)
{
    m_simulate = simulateGPS();

    if (m_simulate)
    {
        m_simulateNmeaSocket.connectToHost(SOCKET_ADDRESS, SOCKET_PORT, QTcpSocket::ReadOnly);

        auto source = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::SimulationMode, this);
        source->setDevice(&m_simulateNmeaSocket);
        m_source = source;
    }
    else
    {
        qDebug() << "GPS - PositionInfo sources: " << QGeoPositionInfoSource::availableSources();
        s_bypassInternalLocationFactory = true;

        #if defined (Q_OS_WIN)
            m_source = QGeoPositionInfoSource::createSource("winrt", this);
        #else
            m_source = QGeoPositionInfoSource::createDefaultSource(this);
        #endif

        s_bypassInternalLocationFactory = false;

        if (!m_source)
        {
            qDebug() << "GPS - Position source not found";
            return;
        }

        m_source->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
    }

    // Outlier detection filter.
    m_speedFilterKph = App::instance()->settings()->locationOutlierMaxSpeed();
    connect(App::instance()->settings(), &Settings::locationOutlierMaxSpeedChanged, this, [&]()
    {
        m_speedFilterKph = App::instance()->settings()->locationOutlierMaxSpeed();
    });

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

        // Skip old readings.
        if (t <= m_lastKnownTimestamp)
        {
            qDebug() << "GPS - positionUpdated: update is old";
            return;
        }

        auto update = _update;

        // Overwrite the GPS simulation timestamp.
        if (m_simulate)
        {
            update.setTimestamp(QDateTime::currentDateTime());
            update.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
            t = update.timestamp().toMSecsSinceEpoch();
        }

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
            if (m_speedFilterKph != 0)
            {
                auto speedKph = update.hasAttribute(QGeoPositionInfo::GroundSpeed) ? update.attribute(QGeoPositionInfo::GroundSpeed) * 3.6 : 0; // Convert to kph.
                if (speedKph > m_speedFilterKph)
                {
                    if (!m_simulate)
                    {
                        qDebug() << "GPS - Outlier detected - speed " << speedKph << " is more than " << m_speedFilterKph;
                        emit App::instance()->showError(tr("GPS outlier detected"));
                        return;
                    }
                }
            }

            // Distance and time requirements.
            if (m_distanceFilter > 0)
            {
                auto distanceToLast = update.coordinate().distanceTo(m_lastUpdate.coordinate());
                if (distanceToLast < m_distanceFilter)
                {
                    qDebug() << "GPS - Distance filter used - distanceToLast = " << distanceToLast << " distanceFilter = " << m_distanceFilter;
                    return;
                }
            }
            else
            {
                auto timeSinceLast = t - m_lastTimestamp + 1000;
                if (timeSinceLast > 0 && timeSinceLast < m_source->updateInterval())
                {
                    // On iOS, the update interval seems to always be 1 second.
                    // Suspect that this is because the Qt iOS GPS interface is a singleton.
                    qDebug() << "GPS - Time filter used - timeSinceLast = " << timeSinceLast << " updateInterval = " << m_source->updateInterval();
                    return;
                }
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

    // "error" is overloaded, so we need to use this technique to forward errors.
    connect(m_source, QOverload<QGeoPositionInfoSource::Error>::of(&QGeoPositionInfoSource::error),
            this, QOverload<QGeoPositionInfoSource::Error>::of(&QGeoPositionInfoSource::error));

    connect(m_source, QOverload<QGeoPositionInfoSource::Error>::of(&QGeoPositionInfoSource::error), [=](QGeoPositionInfoSource::Error positionError)
    {
        qDebug() << "GPS - PositionInfoSource error: " << positionError;
    });
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

    if (m_simulate)
    {
        m_source->stopUpdates();
        m_simulateNmeaSocket.disconnectFromHost();
    }

    platformStop();
}

void PositionInfoSource::platformStart()
{
    if (s_platformRefCount.fetch_add(1) != 0 || m_simulate)
    {
        return;
    }

    #if defined(Q_OS_ANDROID)
        QtAndroid::androidActivity().callMethod<void>("startLocationUpdates", "()V");
    #endif
}

void PositionInfoSource::platformStop()
{
    qFatalIf(s_platformRefCount == 0, "Invalid ref count");

    if (s_platformRefCount.fetch_sub(1) != 1 || m_simulate)
    {
        return;
    }

    #if defined(Q_OS_ANDROID)
        QtAndroid::androidActivity().callMethod<void>("stopLocationUpdates", "()V");
    #endif
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
    platformStart();
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
    platformStop();
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

double PositionInfoSource::distanceFilter() const
{
    return m_distanceFilter;
}

void PositionInfoSource::setDistanceFilter(double meters)
{
    m_distanceFilter = meters;
}

int PositionInfoSource::speedFilter() const
{
    return m_speedFilterKph;
}

void PositionInfoSource::setSpeedFilter(int kph)
{
    m_speedFilterKph = kph;
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

    // TODO(justin): Remove for Qt6.
    #if defined(Q_OS_ANDROID)
        m_source = new SatelliteInfoSourceAndroid(this);
    #else
        s_bypassInternalLocationFactory = true;
        m_source = QGeoSatelliteInfoSource::createDefaultSource(this);
        s_bypassInternalLocationFactory = false;
    #endif

    if (!m_source)
    {
        qDebug() << "GPS - Satellite source not found";
        return;
    }

    connect(m_source, &QGeoSatelliteInfoSource::satellitesInViewUpdated, this, &SatelliteInfoSource::satellitesInViewUpdated);
    connect(m_source, &QGeoSatelliteInfoSource::satellitesInUseUpdated, this, &SatelliteInfoSource::satellitesInUseUpdated);

    // "error" is overloaded, so we need to use this technique to forward errors.
    connect(m_source, QOverload<QGeoSatelliteInfoSource::Error>::of(&QGeoSatelliteInfoSource::error),
            this, QOverload<QGeoSatelliteInfoSource::Error>::of(&QGeoSatelliteInfoSource::error));

    connect(m_source, QOverload<QGeoSatelliteInfoSource::Error>::of(&QGeoSatelliteInfoSource::error), [=](QGeoSatelliteInfoSource::Error satelliteError)
    {
        qDebug() << "GPS - SatelliteInfoSource error: " << satelliteError;
    });
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
    m_d = location.m_d;
    m_a = location.m_a;
    m_s = location.m_s;
    m_ts = location.m_ts;
    m_counter = location.m_counter;
    m_interval = location.m_interval;
    m_distanceFilter = location.m_distanceFilter;
    m_accuracyFilter = location.m_accuracyFilter;
    m_deviceId = location.m_deviceId;
}

Location::Location(const QVariantMap& data, QObject* parent): QObject(parent)
{
    m_x = data.value("x", m_x).toDouble();
    m_y = data.value("y", m_y).toDouble();
    m_z = data.value("z", m_z).toDouble();
    m_d = data.value("d", m_d).toDouble();
    m_a = data.value("a", m_a).toDouble();
    m_s = data.value("s", m_s).toDouble();
    m_ts = data.value("ts", m_ts).toString();
    m_counter = data.value("c", m_counter).toInt();
    m_interval = data.value("interval", m_interval).toInt();
    m_distanceFilter = data.value("distanceFilter", m_distanceFilter).toInt();
    m_accuracyFilter = data.value("accuracyFilter", m_accuracyFilter).toInt();
    m_deviceId = data.value("deviceId", m_deviceId).toString();

    // Fix missing timestamp.
    if (m_ts.isEmpty() && data.contains("t"))
    {
        auto t = data["t"].toLongLong();
        m_ts = (t <= 0) ? "" : App::instance()->timeManager()->formatDateTime(t);
    }

    // Fix missing deviceId.
    if (m_deviceId.isEmpty())
    {
        m_deviceId = ::deviceId();
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
    m_s = f(QGeoPositionInfo::GroundSpeed);
    m_d = f(QGeoPositionInfo::Direction);
    m_a = f(QGeoPositionInfo::HorizontalAccuracy);
    m_ts = Utils::encodeTimestamp(positionInfo.timestamp());

    m_accuracyFilter = ::accuracyFilter();
    m_deviceId = ::deviceId();
}

bool Location::isValid() const
{
    return !m_ts.isEmpty();
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
        { "d", m_d },
        { "a", m_a },
        { "s", m_s },
        { "ts", m_ts },
        { "c", m_counter },
        { "interval", m_interval },
        { "distanceFilter", m_distanceFilter },
        { "accuracyFilter", m_accuracyFilter },
        { "spatialReference", spatialReference() },
        { "deviceId", m_deviceId },
    };
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

QString Location::timestamp() const
{
    return m_ts;
}

void Location::set_timestamp(const QString& value)
{
    m_ts = value;
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

QString Location::deviceId() const
{
    return m_deviceId;
}

void Location::set_deviceId(const QString& value)
{
    m_deviceId = value;
}

QVariantMap Location::spatialReference() const
{
    return QVariantMap {{ "wkid", 4326 }};
}

//=================================================================================================
// LocationStreamer

LocationStreamer::LocationStreamer(QObject* parent): QObject(parent)
{
    m_running = false;
    m_updateInterval = 0;
    m_distanceFilter = 0.0;
    m_locationRecent = false;
    m_distanceCovered = 0;
    m_lastUpdateX = m_lastUpdateY = std::numeric_limits<double>::quiet_NaN();
    m_counter = 0;
    updateDisplay();

    // Timer to sync locationRecent.
    m_timer.setInterval(10 * 1000);
    connect(&m_timer, &QTimer::timeout, this, [&]()
    {
        if (m_source && m_source->lastUpdate().isValid())
        {
            auto timeDelta = QDateTime::currentMSecsSinceEpoch() - m_source->lastTimestamp();
            update_locationRecent(timeDelta < (m_updateInterval + 10 * 1000));
            updateDisplay();
        }
    });
}

LocationStreamer::~LocationStreamer()
{
}

void LocationStreamer::startSource()
{
    qFatalIf(m_source != nullptr, "Source already started");

    if (m_updateInterval == 0 && m_distanceFilter == 0)
    {
        return;
    }

    m_source = new PositionInfoSource(this);
    m_source->setUpdateInterval(1000); // Initial update for first reading.
    m_source->setDistanceFilter(0);    // Initial filter for first reading.
    m_source->startUpdates();

    m_lastUpdateX = m_lastUpdateY = std::numeric_limits<double>::quiet_NaN();

    connect(m_source, &PositionInfoSource::positionUpdated, this, [&](const QGeoPositionInfo& update)
    {
        Location location(update);
        pushUpdate(&location);
    });

    m_timer.start();
}

void LocationStreamer::resetSource()
{
    delete m_source;
    m_source = nullptr;

    m_timer.stop();
}

void LocationStreamer::loadState(const QVariantMap& map)
{
    resetSource();

    m_running = map.value("running", false).toBool();
    m_updateInterval = map.value("updateInterval", 0).toInt();
    m_distanceFilter = map.value("distanceFilter", 0.0).toDouble();
    m_distanceCovered = map.value("distanceCovered", 0.0).toDouble();
    m_counter = map.value("counter", 0).toInt();
    m_lastUpdateX = map.value("lastUpdateX", std::numeric_limits<double>::quiet_NaN()).toDouble();
    m_lastUpdateY = map.value("lastUpdateY", std::numeric_limits<double>::quiet_NaN()).toDouble();

    if (m_running)
    {
        startSource();
    }

    updateDisplay();
}

QVariantMap LocationStreamer::saveState()
{
    return QVariantMap
    {
        { "running", m_running },
        { "updateInterval", m_updateInterval },
        { "distanceFilter", m_distanceFilter },
        { "distanceCovered", m_distanceCovered },
        { "counter", m_counter },
        { "lastUpdateX", m_lastUpdateX },
        { "lastUpdateY", m_lastUpdateY },
    };
}

void LocationStreamer::pushUpdate(Location* update)
{
    if (!m_running)
    {
        return;
    }

    if (!std::isnan(m_lastUpdateX) && !std::isnan(m_lastUpdateY))
    {
        update_distanceCovered(m_distanceCovered + update->distanceTo(m_lastUpdateX, m_lastUpdateY));
    }

    m_lastUpdateX = update->x();
    m_lastUpdateY = update->y();

    update->set_counter(m_counter);
    update->set_interval(m_updateInterval / 1000);
    update->set_distanceFilter(m_distanceFilter);
    update->set_accuracyFilter(accuracyFilter());
    update->set_deviceId(::deviceId());

    emit LocationStreamer::locationUpdated(update);

    update_counter(m_counter + 1);
    update_locationRecent(true);

    // Adjust interval and filter after initial reading.
    if (m_source->updateInterval() != m_updateInterval)
    {
        m_source->setUpdateInterval(m_updateInterval);
    }

    if (m_source->distanceFilter() != m_distanceFilter)
    {
        m_source->setDistanceFilter(m_distanceFilter);
    }

    // Update the display.
    updateDisplay();
}

void LocationStreamer::start(int updateInterval, double distanceFilter, double distanceCovered, int counter, bool initialLocationRecent)
{
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
    update_counter(counter);
    update_locationRecent(initialLocationRecent);
    update_running(true);
    updateDisplay();

    startSource();

    if (!wasRunning)
    {
        emit started();
    }
}

void LocationStreamer::stop()
{
    auto wasRunning = m_running;

    resetSource();

    update_running(false);
    update_updateInterval(0);
    update_distanceFilter(0.0);
    update_locationRecent(false);
    updateDisplay();

    if (wasRunning)
    {
        emit stopped();
    }
}

void LocationStreamer::updateDisplay()
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

    auto rateIcon = QString();
    if (!m_running)
    {
        rateIcon = "qrc:/icons/gps_off.svg";
    }
    else if (m_locationRecent)
    {
        rateIcon = "qrc:/icons/gps_fixed.svg";
    }
    else
    {
        rateIcon = "qrc:/icons/gps_unknown.svg";
    }

    update_rateIcon(rateIcon);
}
