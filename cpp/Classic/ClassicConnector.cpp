#include "ClassicConnector.h"
#include "ClassicProvider.h"
#include "App.h"
#include "../lib/classic/Main/ctSession.h"
#include "../lib/classic/Host/Qt/QtHost.h"
#include <jlcompress.h>

ClassicConnector::ClassicConnector(QObject* parent) : Connector(parent)
{
    update_name(CLASSIC_CONNECTOR);
}

QString getSubtitle(const QString& serverFileName)
{
    if (serverFileName.startsWith("KEY:SMART"))
    {
        return "SMART";
    }
    else
    {
        auto filePath = serverFileName;
        filePath = QFileInfo(filePath.replace("%", "/").replace("\"", "/")).baseName();
        if (filePath.startsWith("$CT"))
        {
            filePath = "";
        }

        return filePath;
    }
}

bool ClassicConnector::canExportData(Project* project) const
{
    auto ctsMetadata = QVariantMap();
    getCTSMetadata(Utils::classicRoot() + "/" + project->connectorParams()["app"].toString() + ".CTS", &ctsMetadata);

    return ctsMetadata["protocol"] == "NONE";
}

bool ClassicConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool ClassicConnector::canShareAuth(Project* /*project*/) const
{
    return false;
}

QVariantMap ClassicConnector::getShareData(Project* project, bool /*auth*/) const
{
    auto webUpdateUrl = project->connectorParams()["webUpdateUrl"].toString();
    if (webUpdateUrl.isEmpty())
    {
        return QVariantMap();
    }

    return QVariantMap
    {
        { "connector", CLASSIC_CONNECTOR },
        { "webUpdateUrl", webUpdateUrl },
        { "launch", true }
    };
}

bool ClassicConnector::canLogin(Project* /*project*/) const
{
    return false;
}

