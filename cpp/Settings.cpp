#include "Settings.h"

Settings::Settings(QObject* parent): QObject(parent)
{
    qFatalIf(true, "Singleton");
}

Settings::Settings(const QString& filePath, QObject* parent): QObject(parent)
{
    m_filePath = filePath;
    m_cache = Utils::variantMapFromJsonFile(m_filePath);
}

QVariant Settings::getSetting(const QString& name, const QVariant& defaultValue) const
{
    return m_cache.value(name, defaultValue);
}

void Settings::setSetting(const QString& name, const QVariant& value)
{
    if (!value.isValid())
    {
        m_cache.remove(name);
    }
    else
    {
        m_cache.insert(name, Utils::variantFromJSValue(value));
    }

    Utils::writeJsonToFile(m_filePath, Utils::variantMapToJson(m_cache));
}

int Settings::font10() const
{
    return (10 * fontSize()) / 100;
}

int Settings::font11() const
{
    return (11 * fontSize()) / 100;
}

int Settings::font12() const
{
    return (12 * fontSize()) / 100;
}

int Settings::font13() const
{
    return (13 * fontSize()) / 100;
}

int Settings::font14() const
{
    return (14 * fontSize()) / 100;
}

int Settings::font15() const
{
    return (15 * fontSize()) / 100;
}

int Settings::font16() const
{
    return (16 * fontSize()) / 100;
}

int Settings::font17() const
{
    return (17 * fontSize()) / 100;
}

int Settings::font18() const
{
    return (18 * fontSize()) / 100;
}

int Settings::font19() const
{
    return (19 * fontSize()) / 100;
}

int Settings::font20() const
{
    return (20 * fontSize()) / 100;
}

QStringList Settings::simulateGPSFiles() const
{
    auto result = QStringList { tr("Default") };
    auto dir = QDir(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).constFirst());
    auto fileInfoList = QFileInfoList(dir.entryInfoList({"*.nmea"}, QDir::Files));
    for (auto it = fileInfoList.constBegin(); it != fileInfoList.constEnd(); it++)
    {
        result.append(it->fileName());
    }

    return result;
}

QString Settings::simulateGPSFilename() const
{
    auto filename = getSetting("simulateGPSFilename", "").toString();

    if (filename.contains('/'))
    {
        auto fileInfo = QFileInfo(filename);
        filename = fileInfo.fileName();
    }

    return filename;
}

void Settings::set_simulateGPSFilename(const QString& value)
{
    if (value != simulateGPSFilename())
    {
        setSetting("simulateGPSFilename", value);
        emit simulateGPSChanged();
    }
}
