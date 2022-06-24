#include "Compass.h"

// Something confusing iOS builds.
#if defined(Q_OS_IOS)
#pragma clang optimize off
#endif

//=====================================================================================================================
// SensorFuser.

SensorFuser::SensorFuser()
{
    reset();
}

void SensorFuser::reset()
{
    m_firstFuse = true;
    m_frequencyInMilliseconds = 0;
    m_accelerometerTimestamp = m_magnetometerTimestamp = m_gyroscopeTimestamp = 0;
    memset(m_accelerometerReading, 0, sizeof(m_accelerometerReading));
    memset(m_magnetometerReading, 0, sizeof(m_magnetometerReading));
    memset(m_gyroscopeRotationMatrix, 0, sizeof(m_gyroscopeRotationMatrix));
    memset(m_gyroscopeOrientationVector, 0, sizeof(m_gyroscopeOrientationVector));
}

int SensorFuser::frequencyInMilliseconds() const
{
    return m_frequencyInMilliseconds;
}

void SensorFuser::setAccelerometerReading(qreal x, qreal y, qreal z, quint64 timestamp)
{
    m_accelerometerMutex.lock();
    m_accelerometerReading[0] = x;
    m_accelerometerReading[1] = y;
    m_accelerometerReading[2] = z;
    m_accelerometerTimestamp = timestamp;
    m_accelerometerMutex.unlock();
}

void SensorFuser::setMagnetometerReading(qreal x, qreal y, qreal z, quint64 timestamp)
{
    m_magnetometerMutex.lock();
    m_magnetometerReading[0] = x;
    m_magnetometerReading[1] = y;
    m_magnetometerReading[2] = z;
    m_magnetometerTimestamp = timestamp;
    m_magnetometerMutex.unlock();
}

void SensorFuser::setGyroscopeReading(qreal x, qreal y, qreal z, quint64 timestamp)
{
    // Compute update frequency.
    m_frequencyInMilliseconds = qMin((quint64)250, (timestamp - m_gyroscopeTimestamp) / 1000);

    // Calculate rotation matrix from gyro reading according to Android developer documentation.
    const qreal EPSILON = 1e-9f;
    const qreal microsecondsToSeconds = 1.0f / (1000*1000);
    const qreal dt = (timestamp - m_gyroscopeTimestamp) * microsecondsToSeconds;

    // Axis of the rotation sample, not normalized yet.
    qreal axisX = DEG2RAD(x);
    qreal axisY = DEG2RAD(y);
    qreal axisZ = DEG2RAD(z);

    // Calculate the angular speed of the sample.
    qreal omegaMagnitude = qSqrt(axisX * axisX + axisY * axisY + axisZ * axisZ);

    // Normalize the rotation vector if it's big enough to get the axis
    if (omegaMagnitude > EPSILON)
    {
        axisX /= omegaMagnitude;
        axisY /= omegaMagnitude;
        axisZ /= omegaMagnitude;
    }

    // Integrate around this axis with the angular speed by the timestep
    // in order to get a delta rotation from this sample over the timestep
    // We will convert this axis-angle representation of the delta rotation
    // into a quaternion before turning it into the rotation matrix.

    qreal thetaOverTwo = omegaMagnitude * dt / 2.0f;
    qreal sinThetaOverTwo = qSin(thetaOverTwo);
    qreal cosThetaOverTwo = qCos(thetaOverTwo);
    qreal deltaRotationVector[4];
    deltaRotationVector[0] = sinThetaOverTwo * axisX;
    deltaRotationVector[1] = sinThetaOverTwo * axisY;
    deltaRotationVector[2] = sinThetaOverTwo * axisZ;
    deltaRotationVector[3] = cosThetaOverTwo;

    qreal rotationMatrix[9];
    getRotationMatrixFromVector(deltaRotationVector, rotationMatrix);

    m_gyroscopeMutex.lock();

    qreal temp[9];
    matrixMultiplication(m_gyroscopeRotationMatrix, rotationMatrix, temp);
    std::copy_n(temp, 9, m_gyroscopeRotationMatrix);
    getOrientationVector(m_gyroscopeRotationMatrix, m_gyroscopeOrientationVector);
    m_gyroscopeTimestamp = timestamp;

    m_gyroscopeMutex.unlock();
}

