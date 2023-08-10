#pragma once
#include "pch.h"
#include "keychain.h"

class Settings: public QObject
{
    Q_OBJECT

    QML_SETTING (QString, deviceId, "")
    QML_SETTING (QString, username, "")

    QML_SETTING (QString, dateFormat, "ddd yyyy/MM/dd")
    QML_SETTING (QString, timeFormat, "hh:mm ap")
    QML_SETTING (QString, dateTimeFormat, "ddd yyyy/MM/dd hh:mm:ss")

    QML_SETTING (bool, darkTheme, false)
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

    QML_SETTING (int, utmZone, 0)
    QML_SETTING (QString, utmHemi, "N")

    QML_SETTING (bool, footerText, true)
    QML_SETTING (bool, fullScreen, false)

    QML_SETTING (int, coordinateFormat, 0)

    QML_SETTING (QString, kioskModeProjectUid, "")

    QML_SETTING (bool, mapMoreDataVisible, false)
    QML_SETTING (int, mapMoreDataViewIndex, 0)
    QML_SETTING (QVariantMap, mapViewpointCenter, QVariantMap())
    QML_SETTING (int, mapPanMode, 1)              // Enums.LocationDisplayAutoPanModeRecenter

    QML_SETTING (QStringList, projects, QStringList())

    QML_SETTING (QString, activeBasemap, "BasemapOpenStreetMap")
    QML_SETTING (QStringList, offlineMapLayers, QStringList())
    QML_SETTING (QVariantMap, offlineMapPackages, QVariantMap())

    QML_SETTING (bool, classicDesktopConnected, false)
    QML_SETTING (bool, classicUpgradeMessage, false)

    QML_SETTING (int, cameraFlashMode, 1)         // Camera.FlashAuto
    QML_SETTING (int, cameraWhiteBalanceMode, 0)  // CameraImageProcessing.WhiteBalanceAuto
    QML_SETTING (int, cameraPosition, 1)          // Camera.BackFace

    QML_SETTING (int, locationPageTabIndex, 0)
    QML_SETTING (int, locationOutlierMaxSpeed, 0)
    QML_SETTING (int, locationAccuracyMeters, 80)

    QML_SETTING (QVariantMap, gotoManager, QVariantMap())
    
    QML_SETTING (QVariantMap, timeManager, QVariantMap())

    QML_SETTING (QString, dropboxAccessToken, "")

    QML_SETTING (QVariantMap, logins, QVariantMap())

    QML_SETTING (bool, metricUnits, QLocale::system().measurementSystem() == QLocale::MetricSystem)

//    QML_SECURE_SETTING (QString, googleEmail, QString())
//    QML_SECURE_SETTING (QString, googleAccessToken, QString())
//    QML_SECURE_SETTING (QString, googleRefreshToken, QString())

    QML_SETTING (QString, autoLaunchProjectUid, QString())
    QML_SETTING (bool, blackviewMessageShown, false)

public:
    explicit Settings(QObject* parent = nullptr);
    explicit Settings(const QString& filePath, const QString& vaultName, QObject* parent = nullptr);

    QVariant getSetting(const QString& name, const QVariant& defaultValue) const;
    void setSetting(const QString& name, const QVariant& value);

    QVariant getSecureSetting(const QString& name, const QVariant& defaultValue) const;
    void setSecureSetting(const QString& name, const QVariant& value);

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

signals:
    void secureSettingChanged();

private:
    QString m_filePath;
    QVariantMap m_cache;
    QVariantMap m_vaultCache;
    QKeychain::ReadPasswordJob* m_vaultReadJob = nullptr;
    QKeychain::WritePasswordJob* m_vaultWriteJob = nullptr;
};
