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

    int minimumUpdateInterval() const override;
    void setUpdateInterval(int msec) override;
    QGeoSatelliteInfoSource::Error error() const override;

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

class Location: public QObject
{
    Q_OBJECT

    Q_PROPERTY (bool isValid READ isValid CONSTANT)
    Q_PROPERTY (bool isAccurate READ isAccurate CONSTANT)
    Q_PROPERTY (double x READ x CONSTANT)
    Q_PROPERTY (double y READ y CONSTANT)
    Q_PROPERTY (double z READ z CONSTANT)
    Q_PROPERTY (double direction READ direction CONSTANT)
    Q_PROPERTY (double accuracy READ accuracy CONSTANT)
    Q_PROPERTY (double speed READ speed CONSTANT)
    Q_PROPERTY (QString timestamp READ timestamp CONSTANT)
    Q_PROPERTY (int counter READ counter CONSTANT)
    Q_PROPERTY (int interval READ interval CONSTANT)
    Q_PROPERTY (int distanceFilter READ distanceFilter CONSTANT)
    Q_PROPERTY (int accuracyFilter READ accuracyFilter CONSTANT)
    Q_PROPERTY (QString deviceId READ deviceId CONSTANT)
    Q_PROPERTY (QVariantMap spatialReference READ spatialReference CONSTANT)
    Q_PROPERTY (QVariantMap toMap READ toMap CONSTANT)

public:
    explicit Location(QObject* parent = nullptr);
    explicit Location(const Location& location, QObject* parent = nullptr);
    explicit Location(const QVariantMap& data, QObject* parent = nullptr);
    explicit Location(const QGeoPositionInfo& positionInfo, QObject* parent = nullptr);

    bool isValid() const;
    bool isAccurate() const;
    QVariant toPoint() const;
    QVariantMap toMap() const;
    double distanceTo(double x, double y) const;
    double headingTo(double x, double y) const;

    double x() const;
    void set_x(double value);
    double y() const;
    void set_y(double value);
    double z() const;
    void set_z(double value);
    double direction() const;
    void set_direction(double value);
    double accuracy() const;
    void set_accuracy(double value);
    double speed() const;
    void set_speed(double value);
    QString timestamp() const;
    void set_timestamp(const QString& value);
    int counter() const;
    void set_counter(int value);
    int interval() const;
    void set_interval(int value);
    int distanceFilter() const;
    void set_distanceFilter(int value);
    int accuracyFilter() const;
    void set_accuracyFilter(int value);
    QString deviceId() const;
    void set_deviceId(const QString& value);
    QVariantMap spatialReference() const;

private:
    double m_x = 0, m_y = 0, m_z = 0;
    double m_d = 0, m_a = 0, m_s = 0;
    QString m_ts;
    int m_counter = -1;
    int m_interval = 0;
    int m_distanceFilter = 0;
    int m_accuracyFilter = 0;
    QString m_deviceId;
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
    QML_READONLY_AUTO_PROPERTY (QString, rateIcon)
    QML_READONLY_AUTO_PROPERTY (QString, rateText)
    QML_READONLY_AUTO_PROPERTY (QString, rateFullText)
    QML_WRITABLE_AUTO_PROPERTY (QString, rateLabel)

public:
    explicit LocationStreamer(QObject* parent = nullptr);
    ~LocationStreamer();

    void loadState(const QVariantMap& map);
    QVariantMap saveState();

    void pushUpdate(Location* update);

    Q_INVOKABLE void start(int updateInterval, double distanceFilter = 0, double distanceCovered = 0, int counter = 0, bool initialRecent = false);
    Q_INVOKABLE void stop();

signals:
    void locationUpdated(Location* update);
    void started();
    void stopped();

private:
    PositionInfoSource* m_source = nullptr;
    QTimer m_timer;
    double m_lastUpdateX; 
    double m_lastUpdateY;
    void startSource();
    void resetSource();
    void updateDisplay();
};