void SensorFuser::matrixMultiplication(qreal* A, qreal* B, qreal* result)
{
    result[0] = A[0] * B[0] + A[1] * B[3] + A[2] * B[6];
    result[1] = A[0] * B[1] + A[1] * B[4] + A[2] * B[7];
    result[2] = A[0] * B[2] + A[1] * B[5] + A[2] * B[8];

    result[3] = A[3] * B[0] + A[4] * B[3] + A[5] * B[6];
    result[4] = A[3] * B[1] + A[4] * B[4] + A[5] * B[7];
    result[5] = A[3] * B[2] + A[4] * B[5] + A[5] * B[8];

    result[6] = A[6] * B[0] + A[7] * B[3] + A[8] * B[6];
    result[7] = A[6] * B[1] + A[7] * B[4] + A[8] * B[7];
    result[8] = A[6] * B[2] + A[7] * B[5] + A[8] * B[8];
}

// Ported from android.hardware.SensorManager.getOrientation().
void SensorFuser::getOrientationVector(qreal* rotationMatrix, qreal* result)
{
    result[0] = qAtan2(rotationMatrix[1], rotationMatrix[4]);
    result[1] = qAsin(-rotationMatrix[7]);
    result[2] = qAtan2(-rotationMatrix[6], rotationMatrix[8]);
}

// Ported from http://www.thousand-thoughts.com/2012/03/android-sensor-fusion-tutorial/2/.
void SensorFuser::getRotationMatrixFromOrientationVector(qreal* orientationVector, qreal* result)
{
    qreal xM[9];
    qreal yM[9];
    qreal zM[9];

    qreal sinX = qSin(orientationVector[1]);
    qreal cosX = qCos(orientationVector[1]);
    qreal sinY = qSin(orientationVector[2]);
    qreal cosY = qCos(orientationVector[2]);
    qreal sinZ = qSin(orientationVector[0]);
    qreal cosZ = qCos(orientationVector[0]);

    // Rotation about x-axis (pitch).
    xM[0] = 1.0f; xM[1] = 0.0f; xM[2] = 0.0f;
    xM[3] = 0.0f; xM[4] = cosX; xM[5] = sinX;
    xM[6] = 0.0f; xM[7] = -sinX; xM[8] = cosX;

    // Rotation about y-axis (roll).
    yM[0] = cosY; yM[1] = 0.0f; yM[2] = sinY;
    yM[3] = 0.0f; yM[4] = 1.0f; yM[5] = 0.0f;
    yM[6] = -sinY; yM[7] = 0.0f; yM[8] = cosY;

    // Rotation about z-axis (azimuth).
    zM[0] = cosZ; zM[1] = sinZ; zM[2] = 0.0f;
    zM[3] = -sinZ; zM[4] = cosZ; zM[5] = 0.0f;
    zM[6] = 0.0f; zM[7] = 0.0f; zM[8] = 1.0f;

    // Rotation order is y, x, z (roll, pitch, azimuth).
    qreal temp[9];
    matrixMultiplication(xM, yM, temp);
    matrixMultiplication(zM, temp, result);
}

// Ported from android.hardware.SensorManager.getRotationMatrix().
bool SensorFuser::getRotationMatrix(qreal* gravity, qreal* geomagnetic, qreal* result)
{
    qreal Ax = gravity[0];
    qreal Ay = gravity[1];
    qreal Az = gravity[2];

    // Change relative to Android version: scale values from teslas to microteslas.
    const qreal to_micro = 1000 * 1000;
    const qreal Ex = to_micro * geomagnetic[0];
    const qreal Ey = to_micro * geomagnetic[1];
    const qreal Ez = to_micro * geomagnetic[2];

    qreal Hx = Ey*Az - Ez*Ay;
    qreal Hy = Ez*Ax - Ex*Az;
    qreal Hz = Ex*Ay - Ey*Ax;

    const qreal normH = qSqrt(Hx*Hx + Hy*Hy + Hz*Hz);
    if (normH < 0.1f)
    {
        // Device is close to free fall (or in space?), or close to
        // magnetic north pole. Typical values are > 100.
        return false;
    }

    const qreal invH = 1.0f / normH;
    Hx *= invH;
    Hy *= invH;
    Hz *= invH;

    const qreal invA = 1.0f / qSqrt(Ax*Ax + Ay*Ay + Az*Az);
    Ax *= invA;
    Ay *= invA;
    Az *= invA;

    const qreal Mx = Ay*Hz - Az*Hy;
    const qreal My = Az*Hx - Ax*Hz;
    const qreal Mz = Ax*Hy - Ay*Hx;

    result[0] = Hx; result[1] = Hy; result[2] = Hz;
    result[3] = Mx; result[4] = My; result[5] = Mz;
    result[6] = Ax; result[7] = Ay; result[8] = Az;

    return true;
}

