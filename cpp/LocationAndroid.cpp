// Copied from Qt sources for QT 6: https://github.com/qt/qtpositioning/commit/dd4200e16b49224ad2ff0f39415cddf165561b79.
// Handles using Gnss API which is required to get satellites on target SDK 31+ (Android 12+).
// TODO(justin): Remove after migrating to Qt 6.

#include "LocationAndroid.h"

// jniPositioning.cpp from
#define UPDATE_FROM_COLD_START 2*60*1000

static JavaVM *javaVM = nullptr;
static jclass positioningClass;

static jmethodID providerListMethodId;
static jmethodID lastKnownPositionMethodId;
static jmethodID startUpdatesMethodId;
static jmethodID stopUpdatesMethodId;
static jmethodID requestUpdateMethodId;
static jmethodID startSatelliteUpdatesMethodId;

static const char logTag[] = "QtPositioning";
static const char classErrorMsg[] = "Can't find class \"%s\"";
static const char methodErrorMsg[] = "Can't find method \"%s%s\"";

namespace AndroidPositioning {
    typedef QMap<int, QGeoPositionInfoSource * > PositionSourceMap;
    typedef QMap<int, SatelliteInfoSourceAndroid * > SatelliteSourceMap;

    Q_GLOBAL_STATIC(PositionSourceMap, idToPosSource)
    Q_GLOBAL_STATIC(SatelliteSourceMap, idToSatSource)

    struct AttachedJNIEnv
    {
        AttachedJNIEnv()
        {
            attached = false;
            if (javaVM && javaVM->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6) < 0) {
                if (javaVM->AttachCurrentThread(&jniEnv, nullptr) < 0) {
                    //__android_log_print(ANDROID_LOG_ERROR, logTag, "AttachCurrentThread failed");
                    jniEnv = nullptr;
                    return;
                }
                attached = true;
            }
        }

