#include "pch.h"

// Copied from Qt sources for QT 6: https://github.com/qt/qtpositioning/commit/dd4200e16b49224ad2ff0f39415cddf165561b79.
// Handles using Gnss API which is required to get satellites on target SDK 31+ (Android 12+).
// TODO(justin): Remove after migrating to Qt 6.

class SatelliteInfoSourceAndroid : public QGeoSatelliteInfoSource
{
    Q_OBJECT
public:
    explicit SatelliteInfoSourceAndroid(QObject *parent = 0);
    ~SatelliteInfoSourceAndroid();

    //From QGeoSatelliteInfoSource
    void setUpdateInterval(int msec);
    int minimumUpdateInterval() const;

    Error error() const;

public Q_SLOTS:
    void startUpdates();
    void stopUpdates();
    void requestUpdate(int timeout = 0);

    void processSatelliteUpdateInView(const QList<QGeoSatelliteInfo> &satsInView, bool isSingleUpdate);
    void processSatelliteUpdateInUse(const QList<QGeoSatelliteInfo> &satsInUse, bool isSingleUpdate);

    void locationProviderDisabled();
private Q_SLOTS:
    void requestTimeout();

private:
    void reconfigureRunningSystem();

    Error m_error;
    int androidClassKeyForUpdate;
    int androidClassKeyForSingleRequest;
    bool updatesRunning;

    QTimer requestTimer;
    QList<QGeoSatelliteInfo> m_satsInUse;
    QList<QGeoSatelliteInfo> m_satsInView;
};