// Ported from android.hardware.SensorManager.getRotationMatrixFromVector().
void SensorFuser::getRotationMatrixFromVector(qreal* rotationVector, qreal* result)
{
    qreal q0;
    qreal q1 = rotationVector[0];
    qreal q2 = rotationVector[1];
    qreal q3 = rotationVector[2];

    q0 = rotationVector[3];

    qreal sq_q1 = 2 * q1 * q1;
    qreal sq_q2 = 2 * q2 * q2;
    qreal sq_q3 = 2 * q3 * q3;
    qreal q1_q2 = 2 * q1 * q2;
    qreal q3_q0 = 2 * q3 * q0;
    qreal q1_q3 = 2 * q1 * q3;
    qreal q2_q0 = 2 * q2 * q0;
    qreal q2_q3 = 2 * q2 * q3;
    qreal q1_q0 = 2 * q1 * q0;

    result[0] = 1 - sq_q2 - sq_q3;
    result[1] = q1_q2 - q3_q0;
    result[2] = q1_q3 + q2_q0;

    result[3] = q1_q2 + q3_q0;
    result[4] = 1 - sq_q1 - sq_q3;
    result[5] = q2_q3 - q1_q0;

    result[6] = q1_q3 - q2_q0;
    result[7] = q2_q3 + q1_q0;
    result[8] = 1 - sq_q1 - sq_q2;
}

qreal SensorFuser::fuseOrientationCoefficient(qreal gyro, qreal acc_mag)
{
    const qreal FILTER_COEFFICIENT = 0.98f;
    const qreal ONE_MINUS_COEFF = 1.0f - FILTER_COEFFICIENT;

    qreal result;

    if (gyro < -0.5 * M_PI && acc_mag > 0.0)
    {
        result = (qreal) (FILTER_COEFFICIENT * (gyro + 2.0 * M_PI) + ONE_MINUS_COEFF * acc_mag);
        result -= (result > M_PI) ? 2.0 * M_PI : 0;
    }
    else if (acc_mag < -0.5 * M_PI && gyro > 0.0)
    {
        result = (qreal) (FILTER_COEFFICIENT * gyro + ONE_MINUS_COEFF * (acc_mag + 2.0 * M_PI));
        result -= (result > M_PI)? 2.0 * M_PI : 0;
    }
    else
    {
        result = FILTER_COEFFICIENT * gyro + ONE_MINUS_COEFF * acc_mag;
    }

    return result;
}

qreal SensorFuser::computeAzimuth()
{
    if (m_accelerometerTimestamp == 0 || m_magnetometerTimestamp == 0 || m_gyroscopeTimestamp == 0)
    {
        return 0;
    }

    m_accelerometerMutex.lock();
    auto accelerometerReading = m_accelerometerReading;
    m_accelerometerMutex.unlock();

    m_magnetometerMutex.lock();
    auto magnetometerReading = m_magnetometerReading;
    m_magnetometerMutex.unlock();

    // Calculate orientation from accelerometer and magnetometer.
    qreal rotationMatrix[9];
    getRotationMatrix(accelerometerReading, magnetometerReading, rotationMatrix);

    qreal orientationVector[3];
    getOrientationVector(rotationMatrix, orientationVector);

    // Filter orientationVector and gyroscopeOrientationVector to fusedOrientationVector.
    m_gyroscopeMutex.lock();

    qreal fusedOrientationVector[3];
    fusedOrientationVector[0] = fuseOrientationCoefficient(m_gyroscopeOrientationVector[0], orientationVector[0]);
    fusedOrientationVector[1] = fuseOrientationCoefficient(m_gyroscopeOrientationVector[1], orientationVector[1]);
    fusedOrientationVector[2] = fuseOrientationCoefficient(m_gyroscopeOrientationVector[2], orientationVector[2]);
    getRotationMatrixFromOrientationVector(fusedOrientationVector, m_gyroscopeRotationMatrix);
    std::copy_n(fusedOrientationVector, 3, m_gyroscopeOrientationVector);

    m_gyroscopeMutex.unlock();

    return RAD2DEG(fusedOrientationVector[0]);
}

//=====================================================================================================================
// Sensor filters.

class AccelerometerFilter: public QAccelerometerFilter
{
public:
    AccelerometerFilter(SensorFuser* sensorFuser): m_sensorFuser(sensorFuser)
    {}

