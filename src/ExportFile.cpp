#include "ExportFile.h"
#include "App.h"
#include "Database.h"
#include "Utils.h"

namespace CyberTracker {

ExportFile::ExportFile(QObject* parent): QObject(parent)
{
}

void ExportFile::registerFile(const QString& filePath, const QVariantMap& data)
{
    auto packedFilePath = Utils::encodeFilePath(filePath);
    App::instance()->database()->addExport(packedFilePath, data);
    Utils::mediaScan(filePath);

    emit changed();
}

void ExportFile::removeFile(const QString& filePath)
{
    auto data = QVariantMap();
    App::instance()->database()->loadExport(filePath, &data);

    auto actualFilePath = Utils::decodeFilePath(filePath);
    if (data.isEmpty() || !QFile::exists(actualFilePath))
    {
        emit App::instance()->showError(QObject::tr("Export missing"));
        return;
    }

    App::instance()->database()->removeExport(filePath);
    QFile::rename(actualFilePath, App::instance()->backupPath() + "/" + QFileInfo(actualFilePath).fileName());
    Utils::enforceFolderQuota(App::instance()->backupPath(), 64);

    emit App::instance()->showToast(QString(QObject::tr("%1 deleted").arg(data["name"].toString())));

    emit changed();
}

QVariantList ExportFile::buildModel(const QString& projectUid, const QString& filter) const
{
    auto result = QVariantList();

    auto exportFiles = QStringList();
    App::instance()->database()->getExports(&exportFiles);

    for (auto it = exportFiles.constBegin(); it != exportFiles.constEnd(); it++)
    {
        auto filePath = *it;

        // Filter if not interested.
        auto fileInfo = QVariantMap();
        App::instance()->database()->loadExport(filePath, &fileInfo);
        if (!projectUid.isEmpty() && projectUid != fileInfo.value("projectUid").toString())
        {
            continue;
        }

        if (!filter.isEmpty() && filter != fileInfo.value("filter").toString())
        {
            continue;
        }

        // Cleanup if file missing.
        auto actualFilePath = Utils::decodeFilePath(filePath);
        QFile file(actualFilePath);
        if (!file.exists())
        {
            App::instance()->database()->removeExport(filePath);
            continue;
        }

        // Append path and size.
        fileInfo["path"] = filePath;
        auto size = file.size();
        auto sizeUnits = QString("b");
        if (size > 1024)
        {
            size /= 1024;
            sizeUnits = "kb";
        }

        if (size > 1024)
        {
            size /= 1024;
            sizeUnits = "mb";
        }

        auto sizeText = QString("%1 %2").arg(QString::number(size), sizeUnits);
        fileInfo["size"] = sizeText;

        auto startTime = Utils::decodeTimestamp(fileInfo["startTime"].toString());
        auto stopTime = Utils::decodeTimestamp(fileInfo["stopTime"].toString());

        fileInfo["model"] = QVariantList
        {
            QVariantMap {{ "name", QObject::tr("Project") }, { "value", fileInfo["projectTitle"] } },
            QVariantMap {{ "name", QObject::tr("Start date") }, { "value", App::instance()->formatDate(startTime.date()) } },
            QVariantMap {{ "name", QObject::tr("Start time") }, { "value", App::instance()->formatTime(startTime.time()) } },
            QVariantMap {{ "name", QObject::tr("Stop date") }, { "value", App::instance()->formatDate(stopTime.date()) } },
            QVariantMap {{ "name", QObject::tr("Stop time") }, { "value", App::instance()->formatTime(stopTime.time()) } },
            QVariantMap {{ "name", QObject::tr("Sightings") }, { "value", fileInfo["sightingCount"] } },
            QVariantMap {{ "name", QObject::tr("Locations") }, { "value", fileInfo["locationCount"] } },
        };

        // Return this.
        result.append(fileInfo);
    }

    return result;
}

void ExportFile::syncWithDownloads()
{
    auto database = App::instance()->database();

    auto exportUids = QStringList();
    database->getExports(&exportUids);
    for (auto exportIt = exportUids.constBegin(); exportIt != exportUids.constEnd(); exportIt++)
    {
        auto exportUid = *exportIt;
        auto filePath = Utils::decodeFilePath(exportUid);
        if (!QFile::exists(filePath))
        {
            database->removeExport(exportUid);
            continue;
        }

        auto downloadFolder = QString();
        if (filePath.contains("SMARTdata"))
        {
            downloadFolder = "SMARTdata";
        }
        else if (filePath.contains("CyberTrackerData"))
        {
            downloadFolder = "CyberTrackerData";
        }
        else
        {
            continue;
        }

        auto data = QVariantMap();
        database->loadExport(exportUid, &data);
        auto downloadUri = data.value("downloadUri").toString();

        // If the downloadUri is not set, then replicate to Downloads.
        if (downloadUri.isEmpty())
        {
            downloadUri = App::instance()->copyToDownload(filePath, downloadFolder);
            data["downloadUri"] = downloadUri;
            database->saveExport(exportUid, data);

            qDebug() << "Replicating export to: " << exportUid << downloadUri;
            continue;
        }

        // If the downloadUri is set, then check if it exists.
        // If not, then this was copied off, so clean it up.
        if (!App::instance()->platformUtils()->hasContent(downloadUri))
        {
            qDebug() << "Cleaning up export: " << exportUid << downloadUri;

            QFile::remove(filePath);
            database->removeExport(exportUid);
            continue;
        }
    }
}

} // namespace CyberTracker
