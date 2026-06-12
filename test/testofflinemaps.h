#pragma once
#include "OfflineMap.h"
#include "App.h"

namespace CyberTracker {

inline void runOfflineMapTests()
{
    int passCount = 0, failCount = 0;

    auto check = [&](bool condition, const QString& description)
    {
        if (condition)
        {
            passCount++;
        }
        else
        {
            qWarning() << "FAIL:" << description;
            failCount++;
        }
    };

    auto tempPath = QDir::cleanPath(App::instance()->tempPath() + "/OfflineMapTests");
    QDir(tempPath).removeRecursively();
    Utils::ensurePath(tempPath);
    auto cleanupOnExit = qScopeGuard([&] { QDir(tempPath).removeRecursively(); });

    auto offlineMapManager = App::instance()->offlineMapManager();
    auto database = App::instance()->database();

    auto findLayerIdByName = [&](const QString& name) -> QString
    {
        auto result = QString();
        database->enumMapLayers([&](const Database::DbMapLayer& layer)
        {
            if (result.isEmpty() && layer.name == name)
            {
                result = layer.id;
            }
        });
        return result;
    };

    // =========================================================================
    // normalizePackage: synthesises package.json with a UUID id when missing
    // =========================================================================

    {
        auto stagingPath = tempPath + "/normalize_synthesise";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/test.geojson", "{}");

        offlineMapManager->normalizePackage(stagingPath);

        auto pkg = Utils::variantMapFromJsonFile(stagingPath + "/package.json");
        auto synthesisedId = pkg["id"].toString();
        check(!synthesisedId.isEmpty(), "normalizePackage synthesises a non-empty package id");
        check(synthesisedId.length() >= 16, "synthesised id is a long opaque token");
        check(synthesisedId != "LegalAtlas", "synthesised id is not LegalAtlas");

        auto layers = Utils::variantListFromJsonFile(stagingPath + "/layers.json");
        check(layers.count() == 1, "normalizePackage discovers one layer from extension scan");

        auto layer = layers.first().toMap();
        check(layer["filename"] == "test.geojson", "normalizePackage sets filename");
        check(layer["name"] == "test", "normalizePackage sets name from basename");

        QDir(stagingPath).removeRecursively();
    }

    // =========================================================================
    // normalizePackage: preserves existing package.json
    // =========================================================================

    {
        auto stagingPath = tempPath + "/normalize_preserve";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/test.geojson", "{}");

        Utils::variantMapToJsonFile(stagingPath + "/package.json",
            QVariantMap {{ "id", "stable-pkg" }, { "name", "Pretty Name" }, { "version", "2" }});

        offlineMapManager->normalizePackage(stagingPath);

        auto pkg = Utils::variantMapFromJsonFile(stagingPath + "/package.json");
        check(pkg["id"] == "stable-pkg", "normalizePackage preserves existing id");
        check(pkg["name"] == "Pretty Name", "normalizePackage preserves existing name");
        check(pkg["version"] == "2", "normalizePackage preserves existing version");

        QDir(stagingPath).removeRecursively();
    }

    // =========================================================================
    // installStagedPackage: writes mapPackages + mapLayers rows and a UUID folder
    // =========================================================================

    {
        auto stagingPath = tempPath + "/install_basic";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/test.geojson", "{}");

        Utils::variantMapToJsonFile(stagingPath + "/package.json",
            QVariantMap {{ "id", "test-install" }, { "name", "Basic Package" }, { "version", "1" }});
        Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
            QVariantMap {{ "filename", "test.geojson" }, { "name", "Basic Layer" }},
        });

        auto result = offlineMapManager->installStagedPackage(stagingPath);
        check(result.success, "installStagedPackage succeeds");

        auto package = Database::DbMapPackage();
        check(database->loadMapPackage("test-install", &package), "mapPackages row exists");
        check(package.name == "Basic Package", "mapPackages.name is persisted");
        check(package.version == "1", "mapPackages.version is persisted");
        check(!package.folder.isEmpty(), "mapPackages.folder is non-empty");
        check(QDir(offlineMapManager->layersPath() + "/" + package.folder).exists(),
              "package folder exists on disk");

        auto layerId = findLayerIdByName("Basic Layer");
        check(!layerId.isEmpty(), "mapLayers row created for the layer");
        check(offlineMapManager->getLayer(layerId).name == "Basic Layer", "getLayer().name matches");
        check(offlineMapManager->getLayers().contains(layerId), "getLayers returns the new layer");

