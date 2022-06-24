#pragma once
#include "pch.h"

class SensorFuser
{
public:
    SensorFuser();

    void reset();

    void setAccelerometerReading(qreal x, qreal y, qreal z, quint64 timestamp);
    void setMagnetometerReading(qreal x, qreal y, qreal z, quint64 timestamp);
    void setGyroscopeReading(qreal x, qreal y, qreal z, quint64 timestamp);

    qreal computeAzimuth();

    int frequencyInMilliseconds() const;

private:
    bool m_firstFuse = true;
    int m_frequencyInMilliseconds = 0;

    QMutex m_accelerometerMutex;
    qreal m_accelerometerReading[3];
    quint64 m_accelerometerTimestamp;

    QMutex m_magnetometerMutex;
    qreal m_magnetometerReading[3];
    quint64 m_magnetometerTimestamp;

    QMutex m_gyroscopeMutex;
    qreal m_gyroscopeRotationMatrix[9];
    qreal m_gyroscopeOrientationVector[3];
    quint64 m_gyroscopeTimestamp;

    void matrixMultiplication(qreal* A, qreal* B, qreal* result);
    void getOrientationVector(qreal* rotationMatrix, qreal* result);
    void getRotationMatrixFromOrientationVector(qreal* orientation, qreal* result);
    bool getRotationMatrix(qreal* gravity, qreal* geomagnetic, qreal* result);
    void getRotationMatrixFromVector(qreal* rotationVector, qreal* result);
    qreal fuseOrientationCoefficient(qreal gyro, qreal acc_mag);
};

class Compass: public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (bool, simulated)
    QML_WRITABLE_AUTO_PROPERTY (qreal, simulatedAzimuth)

public:
    Compass(QObject* parent = nullptr);
    ~Compass() override;

    bool active();
    void start();
    void stop();

    int frequencyInMilliseconds();

signals:
    void azimuthChanged(qreal azimuth);
	
private:
    int m_refCount = 0;
    QThread* m_thread = nullptr;
    SensorFuser m_sensorFuser;
    QAccelerometer m_accelerometer;
    QAccelerometerFilter* m_accelerometerFilter = nullptr;
    QMagnetometer m_magnetometer;
    QMagnetometerFilter* m_magnetometerFilter = nullptr;
    QGyroscope m_gyroscope;
    QGyroscopeFilter* m_gyroscopeFilter = nullptr;
};

