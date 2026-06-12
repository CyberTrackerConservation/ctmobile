#include "BackupFile.h"
#include "App.h"
#include "Database.h"
#include "Utils.h"

namespace CyberTracker {

BackupFile::BackupFile(QObject* parent): QObject(parent)
{
    enforceQuota();
}

void BackupFile::registerFile(const QString& projectTitle, const QString& filePath)
{
    // Copy the file to the backup files directory
    auto filename = QFileInfo(filePath).fileName();
    auto backupFilePath = App::instance()->backupFilesPath() + "/" + filename;
    
    if (!QFile::copy(filePath, backupFilePath))
    {
        qDebug() << "Failed to copy file to backup directory:" << filePath;
        return;
    }

    // Add to database.
    auto backupFile = Database::DbBackupFile();
    backupFile.timestamp = App::instance()->timeManager()->currentDateTimeISO();
    backupFile.projectTitle = projectTitle;
    backupFile.filename = filename;
    App::instance()->database()->addBackupFile(backupFile);

    emit changed();
}

void BackupFile::registerData(const QString& projectTitle, const QByteArray& data, const QString& prefix, const QString& suffix)
{
    auto backupFile = Database::DbBackupFile();

    auto now = App::instance()->timeManager()->currentDateTime();
    backupFile.timestamp = Utils::encodeTimestamp(now);
    backupFile.projectTitle = projectTitle;
    backupFile.filename = Utils::makeUniqueFilename(prefix, now, suffix);
    Utils::writeDataToFile(App::instance()->backupFilesPath() + "/" + backupFile.filename, data);

    App::instance()->database()->addBackupFile(backupFile);

    emit changed();
}

void BackupFile::reset()
{
    // Remove all files from the backup directory
    QDir(App::instance()->backupFilesPath()).removeRecursively();
    Utils::ensurePath(App::instance()->backupFilesPath());
    
    // Clear the database entries
    App::instance()->database()->clearBackupFiles();

    emit changed();
}

QVariantList BackupFile::buildModel() const
{
    auto result = QVariantList();

    App::instance()->database()->enumBackupFiles([&](const Database::DbBackupFile& backupFile)
    {
        auto filePath = App::instance()->backupFilesPath() + "/" + backupFile.filename;
        auto fileInfo = QFileInfo(filePath);
        
        // Skip if file doesn't exist
        if (!fileInfo.exists())
        {
            qDebug() << "Error: Backup file does not exist:" << backupFile.filename;
            return;
        }

        // Calculate file size
        auto size = fileInfo.size();
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

        result.append(QVariantMap
        {
            { "filename", backupFile.filename },
            { "projectTitle", backupFile.projectTitle },
            { "timestamp", backupFile.timestamp },
            { "size", sizeText }
        });
    });

    return result;
}

QString BackupFile::createZip() const
{
    qDebug() << "Create backupfile";

    auto result = App::instance()->tempPath() + "/BackupFiles.zip";
    qFatalIf(QFile::exists(result) && !QFile::remove(result), "Failed to delete old backup files");

    if (!Utils::zipCompressDir(result, App::instance()->backupFilesPath()))
    {
        qDebug() << "Failed to create backups zip file";
        return "";
    }

    return result;
}

void BackupFile::enforceQuota()
{
    auto maxAge = App::instance()->settings()->backupFileQuotaDays();
    if (maxAge == -1)
    {
        return;
    }

    if (maxAge == 0)
    {
        reset();
        return;
    }

    auto now = App::instance()->timeManager()->currentDateTime();
    auto database = App::instance()->database();
    auto filenames = QStringList();

    database->enumBackupFiles([&](const Database::DbBackupFile& backupFile)
    {
        auto timestamp = QDateTime::fromString(backupFile.timestamp, Qt::DateFormat::ISODateWithMs);
        auto age = timestamp.daysTo(now);
        if (age > maxAge)
        {
            filenames.append(backupFile.filename);
        }
    });

    for (auto filenameIt = filenames.constBegin(); filenameIt != filenames.constEnd(); filenameIt++)
    {
        auto filename = *filenameIt;
        database->deleteBackupFile(filename);
        QFile::remove(App::instance()->backupFilesPath() + "/" + filename);
    }
}

} // namespace CyberTracker