        ~AttachedJNIEnv()
        {
            if (attached)
                javaVM->DetachCurrentThread();
        }
        bool attached;
        JNIEnv *jniEnv;
    };

    int registerPositionInfoSource(QObject *obj)
    {
        static bool firstInit = true;
        if (firstInit) {
            firstInit = false;
        }

        int key = -1;
        if (obj->inherits("QGeoPositionInfoSource")) {
            QGeoPositionInfoSource *src = qobject_cast<QGeoPositionInfoSource *>(obj);
            Q_ASSERT(src);
            do {
                key = qAbs(int(QRandomGenerator::global()->generate()));
            } while (idToPosSource()->contains(key));

            idToPosSource()->insert(key, src);
        } else if (obj->inherits("QGeoSatelliteInfoSource")) {
            SatelliteInfoSourceAndroid *src = qobject_cast<SatelliteInfoSourceAndroid *>(obj);
            Q_ASSERT(src);
            do {
                key = qAbs(int(QRandomGenerator::global()->generate()));
            } while (idToSatSource()->contains(key));

            idToSatSource()->insert(key, src);
        }

        return key;
    }

    void unregisterPositionInfoSource(int key)
    {
        idToPosSource()->remove(key);
        idToSatSource()->remove(key);
    }

    enum PositionProvider
    {
        PROVIDER_GPS = 0,
        PROVIDER_NETWORK = 1,
        PROVIDER_PASSIVE = 2
    };


    QGeoPositionInfoSource::PositioningMethods availableProviders()
    {
        QGeoPositionInfoSource::PositioningMethods ret = QGeoPositionInfoSource::NoPositioningMethods;
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return ret;
        jintArray jProviders = static_cast<jintArray>(env.jniEnv->CallStaticObjectMethod(
                                                          positioningClass, providerListMethodId));
        jint *providers = env.jniEnv->GetIntArrayElements(jProviders, nullptr);
        const int size = env.jniEnv->GetArrayLength(jProviders);
        for (int i = 0; i < size; i++) {
            switch (providers[i]) {
            case PROVIDER_GPS:
                ret |= QGeoPositionInfoSource::SatellitePositioningMethods;
                break;
            case PROVIDER_NETWORK:
                ret |= QGeoPositionInfoSource::NonSatellitePositioningMethods;
                break;
            case PROVIDER_PASSIVE:
                //we ignore as Qt doesn't have interface for it right now
                break;
            default:;
            }
        }

        env.jniEnv->ReleaseIntArrayElements(jProviders, providers, 0);
        env.jniEnv->DeleteLocalRef(jProviders);

        return ret;
    }

    //caching originally taken from corelib/kernel/qjni.cpp
    typedef QHash<QByteArray, jmethodID> JMethodIDHash;
    Q_GLOBAL_STATIC(JMethodIDHash, cachedMethodID)

    static jmethodID getCachedMethodID(JNIEnv *env,
                                       jclass clazz,
                                       const char *name,
                                       const char *sig)
    {
        jmethodID id = nullptr;
        uint offset_name = qstrlen(name);
        uint offset_signal = qstrlen(sig);
        QByteArray key(int(offset_name + offset_signal), Qt::Uninitialized);
        memcpy(key.data(), name, offset_name);
        memcpy(key.data()+offset_name, sig, offset_signal);
        QHash<QByteArray, jmethodID>::iterator it = cachedMethodID->find(key);
        if (it == cachedMethodID->end()) {
            id = env->GetMethodID(clazz, name, sig);
            if (env->ExceptionCheck()) {
                id = nullptr;
    #ifdef QT_DEBUG
                env->ExceptionDescribe();
    #endif // QT_DEBUG
                env->ExceptionClear();
            }

            cachedMethodID->insert(key, id);
        } else {
            id = it.value();
        }
        return id;
    }

    QGeoPositionInfo positionInfoFromJavaLocation(JNIEnv * jniEnv, const jobject &location)
    {
        QGeoPositionInfo info;
        jclass thisClass = jniEnv->GetObjectClass(location);
        if (!thisClass)
            return QGeoPositionInfo();

        jmethodID mid = getCachedMethodID(jniEnv, thisClass, "getLatitude", "()D");
        jdouble latitude = jniEnv->CallDoubleMethod(location, mid);
        mid = getCachedMethodID(jniEnv, thisClass, "getLongitude", "()D");
        jdouble longitude = jniEnv->CallDoubleMethod(location, mid);
        QGeoCoordinate coordinate(latitude, longitude);

        //altitude
        mid = getCachedMethodID(jniEnv, thisClass, "hasAltitude", "()Z");
        jboolean attributeExists = jniEnv->CallBooleanMethod(location, mid);
        if (attributeExists) {
            mid = getCachedMethodID(jniEnv, thisClass, "getAltitude", "()D");
            jdouble value = jniEnv->CallDoubleMethod(location, mid);
            coordinate.setAltitude(value);
        }

        info.setCoordinate(coordinate);

        //time stamp
        mid = getCachedMethodID(jniEnv, thisClass, "getTime", "()J");
        jlong timestamp = jniEnv->CallLongMethod(location, mid);
        info.setTimestamp(QDateTime::fromMSecsSinceEpoch(timestamp, Qt::UTC));

        //horizontal accuracy
        mid = getCachedMethodID(jniEnv, thisClass, "hasAccuracy", "()Z");
        attributeExists = jniEnv->CallBooleanMethod(location, mid);
        if (attributeExists) {
            mid = getCachedMethodID(jniEnv, thisClass, "getAccuracy", "()F");
            jfloat accuracy = jniEnv->CallFloatMethod(location, mid);
            info.setAttribute(QGeoPositionInfo::HorizontalAccuracy, qreal(accuracy));
        }

        //vertical accuracy
        mid = getCachedMethodID(jniEnv, thisClass, "hasVerticalAccuracy", "()Z");
        if (mid) {
            attributeExists = jniEnv->CallBooleanMethod(location, mid);
            if (attributeExists) {
                mid = getCachedMethodID(jniEnv, thisClass, "getVerticalAccuracyMeters", "()F");
                if (mid) {
                    jfloat accuracy = jniEnv->CallFloatMethod(location, mid);
                    info.setAttribute(QGeoPositionInfo::VerticalAccuracy, qreal(accuracy));
                }
            }
        }

        if (!mid)
            jniEnv->ExceptionClear();

        //ground speed
        mid = getCachedMethodID(jniEnv, thisClass, "hasSpeed", "()Z");
        attributeExists = jniEnv->CallBooleanMethod(location, mid);
        if (attributeExists) {
            mid = getCachedMethodID(jniEnv, thisClass, "getSpeed", "()F");
            jfloat speed = jniEnv->CallFloatMethod(location, mid);
            info.setAttribute(QGeoPositionInfo::GroundSpeed, qreal(speed));
        }

        //bearing
        mid = getCachedMethodID(jniEnv, thisClass, "hasBearing", "()Z");
        attributeExists = jniEnv->CallBooleanMethod(location, mid);
        if (attributeExists) {
            mid = getCachedMethodID(jniEnv, thisClass, "getBearing", "()F");
            jfloat bearing = jniEnv->CallFloatMethod(location, mid);
            info.setAttribute(QGeoPositionInfo::Direction, qreal(bearing));
        }

        jniEnv->DeleteLocalRef(thisClass);
        return info;
    }

    QList<QGeoSatelliteInfo> satelliteInfoFromJavaLocation(JNIEnv *jniEnv,
                                                           jobjectArray satellites,
                                                           QList<QGeoSatelliteInfo>* usedInFix)
    {
        QList<QGeoSatelliteInfo> sats;
        jsize length = jniEnv->GetArrayLength(satellites);
        for (int i = 0; i<length; i++) {
            jobject element = jniEnv->GetObjectArrayElement(satellites, i);
            if (jniEnv->ExceptionOccurred()) {
                qWarning() << "Cannot process all satellite data due to exception.";
                break;
            }

            jclass thisClass = jniEnv->GetObjectClass(element);
            if (!thisClass)
                continue;

            QGeoSatelliteInfo info;

            //signal strength
            jmethodID mid = getCachedMethodID(jniEnv, thisClass, "getSnr", "()F");
            jfloat snr = jniEnv->CallFloatMethod(element, mid);
            info.setSignalStrength(int(snr));

            //ignore any satellite with no signal whatsoever
            if (qFuzzyIsNull(snr))
                continue;

            //prn
            mid = getCachedMethodID(jniEnv, thisClass, "getPrn", "()I");
            jint prn = jniEnv->CallIntMethod(element, mid);
            info.setSatelliteIdentifier(prn);

            if (prn >= 1 && prn <= 32)
                info.setSatelliteSystem(QGeoSatelliteInfo::GPS);
            else if (prn >= 65 && prn <= 96)
                info.setSatelliteSystem(QGeoSatelliteInfo::GLONASS);

            //azimuth
            mid = getCachedMethodID(jniEnv, thisClass, "getAzimuth", "()F");
            jfloat azimuth = jniEnv->CallFloatMethod(element, mid);
            info.setAttribute(QGeoSatelliteInfo::Azimuth, qreal(azimuth));

            //elevation
            mid = getCachedMethodID(jniEnv, thisClass, "getElevation", "()F");
            jfloat elevation = jniEnv->CallFloatMethod(element, mid);
            info.setAttribute(QGeoSatelliteInfo::Elevation, qreal(elevation));

            //used in a fix
            mid = getCachedMethodID(jniEnv, thisClass, "usedInFix", "()Z");
            jboolean inFix = jniEnv->CallBooleanMethod(element, mid);

            sats.append(info);

            if (inFix)
                usedInFix->append(info);

            jniEnv->DeleteLocalRef(thisClass);
            jniEnv->DeleteLocalRef(element);
        }

        return sats;
    }

    QList<QGeoSatelliteInfo> satelliteInfoFromJavaGnssStatus(JNIEnv *jniEnv,
                                                             jobject gnssStatus,
                                                             QList<QGeoSatelliteInfo>* usedInFix)
    {
        QAndroidJniObject jniStatus(gnssStatus);
        QList<QGeoSatelliteInfo> sats;

        jclass thisClass = jniEnv->GetObjectClass(gnssStatus);
        if (!thisClass)
            return QList<QGeoSatelliteInfo>();

        const int satellitesCount = jniStatus.callMethod<jint>("getSatelliteCount");
        for (int i = 0; i < satellitesCount; ++i) {
            QGeoSatelliteInfo info;

            // signal strength - this is actually a carrier-to-noise density,
            // but the values are very close to what was previously returned by
            // getSnr() method of the GpsSatellite API.

            jmethodID mid = getCachedMethodID(jniEnv, thisClass, "getCn0DbHz", "(I)F");
            const jfloat cn0 = jniEnv->CallFloatMethod(gnssStatus, mid, i);
            info.setSignalStrength(static_cast<int>(cn0));

            // satellite system
            //mid = getCachedMethodID(jniEnv, thisClass, "getConstellationType", "(I)I");
            //const jint constellationType = jniEnv->CallIntMethod(gnssStatus, mid, i);
            //info.setSatelliteSystem(ConstellationMapper::toSatelliteSystem(constellationType));

            // satellite identifier
            mid = getCachedMethodID(jniEnv, thisClass, "getSvid", "(I)I");
            const jint svId = jniEnv->CallIntMethod(gnssStatus, mid, i);
            info.setSatelliteIdentifier(svId);

            // azimuth
            mid = getCachedMethodID(jniEnv, thisClass, "getAzimuthDegrees", "(I)F");
            const jfloat azimuth = jniEnv->CallFloatMethod(gnssStatus, mid, i);
            info.setAttribute(QGeoSatelliteInfo::Azimuth, static_cast<qreal>(azimuth));

            // elevation
            mid = getCachedMethodID(jniEnv, thisClass, "getElevationDegrees", "(I)F");
            const jfloat elevation = jniEnv->CallFloatMethod(gnssStatus, mid, i);
            info.setAttribute(QGeoSatelliteInfo::Elevation, static_cast<qreal>(elevation));

            // Used in fix - true if this satellite is actually used in
            // determining the position.
            mid = getCachedMethodID(jniEnv, thisClass, "usedInFix", "(I)Z");
            const jboolean inFix = jniEnv->CallBooleanMethod(gnssStatus, mid, i);

            sats.append(info);

            if (inFix)
                usedInFix->append(info);
        }

        return sats;
    }

    QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return QGeoPositionInfo();

        jobject location = env.jniEnv->CallStaticObjectMethod(positioningClass,
                                                              lastKnownPositionMethodId,
                                                              fromSatellitePositioningMethodsOnly);
        if (location == nullptr)
            return QGeoPositionInfo();

        const QGeoPositionInfo info = positionInfoFromJavaLocation(env.jniEnv, location);
        env.jniEnv->DeleteLocalRef(location);

        return info;
    }

    inline int positioningMethodToInt(QGeoPositionInfoSource::PositioningMethods m)
    {
        int providerSelection = 0;
        if (m & QGeoPositionInfoSource::SatellitePositioningMethods)
            providerSelection |= 1;
        if (m & QGeoPositionInfoSource::NonSatellitePositioningMethods)
            providerSelection |= 2;

        return providerSelection;
    }

    QGeoPositionInfoSource::Error startUpdates(int androidClassKey)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return QGeoPositionInfoSource::UnknownSourceError;

        QGeoPositionInfoSource *source = AndroidPositioning::idToPosSource()->value(androidClassKey);

        if (source) {
            int errorCode = env.jniEnv->CallStaticIntMethod(positioningClass, startUpdatesMethodId,
                                             androidClassKey,
                                             positioningMethodToInt(source->preferredPositioningMethods()),
                                             source->updateInterval());
            switch (errorCode) {
            case 0:
            case 1:
            case 2:
            case 3:
                return static_cast<QGeoPositionInfoSource::Error>(errorCode);
            default:
                break;
            }
        }

        return QGeoPositionInfoSource::UnknownSourceError;
    }

    //used for stopping regular and single updates
    void stopUpdates(int androidClassKey)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        env.jniEnv->CallStaticVoidMethod(positioningClass, stopUpdatesMethodId, androidClassKey);
    }

    QGeoPositionInfoSource::Error requestUpdate(int androidClassKey)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return QGeoPositionInfoSource::UnknownSourceError;

        QGeoPositionInfoSource *source = AndroidPositioning::idToPosSource()->value(androidClassKey);

        if (source) {
            int errorCode = env.jniEnv->CallStaticIntMethod(positioningClass, requestUpdateMethodId,
                                             androidClassKey,
                                             positioningMethodToInt(source->preferredPositioningMethods()));
            switch (errorCode) {
            case 0:
            case 1:
            case 2:
            case 3:
                return static_cast<QGeoPositionInfoSource::Error>(errorCode);
            default:
                break;
            }
        }
        return QGeoPositionInfoSource::UnknownSourceError;
    }

    QGeoSatelliteInfoSource::Error startSatelliteUpdates(int androidClassKey, bool isSingleRequest, int requestTimeout)
    {
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return QGeoSatelliteInfoSource::UnknownSourceError;

        SatelliteInfoSourceAndroid *source = AndroidPositioning::idToSatSource()->value(androidClassKey);

        if (source) {
            int interval = source->updateInterval();
            if (isSingleRequest)
                interval = requestTimeout;
            int errorCode = env.jniEnv->CallStaticIntMethod(positioningClass, startSatelliteUpdatesMethodId,
                                             androidClassKey,
                                             interval, isSingleRequest);
            switch (errorCode) {
            case -1:
            case 0:
            case 1:
            case 2:
                return static_cast<QGeoSatelliteInfoSource::Error>(errorCode);
            default:
                qWarning() << "startSatelliteUpdates: Unknown error code " << errorCode;
                break;
            }
        }
        return QGeoSatelliteInfoSource::UnknownSourceError;
    }
}