    bool filter(QAccelerometerReading* reading) override
    {
        m_sensorFuser->setAccelerometerReading(reading->x(), reading->y(), reading->z(), reading->timestamp());
        return false;
    }

private:
    SensorFuser* m_sensorFuser;
};

class MagnetometerFilter: public QMagnetometerFilter
{
public:
    MagnetometerFilter(SensorFuser* sensorFuser): m_sensorFuser(sensorFuser)
    {}

    bool filter(QMagnetometerReading* reading) override
    {
        m_sensorFuser->setMagnetometerReading(reading->x(), reading->y(), reading->z(), reading->timestamp());

        return false;
    }

private:
    SensorFuser* m_sensorFuser;
};

class GyroscopeFilter: public QGyroscopeFilter
{
public:
    GyroscopeFilter(SensorFuser* sensorFuser): m_sensorFuser(sensorFuser)
    {}

    bool filter(QGyroscopeReading* reading) override
    {
        m_sensorFuser->setGyroscopeReading(reading->x(), reading->y(), reading->z(), reading->timestamp());

        return false;
    }

private:
    SensorFuser* m_sensorFuser;
};

//=====================================================================================================================
// Polling thread.

class Thread: public QThread
{
public:
    Thread(Compass* compass, SensorFuser* sensorFuser)
    {
        m_compass = compass;
        m_sensorFuser = sensorFuser;
    }

    void run() override
    {
        while (!m_compass->simulated() && m_compass->active() && m_sensorFuser->frequencyInMilliseconds() == 0)
        {
            QThread::msleep(60);
        }

        qreal lastAzimuth = 0;
        while (m_compass->active())
        {
            auto azimuth = 0.0f;
            auto sleepTime = 0;

            if (m_compass->simulated())
            {
                azimuth = m_compass->simulatedAzimuth();
                sleepTime = 250;
            }
            else
            {
                azimuth = m_sensorFuser->computeAzimuth();
                sleepTime = m_sensorFuser->frequencyInMilliseconds();
            }

            if (lastAzimuth != azimuth)
            {
                emit m_compass->azimuthChanged(azimuth);
            }

            lastAzimuth = azimuth;

            QThread::msleep(sleepTime);
        }
    }

private:
    SensorFuser* m_sensorFuser;
    Compass* m_compass;
};

//=====================================================================================================================
// Compass.

Compass::Compass(QObject* parent): QObject(parent)
{
    if (m_accelerometer.isFeatureSupported(QSensor::AccelerationMode))
    {
        m_accelerometer.setAccelerationMode(QAccelerometer::Gravity);
    }

    m_magnetometer.setReturnGeoValues(true);

    m_simulated = false;
    m_simulatedAzimuth = 0;
}

Compass::~Compass()
{
    while (m_refCount > 0)
    {
        stop();
    }
}

bool Compass::active()
{
    return m_refCount > 0;
}

void Compass::start()
{
    m_refCount++;
    if (m_refCount == 1)
    {
        m_sensorFuser.reset();

        m_accelerometerFilter = new AccelerometerFilter(&m_sensorFuser);
        m_accelerometer.addFilter(m_accelerometerFilter);
        m_accelerometer.start();

        m_magnetometerFilter = new MagnetometerFilter(&m_sensorFuser);
        m_magnetometer.addFilter(m_magnetometerFilter);
        m_magnetometer.start();

        m_gyroscopeFilter = new GyroscopeFilter(&m_sensorFuser);
        m_gyroscope.addFilter(m_gyroscopeFilter);
        m_gyroscope.start();

        m_thread = new Thread(this, &m_sensorFuser);
        m_thread->start();
    }
}

void Compass::stop()
{
    m_refCount--;
    if (m_refCount == 0)
	{
        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;

        m_accelerometer.removeFilter(m_accelerometerFilter);
        delete m_accelerometerFilter;
        m_accelerometerFilter = nullptr;

        m_magnetometer.removeFilter(m_magnetometerFilter);
        delete m_magnetometerFilter;
        m_magnetometerFilter = nullptr;

        m_gyroscope.removeFilter(m_gyroscopeFilter);
        delete m_gyroscopeFilter;
        m_gyroscopeFilter = nullptr;

        m_sensorFuser.reset();
	}
}

int Compass::frequencyInMilliseconds()
{
    return m_sensorFuser.frequencyInMilliseconds();
}

//=====================================================================================================================

#if defined(Q_OS_IOS)
#pragma clang optimize on
#endif