ApiResult ClassicConnector::bootstrap(const QVariantMap& params)
{
    auto path = Utils::classicRoot();

    if (!Utils::ensurePath(path, true))
    {
        return Failure(tr("Cannot create path"));
    }

    Utils::ensurePath(path + "/Media", true);
    Utils::ensurePath(path + "/Outgoing", true);
    Utils::ensurePath(path + "/Backup", true);
    Utils::ensurePath(path + "/Temp", true);
    Utils::ensurePath(path + "/Update", true);

    // Create version.txt.
    {
        QFile outFile(path + "/version.txt");
        outFile.remove();
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        ts << "Storage=" << path << Qt::endl;
        ts << "OsVersion=" << 0 << Qt::endl;
        ts << "ResourceVersion=" << ClassicProvider::version() << Qt::endl;

        auto screenSize = qApp->screens()[0]->size();
        ts << "Width=" + QString::number(screenSize.width()) << Qt::endl;
        ts << "Height=" + QString::number(screenSize.height()) << Qt::endl;
        ts << "Dpi=" + QString::number((int)(qApp->screens()[0]->logicalDotsPerInch())) << Qt::endl;

        outFile.close();
        Utils::mediaScan(outFile.fileName());
    }

    // Extract the world map if not already there.
    if (!QFile::exists(path + "/World.ecw"))
    {
        Utils::extractResource(":/Classic/World.ecw", path);
    }

    // Build list of existing projects.
    auto projects = m_projectManager->buildList();

    // Perform web install if specified.
    auto webUpdateUrl = params.value("webUpdateUrl").toString();
    if (!webUpdateUrl.isEmpty())
    {
        qDebug() << "WebInstall: looking for " << webUpdateUrl;

        // Check for a duplicate project.
        for (auto project: projects)
        {
            if (project->provider() != CLASSIC_PROVIDER)
            {
                continue;
            }

            if (project->connectorParams().value("webUpdateUrl").toString().compare(webUpdateUrl, Qt::CaseInsensitive) != 0)
            {
                continue;
            }

            return AlreadyConnected();
        }

        // Check that the url is good.
        if (!App::instance()->pingUrl(webUpdateUrl))
        {
            return Failure(tr("Server not found"));
        }

        auto projectUid = "CLASSIC2_" + QUuid::createUuid().toString(QUuid::Id128);
        m_projectManager->init(projectUid, CLASSIC_PROVIDER, QVariantMap(), CLASSIC_CONNECTOR, QVariantMap());

        auto project = m_projectManager->loadPtr(projectUid);
        project->set_version(0);
        project->set_icon("qrc:/Classic/logo.svg");
        project->set_title("Pending update");
        project->set_subtitle("");
        project->set_updateOnLaunch(true);
        project->set_androidBackgroundLocation(true);
        project->set_androidDisableBatterySaver(true);

        QVariantMap connectorParams;
        connectorParams["webUpdateUrl"] = webUpdateUrl;
        connectorParams["app"] = projectUid;
        project->set_connectorParams(connectorParams);

        // Commit project file.
        auto projectFilePath = m_projectManager->getFilePath(projectUid, "Project.qml");
        project->saveToQmlFile(projectFilePath);

        // Update the project.
        return bootstrapUpdate(projectUid);
    }

    // Extract sample.
    if (params.contains("sampleId"))
    {
        auto sampleId = params["sampleId"].toString();
        QFile::remove(path + ".CTS");
        QFile::remove(path + ".WAY");
        QFile::remove(path + ".DAT");
        QFile::remove(path + ".STA");
        Utils::extractResource(":/Classic/" + sampleId + ".CTS", path);
    }

    // Build a list of CTS files.
    auto dir = QDir(path);
    auto filter = QStringList({"*.CTS", "*.cts"});
    auto ctsFiles = QVariantList();

    auto fileInfoList = QFileInfoList(dir.entryInfoList(filter, QDir::Files));
    for (auto fileInfo: fileInfoList)
    {
        auto ctsFile = fileInfo.filePath();
        auto metadata = QVariantMap();
        if (getCTSMetadata(ctsFile, &metadata))
        {
            metadata["path"] = ctsFile;
            ctsFiles.append(metadata);
        }
    }

    // Set up projects for each newly discovered package.
    for (auto ctsFile: ctsFiles)
    {
        auto ctsMetadata = ctsFile.toMap();
        auto projectUid = "CLASSIC2_" + QUuid::createUuid().toString(QUuid::Id128);
        bool replaceExisting = false;

        // Look for a duplicate to replace.
        for (auto project: projects)
        {
            if (project->provider() != CLASSIC_PROVIDER)
            {
                continue;
            }

            if (project->connectorParams().value("app") != ctsMetadata["app"])
            {
                continue;
            }

            projectUid = project->uid();
            replaceExisting = true;

            break;
        }

        // Create Project.qml files for the project.
        auto ctsFilePath = ctsMetadata["path"].toString();

        QVariantMap connectorParams;
        connectorParams["serverFileName"] = ctsMetadata["serverFileName"];
        connectorParams["app"] = ctsMetadata["app"];
        connectorParams["webUpdateUrl"] = ctsMetadata["webUpdateUrl"];

        if (!replaceExisting)
        {
            m_projectManager->init(projectUid, CLASSIC_PROVIDER, QVariantMap(), CLASSIC_CONNECTOR, connectorParams);
        }

        auto project = m_projectManager->loadPtr(projectUid);
        project->set_version(1);
        project->set_icon("qrc:/Classic/logo.svg");
        project->set_title(ctsMetadata["name"].toString());
        project->set_subtitle(getSubtitle(ctsMetadata["serverFileName"].toString()));
        project->set_updateOnLaunch(!connectorParams["webUpdateUrl"].toString().isEmpty());
        project->set_connectorParams(connectorParams);
        project->set_androidBackgroundLocation(true);
        project->set_androidDisableBatterySaver(true);
        project->set_telemetry(ctsMetadata["telemetry"].toMap());

        // Commit project file.
        auto projectFilePath = m_projectManager->getFilePath(projectUid, "Project.qml");
        project->saveToQmlFile(projectFilePath);

        // Ensure we have a valid project in the projects table.
        // Normally this would be handled in the ProjectManager::update, but if the
        // classic app is from a web install, then the update may fail
        // if it is already up to date.
        m_database->addProject(projectUid);

        // Message has been seen and acted upon.
        App::instance()->settings()->set_classicUpgradeMessage(false);
    }

    emit m_projectManager->projectsChanged();

    App::instance()->settings()->set_classicDesktopConnected(true);

    return Success();
}