static void positionUpdated(JNIEnv *env, jobject /*thiz*/, jobject location, jint androidClassKey, jboolean isSingleUpdate)
{
    QGeoPositionInfo info = AndroidPositioning::positionInfoFromJavaLocation(env, location);

    QGeoPositionInfoSource *source = AndroidPositioning::idToPosSource()->value(androidClassKey);
    if (!source) {
        qWarning("positionUpdated: source == 0");
        return;
    }

    //we need to invoke indirectly as the Looper thread is likely to be not the same thread
    if (!isSingleUpdate)
        QMetaObject::invokeMethod(source, "processPositionUpdate", Qt::AutoConnection,
                              Q_ARG(QGeoPositionInfo, info));
    else
        QMetaObject::invokeMethod(source, "processSinglePositionUpdate", Qt::AutoConnection,
                              Q_ARG(QGeoPositionInfo, info));
}

static void locationProvidersDisabled(JNIEnv *env, jobject /*thiz*/, jint androidClassKey)
{
    Q_UNUSED(env);
    QObject *source = AndroidPositioning::idToPosSource()->value(androidClassKey);
    if (!source)
        source = AndroidPositioning::idToSatSource()->value(androidClassKey);
    if (!source) {
        qWarning("locationProvidersDisabled: source == 0");
        return;
    }

    QMetaObject::invokeMethod(source, "locationProviderDisabled", Qt::AutoConnection);
}

