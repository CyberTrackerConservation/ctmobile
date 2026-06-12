#include "pch.h"

#include <quazip/quazip/JlCompress.h>
#include <quazip/quazip/quazip.h>

namespace CyberTracker {

namespace Utils {

bool isZipFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    static const auto ZIP_MAGIC = QByteArray("PK\x03\x04", 4);
    return file.read(4) == ZIP_MAGIC;
}

QString zipUnpack(const QString& targetPath, const QString& targetFolder, const QString& zipFilePath)
{
    auto path = QDir::cleanPath(targetPath + "/" + targetFolder);
    QDir(path).removeRecursively();
    qFatalIf(!Utils::ensurePath(path), "Failed to create target folder");

    auto extractedFiles = Utils::zipExtractToDir(zipFilePath, path);

    if (extractedFiles.isEmpty())
    {
        QDir(path).removeRecursively();
        return QString();
    }

    return path;
}

QStringList zipExtractToDir(const QString& zipFilePath, const QString& targetDirPath)
{
    return JlCompress::extractDir(zipFilePath, targetDirPath);
}

bool zipCompressDir(const QString& zipFilePath, const QString& sourceDirPath)
{
    return JlCompress::compressDir(zipFilePath, sourceDirPath, true);
}

QStringList zipGetFilenames(const QString& zipFilePath)
{
    QuaZip zip(zipFilePath);
    if (!zip.open(QuaZip::mdUnzip))
    {
        qDebug() << "Zip file not found: " << zipFilePath;
        return QStringList();
    }

    return zip.getFileNameList();
}

QVariantMap zipGetVariantMapFromFile(const QString& zipFilePath, const QString& jsonFileName)
{
    QuaZip zip(zipFilePath);
    if (!zip.open(QuaZip::mdUnzip))
    {
        return QVariantMap();
    }

    zip.setCurrentFile(jsonFileName, QuaZip::csInsensitive);
    if (!zip.hasCurrentFile())
    {
        return QVariantMap();
    }

    QuaZipFile jsonFile(&zip);
    if (!jsonFile.open(QIODevice::ReadOnly))
    {
        return QVariantMap();
    }

    QJsonParseError jsonErrorCode;
    auto json = QJsonDocument::fromJson(jsonFile.readAll(), &jsonErrorCode);
    if (jsonErrorCode.error != QJsonParseError::NoError)
    {
        qDebug() << "Bad json";
        return QVariantMap();
    }

    return json.object().toVariantMap();
}

}

} // namespace CyberTracker