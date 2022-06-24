#include "Location.h"
#include "App.h"

Q_IMPORT_PLUGIN(LocationFactory)

namespace {

    TimeManager* timeManager()
    {
        return App::instance()->timeManager();
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
        qDebug() << "Failed to open nmea file";
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
        qDebug() << "Failed to start server";
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
    m_simulate = App::instance()->settings()->simulateGPS();

    if (m_simulate)
    {
        m_simulateNmeaSocket.connectToHost(SOCKET_ADDRESS, SOCKET_PORT, QTcpSocket::ReadOnly);

        auto source = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::SimulationMode, this);
        source->setDevice(&m_simulateNmeaSocket);
        m_source = source;
    }
    else
    {
        s_bypassInternalLocationFactory = true;
        m_source = QGeoPositionInfoSource::createDefaultSource(this);
        s_bypassInternalLocationFactory = false;

        if (!m_source)
        {
            qDebug() << "Position source not found";
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
        if (qIsNaN(_update.coordinate().longitude()) ||
            qIsNaN(_update.coordinate().latitude()) ||
            !_update.timestamp().isValid() ||
            _update.timestamp().toMSecsSinceEpoch() == -10800000)
        {
            qDebug() << "positionUpdated: ignoring invalid position";
            return;
        }

        auto update = _update;
        auto updateTimestamp = update.timestamp().toMSecsSinceEpoch();

        // Overwrite the GPS simulation timestamp.
        if (App::instance()->settings()->simulateGPS())
        {
            update.setTimestamp(timeManager()->currentDateTime());
            updateTimestamp = update.timestamp().toMSecsSinceEpoch();
            update.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1.0);
        }

        // Try to avoid old readings being returned.
        if (updateTimestamp == m_lastKnownTimestamp)
        {
            return;
        }

        // Skip readings if conditions are not met.
        if (m_lastUpdate.isValid())
        {
            // Outlier detection if the speed is too fast.
            if (m_speedFilterKph != 0)
            {
                auto s = update.hasAttribute(QGeoPositionInfo::GroundSpeed) ? update.attribute(QGeoPositionInfo::GroundSpeed) * 3.6 : 0; // Convert to kph.
                if (s > m_speedFilterKph)
                {
                    if (!m_simulate)
                    {
                        qDebug() << "Outlier detected - speed " << s << " is more than " << m_speedFilterKph;
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
                    return;
                }
            }
            else
            {
                auto timeSinceLast = updateTimestamp - m_lastTimestamp + 1000;
                if (timeSinceLast < m_source->updateInterval())
                {
                    // On iOS, the update interval seems to always be 1 second.
                    // Suspect that this is because the Qt iOS GPS interface is a singleton.
                    return;
                }
            }
        }

        m_lastUpdate = update;
        m_lastTimestamp = updateTimestamp;

        // Tell the app first, so it can compute declination and time corrections.
        App::instance()->positionUpdated(update);

        // Publish update to listeners.
        emit positionUpdated(update);
    });

    // "error" is overloaded, so we need to use this technique to forward errors.
    connect(m_source, QOverload<QGeoPositionInfoSource::Error>::of(&QGeoPositionInfoSource::error),
            this, QOverload<QGeoPositionInfoSource::Error>::of(&QGeoPositionInfoSource::error));
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
    m_simulate = App::instance()->settings()->simulateGPS();

    if (m_simulate)
    {
        m_simulateTimer.setInterval(1000);
        connect(&m_simulateTimer, &QTimer::timeout, this, &SatelliteInfoSource::simulateData);
    }
    else
    {
        s_bypassInternalLocationFactory = true;
        m_source = QGeoSatelliteInfoSource::createDefaultSource(this);
        s_bypassInternalLocationFactory = false;

        if (!m_source)
        {
            qDebug() << "Satellite source not found";
            return;
        }

        connect(m_source, &QGeoSatelliteInfoSource::satellitesInViewUpdated, this, &SatelliteInfoSource::satellitesInViewUpdated);
        connect(m_source, &QGeoSatelliteInfoSource::satellitesInUseUpdated, this, &SatelliteInfoSource::satellitesInUseUpdated);
    }
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
    return !m_source || m_simulate ? NoError : m_source->error();
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

    // Timer to sync locationRecent.
    m_timer.setInterval(10 * 1000);
    connect(&m_timer, &QTimer::timeout, this, [&]()
    {
        if (m_source && m_source->lastUpdate().isValid())
        {
            auto timeDelta = QDateTime::currentMSecsSinceEpoch() - m_source->lastTimestamp();
            update_locationRecent(timeDelta < (m_updateInterval + 10 * 1000));
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
    m_source->setUpdateInterval(m_updateInterval);
    m_source->setDistanceFilter(m_distanceFilter);
    m_source->startUpdates();

    m_lastUpdateX = m_lastUpdateY = std::numeric_limits<double>::quiet_NaN();

    connect(m_source, &PositionInfoSource::positionUpdated, this, [&](const QGeoPositionInfo& update)
    {
        if (!update.isValid())
        {
            return;
        }

        if (!std::isnan(m_lastUpdateX) && !std::isnan(m_lastUpdateY))
        {
            update_distanceCovered(m_distanceCovered + update.coordinate().distanceTo(QGeoCoordinate(m_lastUpdateY, m_lastUpdateX)));
        }

        m_lastUpdateX = update.coordinate().longitude();
        m_lastUpdateY = update.coordinate().latitude();

        emit LocationStreamer::positionUpdated(update);

        update_counter(m_counter + 1);
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

void LocationStreamer::pushUpdate(const QGeoPositionInfo& update)
{
    if (m_running)
    {
        emit m_source->positionUpdated(update);
    }
}

void LocationStreamer::pushUpdate(const QVariantMap& update)
{
    if (m_running)
    {
        auto x = update["x"].toDouble();
        auto y = update["y"].toDouble();
        auto z = update["z"].toDouble();
        auto dt = Utils::decodeTimestamp(update["ts"].toString());
        emit m_source->positionUpdated(QGeoPositionInfo(QGeoCoordinate(y, x, z), dt));
    }
}

void LocationStreamer::start(int updateInterval, double distanceFilter, double distanceCovered, int counter, bool initialLocationRecent)
{
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

    if (wasRunning)
    {
        emit stopped();
    }
}

QString LocationStreamer::rateText()
{
    if (!m_running)
    {
        return tr("Off");
    }
    else if (m_distanceFilter > 0)
    {
        return App::instance()->getDistanceText(m_distanceFilter);
    }
    else
    {
        return QString("%1 seconds").arg(qRound((double)m_updateInterval / 1000));
    }
}
