#pragma once
#include "pch.h"

class OfflineMapManager : public QObject
{
    Q_OBJECT

public:
    explicit OfflineMapManager(QObject* parent = nullptr);
    explicit OfflineMapManager(const QString& offlineMapsPath, QObject *parent = nullptr);
    ~OfflineMapManager();

    Q_INVOKABLE bool canInstallPackage(const QString& filePath) const;
    ApiResult installPackage(const QString& filePath, QString* packageUidOut = nullptr);

    ApiResult hasUpdate(QNetworkAccessManager* networkAccessManager, const QString& offlineMapUrl, QString* foundPackageUidOut = nullptr, QString* responseEtagOut = nullptr) const;
    ApiResult installOrUpdatePackage(const QString& offlineMapUrl);

    ApiResult installProjectMaps(const QString& path, const QVariantList& mapList);

    Q_INVOKABLE QString createPackage(const QString& layerId) const;

    Q_INVOKABLE void moveLayer(const QString& layerId, int delta);

    Q_INVOKABLE bool getLayerActive(const QString& layerId) const;
    Q_INVOKABLE void setLayerActive(const QString& layerId, bool value);

    Q_INVOKABLE void deleteLayer(const QString& layerId);

    Q_INVOKABLE QStringList getLayers() const;
    Q_INVOKABLE QString getLayerName(const QString& layerId) const;
    Q_INVOKABLE double getLayerOpacity(const QString& layerId) const;
    Q_INVOKABLE QString getLayerFilePath(const QString& layerId) const;
    Q_INVOKABLE QString getLayerTimestamp(const QString& layerId) const;

signals:
    void layersChanged();

private:
    QString m_packagesPath;

    void installLegacyGotos();
    void garbageCollect();
};
