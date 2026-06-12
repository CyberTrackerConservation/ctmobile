#pragma once
#include "pch.h"
#include <QGeoPositionInfoSourceFactory.h>

namespace CyberTracker {

class LocationFactory : public QObject, public QGeoPositionInfoSourceFactory
{
    Q_OBJECT

    Q_PLUGIN_METADATA (IID "org.qt-project.qt.position.sourcefactory/6.0" FILE "LocationPlugin.json")

    Q_INTERFACES (QGeoPositionInfoSourceFactory)

public:
    QGeoPositionInfoSource* positionInfoSource(QObject* parent, const QVariantMap& parameters) override;
    QGeoSatelliteInfoSource* satelliteInfoSource(QObject* parent, const QVariantMap& parameters) override;
    QGeoAreaMonitorSource* areaMonitor(QObject* parent, const QVariantMap& parameters) override;
};

class SimulateNmeaServer : public QObject
{
    Q_OBJECT

public:
    SimulateNmeaServer(QObject* parent = nullptr);
    ~SimulateNmeaServer();

    bool start(const QString& nmeaFilePath);
    void stop();

    QGeoPositionInfo currentPositionInfo() const;
    
private slots:
    void onTimer();

private:
    QTimer m_timer;
    QVector<QGeoPositionInfo> m_positions;
    int m_lineCounter = 0;
    int m_lineStep = 1;
    QGeoPositionInfo m_currentPositionInfo;
};

class PositionInfoSource : public QGeoPositionInfoSource
{
    Q_OBJECT

public:
    explicit PositionInfoSource(QObject* parent = nullptr);
    ~PositionInfoSource();

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
    qint64 m_lastKnownTimestamp = 0;
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
    Q_PROPERTY (qint64 t READ t CONSTANT)
    Q_PROPERTY (double direction READ direction CONSTANT)
    Q_PROPERTY (double accuracy READ accuracy CONSTANT)
    Q_PROPERTY (double speed READ speed CONSTANT)
    Q_PROPERTY (bool stationary READ stationary CONSTANT)
    Q_PROPERTY (QString timestamp READ timestamp CONSTANT)
    Q_PROPERTY (int counter READ counter CONSTANT)
    Q_PROPERTY (int interval READ interval CONSTANT)
    Q_PROPERTY (int distanceFilter READ distanceFilter CONSTANT)
    Q_PROPERTY (int accuracyFilter READ accuracyFilter CONSTANT)
    Q_PROPERTY (QVariantMap toMap READ toMap CONSTANT)

public:
    explicit Location(QObject* parent = nullptr);
    explicit Location(const Location& location, QObject* parent = nullptr);
    explicit Location(const QVariantMap& data, QObject* parent = nullptr);
    explicit Location(const QGeoPositionInfo& positionInfo, QObject* parent = nullptr);
    explicit Location(const char* nmeaBuffer, int size, QObject* parent = nullptr);

    bool isValid() const;
    bool isAccurate() const;
    QVariant toPoint() const;
    QVariantMap toMap() const;
    QGeoPositionInfo toGeoPositionInfo() const;
    double distanceTo(double x, double y) const;
    double headingTo(double x, double y) const;

    double x() const;
    void set_x(double value);
    double y() const;
    void set_y(double value);
    double z() const;
    void set_z(double value);
    qint64 t() const;
    void set_t(qint64 value);
    double direction() const;
    void set_direction(double value);
    double accuracy() const;
    void set_accuracy(double value);
    double speed() const;
    void set_speed(double value);
    bool stationary() const;
    void set_stationary(bool value);
    QString timestamp() const;
    void set_timestamp(const QString& isoDateTime);
    int counter() const;
    void set_counter(int value);
    int interval() const;
    void set_interval(int value);
    int distanceFilter() const;
    void set_distanceFilter(int value);
    int accuracyFilter() const;
    void set_accuracyFilter(int value);

private:
    double m_x = 0, m_y = 0, m_z = 0;
    qint64 m_t = 0;
    double m_d = 0, m_a = 0, m_s = 0;
    bool m_stationary = false;
    int m_counter = -1;
    int m_interval = 0;
    int m_distanceFilter = 0;
    int m_accuracyFilter = 0;
};

class LocationReader
{
public:
    explicit LocationReader(const QString& filePath);

