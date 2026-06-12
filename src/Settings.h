#pragma once
#include "pch.h"

namespace CyberTracker {

class Settings: public QObject
{
    Q_OBJECT

    QML_SETTING (QString, deviceId, "")
    QML_SETTING (QString, username, "")

    QML_SETTING (QString, dateFormat, "ddd yyyy/MM/dd")
    QML_SETTING (QString, timeFormat, "hh:mm ap")
    QML_SETTING (QString, dateTimeFormat, "ddd yyyy/MM/dd hh:mm:ss")

    QML_SETTING (bool, darkTheme, false)
    QML_SETTING (QString, theme, "")
    QML_SETTING (QString, languageCode, "system")
    QML_SETTING (int, fontSize, 125)

    Q_PROPERTY (int font10 READ font10 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font11 READ font11 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font12 READ font12 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font13 READ font13 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font14 READ font14 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font15 READ font15 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font16 READ font16 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font17 READ font17 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font18 READ font18 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font19 READ font19 NOTIFY fontSizeChanged)
    Q_PROPERTY (int font20 READ font20 NOTIFY fontSizeChanged)

    QML_SETTING (QString, buildString, "")

    QML_SETTING (float, magneticDeclination, 0.0)

    QML_SETTING (bool, simulateGPS, OS_DESKTOP)

    Q_PROPERTY (QStringList simulateGPSFiles READ simulateGPSFiles NOTIFY simulateGPSChanged)
    Q_PROPERTY (QString simulateGPSFilename READ simulateGPSFilename WRITE set_simulateGPSFilename NOTIFY simulateGPSChanged)

    QML_SETTING (bool, uploadRequiresWiFi, false)

    QML_SETTING (bool, footerText, true)
    QML_SETTING (bool, fullScreen, false)

    QML_SETTING (QVariantMap, projection, QVariantMap())

    QML_SETTING (QString, kioskModeProjectUid, "")

    QML_SETTING (QString, patrolProjectUid, "")

    QML_SETTING (bool, mapMoreDataVisible, false)
    QML_SETTING (int, mapMoreDataViewIndex, 0)
    QML_SETTING (QVariantMap, mapViewpointCenter, QVariantMap())
    QML_SETTING (int, mapPanMode, 1)              // Enums.LocationDisplayAutoPanModeRecenter

    QML_SETTING (QStringList, projects, QStringList())

    QML_SETTING (QString, activeBasemap, "BasemapTopographic")
    QML_SETTING (QVariantMap, dataMapLayers, QVariantMap())

    QML_SETTING (bool, classicDesktopConnected, false)
    QML_SETTING (bool, classicUpgradeMessage, false)

    QML_SETTING (int, cameraFlashMode, 2)         // Camera.FlashAuto
    QML_SETTING (int, cameraWhiteBalanceMode, 0)  // CameraImageProcessing.WhiteBalanceAuto
    QML_SETTING (int, cameraIndex, 0)             // First camera

    QML_SETTING (int, locationPageTabIndex, 0)
    QML_SETTING (int, locationOutlierMaxSpeedKph, 0)
    QML_SETTING (int, locationAccuracyMeters, 80)

    QML_SETTING (QVariantMap, gotoManager, QVariantMap())
    
    QML_SETTING (QVariantMap, timeManager, QVariantMap())

    QML_SETTING (QVariantMap, logins, QVariantMap())

    QML_SETTING (bool, metricUnits, QLocale::system().measurementSystem() == QLocale::MetricSystem)

    QML_SETTING (QString, autoLaunchProjectUid, QString())

    QML_SETTING (bool, replicateMedia, false)

    QML_SETTING (bool, archiveSupport, true)
    QML_SETTING (bool, archiveData, false)
    QML_SETTING (QVariantMap, sightingFilter, QVariantMap())
    QML_SETTING (int, sightingGroupBy, 0)
    QML_SETTING (bool, sightingSortReverse, false)
    QML_SETTING (QVariantMap, sightingSectionExpands, QVariantMap())

    QML_SETTING (bool, heatmapActive, false)
    QML_SETTING (int, heatmapMaxIntensity, 2000)

    QML_SETTING (QString, webhookUrl, QString())
    QML_SETTING (QString, webhookTokenType, "Bearer")
    QML_SETTING (QString, webhookFormat, "json")

    QML_SETTING (int, backupFileQuotaDays, 90);

    QML_SETTING (bool, formFiller, false)

    QML_SETTING (bool, bluetoothAudio, false)

public:
    explicit Settings(QObject* parent = nullptr);
    explicit Settings(const QString& filePath, const QString& vaultName, QObject* parent = nullptr);

    QVariant getSetting(const QString& name, const QVariant& defaultValue) const;
    void setSetting(const QString& name, const QVariant& value);

    int font10() const;
    int font11() const;
    int font12() const;
    int font13() const;
    int font14() const;
    int font15() const;
    int font16() const;
    int font17() const;
    int font18() const;
    int font19() const;
    int font20() const;

    QStringList simulateGPSFiles() const;
    QString simulateGPSFilename() const;
    void set_simulateGPSFilename(const QString& value);

private:
    QString m_filePath;
    QVariantMap m_cache;
};

} // namespace CyberTracker
