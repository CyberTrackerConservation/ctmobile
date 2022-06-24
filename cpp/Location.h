#pragma once
#include "pch.h"
#include <qgeopositioninfosourcefactory.h>

class LocationFactory : public QObject, public QGeoPositionInfoSourceFactory
{
    Q_OBJECT

    Q_PLUGIN_METADATA (IID "org.qt-project.qt.position.sourcefactory/5.0" FILE "LocationPlugin.json")

    Q_INTERFACES (QGeoPositionInfoSourceFactory)

public:
    QGeoPositionInfoSource* positionInfoSource(QObject* parent) override;
    QGeoSatelliteInfoSource* satelliteInfoSource(QObject* parent) override;
    QGeoAreaMonitorSource* areaMonitor(QObject* parent) override;
};

class SimulateNmeaServer : public QObject
{
    Q_OBJECT

public:
    SimulateNmeaServer(QObject* parent = nullptr);
    ~SimulateNmeaServer();

    bool start(const QString& nmeaFilePath);
    void stop();
    
public slots:
    void onNewConnection();
    void onTimer();
    void onClientDisconnected();

private:
    QTcpServer m_server;
    QTimer m_timer;
    QStringList m_nmeaLines;
    QList<QTcpSocket*> m_clients;
    int m_lineCounter = 0;
};

class PositionInfoSource : public QGeoPositionInfoSource
{
    Q_OBJECT

public:
    explicit PositionInfoSource(QObject* parent = nullptr);
    ~PositionInfoSource();

    double distanceFilter() const;
    void setDistanceFilter(double meters);

    int speedFilter() const;
    void setSpeedFilter(int kph);

    QGeoPositionInfo lastUpdate() const;
    qint64 lastTimestamp() const;

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const override;
    void setPreferredPositioningMethods(QGeoPositionInfoSource::PositioningMethods methods) override;
    PositioningMethods supportedPositioningMethods() const override;
    void setUpdateInterval(int msec) override;
    int minimumUpdateInterval() const override;
    QGeoPositionInfoSource::Error error() const override;

public slots:
    void startUpdates() override;
    void stopUpdates() override;
    void requestUpdate(int timeout = 0) override;

private:
    bool m_simulate = false;
    bool m_started = false;
    QGeoPositionInfoSource* m_source = nullptr;
    QGeoPositionInfo m_lastUpdate;
    qint64 m_lastTimestamp = 0;
    double m_distanceFilter = 0;
    int m_speedFilterKph = 0;
    qint64 m_lastKnownTimestamp = 0;

    // Simulation.
    QTcpSocket m_simulateNmeaSocket;

    // Platform.
    static std::atomic<int> s_platformRefCount;
    void platformStart();
    void platformStop();
};

class SatelliteInfoSource: public QGeoSatelliteInfoSource
{
    Q_OBJECT

public:
    explicit SatelliteInfoSource(QObject* parent = nullptr);
    ~SatelliteInfoSource();

    Error error() const override;
    int minimumUpdateInterval() const override;
    void setUpdateInterval(int msec) override;

public slots:
    void requestUpdate(int timeout = 0) override;
    void startUpdates() override;
    void stopUpdates() override;

private:
    bool m_simulate = false;
    bool m_started = false;

    QTimer m_simulateTimer;
    void simulateData();

    QGeoSatelliteInfoSource* m_source = nullptr;
};

class LocationStreamer: public QObject
{
    Q_OBJECT
    QML_READONLY_AUTO_PROPERTY (bool, running)
    QML_READONLY_AUTO_PROPERTY (int, updateInterval)
    QML_READONLY_AUTO_PROPERTY (double, distanceFilter)
    QML_READONLY_AUTO_PROPERTY (bool, locationRecent)
    QML_READONLY_AUTO_PROPERTY (double, distanceCovered)
    QML_READONLY_AUTO_PROPERTY (int, counter)

public:
    explicit LocationStreamer(QObject* parent = nullptr);
    ~LocationStreamer();

    void loadState(const QVariantMap& map);
    QVariantMap saveState();

    void pushUpdate(const QGeoPositionInfo& update);
    void pushUpdate(const QVariantMap& update);

    Q_INVOKABLE void start(int updateInterval, double distanceFilter = 0, double distanceCovered = 0, int counter = 0, bool initialRecent = false);
    Q_INVOKABLE void stop();

    Q_INVOKABLE QString rateText();

signals:
    void positionUpdated(const QGeoPositionInfo& update);
    void started();
    void stopped();

private:
    PositionInfoSource* m_source = nullptr;
    QTimer m_timer;
    double m_lastUpdateX; 
    double m_lastUpdateY;
    void startSource();
    void resetSource();
};