    int lineCount() const;
    void enumLines(int startIndex, std::function<void(int index, const QString& uid, qint64 t, Location* location)> callback);

private:
    QFile m_reader;
};

class LocationWriter
{
public:
    explicit LocationWriter(const QString& filePath);
    ~LocationWriter();

    void reset();
    void append(const QString& uid, double x, double y, double z, qint64 t, double accuracy, double speed, double direction, int satelliteCount, bool stationary);

private:
    QFile m_writer;
};

class LocationStreamer: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (bool, running)
    QML_READONLY_AUTO_PROPERTY (int, updateInterval)
    QML_READONLY_AUTO_PROPERTY (double, distanceFilter)
    QML_READONLY_AUTO_PROPERTY (double, distanceCovered)
    QML_READONLY_AUTO_PROPERTY (int, counter)
    QML_READONLY_AUTO_PROPERTY (QString, rateText)
    QML_READONLY_AUTO_PROPERTY (QString, rateFullText)
    QML_WRITABLE_AUTO_PROPERTY (QString, rateLabel)

    QML_READONLY_AUTO_PROPERTY (int, lastUpdateInterval)
    QML_READONLY_AUTO_PROPERTY (int, lastDistanceFilter)

public:
    explicit LocationStreamer(QObject* parent = nullptr);
    explicit LocationStreamer(const QString& filename, int updateInterval, double distanceFilter = 0, double distanceCovered = 0, int counter = 0, QObject* parent = nullptr);
    ~LocationStreamer();

    void processLog();

    void loadState(const QVariantMap& map);
    QVariantMap saveState() const;

    void pushUpdate(const QString& uid, Location* location);

    bool isQueueEmpty() const;
    void enumQueue(const std::function<void(const QString& locationUid, Location* location)>& callback) const;
    void clearQueue();

    Q_INVOKABLE void start(int updateInterval, double distanceFilter = 0, double distanceCovered = 0, int counter = 0);
    Q_INVOKABLE void stop();

signals:
    void locationsAvailable(int count);
    void started();
    void stopped();

private slots:
    void timeout();

private:
    bool m_simulate = false;
    QVector<QPair<QString, Location*>> m_queue;
    LocationReader m_reader;
    QTimer m_timer;
    QString m_interestUid;
    double m_lastUpdateX;
    double m_lastUpdateY;
    qint64 m_lastUpdateT;
    uint m_lastUpdateLine;
    void startSource();
    void resetSource();
    void updateRateText();
    void appendToQueue(const QString& uid, Location* location);
};

class LocationLogger: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (bool, running)
    QML_READONLY_AUTO_PROPERTY (QString, filePath)

public:
    enum class State {
        StateOff = 0,
        StateNoData = 1,
        StateLowAccuracy = 2,
        StateOutlier = 3,
        StateFix = 4
    };
    Q_ENUM(State)

    QML_READONLY_AUTO_PROPERTY(State, state)
    QML_READONLY_AUTO_PROPERTY(QString, stateIcon)
    QML_READONLY_AUTO_PROPERTY(QColor, stateColor)

public:
    explicit LocationLogger(const QString& filePath, QObject* parent = nullptr);
    ~LocationLogger();

    bool hasInterest(const QString& interestUid) const;
    void takeInterest(const QString& interestUid);
    void loseInterest(const QString& interestUid);

    bool requestServiceSync();

    Q_INVOKABLE void updateState();

private slots:
    void positionUpdated(const QGeoPositionInfo& update);
    void serviceSync();

private:
    LocationWriter* m_writer = nullptr;
    QMap<QString, bool> m_interest;
    PositionInfoSource* m_source = nullptr;
    LocationReader m_reader;
    QTimer m_stateTimer;
    QTimer m_serviceSyncTimer;
    int m_lastLineCount = 0;

    bool start();
    void stop();
};

} // namespace CyberTracker