        offlineMapManager->deleteLayersByPackageId("test-install");
    }

    // =========================================================================
    // installStagedPackage: re-install with same packageId replaces
    // =========================================================================

    {
        auto stagingPath = tempPath + "/install_replace_a";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/a.geojson", "{}");
        Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", "replace-pkg" }});
        Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
            QVariantMap {{ "filename", "a.geojson" }, { "name", "ReplaceA" }},
        });
        offlineMapManager->installStagedPackage(stagingPath);
        QDir(stagingPath).removeRecursively();

        auto firstPackage = Database::DbMapPackage();
        database->loadMapPackage("replace-pkg", &firstPackage);
        auto firstFolder = firstPackage.folder;

        stagingPath = tempPath + "/install_replace_b";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/b.geojson", "{}");
        Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", "replace-pkg" }});
        Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
            QVariantMap {{ "filename", "b.geojson" }, { "name", "ReplaceB" }},
        });
        offlineMapManager->installStagedPackage(stagingPath);
        QDir(stagingPath).removeRecursively();

        auto secondPackage = Database::DbMapPackage();
        check(database->loadMapPackage("replace-pkg", &secondPackage), "package still exists after replace");
        check(secondPackage.folder != firstFolder, "package folder is regenerated on re-install");

        check(findLayerIdByName("ReplaceA").isEmpty(), "old layer A is removed by re-install");
        check(!findLayerIdByName("ReplaceB").isEmpty(), "new layer B is present after re-install");

        offlineMapManager->deleteLayersByPackageId("replace-pkg");
    }

    // =========================================================================
    // deleteLayer: single layer in package removes both the layer and the package
    // =========================================================================

    {
        auto stagingPath = tempPath + "/delete_solo";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/solo.geojson", "{}");
        Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", "solo-pkg" }});
        Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
            QVariantMap {{ "filename", "solo.geojson" }, { "name", "Solo" }},
        });
        offlineMapManager->installStagedPackage(stagingPath);
        QDir(stagingPath).removeRecursively();

        auto packageBefore = Database::DbMapPackage();
        check(database->loadMapPackage("solo-pkg", &packageBefore), "package row created");
        auto folderPath = offlineMapManager->layersPath() + "/" + packageBefore.folder;
        check(QDir(folderPath).exists(), "package folder exists on disk");

        auto layerId = findLayerIdByName("Solo");
        offlineMapManager->deleteLayer(layerId);

        auto packageAfter = Database::DbMapPackage();
        check(!database->loadMapPackage("solo-pkg", &packageAfter), "mapPackages row removed when last layer deleted");
        check(findLayerIdByName("Solo").isEmpty(), "mapLayers row removed");
        check(!QDir(folderPath).exists(), "package folder removed from disk");
    }

    // =========================================================================
    // deleteLayer: with multiple layers, only the named layer is removed
    // =========================================================================

    {
        auto stagingPath = tempPath + "/delete_multi";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/x.geojson", "{}");
        Utils::writeJsonToFile(stagingPath + "/y.geojson", "{}");
        Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", "multi-pkg" }});
        Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
            QVariantMap {{ "filename", "x.geojson" }, { "name", "MultiX" }},
            QVariantMap {{ "filename", "y.geojson" }, { "name", "MultiY" }},
        });
        offlineMapManager->installStagedPackage(stagingPath);
        QDir(stagingPath).removeRecursively();

        auto layerIdX = findLayerIdByName("MultiX");
        auto layerIdY = findLayerIdByName("MultiY");

        offlineMapManager->deleteLayer(layerIdX);

        auto package = Database::DbMapPackage();
        check(database->loadMapPackage("multi-pkg", &package), "package remains while another layer exists");
        check(findLayerIdByName("MultiX").isEmpty(), "deleted layer is gone");
        check(!findLayerIdByName("MultiY").isEmpty(), "untouched layer is still present");

        // Now delete the last one — package and folder should follow.
        offlineMapManager->deleteLayer(layerIdY);
        check(!database->loadMapPackage("multi-pkg", &package), "package removed when last layer deleted");
    }

    // =========================================================================
    // deleteLayersByPackageId: removes everything for that package
    // =========================================================================

    {
        auto stagingPath = tempPath + "/delete_pkg";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/p.geojson", "{}");
        Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", "purge-pkg" }});
        Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
            QVariantMap {{ "filename", "p.geojson" }, { "name", "Purge" }},
        });
        offlineMapManager->installStagedPackage(stagingPath);
        QDir(stagingPath).removeRecursively();

        check(!findLayerIdByName("Purge").isEmpty(), "layer installed");

        offlineMapManager->deleteLayersByPackageId("purge-pkg");

        auto package = Database::DbMapPackage();
        check(!database->loadMapPackage("purge-pkg", &package), "package gone after deleteLayersByPackageId");
        check(findLayerIdByName("Purge").isEmpty(), "layer gone after deleteLayersByPackageId");
    }

    // =========================================================================
    // deletePackagesByProjectUid: removes every package tagged with the project
    // =========================================================================

    {
        for (const auto& pkgId : { "proj-a", "proj-b" })
        {
            auto stagingPath = tempPath + "/install_proj_" + pkgId;
            Utils::ensurePath(stagingPath);
            Utils::writeJsonToFile(stagingPath + "/m.geojson", "{}");
            Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", pkgId }});
            Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
                QVariantMap {{ "filename", "m.geojson" }, { "name", QString("Proj-") + pkgId }},
            });
            offlineMapManager->installStagedPackage(stagingPath, "test-project");
            QDir(stagingPath).removeRecursively();
        }

        // Plus an unrelated package that should NOT be deleted.
        {
            auto stagingPath = tempPath + "/install_other_proj";
            Utils::ensurePath(stagingPath);
            Utils::writeJsonToFile(stagingPath + "/o.geojson", "{}");
            Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", "proj-other" }});
            Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
                QVariantMap {{ "filename", "o.geojson" }, { "name", "Other" }},
            });
            offlineMapManager->installStagedPackage(stagingPath, "other-project");
            QDir(stagingPath).removeRecursively();
        }

        offlineMapManager->deletePackagesByProjectUid("test-project");

        auto pkgA = Database::DbMapPackage();
        auto pkgB = Database::DbMapPackage();
        auto pkgOther = Database::DbMapPackage();
        check(!database->loadMapPackage("proj-a", &pkgA), "proj-a removed");
        check(!database->loadMapPackage("proj-b", &pkgB), "proj-b removed");
        check(database->loadMapPackage("proj-other", &pkgOther), "unrelated project's package preserved");

        offlineMapManager->deleteLayersByPackageId("proj-other");
    }

    // =========================================================================
    // moveLayer: shifts sequence values in the global ordering
    // =========================================================================

    {
        for (const auto& pkgId : { "seq-a", "seq-b", "seq-c" })
        {
            auto stagingPath = tempPath + "/install_seq_" + pkgId;
            Utils::ensurePath(stagingPath);
            Utils::writeJsonToFile(stagingPath + "/s.geojson", "{}");
            Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", pkgId }});
            Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
                QVariantMap {{ "filename", "s.geojson" }, { "name", QString("Seq-") + pkgId }},
            });
            offlineMapManager->installStagedPackage(stagingPath);
            QDir(stagingPath).removeRecursively();
        }

        auto idA = findLayerIdByName("Seq-seq-a");
        auto idC = findLayerIdByName("Seq-seq-c");

        auto layersBefore = offlineMapManager->getLayers();
        auto indexAB = layersBefore.indexOf(idA);
        auto indexCB = layersBefore.indexOf(idC);
        check(indexAB < indexCB, "seq-a precedes seq-c initially");

        offlineMapManager->moveLayer(idA, +2);

        auto layersAfter = offlineMapManager->getLayers();
        auto indexAA = layersAfter.indexOf(idA);
        auto indexCA = layersAfter.indexOf(idC);
        check(indexAA > indexCA, "seq-a follows seq-c after moveLayer +2");

        offlineMapManager->deleteLayersByPackageId("seq-a");
        offlineMapManager->deleteLayersByPackageId("seq-b");
        offlineMapManager->deleteLayersByPackageId("seq-c");
    }

    // =========================================================================
    // setLayerActive: persists the value through the DB
    // =========================================================================

    {
        auto stagingPath = tempPath + "/active";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/a.geojson", "{}");
        Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", "active-pkg" }});
        Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
            QVariantMap {{ "filename", "a.geojson" }, { "name", "ActiveLayer" }},
        });
        offlineMapManager->installStagedPackage(stagingPath);
        QDir(stagingPath).removeRecursively();

        auto layerId = findLayerIdByName("ActiveLayer");
        check(offlineMapManager->getLayer(layerId).active, "default active is true");

        offlineMapManager->setLayerActive(layerId, false);
        check(!offlineMapManager->getLayer(layerId).active, "setLayerActive(false) persists");

        offlineMapManager->setLayerActive(layerId, true);
        check(offlineMapManager->getLayer(layerId).active, "setLayerActive(true) persists");

        offlineMapManager->deleteLayersByPackageId("active-pkg");
    }

    // =========================================================================
    // file path: joins layersPath + package folder + layer filename
    // =========================================================================

    {
        auto stagingPath = tempPath + "/path_join";
        Utils::ensurePath(stagingPath);
        Utils::writeJsonToFile(stagingPath + "/joined.geojson", "{}");
        Utils::variantMapToJsonFile(stagingPath + "/package.json", QVariantMap {{ "id", "path-pkg" }});
        Utils::variantListToJsonFile(stagingPath + "/layers.json", QVariantList {
            QVariantMap {{ "filename", "joined.geojson" }, { "name", "Joined" }},
        });
        offlineMapManager->installStagedPackage(stagingPath);
        QDir(stagingPath).removeRecursively();

        auto layerId = findLayerIdByName("Joined");
        auto layer = offlineMapManager->getLayer(layerId);
        auto package = offlineMapManager->getPackage(layer.packageId);
        auto filePath = offlineMapManager->layersPath() + "/" + package.folder + "/" + layer.filename;
        check(QFile::exists(filePath), "joined file path resolves to an existing file");
        check(filePath.endsWith("/joined.geojson"), "joined file path ends with the layer filename");

        offlineMapManager->deleteLayersByPackageId("path-pkg");
    }

    // =========================================================================
    // Summary
    // =========================================================================

    qDebug() << "OfflineMap tests:" << passCount << "passed," << failCount << "failed.";
    Q_ASSERT(failCount == 0);
}

} // namespace CyberTracker
