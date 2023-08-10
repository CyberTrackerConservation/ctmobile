#include "pch.h"
#include "Project.h"
#include "App.h"
#include "QZXing.h"
#include <jlcompress.h>
#include "Classic/ClassicConnector.h"
#include "Native/NativeConnector.h"
#include "SMART/SMARTConnector.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Project

Project::Project(QObject* parent): QObject(parent)
{
    m_projectManager = App::instance()->projectManager();

    m_version = 0;
    m_kioskMode = m_updateOnLaunch = m_androidBackgroundLocation = m_androidDisableBatterySaver =  m_defaultWizardMode = m_defaultImmersive = false;
    m_defaultSendLocationInterval = 0;
    m_defaultSubmitInterval = 0;
}

void Project::saveToQmlFile(const QString& filePath)
{
    QFile file(filePath);
    file.remove();
    qFatalIf(!file.open(QIODevice::WriteOnly | QIODevice::Text), QString("Failed to create file %1").arg(filePath));

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream.setGenerateByteOrderMark(true);
    stream << "import CyberTracker 1.0\n\n";
    Utils::writeQml(stream, 0, "Project {");
    Utils::writeQml(stream, 1, "uid", m_uid);
    Utils::writeQml(stream, 1, "version", m_version);
    Utils::writeQml(stream, 1, "icon", m_icon);
    Utils::writeQml(stream, 1, "iconDark", m_iconDark);
    Utils::writeQml(stream, 1, "title", m_title);
    Utils::writeQml(stream, 1, "subtitle", m_subtitle);
    Utils::writeQml(stream, 1, "connector", m_connector);
    Utils::writeQml(stream, 1, "connectorParams", m_connectorParams);
    Utils::writeQml(stream, 1, "provider", m_provider);
    Utils::writeQml(stream, 1, "providerParams", m_providerParams);
    Utils::writeQml(stream, 1, "maps", m_maps);
    Utils::writeQml(stream, 1, "colors", m_colors);
    Utils::writeQml(stream, 1, "kioskMode", m_kioskMode, false);
    Utils::writeQml(stream, 1, "kioskModePin", m_kioskModePin);
    Utils::writeQml(stream, 1, "androidPermissions", m_androidPermissions);
    Utils::writeQml(stream, 1, "androidBackgroundLocation", m_androidBackgroundLocation, false);
    Utils::writeQml(stream, 1, "androidDisableBatterySaver", m_androidDisableBatterySaver, false);
    Utils::writeQml(stream, 1, "updateOnLaunch", m_updateOnLaunch, false);
    Utils::writeQml(stream, 1, "startPage", m_startPage);
    Utils::writeQml(stream, 1, "telemetry", m_telemetry);
    Utils::writeQml(stream, 1, "defaultWizardMode", m_defaultWizardMode, false);
    Utils::writeQml(stream, 1, "defaultImmersive", m_defaultImmersive, false);
    Utils::writeQml(stream, 1, "defaultSendLocationInterval", m_defaultSendLocationInterval);
    Utils::writeQml(stream, 1, "esriLocationServiceUrl", m_esriLocationServiceUrl);
    Utils::writeQml(stream, 1, "defaultSubmitInterval", m_defaultSubmitInterval);
    Utils::writeQml(stream, 1, "offlineMapUrl", m_offlineMapUrl);

    Utils::writeQml(stream, 0, "}");
}

QVariant Project::getSetting(const QString& name, const QVariant& defaultValue) const
{
    auto data = QVariantMap();
    auto database = App::instance()->createDatabasePtr();
    database->loadProject(m_uid, &data);

    return data.value(name, defaultValue);
}

void Project::setSetting(const QString& name, const QVariant& value)
{
    auto data = QVariantMap();
    auto database = App::instance()->createDatabasePtr();
    database->loadProject(m_uid, &data);

    if (value == data.value(name))
    {
        return;
    }

    if (value.isValid())
    {
        data.insert(name, Utils::variantFromJSValue(value));
    }
    else
    {
        data.remove(name);
    }

    database->saveProject(m_uid, data);

    emit m_projectManager->projectSettingChanged(m_uid, name, value);
}

QVariant Project::getConnectorParam(const QString& name, const QVariant& defaultValue) const
{
    return m_connectorParams.value(name, defaultValue);
}

void Project::setConnectorParam(const QString& name, const QVariant& value)
{
    if (value.isValid())
    {
        m_connectorParams.insert(name, value);
    }
    else
    {
        m_connectorParams.remove("name");
    }
}

QString Project::languageCode() const
{
    auto data = QVariantMap();
    auto database = App::instance()->createDatabasePtr();
    database->loadProject(m_uid, &data);

    auto languages = data.value("languages").toList();
    auto languageCount = languages.count();
    auto lastLanguageIndex = data.value("languageIndex", -1).toInt();

    auto nextLanguageIndex = lastLanguageIndex;
    if (languageCount > 0 && (lastLanguageIndex == -1 || lastLanguageIndex >= languageCount))
    {
        nextLanguageIndex = 0;
    }

    if (lastLanguageIndex != nextLanguageIndex)
    {
        data.insert("languageIndex", nextLanguageIndex);
        database->saveProject(m_uid, data);
    }

    return nextLanguageIndex == -1 ? QLocale::system().name() : languages[nextLanguageIndex].toMap().value("code").toString();
}

