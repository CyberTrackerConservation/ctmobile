#pragma once
#include "pch.h"
#include "Database.h"

namespace CyberTracker {

class OfflineMapManager : public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY(bool, updatesAvailable)
    QML_READONLY_AUTO_PROPERTY(bool, hasUpdateableMaps)

public:
    explicit OfflineMapManager(QObject* parent = nullptr);
    explicit OfflineMapManager(const QString& offlineMapsPath, QObject *parent = nullptr);
    ~OfflineMapManager();

    Q_INVOKABLE QString createPackage(const QString& layerId) const;
    Q_INVOKABLE QString getShareUrl(const QString& layerId) const;

    Q_INVOKABLE bool canInstallPackage(const QString& filePath) const;
    ApiResult installPackage(const QString& filePath, const QString& projectUid = {}, const QString& updateUrl = {}, const QString& eTag = {});

    ApiResult installOrUpdatePackage(const QString& offlineMapUrl, const QString& projectUid = {});

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void updateAll();

    ApiResult installProjectMaps(const QString& path, const QVariantList& mapList, const QString& projectUid = {});

    void installClassicLayers();

    void normalizePackage(const QString& stagingPath) const;
    ApiResult installStagedPackage(const QString& stagingPath, const QString& projectUid = {}, const QString& updateUrl = {}, const QString& eTag = {});

    Q_INVOKABLE void moveLayer(const QString& layerId, int delta);

    Database::DbMapLayer getLayer(const QString& layerId) const;
    Database::DbMapPackage getPackage(const QString& packageId) const;
    QString layersPath() const { return m_layersPath; }

    Q_INVOKABLE void setLayerActive(const QString& layerId, bool value);

    Q_INVOKABLE QString getLayerPackageId(const QString& layerId) const;
    Q_INVOKABLE int getPackageLayerCount(const QString& packageId) const;

    Q_INVOKABLE void deleteLayer(const QString& layerId);
    Q_INVOKABLE void deleteLayersByPackageId(const QString& packageId);
    void deletePackagesByProjectUid(const QString& projectUid);

    Q_INVOKABLE QStringList getLayers() const;

    Q_INVOKABLE bool hasLookupLayers() const;

signals:
    void layersChanged() const;
    void layerAdded(const QString& layerName) const;
    void layerDeleted(const QString& layerName) const;

private:
    QString m_layersPath;
    int m_pendingChecks = 0;

    ApiResult hasUpdate(const QString& offlineMapUrl, QString* responseEtagOut = nullptr) const;
    QStringList findLayersByPackageId(const QString& packageId) const;
    bool findPackageByUrl(const QString& offlineMapUrl, Database::DbMapPackage* packageOut) const;
    void removePackage(const Database::DbMapPackage& package);
    void migrateLegacyFolders();
    void recomputeHasUpdateableMaps();
};

} // namespace CyberTracker