static void locationProvidersChanged(JNIEnv *env, jobject /*thiz*/, jint androidClassKey)
{
    Q_UNUSED(env);
    QObject *source = AndroidPositioning::idToPosSource()->value(androidClassKey);
    if (!source) {
        qWarning("locationProvidersChanged: source == 0");
        return;
    }

    QMetaObject::invokeMethod(source, "locationProvidersChanged", Qt::AutoConnection);
}

static void satelliteGpsUpdated(JNIEnv *env, jobject /*thiz*/, jobjectArray satellites, jint androidClassKey, jboolean isSingleUpdate)
{
    QList<QGeoSatelliteInfo> inUse;
    QList<QGeoSatelliteInfo> sats = AndroidPositioning::satelliteInfoFromJavaLocation(env, satellites, &inUse);

    SatelliteInfoSourceAndroid *source = AndroidPositioning::idToSatSource()->value(androidClassKey);
    if (!source) {
        //qWarning("satelliteUpdated: source == 0");
        return;
    }

    QMetaObject::invokeMethod(source, "processSatelliteUpdateInView", Qt::AutoConnection,
                              Q_ARG(QList<QGeoSatelliteInfo>, sats), Q_ARG(bool, isSingleUpdate));

    QMetaObject::invokeMethod(source, "processSatelliteUpdateInUse", Qt::AutoConnection,
                              Q_ARG(QList<QGeoSatelliteInfo>, inUse), Q_ARG(bool, isSingleUpdate));
}