QVariantMap Project::save(bool keepAuth) const
{
    auto settings = QVariantMap();
    App::instance()->createDatabasePtr()->loadProject(m_uid, &settings);

    // Remove volatile settings.
    settings.remove("lastBuildString");
    settings.remove("webUpdateUrl");
    settings.remove("webUpdateMetadata");
    settings.remove("reportedBy");
    settings.remove("reportedByDefault");

    // Remove access tokens.
    if (!keepAuth)
    {
        settings.remove("accessToken");
        settings.remove("refreshToken");
    }

    return QVariantMap
    {
        { "uid", m_uid },
        { "version", m_version },
        { "icon", m_icon },
        { "iconDark", m_iconDark },
        { "title", m_title },
        { "subtitle", m_subtitle },
        { "connector", m_connector },
        { "connectorParams", m_connectorParams },
        { "provider", m_provider },
        { "providerParams", m_providerParams },
        { "maps", m_maps },
        { "colors", m_colors },
        { "kioskMode", m_kioskMode },
        { "kioskModePin", m_kioskModePin },
        { "androidPermissions", m_androidPermissions },
        { "androidDisableBatterySaver", m_androidDisableBatterySaver },
        { "updateOnLaunch", m_updateOnLaunch },
        { "startPage", m_startPage },
        { "telemetry", m_telemetry },
        { "defaultWizardMode", m_defaultWizardMode },
        { "defaultImmersive", m_defaultImmersive },
        { "defaultSendLocationInterval", m_defaultSendLocationInterval },
        { "esriLocationServiceUrl", m_esriLocationServiceUrl },
        { "defaultSubmitInterval", m_defaultSubmitInterval },
        { "offlineMapUrl", m_offlineMapUrl },
        { "settings", settings },
    };
}

void Project::load(const QVariantMap& data, bool loadSettings)
{
    m_uid = data["uid"].toString();
    m_version = data["version"].toInt();
    m_icon = data["icon"].toString();
    m_iconDark = data["iconDark"].toString();
    m_title = data["title"].toString();
    m_subtitle = data["subtitle"].toString();
    m_connector = data["connector"].toString();
    m_connectorParams = data["connectorParams"].toMap();
    m_provider = data["provider"].toString();
    m_providerParams = data["providerParams"].toMap();
    m_maps = data["maps"].toList();
    m_colors = data["colors"].toMap();
    m_kioskMode = data["kioskMode"].toBool();
    m_kioskModePin = data["kioskModePin"].toString();
    m_androidPermissions = data["androidPermissions"].toStringList();
    m_androidBackgroundLocation = data["androidBackgroundLocation"].toBool();
    m_androidDisableBatterySaver = data["androidDisableBatterySaver"].toBool();
    m_updateOnLaunch = data["updateOnLaunch"].toBool();
    m_startPage = data["startPage"].toString();
    m_telemetry = data["telemetry"].toMap();
    m_defaultWizardMode = data["defaultWizardMode"].toBool();
    m_defaultImmersive = data["defaultImmersive"].toBool();
    m_defaultSendLocationInterval = data["defaultSendLocationInterval"].toInt();
    m_esriLocationServiceUrl = data["esriLocationServiceUrl"].toString();
    m_defaultSubmitInterval = data["defaultSubmitInterval"].toInt();
    m_offlineMapUrl = data["offlineMapUrl"].toString();

    if (loadSettings)
    {
        App::instance()->createDatabasePtr()->saveProject(m_uid, data["settings"].toMap());
    }
}

void Project::reload()
{
    auto project = m_projectManager->loadPtr(m_uid);
    auto data = project->save(false);
    load(data, false);
}

QUrl Project::displayIcon() const
{
    auto filePath = App::instance()->settings()->darkTheme() && !m_iconDark.isEmpty() ? m_iconDark : m_icon;

    return filePath.isEmpty() ? QUrl() : m_projectManager->getFileUrl(m_uid, filePath);
}

QUrl Project::loginIcon() const
{
    auto connector = App::instance()->createConnectorPtr(m_connector);
    if (!connector)
    {
        return QUrl();
    }

    return connector->icon();
}

bool Project::canLogin()
{
    auto connector = App::instance()->createConnectorPtr(m_connector);
    if (!connector)
    {
        return true;
    }

    return connector->canLogin(this);
}

bool Project::loggedIn()
{
    auto connector = App::instance()->createConnectorPtr(m_connector);
    if (!connector)
    {
        return true;
    }

    return connector->loggedIn(this);
}

bool Project::login(const QString& server, const QString& username, const QString& password)
{
    auto connector = App::instance()->createConnectorPtr(m_connector);
    if (!connector)
    {
        return false;
    }

    // Logout in all cases.
    connector->logout(this);

    // Login if a password is provided.
    auto result = false;
    if (!password.isEmpty())
    {
        result = connector->login(this, server, username, password);
    }

    // Update the user name.
    set_username(username);

    // Send a change.
    emit loggedInChanged(result);
    emit m_projectManager->projectLoggedInChanged(m_uid, result);

    return result;
}

void Project::login(const QString& username)
{
    auto connector = App::instance()->createConnectorPtr(m_connector);
    if (!connector)
    {
        return;
    }

    // Update the user name.
    set_username(username);

    // Send a change.
    auto loggedIn = connector->loggedIn(this);
    emit loggedInChanged(loggedIn);
    emit m_projectManager->projectLoggedInChanged(m_uid, loggedIn);
}