ApiResult ClassicConnector::hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project)
{
    auto webUpdateUrl = project->connectorParams().value("webUpdateUrl").toString();

    auto response = Utils::httpGet(networkAccessManager, webUpdateUrl);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto defList = QString(response.data).split("\r\n");
    if (defList.length() < 4)
    {
        return Failure(tr("DEF file corrupt"));
    }

    auto ctsMetadata = QVariantMap();
    getCTSMetadata(Utils::classicRoot() + "/" + project->connectorParams()["app"].toString() + ".CTS", &ctsMetadata);

    // Check the DEF file to see if it matches the stampid
    if (ctsMetadata["stampId"].toString().compare(defList[1], Qt::CaseSensitivity::CaseInsensitive) == 0)
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

bool ClassicConnector::canUpdate(Project* project) const
{
    return !project->connectorParams().value("webUpdateUrl").toString().isEmpty();
}

ApiResult ClassicConnector::update(Project* project)
{
    auto updateFolder = m_projectManager->getUpdateFolder(project->uid());
    auto webUpdateUrl = project->connectorParams().value("webUpdateUrl").toString();
    project->saveToQmlFile(updateFolder + "/Project.qml");

    // Update only works for web update.
    if (webUpdateUrl.isEmpty())
    {
        return Failure(tr("Not supported"));
    }

    // Skip update if there is any unsent data.
    FXPROFILE profile = {};
    profile.Magic = MAGIC_PROFILE;
    auto host = std::unique_ptr<CHost_Qt>(new CHost_Qt(NULL, nullptr, &profile, Utils::classicRoot().toStdString().c_str()));
    auto appName = project->connectorParams()["app"].toString().toStdString();
    auto hasData = CctUpdateManager::HasData(host.get(), appName.c_str());

    if (hasData)
    {
        // FlushData will destroy host.
        if (!CctTransferManager::FlushData(host.release(), appName.c_str()))
        {
            return Failure(tr("Unsent data"));
        }
    }

    auto defGet = Utils::httpGet(App::instance()->networkAccessManager(), webUpdateUrl);
    if (!defGet.success)
    {
        return Failure(tr("Not found"));
    }

    auto defList = QString(defGet.data).split("\r\n");
    if (defList.length() < 4)
    {
        return Failure(tr("DEF file corrupt"));
    }

    // Check the version number of CT
//    auto clientVersion = QString("3.") + QString::number(RESOURCE_VERSION);
//    if (defList[0] != clientVersion)
//    {
//        error(4, tr("Wrong version"));
//        return false;
//    }

    auto ctsMetadata = QVariantMap();
    getCTSMetadata(Utils::classicRoot() + "/" + project->connectorParams()["app"].toString() + ".CTS", &ctsMetadata);

    // Check the DEF file to see if it matches the stampid
    if (ctsMetadata["stampId"].toString().compare(defList[1], Qt::CaseSensitivity::CaseInsensitive) == 0)
    {
        return AlreadyUpToDate();
    }

    // Stamp is different, so try and download the ZIP file
    auto zipFilePath = App::instance()->downloadFile(defList[2]);
    if (zipFilePath.isEmpty())
    {
        return Failure(tr("Download failed"));
    }

    auto tempPath = App::instance()->tempPath();
    auto extractedFiles = JlCompress::extractDir(zipFilePath, tempPath);
    QFile::remove(zipFilePath);

    if (extractedFiles.isEmpty())
    {
        return Failure(tr("Bad archive"));
    }

    getCTSMetadata(tempPath + "/Application.CTS", &ctsMetadata);
    project->set_version(1);
    project->set_title(ctsMetadata["name"].toString());
    project->set_subtitle(getSubtitle(ctsMetadata["serverFileName"].toString()));
    project->set_androidBackgroundLocation(true);
    project->set_androidDisableBatterySaver(true);
    project->set_telemetry(ctsMetadata["telemetry"].toMap());

    // Success.
    auto tag = defList.length() > 4 ? defList[4] : "";

    CctUpdateManager::Install(
        tempPath.toStdString().c_str(),
        Utils::classicRoot().toStdString().c_str(),
        project->connectorParams()["app"].toString().toStdString().c_str(),
        tag.toStdString().c_str(),
        webUpdateUrl.toStdString().c_str());

    project->saveToQmlFile(updateFolder + "/Project.qml");

    return Success();
}

void ClassicConnector::reset(Project* project)
{
    auto name = project->connectorParams().value("app").toString();

    auto rootFilePath = Utils::classicRoot() + "/" + name;
    QFile::remove(rootFilePath + ".STA");
    QFile::remove(rootFilePath + ".WAY");
    QFile::remove(rootFilePath + ".DAT");

    auto projectPath = m_projectManager->getFilePath(project->uid());
    QFile::remove(projectPath + "/Elements.qml");
    QFile::remove(projectPath + "/Fields.qml");
}

void ClassicConnector::remove(Project* project)
{
    auto name = project->connectorParams().value("app").toString();

    auto rootFilePath = Utils::classicRoot() + "/" + name;
    QFile::remove(rootFilePath + ".CTS");
    QFile::remove(rootFilePath + ".TXT");
    QFile::remove(rootFilePath + ".WAY");
    QFile::remove(rootFilePath + ".DAT");
    QFile::remove(rootFilePath + ".STA");
    QFile::remove(rootFilePath + ".DEF");
    QFile::remove(rootFilePath + ".TAG");
    QFile::remove(rootFilePath + ".HSI");
    QFile::remove(rootFilePath + ".HST");
}

bool ClassicConnector::getCTSMetadata(const QString& ctsFile, QVariantMap* metadataOut)
{
    auto metadata = QVariantMap();

    auto fileMap = FXFILEMAP();
    if (!FxMapFile(ctsFile.toStdString().c_str(), &fileMap))
    {
        return false;
    }

    auto result = false;
    auto header = static_cast<CTRESOURCEHEADER*>(fileMap.BasePointer);

    if (header->Magic == MAGIC_RESOURCE)
    {
        result = true;
        metadata["name"] = header->Name;
        metadata["serverFileName"] = header->ServerFileName;
        metadata["app"] = QFileInfo(ctsFile).baseName();
        metadata["webUpdateUrl"] = QString(header->WebUpdateUrl);

        auto protocolTable = QMap<int, QString>
        {
            { FXSENDDATA_PROTOCOL_UNKNOWN, "NONE"   },
            { FXSENDDATA_PROTOCOL_FTP,     "FTP"    },
            { FXSENDDATA_PROTOCOL_UNC,     "UNC"    },
            { FXSENDDATA_PROTOCOL_HTTP,    "HTTP"   },
            { FXSENDDATA_PROTOCOL_HTTPS,   "HTTPS"  },
            { FXSENDDATA_PROTOCOL_BACKUP,  "BACKUP" },
            { FXSENDDATA_PROTOCOL_HTTPZ,   "HTTPZ"  },
            { FXSENDDATA_PROTOCOL_ESRI,    "ESRI"   },
            { FXSENDDATA_PROTOCOL_AZURE,   "AZURE"  },
        };

        auto protocol = protocolTable.value(header->SendData.Protocol);
        metadata["protocol"] = protocol;

        metadata["telemetry"] = QVariantMap
        {
            { "sendData", protocol },
        };

        auto appGuid = header->SequenceId;
        CHAR appId[39] = { 0 };
        sprintf(appId, "%08lx%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                appGuid.Data1, appGuid.Data2, appGuid.Data3, appGuid.Data4[0],
                appGuid.Data4[1], appGuid.Data4[2], appGuid.Data4[3], appGuid.Data4[4],
                appGuid.Data4[5], appGuid.Data4[6], appGuid.Data4[7]);

        metadata["appId"] = QString(appId);

        auto stampGuid = header->StampId;
        CHAR stampId[39] = { 0 };
        sprintf(stampId, "%08lx%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                stampGuid.Data1, stampGuid.Data2, stampGuid.Data3, stampGuid.Data4[0],
                stampGuid.Data4[1], stampGuid.Data4[2], stampGuid.Data4[3],
                stampGuid.Data4[4], stampGuid.Data4[5], stampGuid.Data4[6],
                stampGuid.Data4[7]);

        metadata["stampId"] = QString(stampId);

        auto movingMaps = QStringList();
        if (header->MovingMapsOffset != 0)
        {
            for (auto i = 0; i < (int)header->MovingMapsCount; i++)
            {
                auto movingMap = (FXMOVINGMAP *)((quint64)fileMap.BasePointer + (quint64)header->MovingMapsOffset + i * sizeof(FXMOVINGMAP));
                movingMaps.append(movingMap->FileName);
            }
        }
        metadata["movingMaps"] = movingMaps;
    }

    FxUnmapFile(&fileMap);

    *metadataOut = metadata;

    return result;
}

ApiResult ClassicConnector::copyAppFiles(Project* project, const QString& targetPath)
{
    auto rootPath = Utils::classicRoot();
    if (!Utils::ensurePath(rootPath, true))
    {
        return Failure(tr("Cannot create path"));
    }

    auto app = project->connectorParams().value("app").toString();

    auto metadata = QVariantMap();
    if (!getCTSMetadata(rootPath + "/" + app + ".CTS", &metadata))
    {
        return Failure(tr("Failed to read classic project"));
    }

    auto copyFile = [&](const QString& suffix)
    {
        auto fileInfo = QFileInfo(rootPath + "/" + app + suffix);
        if (fileInfo.exists())
        {
            QFile::copy(fileInfo.filePath(), targetPath + "/app" + suffix.toLower());
        }
    };

    auto suffixes = QStringList { ".CTS", ".TXT", ".DEF", ".HSI", ".HST" };
    for (auto it = suffixes.constBegin(); it != suffixes.constEnd(); it++)
    {
        copyFile(*it);
    }

    auto movingMaps = metadata.value("movingMaps").toStringList();
    for (auto it = movingMaps.constBegin(); it != movingMaps.constEnd(); it++)
    {
        QFile::copy(rootPath + "/" + *it, targetPath + "/" + *it);
    }

    return Success();
}

ApiResult ClassicConnector::installAppFiles(Project* project, const QString& sourcePath)
{
    auto rootPath = Utils::classicRoot();
    if (!Utils::ensurePath(rootPath, true))
    {
        return Failure(tr("Cannot create path"));
    }

    auto app = project->connectorParams().value("app").toString();

    auto fileInfoList = QDir(sourcePath).entryInfoList(QDir::Files);
    for (auto it = fileInfoList.constBegin(); it != fileInfoList.constEnd(); it++)
    {
        auto fileInfo = *it;
        if (fileInfo.fileName().startsWith("app."))
        {
            QFile::copy(fileInfo.filePath(), rootPath + "/" + app + "." + fileInfo.suffix().toUpper());
            continue;
        }

        QFile::copy(fileInfo.filePath(), rootPath + "/" + fileInfo.fileName());
    }

    return Success();
}