static void satelliteGnssUpdated(JNIEnv *env, jobject /*thiz*/, jobject status, jint androidClassKey, jboolean isSingleUpdate)
{
    QList<QGeoSatelliteInfo> inUse;
    QList<QGeoSatelliteInfo> sats = AndroidPositioning::satelliteInfoFromJavaGnssStatus(env, status, &inUse);

    SatelliteInfoSourceAndroid *source = AndroidPositioning::idToSatSource()->value(androidClassKey);
    if (!source) {
        //qWarning("satelliteUpdated: source == 0");
        return;
    }

    QMetaObject::invokeMethod(source, "processSatelliteUpdateInView", Qt::AutoConnection,
                              Q_ARG(QList<QGeoSatelliteInfo>, sats), Q_ARG(bool, isSingleUpdate));

    QMetaObject::invokeMethod(source, "processSatelliteUpdateInUse", Qt::AutoConnection,
                              Q_ARG(QList<QGeoSatelliteInfo>, inUse), Q_ARG(bool, isSingleUpdate));
}


#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
clazz = env->FindClass(CLASS_NAME); \
if (!clazz) { \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    return JNI_FALSE; \
}

static JNINativeMethod methods[] = {
    {"positionUpdated", "(Landroid/location/Location;IZ)V", (void *)positionUpdated},
    {"locationProvidersDisabled", "(I)V", (void *) locationProvidersDisabled},
    {"locationProvidersChanged", "(I)V", (void *) locationProvidersChanged},
    {"satelliteGpsUpdated", "([Ljava/lang/Object;IZ)V", (void *)satelliteGpsUpdated},
    {"satelliteGnssUpdated", "(Landroid/location/GnssStatus;IZ)V", (void *)satelliteGnssUpdated},
};