void Project::logout()
{
    auto connector = App::instance()->createConnectorPtr(m_connector);
    if (!connector)
    {
        return;
    }

    connector->logout(this);

    emit loggedInChanged(false);
    emit m_projectManager->projectLoggedInChanged(m_uid, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HasUpdateTask

UpdateScanTask::UpdateScanTask(QObject* parent, const QString& projectUid): QObject(parent), m_projectUid(projectUid)
{
}

void UpdateScanTask::run()
{
    // Updates not allowed.
    if (!App::instance()->updateAllowed())
    {
        emit completed(m_projectUid, false, false);
        return;
    }

    // Project removed.
    auto project = App::instance()->projectManager()->loadPtr(m_projectUid);
    if (!project.get())
    {
        emit completed(m_projectUid, false, false);
        return;
    }

    // Not finalized.
    if (project->version() == 0)
    {
        emit completed(m_projectUid, false, false);
        return;
    }

    // Connector not valid.
    auto connector = App::instance()->createConnectorPtr(project->connector());
    if (!connector || !connector->canUpdate(project.get()))
    {
        emit completed(m_projectUid, false, false);
        return;
    }

    QNetworkAccessManager networkAccessManager;
    networkAccessManager.setTransferTimeout(10000); // 10s timeout.

    // Check project update.
    auto result = connector->hasUpdate(&networkAccessManager, project.get());
    if (!result.expected)
    {
        // Unexpected failure, e.g. network error.
        emit completed(m_projectUid, false, false);
        return;
    }

    // Check offline map update.
    if (!result.success && !project->offlineMapUrl().isEmpty())
    {
        result = App::instance()->offlineMapManager()->hasUpdate(&networkAccessManager, project->offlineMapUrl());
        if (!result.expected)
        {
            emit completed(m_projectUid, false, false);
            return;
        }
    }

    // Task completed.
    emit completed(m_projectUid, true, result.success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ProjectManager

ProjectManager::ProjectManager(QObject* parent): QObject(parent)
{
    qFatalIf(true, "Singleton");
}

ProjectManager::ProjectManager(const QString& projectsPath, QObject* parent): QObject(parent)
{
    m_database = App::instance()->database();
    m_appLink = App::instance()->appLink();

    m_projectsPath = projectsPath;

    qFatalIf(!Utils::ensurePath(m_projectsPath), "Failed to create projects folder");
}

QString ProjectManager::getFilePath(const QString& projectUid, const QString& filename) const
{
    qFatalIf(projectUid.isEmpty(), "ProjectUid not set");

    auto path = projectUid;

    if (!filename.isEmpty())
    {
        path += "/";
        path += filename;
    }

    return m_projectsPath + "/" + path;
}

QUrl ProjectManager::getFileUrl(const QString& projectUid, const QString& filename) const
{
    if (filename.startsWith("qrc:/"))
    {
        return QUrl(filename);
    }
    else
    {
        return QUrl::fromLocalFile(getFilePath(projectUid, filename));
    }
}

QString ProjectManager::getUpdateFolder(const QString& projectUid) const
{
    return m_projectsPath + "/_" + projectUid;
}

Project* ProjectManager::load(const QString& projectUid) const
{
    auto filePath = getFilePath(projectUid, "Project.qml");
    auto result = qobject_cast<Project*>(Utils::loadQmlFromFile(filePath));

    //
    // Migrate field maps to the offline manager. Build 442.
    //
    if (result && !result->maps().isEmpty())
    {
        App::instance()->offlineMapManager()->installProjectMaps(getFilePath(projectUid), result->maps());
        result->set_maps(QVariantList());
        result->saveToQmlFile(filePath);
    }

    //
    // Fixup due to not needing read and write storage and Android 12+.
    //
    auto androidPermissions = result->androidPermissions();
    androidPermissions.removeOne("READ_EXTERNAL_STORAGE");
    androidPermissions.removeOne("WRITE_EXTERNAL_STORAGE");
    result->set_androidPermissions(androidPermissions);

    //
    // Fixup due to design change.
    //
    if (result && !result->connector().isEmpty())
    {
        return result;
    }

    // Read the old file into a stringlist.
    auto lines = QStringList();
    {
        QFile file(filePath);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            return nullptr;
        }

        lines = QString(file.readAll()).split('\n');
    }

    // Fix the Connector.
    auto connector = QString();
    auto connectorParams = QVariantMap();
    auto connectorIndex = -1;
    {
        // Lookup the old connector which was a JSON object.
        for (auto i = 0; i < lines.count(); i++)
        {
            if (lines[i].startsWith("    connector: {"))
            {
                connectorIndex = i;
                break;
            }
        }

        if (connectorIndex != -1)
        {
            // Extract the name.
            auto connectorJson = lines[connectorIndex].mid(14);
            connectorParams = QJsonDocument::fromJson(connectorJson.toLatin1()).object().toVariantMap();
            connector = connectorParams.value("Name", connectorParams.value("name").toString()).toString();
            connectorParams.remove("Name");
            connectorParams.remove("name");
            lines.removeAt(connectorIndex);

            // Make the first character of the properties lower case.
            auto keys = connectorParams.keys();
            while (keys.count() > 0)
            {
                auto key = keys[0];
                auto value = connectorParams.value(key);
                connectorParams.remove(key);

                key[0] = key[0].toLower();
                connectorParams.insert(key, value);
                keys.removeFirst();
            }
        }
    }

    // Fix the colors.
    auto colors = QVariantMap();
    {
        auto colorIndex = -1;
        for (auto i = 0; i < lines.count(); i++)
        {
            if (lines[i].startsWith("    colors: {"))
            {
                colorIndex = i;
                break;
            }
        }

        if (colorIndex != -1)
        {
            auto colorJson = lines[colorIndex].mid(11);
            colors = QJsonDocument::fromJson(colorJson.toLatin1()).object().toVariantMap();

            // Make the first character of the properties lower case.
            auto keys = colors.keys();
            while (keys.count() > 0)
            {
                auto key = keys[0];
                auto value = colors.value(key);
                colors.remove(key);

                key[0] = key[0].toLower();
                colors.insert(key, value);
                keys.removeFirst();
            }
        }
    }

    // Fix the subtitle: subTitle -> subtitle.
    {
        auto subtitleIndex = -1;
        for (auto i = 0; i < lines.count(); i++)
        {
            if (lines[i].startsWith("    subTitle: "))
            {
                subtitleIndex = i;
                break;
            }
        }

        if (subtitleIndex != -1)
        {
            lines[subtitleIndex] = lines[subtitleIndex].replace("subTitle", "subtitle");
        }
    }

    // Rewrite the file (without the "connector").
    {
        QFile file(filePath);
        if (!file.open(QFile::WriteOnly | QFile::Text))
        {
            return nullptr;
        }

        if (!file.write(lines.join('\n').toLatin1()))
        {
            return nullptr;
        }
    }

    // Reload the file - this should succeed.
    result = qobject_cast<Project*>(Utils::loadQmlFromFile(filePath));
    if (!result)
    {
        return nullptr;
    }

    // Update the connector parameters.
    if (connectorIndex != -1)
    {
        result->set_connector(connector);
        result->set_connectorParams(connectorParams);
    }

    // Update the colors.
    result->set_colors(colors);

    // Persist the file.
    result->saveToQmlFile(filePath);

    return result;
}

std::unique_ptr<Project> ProjectManager::loadPtr(const QString& projectUid) const
{
    return std::unique_ptr<Project>(load(projectUid));
}

bool ProjectManager::init(const QString& projectUid, const QString& provider, const QVariantMap& providerParams, const QString& connector, const QVariantMap& connectorParams)
{
    qFatalIf(exists(projectUid), "Project folder already exists");

    auto projectDir = QDir::cleanPath(m_projectsPath + "/" + projectUid);
    QDir(projectDir).removeRecursively();
    qFatalIf(QDir().exists(projectDir), "Project folder already exists");
    qFatalIf(!QDir().mkdir(projectDir), "Failed to create project folder");

    Project project;
    project.set_uid(projectUid);
    project.set_version(0);
    project.set_provider(provider);
    project.set_providerParams(providerParams);
    project.set_connector(connector);
    project.set_connectorParams(connectorParams);
    project.saveToQmlFile(getFilePath(projectUid, "Project.qml"));

    m_database->removeProject(projectUid);
    m_database->addProject(projectUid);

    update_lastNewProjectUid(projectUid);

    return true;
}

bool ProjectManager::exists(const QString& projectUid) const
{
    if (!QFile::exists(getFilePath(projectUid, "Project.qml")))
    {
        return false;
    }

    auto project = loadPtr(projectUid);
    return project && project->version() != 0;
}

QList<std::shared_ptr<Project>> ProjectManager::buildList() const
{
    auto projectList = QList<std::shared_ptr<Project>>();
    auto orderList = App::instance()->settings()->projects();

    QDir projectDir(m_projectsPath);
    QStringList projectFolders = projectDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs);

    for (auto projectIt = projectFolders.constBegin(); projectIt != projectFolders.constEnd(); projectIt++)
    {
        auto projectUid = *projectIt;

        // Skip updates.
        if (projectUid[0] == "_")
        {
            continue;
        }

        // Ensure the Project.qml is valid.
        auto project = loadPtr(projectUid);
        if (!project)
        {
            continue;
        }

        // Version 0 means its waiting for an update.
        if (project->version() == 0)
        {
            continue;
        }

        // Remove Classic broken folders.
        #if defined(Q_OS_ANDROID)
        if (projectUid.startsWith("CLASSIC_"))
        {
            App::instance()->settings()->set_classicUpgradeMessage(true);

            // Remove the project folder.
            auto projectDir = getFilePath(projectUid);
            qFatalIf(!QDir(projectDir).removeRecursively(), "Failed to delete project folder");
            m_database->removeProject(projectUid);

            continue;
        }
        #endif

        // This is a real project.
        m_database->addProject(projectUid);
        projectList.append(std::shared_ptr<Project>(project.release()));

        if (!orderList.contains(projectUid))
        {
            orderList.append(projectUid);
        }
    }

    // Sort by order list.
    std::sort(projectList.begin(), projectList.end(), [&](const std::shared_ptr<Project>& p1, const std::shared_ptr<Project>& p2) -> bool
    {
        return orderList.indexOf(p1->uid()) < orderList.indexOf(p2->uid());
    });

    orderList.clear();
    for (auto projectIt = projectList.constBegin(); projectIt != projectList.constEnd(); projectIt++)
    {
        orderList.append(projectIt->get()->uid());
    }

    App::instance()->settings()->set_projects(orderList);

    return projectList;
}

void ProjectManager::moveProject(const QString& projectUid, int delta)
{
    auto projects = App::instance()->settings()->projects();
    auto index = projects.indexOf(projectUid);

    if (index == -1)
    {
        qDebug() << "Error bad index";
        return;
    }

    projects.move(index, index + delta);
    App::instance()->settings()->set_projects(projects);

    emit projectsChanged();
}

int ProjectManager::count() const
{
    return buildList().count();
}

QVariantMap ProjectManager::getShareData(const QString& projectUid, bool auth) const
{
    auto project = loadPtr(projectUid);
    if (!project)
    {
        return QVariantMap();
    }

    auto connector = App::instance()->createConnectorPtr(project->connector());
    if (!connector)
    {
        return QVariantMap();
    }

    return connector->getShareData(project.get(), auth);
}

bool ProjectManager::canShareLink(const QString& projectUid) const
{
    return projectUid.isEmpty() ? false : !getShareData(projectUid, true).isEmpty() || !getShareData(projectUid, false).isEmpty();
}

bool ProjectManager::canShareAuth(const QString& projectUid) const
{
    auto project = loadPtr(projectUid);
    if (!project)
    {
        return false;
    }

    auto connector = App::instance()->createConnectorPtr(project->connector());
    if (!connector)
    {
        return false;
    }

    return connector->canShareAuth(project.get());
}

QString ProjectManager::getShareUrl(const QString& projectUid, bool auth) const
{
    auto data = getShareData(projectUid, auth);
    if (data.isEmpty())
    {
        return QString();
    }

    return QString("%1?x=%2").arg(App::instance()->config()["appLinkUrls"].toStringList().constFirst(), m_appLink->encode(data));
}

QString ProjectManager::createQRCode(const QString& projectUid, bool auth, int size) const
{
    auto data = getShareUrl(projectUid, auth);

    auto result = App::instance()->sendFilePath() + "/QRCode.png";

    auto image = QZXing::encodeData(data, QZXing::EncoderFormat_QR_CODE, QSize(size, size), QZXing::EncodeErrorCorrectionLevel_L, true, false);
    image.save(result, "PNG");

    return result;
}

bool ProjectManager::canExportData(const QString& projectUid) const
{
    auto project = loadPtr(projectUid);
    if (!project)
    {
        return false;
    }

    auto connector = App::instance()->createConnectorPtr(project->connector());
    if (!connector)
    {
        return false;
    }

    return connector->canExportData(project.get());
}

bool ProjectManager::canSharePackage(const QString& projectUid) const
{
    auto project = loadPtr(projectUid);
    if (!project)
    {
        return false;
    }

    auto connector = App::instance()->createConnectorPtr(project->connector());
    if (!connector)
    {
        return false;
    }

    return connector->canSharePackage(project.get());
}

QString ProjectManager::createPackage(const QString& projectUid, bool includeProject, bool keepAuth, bool includeData) const
{
    auto filePath = App::instance()->sendFilePath() + "/Form.zip";
    qDebug() << "Create package for project: " << projectUid;
    qFatalIf(QFile::exists(filePath) && !QFile::remove(filePath), "Failed to delete old package");

    auto packagePath = QDir::cleanPath(App::instance()->tempPath() + "/Package");
    QDir(packagePath).removeRecursively();
    qFatalIf(!Utils::ensurePath(packagePath), "Failed to create temp folder");

    // Create the project.json.
    auto project = loadPtr(projectUid);
    auto projectJson = project->save(keepAuth);

    // Copy the project.
    if (includeProject)
    {
        auto projectPath = packagePath + "/project";
        qFatalIf(!Utils::ensurePath(projectPath), "Failed to create temp folder");

        if (project->connector() == CLASSIC_CONNECTOR)
        {
            ClassicConnector::copyAppFiles(project.get(), projectPath);
        }
        else
        {
            Utils::copyPath(getFilePath(projectUid), projectPath);
        }

        // Let the Provider make changes - typically cleanup of cached QML files.
        if (!App::instance()->createFormPtr(projectUid, "")->provider()->finalizePackage(projectPath))
        {
            qDebug() << "Provider failed to finalize the package";
            return QString();
        }

        // Remove the Project.qml - contents now in project.json.
        QFile::remove(projectPath + "/Project.qml");
    }

    // Copy data.
    if (includeData)
    {
        auto dataPath = packagePath + "/data";
        qFatalIf(!Utils::ensurePath(dataPath), "Failed to create data folder");
        auto attachmentsPath = packagePath + "/attachments";
        qFatalIf(!Utils::ensurePath(attachmentsPath), "Failed to create attachments folder");

        auto sightingUids = QStringList();
        m_database->getSightings(projectUid, "*", Sighting::DB_SIGHTING_FLAG | Sighting::DB_LOCATION_FLAG, &sightingUids);

        auto fileCounter = 0;
        auto sightingBatch = QVariantList();

        auto flushSightingBatch = [&]()
        {
            Utils::writeJsonToFile(QString("%1%2%3%4").arg(dataPath, "/", QString::number(fileCounter), ".json"), Utils::variantListToJson(sightingBatch));
            fileCounter++;
            sightingBatch.clear();
        };

        auto sightingCount = 0;
        auto locationCount = 0;
        auto attachmentCount = 0;
        auto forms = QMap<QString, Form*>();

        for (auto sightingIt = sightingUids.constBegin(); sightingIt != sightingUids.constEnd(); sightingIt++)
        {
            auto sightingUid = *sightingIt;

            auto data = QVariantMap();
            auto flags = (uint)0;
            auto stateSpace = QString();
            auto attachments = QStringList();
            m_database->loadSighting(projectUid, sightingUid, &data, &flags, &stateSpace, nullptr);

            if (flags & Sighting::DB_LOCATION_FLAG)      // Export location.
            {
                locationCount++;
            }
            else if (flags & Sighting::DB_SIGHTING_FLAG) // Export sighting.
            {
                sightingCount++;

                if (!forms.contains(stateSpace))
                {
                    forms[stateSpace] = App::instance()->createFormPtr(projectUid, stateSpace).release();
                }

                auto sighting = forms[stateSpace]->createSightingPtr(data);
                data = sighting->saveForExport(&attachments);

                // Copy attachments.
                for (auto attachmentIt = attachments.constBegin(); attachmentIt != attachments.constEnd(); attachmentIt++)
                {
                    QFile file(App::instance()->getMediaFilePath(*attachmentIt));
                    if (!file.exists())
                    {
                        qDebug() << "Failed to copy attachment: " << *attachmentIt;
                        continue;
                    }

                    if (!file.copy(attachmentsPath + "/" + QFileInfo(file).fileName()))
                    {
                        qDebug() << "Failed to copy attachment";
                        continue;
                    }

                    attachmentCount++;
                }
            }

            // Add the uid.
            data["uid"] = sightingUid;

            // Add the flags.
            data["flags"] = flags;

            // Set state space if present.
            if (!stateSpace.isEmpty())
            {
                data["stateSpace"] = stateSpace;
            }

            // Add attachments if present.
            if (!attachments.isEmpty())
            {
                data["attachments"] = attachments;
            }

            // Batch the sighting.
            sightingBatch.append(data);

            // Flush batches of 1000 sightings to avoid giant JSON files.
            if (sightingBatch.count() == 1000)
            {
                flushSightingBatch();
            }
        }

        qDeleteAll(forms);

        // Flush any remaining sightings.
        flushSightingBatch();

        // Remove data folder if empty.
        if (locationCount == 0 && sightingCount == 0)
        {
            QDir(dataPath).removeRecursively();
        }

        // Remove attachments folder if empty.
        if (attachmentCount == 0)
        {
            QDir(attachmentsPath).removeRecursively();
        }
    }

    // Write the final project file.
    Utils::writeJsonToFile(packagePath + "/project.json", Utils::variantMapToJson(projectJson));

    // Zip the file.
    if (!JlCompress::compressDir(filePath, packagePath, true))
    {
        qDebug() << "ZIP failed to finalize the package";
        return QString();
    }

    // Cleanup temp.
    QDir(packagePath).removeRecursively();

    // Return the package.
    Utils::mediaScan(filePath);

    return filePath;
}

bool ProjectManager::canInstallPackage(const QString& filePath) const
{
    QuaZip zip(filePath);
    if (!zip.open(QuaZip::mdUnzip))
    {
        return false;
    }

    auto fileNameList = zip.getFileNameList();
    return fileNameList.contains("project.json");
}

ApiResult ProjectManager::installPackage(const QString& filePath)
{
    pauseUpdateScan();
    auto resumeUpdateScanOnExit = qScopeGuard([&] { resumeUpdateScan(); });

    // Detect SMART package.
    auto smartProject = Utils::variantMapFromFileInZip(filePath, "project.json");
    if (smartProject.contains("smart_version"))
    {
        return App::instance()->createConnectorPtr(SMART_CONNECTOR)->bootstrap(QVariantMap {{"filePath", filePath }});
    }

    // Unpack zip to package path.
    auto packagePath = Utils::unpackZip(App::instance()->tempPath(), "Package", filePath);

    if (packagePath.isEmpty())
    {
        return ApiResult::Error(tr("Failed to extract"));
    }

    // Read the project json.
    auto projectJson = Utils::variantMapFromJsonFile(packagePath + "/project.json");
    auto projectUid = projectJson.value("uid").toString();
    if (projectJson.isEmpty() || projectUid.isEmpty())
    {
        return ApiResult::Error(tr("Invalid package"));
    }

    // Install project if one is in the package and it does not already exist.
    auto projectAdded = false;
    auto projectPath = packagePath + "/project";
    auto hasProject = QDir(projectPath).exists();
    if (!exists(projectUid) && hasProject)
    {
        // Initialize the project and Project.qml file.
        if (!init(projectUid, "", QVariantMap(), "", QVariantMap()))
        {
            return ApiResult::Error(QString(tr("Failed to initialize %1")).arg(App::instance()->alias_project()));
        }

        projectAdded = true;

        auto project = loadPtr(projectUid);
        project->load(projectJson, true);

        // Copy project contents.
        if (projectJson.value("connector").toString() == CLASSIC_CONNECTOR)
        {
            ClassicConnector::installAppFiles(project.get(), projectPath);
        }
        else
        {
            Utils::copyPath(projectPath, getFilePath(projectUid));
        }

        // Ensure the project has a valid version.
        if (project->version() == 0)
        {
            project->set_version(1);
        }

        // Overwrite Project.qml.
        project->saveToQmlFile(getFilePath(projectUid, "Project.qml"));
    }

    // Add data if there is any in the package and the project exists.
    auto dataAdded = false;
    auto dataPath = packagePath + "/data";
    auto hasData = QDir(dataPath).exists();
    if (exists(projectUid) && hasData)
    {
        auto dataFiles = QDir(dataPath).entryInfoList(QStringList({"*.json"}), QDir::Files);

        // Sort the data files so they are imported in the right order.
        std::sort(dataFiles.begin(), dataFiles.end(), [&](QFileInfo& f1, QFileInfo& f2) -> bool
        {
            auto n1 = f1.baseName().toInt();
            auto n2 = f2.baseName().toInt();
            return n1 < n2;
        });

        // Iterate over the files and import the data into the database.
        for (auto fileIt = dataFiles.constBegin(); fileIt != dataFiles.constEnd(); fileIt++)
        {
            dataAdded = true;

            auto sightings = Utils::variantListFromJsonFile(fileIt->filePath());

            for (auto sightingIt = sightings.constBegin(); sightingIt != sightings.constEnd(); sightingIt++)
            {
                auto data = sightingIt->toMap();
                auto uid = data.value("uid").toString();
                data.remove("uid");
                auto stateSpace = data.value("stateSpace").toString();
                data.remove("stateSpace");
                auto flags = data.value("flags").toUInt();
                data.remove("flags");
                auto attachments = data.value("attachments").toStringList();
                data.remove("attachments");

                m_database->saveSighting(projectUid, stateSpace, uid, flags, data, attachments);
            }
        }

        // Copy the attachments to the media folder.
        auto attachmentsPath = packagePath + "/attachments";
        if (QDir(attachmentsPath).exists())
        {
            Utils::copyPath(attachmentsPath, App::instance()->mediaPath());
        }
    }

    QDir(packagePath).removeRecursively();

    if (hasProject && !projectAdded && !dataAdded)
    {
        return ApiResult::Expected(tr("Already installed"));
    }

    if (!projectAdded && !dataAdded)
    {
        return ApiResult::Expected(tr("Nothing to do"));
    }

    if (projectAdded)
    {
        emit projectsChanged();
    }

    return ApiResult::Success();
}

QVariantMap ProjectManager::webInstall(const QString& webUpdateUrl)
{
    auto finalWebUpdateUrl = Utils::redirectOnlineDriveUrl(webUpdateUrl);

    // Get definition file.
    auto defGet = Utils::httpGet(App::instance()->networkAccessManager(), finalWebUpdateUrl);
    if (!defGet.success)
    {
        return ApiResult::ErrorMap(tr("File not found"));
    }

    // Classic - def file is text format.
    if (QString(defGet.data).startsWith("3."))
    {
        return bootstrap(CLASSIC_CONNECTOR, QVariantMap {{ "webUpdateUrl", finalWebUpdateUrl }});
    }

    // Native - def file is json.
    auto defMap = Utils::variantMapFromJson(defGet.data);
    if (defMap.contains("ctVersion"))
    {
        return bootstrap("Native", QVariantMap {{ "webUpdateUrl", finalWebUpdateUrl }});
    }

    return ApiResult::ErrorMap(tr("Bad definition format"));
}

QVariantMap ProjectManager::bootstrap(const QString& connectorName, const QVariantMap& params, bool showToasts)
{
    pauseUpdateScan();
    auto resumeUpdateScanOnExit = qScopeGuard([&] { resumeUpdateScan(); });

    auto connector = App::instance()->createConnectorPtr(connectorName);
    qFatalIf(!connector.get(), "Provider not found");

    auto result = connector->bootstrap(params);

    if (showToasts && !result.errorString.isEmpty())
    {
        if (result.expected)
        {
            emit App::instance()->showToast(result.errorString);
        }
        else if (!result.success)
        {
            emit App::instance()->showError(result.errorString);
        }
    }

    if (!params.contains("no-telemetry"))
    {
        App::instance()->telemetry()->aiSendEvent("bootstrap", QVariantMap {{ "connector", connectorName }, { "success", result.success }, { "errorString", result.errorString }, { "expected", result.expected }});
    }

    return result.toMap();
}

void ProjectManager::hasUpdateAsync(const QString& projectUid)
{
    // Skip running tasks.
    if (m_updateScanTasks.value(projectUid))
    {
        return;
    }

    // Start task.
    auto task = new UpdateScanTask(nullptr, projectUid);
    task->setAutoDelete(true);
    connect(task, &UpdateScanTask::completed, this, [&](const QString& projectUid, bool success, bool hasUpdate)
    {
        m_updateScanTasks.remove(projectUid);

        if (m_updateScanTasks.isEmpty())
        {
            emit projectUpdateScanComplete();
        }

        if (m_blockUpdateScanCounter == 0 && success)
        {
            auto project = loadPtr(projectUid);
            if (project.get())
            {
                project->set_hasUpdate(hasUpdate);
            }
        }
    });

    m_updateScanTasks.insert(projectUid, true);
    QThreadPool::globalInstance()->start(task);
}

void ProjectManager::hasUpdatesAsync()
{
    // Check if updates allowed.
    if (!App::instance()->updateAllowed())
    {
        return;
    }

    // Enumerate the projects.
    auto projects = QDir(m_projectsPath).entryList(QDir::NoDotAndDotDot | QDir::Dirs);
    for (auto projectIt = projects.constBegin(); projectIt != projects.constEnd(); projectIt++)
    {
        auto projectUid = *projectIt;
        if (projectUid[0] != "_")
        {
            hasUpdateAsync(projectUid);
        }
    }
}

void ProjectManager::pauseUpdateScan()
{
    m_blockUpdateScanCounter++;
    if (m_blockUpdateScanCounter > 1)
    {
        return;
    }

    // Rundown the tasks.
    if (m_updateScanTasks.isEmpty())
    {
        return;
    }

    QEventLoop eventLoop;
    connect(this, &ProjectManager::projectUpdateScanComplete, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
}

void ProjectManager::resumeUpdateScan()
{
    m_blockUpdateScanCounter--;
    qFatalIf(m_blockUpdateScanCounter < 0, "Bad block counter");
}

bool ProjectManager::canLogin(const QString& projectUid) const
{
    auto project = loadPtr(projectUid);

    // Override for webUpdateUrl.
    auto connector = project->webUpdateUrl().isEmpty() ? project->connector() : NATIVE_CONNECTOR;

    return App::instance()->createConnectorPtr(connector)->canLogin(project.get());
}

bool ProjectManager::canUpdate(const QString& projectUid) const
{
    auto project = loadPtr(projectUid);
    if (!project->loggedIn())
    {
        return false;
    }

    // Override for webUpdateUrl.
    auto connector = project->webUpdateUrl().isEmpty() ? project->connector() : NATIVE_CONNECTOR;

    return App::instance()->createConnectorPtr(connector)->canUpdate(project.get());
}

QVariantMap ProjectManager::update(const QString& projectUid, bool showToasts)
{
    auto resultsMap = updateAll("", projectUid);
    auto result = ApiResult::fromMap(resultsMap[projectUid].toMap());

    if (showToasts)
    {
        if (result.success)
        {
            emit App::instance()->showToast(tr("Update Complete"));
        }
        else if (result.expected)
        {
            emit App::instance()->showToast(result.errorString);
        }
        else
        {
            emit App::instance()->showError(result.errorString);
        }
    }

    return result.toMap();
}

QVariantMap ProjectManager::updateAll(const QString& filterConnectorName, const QString& filterProjectUid)
{
    pauseUpdateScan();
    auto resumeUpdateScanOnExit = qScopeGuard([&] { resumeUpdateScan(); });

    auto result = QVariantMap();

    QDir projectDir(m_projectsPath);
    QStringList projectFolders = projectDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs);
    int okayCount = 0;
    int failCount = 0;

    for (auto projectIt = projectFolders.constBegin(); projectIt != projectFolders.constEnd(); projectIt++)
    {
        auto projectUid = *projectIt;

        // Cleanup any in-flight update folder in case we crashed during the last sync.
        auto projectFolder = m_projectsPath + "/" + projectUid;
        if (projectUid[0] == "_")
        {
            QDir(projectFolder).removeRecursively();
            continue;
        }

        // Skip if the project filter doesn't match.
        if (!filterProjectUid.isEmpty() && (filterProjectUid != projectUid))
        {
            continue;
        }

        // Skip over any broken projects.
        auto project = loadPtr(projectUid);
        if (!project.get())
        {
            continue;
        }

        // Get the Connector.
        auto connector = App::instance()->createConnectorPtr(project->connector());

        // Skip if the provider filter doesn't match.
        if (!filterConnectorName.isEmpty() && (connector->name() != filterConnectorName))
        {
            continue;
        }

        // Skip if the project is not logged in.
        if (!project->loggedIn())
        {
            continue;
        }

        // Create a temporary update folder.
        auto updateFolder = getUpdateFolder(projectUid);
        if (!QDir(updateFolder).removeRecursively())
        {
            continue;
        }

        if (!Utils::ensurePath(updateFolder))
        {
            continue;
        }

        // Override for webUpdateUrl.
        if (!project->webUpdateUrl().isEmpty())
        {
            connector.reset();
            connector = App::instance()->createConnectorPtr(NATIVE_CONNECTOR);
        }

        // Start the updates.
        auto updateResult = connector->update(project.get());

        // If the update was successful, update the Project.qml file with the connection settings.
        if (updateResult.success)
        {
            auto updateProject = std::unique_ptr<Project>(qobject_cast<Project*>(Utils::loadQmlFromFile(updateFolder + "/Project.qml")));
            if (updateProject)
            {
                // Project was updated, so clear out unreferenced cached Qml files.
                App::instance()->garbageCollectQmlCache();

                // TODO(justin): do we need this.
                // Update the project file.
                updateProject->set_uid(projectUid);
                updateProject->saveToQmlFile(updateFolder + "/Project.qml");
            }
            else
            {
                updateResult = ApiResult::Error(QString(tr("Bad %1 file")).arg(App::instance()->alias_project()));
            }
        }

        // Get or update the offline map.
        auto mapUpdateResult = ApiResult::Expected(tr("Up to date"));
        if (updateResult.success || updateResult.expected)
        {
            auto updateProject = std::unique_ptr<Project>(qobject_cast<Project*>(Utils::loadQmlFromFile(updateFolder + "/Project.qml")));
            auto offlineMapUrl = updateProject ? updateProject->offlineMapUrl() : project->offlineMapUrl();
            if (!offlineMapUrl.isEmpty())
            {
                mapUpdateResult = App::instance()->offlineMapManager()->installOrUpdatePackage(offlineMapUrl);
            }
        }

        // Check if map update failed.
        if (!mapUpdateResult.expected)
        {
            // This means that the project and offline map are atomically updated.
            updateResult = mapUpdateResult;
        }

        // The update was successful, so switch the folders.
        if (updateResult.success)
        {
            m_database->addProject(project->uid());

            auto projectFolderOld = App::instance()->backupPath() + "/" + project->uid();
            QDir(projectFolderOld).removeRecursively();

            auto success = QDir().rename(projectFolder, projectFolderOld) && QDir().rename(updateFolder, projectFolder);
            if (success)
            {
                // Ensure the backup does not contain a potentially large SMART map.
                QFile::remove(projectFolderOld + "/map.mbtiles");
            }
            else
            {
                updateResult = ApiResult::Error(tr("Update error"));
            }
        }

        // If the only update was the offline map, then mark the update as successful.
        if (updateResult.expected && mapUpdateResult.success)
        {
            updateResult = ApiResult::Success();
        }

        // Set the lastUpdate if something happened.
        if (updateResult.success)
        {
            project->set_lastUpdate(App::instance()->timeManager()->currentDateTimeISO());
        }

        // Clear the hasUpdate if there was no failure.
        if (updateResult.success || updateResult.expected)
        {
            project->set_hasUpdate(false);
        }

        // Track stats.
        updateResult.success ? okayCount++ : failCount++;

        // Purge the update folder.
        if (!QDir(updateFolder).removeRecursively())
        {
            // Fail?
        }

        result[projectUid] = updateResult.toMap();

        emit projectUpdateComplete(projectUid, updateResult.toMap());
    }

    if (okayCount > 0)
    {
        emit projectsChanged();
    }

    return result;
}

void ProjectManager::startUpdateScanner()
{
    connect(&m_updateScanTimer, &QTimer::timeout, this, [&]()
    {
        m_updateScanTimer.setInterval(App::instance()->desktopOS() ? 10000 : 40000);   // Next interval.

        if (m_blockUpdateScanCounter == 0)
        {
            hasUpdatesAsync();
        }
    });

    m_updateScanTimer.setInterval(5000);        // Initial interval.
    m_updateScanTimer.start();
}

void ProjectManager::modify(const QString& projectUid, const std::function<void(Project*)>& callback)
{
    auto project = loadPtr(projectUid);
    callback(project.get());
    project->saveToQmlFile(getFilePath(projectUid, "Project.qml"));
}

void ProjectManager::reset(const QString& projectUid, bool keepData)
{
    pauseUpdateScan();
    auto resumeUpdateScanOnExit = qScopeGuard([&] { resumeUpdateScan(); });

    // Connector clean up.
    auto project = loadPtr(projectUid);
    App::instance()->createConnectorPtr(project->connector())->reset(project.get());

    // Cleanup all project state, but keep the values minus provider state.
    auto projectValues = QVariantMap();
    m_database->loadProject(projectUid, &projectValues);
    projectValues.remove("providerState");

    if (keepData)
    {
        // Only clear the form state.
        m_database->deleteFormState(projectUid, "*");
    }
    else
    {
        // Remove and re-add the project: this clears all state.
        m_database->removeProject(projectUid);
        m_database->addProject(projectUid);
        m_database->saveProject(projectUid, projectValues);
    }

    // Cleanup attachments.
    App::instance()->garbageCollectMedia();
}

void ProjectManager::remove(const QString& projectUid)
{
    pauseUpdateScan();
    auto resumeUpdateScanOnExit = qScopeGuard([&] { resumeUpdateScan(); });

    // Connector clean up.
    auto project = loadPtr(projectUid);
    if (project)
    {
        App::instance()->createConnectorPtr(project->connector())->remove(project.get());
    }

    // Remove the project.
    auto projectDir = getFilePath(projectUid);
    qFatalIf(!QDir(projectDir).removeRecursively(), "Failed to delete project folder");
    m_database->removeProject(projectUid);

    // Cleanup attachments.
    App::instance()->garbageCollectMedia();

    emit projectsChanged();
}