static bool registerNatives(JNIEnv *env)
{
    jclass clazz;
    FIND_AND_CHECK_CLASS("org/cybertracker/mobile/QtPositioning");
    positioningClass = static_cast<jclass>(env->NewGlobalRef(clazz));

    if (env->RegisterNatives(positioningClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        return JNI_FALSE;
    }

    GET_AND_CHECK_STATIC_METHOD(providerListMethodId, positioningClass, "providerList", "()[I");
    GET_AND_CHECK_STATIC_METHOD(lastKnownPositionMethodId, positioningClass, "lastKnownPosition", "(Z)Landroid/location/Location;");
    GET_AND_CHECK_STATIC_METHOD(startUpdatesMethodId, positioningClass, "startUpdates", "(III)I");
    GET_AND_CHECK_STATIC_METHOD(stopUpdatesMethodId, positioningClass, "stopUpdates", "(I)V");
    GET_AND_CHECK_STATIC_METHOD(requestUpdateMethodId, positioningClass, "requestUpdate", "(II)I");
    GET_AND_CHECK_STATIC_METHOD(startSatelliteUpdatesMethodId, positioningClass, "startSatelliteUpdates", "(IIZ)I");

    return true;
}

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    typedef union {
        JNIEnv *nativeEnvironment;
        void *venv;
    } UnionJNIEnvToVoid;

    UnionJNIEnvToVoid uenv;
    uenv.venv = nullptr;
    javaVM = nullptr;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    JNIEnv *env = uenv.nativeEnvironment;
    if (!registerNatives(env)) {
        return -1;
    }

    javaVM = vm;
    return JNI_VERSION_1_6;
}
Q_DECLARE_METATYPE(QGeoSatelliteInfo)
Q_DECLARE_METATYPE(QList<QGeoSatelliteInfo>)
SatelliteInfoSourceAndroid::SatelliteInfoSourceAndroid(QObject *parent) :
    QGeoSatelliteInfoSource(parent), m_error(NoError), updatesRunning(false)
{
    qRegisterMetaType< QGeoSatelliteInfo >();
    qRegisterMetaType< QList<QGeoSatelliteInfo> >();
    androidClassKeyForUpdate = AndroidPositioning::registerPositionInfoSource(this);
    androidClassKeyForSingleRequest = AndroidPositioning::registerPositionInfoSource(this);

    requestTimer.setSingleShot(true);
    QObject::connect(&requestTimer, SIGNAL(timeout()), this, SLOT(requestTimeout()));
}

SatelliteInfoSourceAndroid::~SatelliteInfoSourceAndroid()
{
    stopUpdates();

    if (requestTimer.isActive())
    {
        requestTimer.stop();
        AndroidPositioning::stopUpdates(androidClassKeyForSingleRequest);
    }

    AndroidPositioning::unregisterPositionInfoSource(androidClassKeyForUpdate);
    AndroidPositioning::unregisterPositionInfoSource(androidClassKeyForSingleRequest);
}


void SatelliteInfoSourceAndroid::setUpdateInterval(int msec)
{
    int previousInterval = updateInterval();
    msec = (((msec > 0) && (msec < minimumUpdateInterval())) || msec < 0)? minimumUpdateInterval() : msec;

    if (msec == previousInterval)
    {
        return;
    }

    QGeoSatelliteInfoSource::setUpdateInterval(msec);

    if (updatesRunning)
    {
        reconfigureRunningSystem();
    }
}

int SatelliteInfoSourceAndroid::minimumUpdateInterval() const
{
    return 50;
}

QGeoSatelliteInfoSource::Error SatelliteInfoSourceAndroid::error() const
{
    return m_error;
}

void SatelliteInfoSourceAndroid::startUpdates()
{
    if (updatesRunning)
    {
        return;
    }

    updatesRunning = true;

    QGeoSatelliteInfoSource::Error error = AndroidPositioning::startSatelliteUpdates(androidClassKeyForUpdate, false, updateInterval());
    if (error != QGeoSatelliteInfoSource::NoError)
    {
        updatesRunning = false;
        m_error = error;
        emit QGeoSatelliteInfoSource::error(m_error);
    }
}

void SatelliteInfoSourceAndroid::stopUpdates()
{
    if (!updatesRunning)
    {
        return;
    }

    updatesRunning = false;
    AndroidPositioning::stopUpdates(androidClassKeyForUpdate);
}

void SatelliteInfoSourceAndroid::requestUpdate(int timeout)
{
    if (requestTimer.isActive())
    {
        return;
    }

    if (timeout != 0 && timeout < minimumUpdateInterval())
    {
        emit requestTimeout();
        return;
    }

    if (timeout == 0)
    {
        timeout = UPDATE_FROM_COLD_START;
    }

    requestTimer.start(timeout);

    // if updates already running with interval equal or less then timeout
    // then we wait for next update coming through
    // assume that a single update will not be quicker than regular updates anyway
    if (updatesRunning && updateInterval() <= timeout)
    {
        return;
    }

    QGeoSatelliteInfoSource::Error error = AndroidPositioning::startSatelliteUpdates(androidClassKeyForSingleRequest, true, timeout);
    if (error != QGeoSatelliteInfoSource::NoError)
    {
        requestTimer.stop();
        m_error = error;
        emit QGeoSatelliteInfoSource::error(m_error);
    }
}

void SatelliteInfoSourceAndroid::processSatelliteUpdateInView(const QList<QGeoSatelliteInfo> &satsInView, bool isSingleUpdate)
{
    if (!isSingleUpdate)
    {
        //if requested while regular updates were running
        if (requestTimer.isActive())
        {
            requestTimer.stop();
        }
        emit QGeoSatelliteInfoSource::satellitesInViewUpdated(satsInView);
        return;
    }

    m_satsInView = satsInView;
}

void SatelliteInfoSourceAndroid::processSatelliteUpdateInUse(const QList<QGeoSatelliteInfo> &satsInUse, bool isSingleUpdate)
{
    if (!isSingleUpdate)
    {
        //if requested while regular updates were running
        if (requestTimer.isActive())
            requestTimer.stop();
        emit QGeoSatelliteInfoSource::satellitesInUseUpdated(satsInUse);
        return;
    }

    m_satsInUse = satsInUse;
}

void SatelliteInfoSourceAndroid::requestTimeout()
{
    AndroidPositioning::stopUpdates(androidClassKeyForSingleRequest);

    const int count = m_satsInView.count();
    if (!count)
    {
        emit requestTimeout();
        return;
    }

    emit QGeoSatelliteInfoSource::satellitesInViewUpdated(m_satsInView);
    emit QGeoSatelliteInfoSource::satellitesInUseUpdated(m_satsInUse);

    m_satsInUse.clear();
    m_satsInView.clear();
}

// Updates the system assuming that updateInterval and/or preferredPositioningMethod have changed.
void SatelliteInfoSourceAndroid::reconfigureRunningSystem()
{
    if (!updatesRunning)
    {
        return;
    }

    stopUpdates();
    startUpdates();
}

void SatelliteInfoSourceAndroid::locationProviderDisabled()
{
    m_error = QGeoSatelliteInfoSource::ClosedError;
    emit QGeoSatelliteInfoSource::error(m_error);
}
