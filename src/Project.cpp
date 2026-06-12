#include "pch.h"
#include "Project.h"
#include "App.h"
#include "Classic/ClassicConnector.h"
#include "Native/NativeConnector.h"
#include "SMART/SMARTConnector.h"

namespace CyberTracker {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Project

Project::Project(QObject* parent): QObject(parent)
{
    m_projectManager = App::instance()->projectManager();

    m_version = 0;
    m_androidBackgroundLocation = m_androidDisableBatterySaver = false; // Legacy.
    m_kioskMode = m_updateOnLaunch = m_defaultWizardMode = m_defaultImmersive = m_defaultSimpleCamera = false;
    m_requireCamera = m_requireMicrophone = m_requireLocation = m_requireBackgroundLocation = false;
    m_defaultLocationPointInterval = m_defaultLocationTrackInterval = m_defaultLocationTrackDistance = 0;
    m_defaultSubmitInterval = 0;
    m_disableArchive = false;

    connect(App::instance()->settings(), &Settings::languageCodeChanged, this, [this]()
    {
        if (languageIndex() == -1)
        {
            emit titleChanged();
        }
    });

    connect(m_projectManager, &ProjectManager::projectSettingChanged, this, [this](const QString& projectUid, const QString& name, const QVariant& value)
    {
        if (projectUid == m_uid && name == "languageIndex")
        {
            emit titleChanged();
        }
    });
}

Project::~Project()
{
}

void Project::saveToQmlFile(const QString& filePath)
{
    QFile file(filePath);
    file.remove();
    qFatalIf(!file.open(QIODevice::WriteOnly | QIODevice::Text), QString("Failed to create file %1").arg(filePath));

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Encoding::Utf8);
    stream.setGenerateByteOrderMark(true);
    stream << "import CyberTracker 1.0\n\n";
    Utils::writeQml(stream, 0, "Project {");
    Utils::writeQml(stream, 1, "uid", m_uid);
    Utils::writeQml(stream, 1, "version", m_version);
    Utils::writeQml(stream, 1, "icon", m_icon);
    Utils::writeQml(stream, 1, "iconDark", m_iconDark);
    Utils::writeQml(stream, 1, "name", m_name);
    Utils::writeQml(stream, 1, "names", m_names);
    Utils::writeQml(stream, 1, "subtitle", m_subtitle);
    Utils::writeQml(stream, 1, "connector", m_connector);
    Utils::writeQml(stream, 1, "connectorParams", m_connectorParams);
    Utils::writeQml(stream, 1, "provider", m_provider);
    Utils::writeQml(stream, 1, "providerParams", m_providerParams);
    Utils::writeQml(stream, 1, "maps", m_maps);
    Utils::writeQml(stream, 1, "colors", m_colors);
    Utils::writeQml(stream, 1, "kioskMode", m_kioskMode, false);
    Utils::writeQml(stream, 1, "kioskModePin", m_kioskModePin);
    Utils::writeQml(stream, 1, "updateOnLaunch", m_updateOnLaunch, false);
    Utils::writeQml(stream, 1, "startPage", m_startPage);
    Utils::writeQml(stream, 1, "telemetry", m_telemetry);
    Utils::writeQml(stream, 1, "defaultWizardMode", m_defaultWizardMode, false);
    Utils::writeQml(stream, 1, "defaultImmersive", m_defaultImmersive, false);
    Utils::writeQml(stream, 1, "defaultSimpleCamera", m_defaultSimpleCamera, false);
    Utils::writeQml(stream, 1, "defaultLocationPointInterval", m_defaultLocationPointInterval, 0);
    Utils::writeQml(stream, 1, "defaultLocationTrackInterval", m_defaultLocationTrackInterval, 0);
    Utils::writeQml(stream, 1, "defaultLocationTrackDistance", m_defaultLocationTrackDistance, 0);
    Utils::writeQml(stream, 1, "defaultLocationServiceUrl", m_defaultLocationServiceUrl);
    Utils::writeQml(stream, 1, "defaultTrackFieldUid", m_defaultTrackFieldUid);
    Utils::writeQml(stream, 1, "defaultTrackFileFormat", m_defaultTrackFileFormat);
    Utils::writeQml(stream, 1, "defaultSubmitInterval", m_defaultSubmitInterval);
    Utils::writeQml(stream, 1, "offlineMapUrl", m_offlineMapUrl);
    Utils::writeQml(stream, 1, "disableArchive", m_disableArchive, false);
    Utils::writeQml(stream, 1, "requireLocation", m_requireLocation, false);
    Utils::writeQml(stream, 1, "requireBackgroundLocation", m_requireBackgroundLocation, false);
    Utils::writeQml(stream, 1, "requireMicrophone", m_requireMicrophone, false);
    Utils::writeQml(stream, 1, "requireCamera", m_requireCamera, false);

    Utils::writeQml(stream, 0, "}");

    m_projectManager->setCacheEntry(m_uid, save(false, false));
}

QVariant Project::getSetting(const QString& name, const QVariant& defaultValue) const
{
    auto data = QVariantMap();
    auto threadDatabase = std::unique_ptr<Database>();
    auto database = App::instance()->database();

    if (!Utils::isMainThread())
    {
        threadDatabase = App::instance()->createDatabasePtr();
        database = threadDatabase.get();
    }

    // Ensure the database is still valid. Fixes a race where the database is removed
    // before the deleted signal is sent and then something on a timer comes through
    // like "hasStreamers" and then it faults. The fix could be to signal deleted before
    // deleting, but this has its own issues.
    if (!database->testProject(m_uid))
    {
        qDebug() << "Error: project is not valid: " << m_uid;
        return defaultValue;
    }

    database->loadProject(m_uid, &data);

    return data.value(name, defaultValue);
}

void Project::setSetting(const QString& name, const QVariant& value)
{
    auto data = QVariantMap();
    auto threadDatabase = std::unique_ptr<Database>();
    auto database = App::instance()->database();

    if (!Utils::isMainThread())
    {
        threadDatabase = App::instance()->createDatabasePtr();
        database = threadDatabase.get();
    }

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
        m_connectorParams.remove(name);
    }
}

QString Project::accessToken() const
{
    return App::instance()->vault()->getAccessToken(m_uid);
}

void Project::set_accessToken(const QString& token)
{
    App::instance()->vault()->setAccessToken(m_uid, token);
}

QString Project::uploadToken() const
{
    return App::instance()->vault()->getUploadToken(m_uid);
}

void Project::set_uploadToken(const QString& token)
{
    App::instance()->vault()->setUploadToken(m_uid, token);
}

QString Project::refreshToken() const
{
    return App::instance()->vault()->getRefreshToken(m_uid);
}

void Project::set_refreshToken(const QString& token)
{
    App::instance()->vault()->setRefreshToken(m_uid, token);
}

QString Project::languageCode() const
{
    auto data = QVariantMap();
    auto database = App::instance()->createDatabasePtr();
    database->loadProject(m_uid, &data);

    auto languages = data.value("languages").toList();
    auto languageCount = languages.count();
    auto languageIndex = data.value("languageIndex", -1).toInt();

    if (languageIndex >= 0 && languageIndex < languageCount)
    {
        return languages[languageIndex].toMap().value("code").toString();
    }

    auto systemCode = App::instance()->locale().name();
    for (auto i = 0; i < languageCount; i++)
    {
        auto code = languages[i].toMap()["code"].toString();
        if (code == systemCode)
        {
            languageIndex = i;
            break; // Exact match -> done.
        }

        if (code.left(2) == systemCode.left(2))
        {
            languageIndex = i;
            continue; // Partial match -> continue.
        }
    }

    // Matched, so persist this option.
    if (languageIndex != -1)
    {
        data.insert("languageIndex", languageIndex);
        database->saveProject(m_uid, data);

        return languages[languageIndex].toMap().value("code").toString();
    }

    // Pick the first language or the system language.
    return languageCount == 0 ? systemCode : languages.constFirst().toMap().value("code").toString();
}

QVariantMap Project::save(bool keepSettings, bool keepAuth) const
{
    auto result = QVariantMap
    {
        { "uid", m_uid },
        { "version", m_version },
        { "icon", m_icon },
        { "iconDark", m_iconDark },
        { "name", m_name },
        { "names", m_names },
        { "subtitle", m_subtitle },
        { "connector", m_connector },
        { "connectorParams", m_connectorParams },
        { "provider", m_provider },
        { "providerParams", m_providerParams },
        { "maps", m_maps },
        { "colors", m_colors },
        { "kioskMode", m_kioskMode },
        { "kioskModePin", m_kioskModePin },
        { "requireCamera", m_requireCamera },
        { "requireMicrophone", m_requireMicrophone },
        { "requireLocation", m_requireLocation },
        { "requireBackgroundLocation", m_requireBackgroundLocation },
        { "updateOnLaunch", m_updateOnLaunch },
        { "startPage", m_startPage },
        { "telemetry", m_telemetry },
        { "defaultWizardMode", m_defaultWizardMode },
        { "defaultImmersive", m_defaultImmersive },
        { "defaultSimpleCamera", m_defaultSimpleCamera },
        { "defaultLocationPointInterval", m_defaultLocationPointInterval },
        { "defaultLocationTrackInterval", m_defaultLocationTrackInterval },
        { "defaultLocationTrackDistance", m_defaultLocationTrackDistance },
        { "defaultLocationServiceUrl", m_defaultLocationServiceUrl },
        { "defaultTrackFieldUid", m_defaultTrackFieldUid },
        { "defaultTrackFileFormat", m_defaultTrackFileFormat },
        { "defaultSubmitInterval", m_defaultSubmitInterval },
        { "offlineMapUrl", m_offlineMapUrl },
        { "disableArchive", m_disableArchive },
    };

    if (keepSettings)
    {
        auto settings = QVariantMap();
        App::instance()->createDatabasePtr()->loadProject(m_uid, &settings);

        // Remove volatile settings.
        settings.remove("lastBuildString");
        settings.remove("webUpdateUrl");
        settings.remove("webUpdateMetadata");
        settings.remove("webUpdateETag");
        settings.remove("reportedBy");
        settings.remove("reportedByDefault");

        // Load tokens.
        if (keepAuth)
        {
            settings.insert("accessToken", accessToken());
            settings.insert("refreshToken", refreshToken());
        }

        result.insert("settings", QVariant(settings));
    }

    return result;
}

void Project::load(const QVariantMap& data, bool loadSettings)
{
    m_uid = data["uid"].toString();
    m_version = data["version"].toInt();
    m_icon = data["icon"].toString();
    m_iconDark = data["iconDark"].toString();
    m_name = data.contains("title") ? data["title"].toString() : data["name"].toString();
    m_names = data["names"].toMap();
    m_subtitle = data["subtitle"].toString();
    m_connector = data["connector"].toString();
    m_connectorParams = data["connectorParams"].toMap();
    m_provider = data["provider"].toString();
    m_providerParams = data["providerParams"].toMap();
    m_maps = data["maps"].toList();
    m_colors = data["colors"].toMap();
    m_kioskMode = data["kioskMode"].toBool();
    m_kioskModePin = data["kioskModePin"].toString();
    m_requireCamera = data["requireCamera"].toBool();
    m_requireMicrophone = data["requireMicrophone"].toBool();
    m_requireLocation = data["requireLocation"].toBool();
    m_requireBackgroundLocation = data["requireBackgroundLocation"].toBool();
    m_updateOnLaunch = data["updateOnLaunch"].toBool();
    m_startPage = data["startPage"].toString();
    m_telemetry = data["telemetry"].toMap();
    m_defaultWizardMode = data["defaultWizardMode"].toBool();
    m_defaultImmersive = data["defaultImmersive"].toBool();
    m_defaultSimpleCamera = data["defaultSimpleCamera"].toBool();
    m_defaultLocationPointInterval = data.value("defaultLocationPointInterval", 0).toInt();
    m_defaultLocationTrackInterval = data.value("defaultLocationTrackInterval", 0).toInt();
    m_defaultLocationTrackDistance = data.value("defaultLocationTrackDistance", 0).toInt();
    m_defaultLocationServiceUrl = data["defaultLocationServiceUrl"].toString();
    m_defaultTrackFieldUid = data["defaultTrackFieldUid"].toString();
    m_defaultTrackFileFormat = data["defaultTrackFileFormat"].toString();
    m_defaultSubmitInterval = data["defaultSubmitInterval"].toInt();
    m_offlineMapUrl = data["offlineMapUrl"].toString();
    m_disableArchive = data["disableArchive"].toBool();

    if (loadSettings)
    {
        auto settings = data["settings"].toMap();

        App::instance()->vault()->setAccessToken(m_uid, settings.value("accessToken").toString());
        App::instance()->vault()->setRefreshToken(m_uid, settings.value("refreshToken").toString());
        settings.remove("accessToken");
        settings.remove("refreshToken");

        App::instance()->createDatabasePtr()->saveProject(m_uid, settings);
    }
}

void Project::reload()
{
    auto project = m_projectManager->loadPtr(m_uid);
    auto data = project->save(true, false);
    load(data, false);
}

QString Project::archiveFile(const QString& filename)
{
    return archiveFiles(QStringList { filename } ).value(filename).toString();
}

QVariantMap Project::archiveFiles(const QStringList& files)
{
    auto result = QVariantMap();
    auto cache = hashCache();

    for (auto fileIt = files.constBegin(); fileIt != files.constEnd(); fileIt++)
    {
        auto filename = *fileIt;
        if (cache.contains(filename))
        {
            if (QFile::exists(App::instance()->getArchiveFilePath(filename)))
            {
                result.insert(filename, cache.value(filename).toString());
                continue;
            }
            else
            {
                cache.remove(filename);
            }
        }

        auto filePath = m_projectManager->getFilePath(m_uid, filename);
        if (!QFile::exists(filePath))
        {
            continue;
        }

        auto hashFilename = App::instance()->copyToArchive(filePath);
        result.insert(filename, hashFilename);
        cache.insert(filename, hashFilename);
    }

    set_hashCache(cache);

    return result;
}

QString Project::displayIcon() const
{
    auto filePath = App::instance()->settings()->darkTheme() && !m_iconDark.isEmpty() ? m_iconDark : m_icon;

    return filePath.isEmpty() ? QString() : m_projectManager->getFileUrl(m_uid, filePath);
}

QString Project::loginIcon() const
{
    auto connector = App::instance()->createConnectorPtr(m_connector);
    if (!connector)
    {
        return QString();
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

void Project::login2(const QString& username)
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

QString Project::title() const
{
    if (m_names.isEmpty())
    {
        return m_name;
    }

    auto languageCode = this->languageCode();

    // Exact match.
    auto result = m_names.value(languageCode);
    if (result.isValid())
    {
        // Language found!
        return result.toString();
    }

    // First two letters.
    languageCode = languageCode.left(2);
    result = m_names.value(languageCode);
    if (result.isValid())
    {
        return result.toString();
    }

    for (auto languageIt = m_names.constKeyValueBegin(); languageIt != m_names.constKeyValueEnd(); languageIt++)
    {
        if (languageIt->first.left(2) == languageCode)
        {
            return languageIt->second.toString();
        }
    }

    auto fallback = m_names.contains("default") ? "default" : m_names.firstKey();
    return m_names[fallback].toString();
}

QVariantMap Project::summary() const
{
    return QVariantMap
    {
        { "uid", m_uid },
        { "title", title() },
        { "subtitle", m_subtitle },
        { "connector", m_connector },
        { "provider", m_provider },
        { "username", username() },
        { "reportedBy", reportedBy() },
    };
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

QString ProjectManager::getFileUrl(const QString& projectUid, const QString& filename) const
{
    if (filename.startsWith("qrc:/"))
    {
        return filename;
    }
    else
    {
        return QUrl::fromLocalFile(getFilePath(projectUid, filename)).toString();
    }
}

QString ProjectManager::getUpdateFolder(const QString& projectUid) const
{
    return m_projectsPath + "/_" + projectUid;
}

// Legacy fix-ups for older Project.qml formats. Delete this function (and its call site in
// buildCache) when legacy support is dropped.
static Project* applyLegacyProjectMigrations(QQmlEngine* qmlEngine, const QString& filePath, const QString& projectUid)
{
    auto result = qobject_cast<Project*>(Utils::loadQmlFromFile(qmlEngine, filePath));
    if (!result)
    {
        // Migrate "title" to "name".
        auto data = Utils::readDataFromFile(filePath);
        if (!data.contains("    title: "))
        {
            return nullptr;
        }

        data.replace("    title: ", "    name: ");
        Utils::writeDataToFile(filePath, data);

        result = qobject_cast<Project*>(Utils::loadQmlFromFile(qmlEngine, filePath));
        if (!result)
        {
            return nullptr;
        }
    }

    //
    // Convert to new permissions.
    //
    auto androidPermissions = result->androidPermissions();
    auto permissionsChanged = false;
    if (androidPermissions.contains("CAMERA"))
    {
        permissionsChanged = true;
        result->set_requireCamera(true);
    }

    if (androidPermissions.contains("RECORD_AUDIO"))
    {
        permissionsChanged = true;
        result->set_requireMicrophone(true);
    }

    if (androidPermissions.contains("FINE_LOCATION"))
    {
        permissionsChanged = true;
        result->set_requireLocation(true);
    }

    if (result->androidBackgroundLocation())
    {
        permissionsChanged = true;
        result->set_requireBackgroundLocation(true);
    }

    result->set_androidPermissions(QStringList());
    result->set_androidBackgroundLocation(false);

    if (permissionsChanged)
    {
        result->saveToQmlFile(filePath);
    }

    //
    // Fixup due to design change.
    //
    if (!result->connector().isEmpty())
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
    result = qobject_cast<Project*>(Utils::loadQmlFromFile(qmlEngine, filePath));
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

Project* ProjectManager::load(const QString& projectUid) const
{
    if (!QFile::exists(getFilePath(projectUid, "Project.qml")))
    {
        return nullptr;
    }

    static bool s_cacheBuilt = false;
    if (!s_cacheBuilt)
    {
        s_cacheBuilt = true;
        buildCache();
    }

    QMutexLocker locker(&m_cacheLock);
    if (!m_cache.contains(projectUid))
    {
        return nullptr;
    }

    auto result = new Project();
    result->load(m_cache.value(projectUid), false);
    return result;
}

void ProjectManager::buildCache() const
{
    qFatalIf(!Utils::isMainThread(), "buildCache must run on main thread");

    auto qmlEngine = App::instance()->qmlEngine();

    QMutexLocker locker(&m_cacheLock);
    m_cache.clear();

    Utils::enumFolders(m_projectsPath, [&](const QFileInfo& fi)
    {
        auto projectUid = fi.fileName();
        if (projectUid.isEmpty() || projectUid[0] == '_')
        {
            return;
        }

        auto filePath = m_projectsPath + "/" + projectUid + "/Project.qml";

        // Cleanup legacy Project.qml.json sidecar.
        QFile::remove(filePath + ".json");

        auto project = std::unique_ptr<Project>(applyLegacyProjectMigrations(qmlEngine, filePath, projectUid));
        if (project)
        {
            m_cache[projectUid] = project->save(false, false);
        }
    });
}

void ProjectManager::setCacheEntry(const QString& projectUid, const QVariantMap& data)
{
    QMutexLocker locker(&m_cacheLock);
    m_cache[projectUid] = data;
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
    if (projectUid.isEmpty() || !QFile::exists(getFilePath(projectUid, "Project.qml")))
    {
        return false;
    }

    auto project = loadPtr(projectUid);
    return project && project->version() != 0;
}

void ProjectManager::enumProjects(const std::function<void(const QString& projectUid)>& callback) const
{
    Utils::enumFolders(m_projectsPath, [callback](const QFileInfo& fileInfo)
    {
        auto projectUid = fileInfo.fileName();

        // Skip updates.
        if (projectUid[0] == '_')
        {
            return;
        }

        callback(projectUid);
    });
}

QStringList ProjectManager::buildList() const
{
    auto result = QStringList();

    enumProjects([&](const QString& projectUid)
    {
        result.append(projectUid);
    });

    return result;
}

QList<std::shared_ptr<Project>> ProjectManager::buildListPtr() const
{
    auto projectList = QList<std::shared_ptr<Project>>();
    auto orderList = App::instance()->settings()->projects();

    auto projectUids = buildList();
    for (auto projectIt = projectUids.constBegin(); projectIt != projectUids.constEnd(); projectIt++)
    {
        auto projectUid = *projectIt;

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

    auto image = Utils::renderQRCode(data, size, size);
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

QString ProjectManager::createArchivePackage() const
{
    return createPackage(App::instance()->timeManager()->makeUniqueFilename("Archive.zip"), PROJECT_ARCHIVE, false, false, true);
}

QString ProjectManager::createPackage(const QString& filename, const QString& projectUid, bool includeProject, bool keepAuth, bool includeData) const
{
    auto app = App::instance();

    auto archiveMode = projectUid == PROJECT_ARCHIVE;
    qFatalIf(archiveMode && includeProject, "Cannot include project in archive mode");
    qFatalIf(archiveMode && keepAuth, "Cannot include auth in archive mode");
    qFatalIf(archiveMode && !includeData, "Cannot exclude data in archive mode");

    auto filePath = app->sendFilePath() + "/" + filename;
    qDebug() << "Create package for project: " << projectUid;
    qFatalIf(QFile::exists(filePath) && !QFile::remove(filePath), "Failed to delete old package");

    auto packagePath = QDir::cleanPath(app->tempPath() + "/Package");
    QDir(packagePath).removeRecursively();
    qFatalIf(!Utils::ensurePath(packagePath), "Failed to create temp folder");

    // Copy the project.
    if (includeProject)
    {
        // Create the project.json.
        auto project = loadPtr(projectUid);

        auto projectPath = packagePath + "/project";
        qFatalIf(!Utils::ensurePath(projectPath), "Failed to create temp folder");

        if (project->connector() == ClassicConnector::CONNECTOR_NAME)
        {
            ClassicConnector::copyAppFiles(project.get(), projectPath);
        }
        else
        {
            Utils::copyPath(getFilePath(projectUid), projectPath);
        }

        // Let the Provider make changes - typically cleanup of cached QML files.
        if (!app->createFormPtr(projectUid, "")->provider()->finalizePackage(projectPath))
        {
            qDebug() << "Provider failed to finalize the package";
            return QString();
        }

        // Remove the Project.qml - contents now in project.json.
        QFile::remove(projectPath + "/Project.qml");

        // Write the final project file.
        auto projectJson = project->save(true, keepAuth);
        Utils::writeJsonToFile(packagePath + "/project.json", Utils::variantMapToJson(projectJson));
    }

    // Copy data.
    if (includeData)
    {
        auto dataPath = packagePath + "/data";
        qFatalIf(!Utils::ensurePath(dataPath), "Failed to create data folder");
        auto attachmentsPath = packagePath + "/attachments";
        qFatalIf(!Utils::ensurePath(attachmentsPath), "Failed to create attachments folder");
        auto archivePath = packagePath + "/archive";
        qFatalIf(!Utils::ensurePath(archivePath), "Failed to create archive folder");

        auto sightingCount = 0;
        auto locationCount = 0;
        auto attachmentCount = 0;
        auto archiveCount = 0;
        auto fileCounter = 0;
        auto sightingBatch = QVariantList();
        auto archiveFiles = QStringList();

        auto flushSightingBatch = [&]()
        {
            Utils::variantListToJsonFile(QString("%1%2%3%4").arg(dataPath, "/", QString::number(fileCounter), ".json"), sightingBatch);
            fileCounter++;
            sightingBatch.clear();
        };

        m_database->enumData(projectUid, "*", Database::DataFlag::DB_SIGHTING_FLAG,
            [&](const QString& /*projectUid*/, const QString& uid, const Database::DbData& dbData)
            {
                auto exportData = QVariantMap();

                if (dbData.flags.testFlag(Database::DataFlag::DB_SIGHTING_FLAG)) // Export sighting.
                {
                    auto sighting = std::unique_ptr<Sighting>(new Sighting(dbData.data, dbData.stateSpace, Schema::acquire(dbData.schemaHash), app->settings()->languageCode()));
                    auto onExit = qScopeGuard([&] { Schema::release(dbData.schemaHash); });

                    sightingCount++;

                    auto attachments = QStringList();
                    exportData = sighting->saveForExport(&attachments);

                    auto db = QVariantMap {
                        { "uid", uid },
                        { "stateSpace", dbData.stateSpace },
                        { "flags", static_cast<uint>(dbData.flags) },
                        { "x", dbData.x },
                        { "y", dbData.y },
                    };

                    if (!attachments.isEmpty())
                    {
                        db["attachments"] = attachments;
                    }

                    exportData["db"] = db;

                    // Copy archives.
                    if (archiveMode)
                    {
                        archiveFiles.append(sighting->archiveFiles());
                        for (auto archiveFileIt = archiveFiles.constBegin(); archiveFileIt != archiveFiles.constEnd(); archiveFileIt++)
                        {
                            auto filename = *archiveFileIt;
                            auto filePath = app->getArchiveFilePath(filename);
                            auto targetFilePath = archivePath + "/" + filename;
                            if (QFile::exists(targetFilePath))
                            {
                                continue;
                            }

                            if (!QFile::copy(filePath, targetFilePath))
                            {
                                qDebug() << "Failed to copy archived file: " << filename;
                                continue;
                            }

                            archiveCount++;
                        }
                    }

                    // Copy attachments.
                    for (auto attachmentIt = attachments.constBegin(); attachmentIt != attachments.constEnd(); attachmentIt++)
                    {
                        auto filename = *attachmentIt;
                        auto filePath = app->getMediaFilePath(*attachmentIt);

                        if (!QFile::copy(filePath, attachmentsPath + "/" + filename))
                        {
                            qDebug() << "Failed to copy attachment: " << filename;
                            continue;
                        }

                        attachmentCount++;
                    }
                }

                // Batch the sighting.
                sightingBatch.append(exportData);

                // Flush batches of 1000 sightings to avoid giant JSON files.
                if (sightingBatch.count() == 1000)
                {
                    flushSightingBatch();
                }
            });

        // Flush any remaining sightings.
        flushSightingBatch();

        // Remove data folder if empty.
        if (locationCount == 0 && sightingCount == 0)
        {
            QDir(dataPath).removeRecursively();
        }

        // Remove archive folder if empty.
        if (archiveCount == 0)
        {
            QDir(archivePath).removeRecursively();
        }

        // Remove attachments folder if empty.
        if (attachmentCount == 0)
        {
            QDir(attachmentsPath).removeRecursively();
        }

        if (archiveMode)
        {
            auto settings = app->settings();
            Utils::variantMapToJsonFile(packagePath + "/project.json", QVariantMap {
                { "uid", PROJECT_ARCHIVE },
                { "author", settings->username() },
                { "deviceId", settings->deviceId() },
                { "sightingCount", sightingCount },
                { "locationCount", locationCount },
            });
        }
   }

    // Zip the file.
    if (!Utils::zipCompressDir(filePath, packagePath))
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

QString ProjectManager::createArchivePackage(const QString& filename, const QStringList& sightingUids) const
{
    auto app = App::instance();

    auto filePath = app->sendFilePath() + "/" + filename;
    qDebug() << "Create archive package with data points: " << sightingUids.count();
    qFatalIf(QFile::exists(filePath) && !QFile::remove(filePath), "Failed to delete old package");

    auto packagePath = QDir::cleanPath(app->tempPath() + "/Package");
    QDir(packagePath).removeRecursively();
    qFatalIf(!Utils::ensurePath(packagePath), "Failed to create temp folder");

    // Copy data.
    auto dataPath = packagePath + "/data";
    qFatalIf(!Utils::ensurePath(dataPath), "Failed to create data folder");
    auto attachmentsPath = packagePath + "/attachments";
    qFatalIf(!Utils::ensurePath(attachmentsPath), "Failed to create attachments folder");
    auto archivePath = packagePath + "/archive";
    qFatalIf(!Utils::ensurePath(archivePath), "Failed to create archive folder");

    auto sightingCount = 0;
    auto attachmentCount = 0;
    auto archiveCount = 0;
    auto fileCounter = 0;
    auto sightingBatch = QVariantList();
    auto archiveFiles = QStringList();

    auto flushSightingBatch = [&]()
    {
        Utils::variantListToJsonFile(QString("%1%2%3%4").arg(dataPath, "/", QString::number(fileCounter), ".json"), sightingBatch);
        fileCounter++;
        sightingBatch.clear();
    };

    m_database->enumData("*", "*", Database::DataFlag::DB_SIGHTING_FLAG,
        [&](const QString& /*projectUid*/, const QString& uid, const Database::DbData& dbData)
        {
            if (!sightingUids.contains(uid))
            {
                return;
            }

            auto exportData = QVariantMap();
            auto sighting = std::unique_ptr<Sighting>(new Sighting(dbData.data, dbData.stateSpace, Schema::acquire(dbData.schemaHash), app->settings()->languageCode()));
            auto onExit = qScopeGuard([&] { Schema::release(dbData.schemaHash); });

            sightingCount++;

            auto attachments = QStringList();
            exportData = sighting->saveForExport(&attachments);

            auto db = QVariantMap
            {
                { "uid", uid },
                { "stateSpace", dbData.stateSpace },
                { "flags", static_cast<uint>(dbData.flags) },
                { "x", dbData.x },
                { "y", dbData.y },
            };

            if (!attachments.isEmpty())
            {
                db["attachments"] = attachments;
            }

            exportData["db"] = db;

            // Copy archives.
            archiveFiles.append(sighting->archiveFiles());
            for (auto archiveFileIt = archiveFiles.constBegin(); archiveFileIt != archiveFiles.constEnd(); archiveFileIt++)
            {
                auto filename = *archiveFileIt;
                auto filePath = app->getArchiveFilePath(filename);
                auto targetFilePath = archivePath + "/" + filename;
                if (QFile::exists(targetFilePath))
                {
                    continue;
                }

                if (!QFile::copy(filePath, targetFilePath))
                {
                    qDebug() << "Failed to copy archived file: " << filename;
                    continue;
                }

                archiveCount++;
            }

            // Copy attachments.
            for (auto attachmentIt = attachments.constBegin(); attachmentIt != attachments.constEnd(); attachmentIt++)
            {
                auto filename = *attachmentIt;
                auto filePath = app->getMediaFilePath(*attachmentIt);

                if (!QFile::copy(filePath, attachmentsPath + "/" + filename))
                {
                    qDebug() << "Failed to copy attachment: " << filename;
                    continue;
                }

                attachmentCount++;
            }

            // Batch the sighting.
            sightingBatch.append(exportData);

            // Flush batches of 1000 sightings to avoid giant JSON files.
            if (sightingBatch.count() == 1000)
            {
                flushSightingBatch();
            }
        });

    // Flush any remaining sightings.
    flushSightingBatch();

    // Remove data folder if empty.
    if (sightingCount == 0)
    {
        QDir(dataPath).removeRecursively();
    }

    // Remove archive folder if empty.
    if (archiveCount == 0)
    {
        QDir(archivePath).removeRecursively();
    }

    // Remove attachments folder if empty.
    if (attachmentCount == 0)
    {
        QDir(attachmentsPath).removeRecursively();
    }

    auto settings = app->settings();
    Utils::variantMapToJsonFile(packagePath + "/project.json", QVariantMap
    {
        { "uid", PROJECT_ARCHIVE },
        { "author", settings->username() },
        { "deviceId", settings->deviceId() },
        { "sightingCount", sightingCount },
    });

    // Zip the file.
    if (!Utils::zipCompressDir(filePath, packagePath))
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
    if (!Utils::isZipFile(filePath))
    {
        return false;
    }

    auto projectJson = Utils::zipGetVariantMapFromFile(filePath, "project.json");
    return !projectJson.value("uid").toString().isEmpty()
        || projectJson.contains("smart_version");
}

ApiResult ProjectManager::installPackage(const QString& filePath, QString* projectUidOut)
{
    auto hasUpdateLock = HasUpdate::Lock();
    auto uploadQueueLock = UploadQueue::Lock();

    // Detect SMART package.
    auto smartProject = Utils::zipGetVariantMapFromFile(filePath, "project.json");
    if (smartProject.contains("smart_version"))
    {
        return App::instance()->createConnectorPtr(SMARTConnector::CONNECTOR_NAME)->bootstrap(QVariantMap {{ "filePath", filePath }});
    }

    // Unpack zip to package path.
    auto packagePath = Utils::zipUnpack(App::instance()->tempPath(), "Package", filePath);
    if (packagePath.isEmpty())
    {
        return ApiResult::Error(tr("Failed to extract"));
    }
    auto cleanupPackagePath = qScopeGuard([&] { QDir(packagePath).removeRecursively(); });

    // Read the project json.
    auto projectJson = Utils::variantMapFromJsonFile(packagePath + "/project.json");
    auto projectUid = projectJson.value("uid").toString();
    if (projectJson.isEmpty() || projectUid.isEmpty())
    {
        return ApiResult::Error(tr("Invalid package"));
    }

    if (projectUidOut)
    {
        *projectUidOut = projectUid;
    }

    // Ensure a valid connector.
    auto connector = projectJson.value("connector").toString();
    if (!connector.isEmpty() && !App::instance()->createConnectorPtr(connector))
    {
        return ApiResult::Error(tr("Not supported"));
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
        if (projectJson.value("connector").toString() == ClassicConnector::CONNECTOR_NAME)
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

        emit projectInstalled(projectUid, ApiResult::SuccessMap());
    }

    // Add data if there is any in the package and the project exists.
    auto dataAdded = false;
    auto dataPath = packagePath + "/data";
    auto hasData = QDir(dataPath).exists();
    if (hasData && (projectUid == PROJECT_ARCHIVE || exists(projectUid)))
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

            auto data = Utils::variantListFromJsonFile(fileIt->filePath());

            for (auto dataIt = data.constBegin(); dataIt != data.constEnd(); dataIt++)
            {
                auto dbData = Database::DbData();
                dbData.data = dataIt->toMap();

                auto db = dbData.data["db"].toMap();
                dbData.data.remove("db");

                auto uid = db["uid"].toString();
                auto flags = Database::DataFlags(db["flags"].toUInt());
                dbData.flags = flags;

                if (flags.testFlag(Database::DataFlag::DB_SIGHTING_FLAG))
                {
                    dbData.schemaHash = dbData.data["schemaHash"].toString();   // From sighting.
                    dbData.timestamp = dbData.data["updateTime"].toString();    // From sighting.

                    dbData.stateSpace = db["stateSpace"].toString();            // From metadata.
                    dbData.x = db["x"].toDouble();                              // From metadata.
                    dbData.y = db["y"].toDouble();                              // From metadata.
                    dbData.attachments = db["attachments"].toStringList();      // From metadata.
                }

                m_database->saveData(projectUid, uid, dbData);
            }
        }

        // Copy the attachments to the media folder.
        auto attachmentsPath = packagePath + "/attachments";
        if (QDir(attachmentsPath).exists())
        {
            Utils::copyPath(attachmentsPath, App::instance()->mediaPath());
        }

        // Copy the archive files.
        auto archivePath = packagePath + "/archive";
        if (QDir(archivePath).exists())
        {
            Utils::copyPath(archivePath, App::instance()->archivePath());
        }
    }

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
    auto result = QVariantMap();

    auto filePath = App::instance()->downloadFile(finalWebUpdateUrl);
    auto cleanupFile = qScopeGuard([&] { if (!filePath.isEmpty()) QFile::remove(filePath); });

    if (filePath.isEmpty())
    {
        result = ApiResult::ErrorMap(tr("Download failed"));
    }
    else if (canInstallPackage(filePath))
    {
        auto projectUid = QString();
        result = installPackage(filePath, &projectUid).toMap();
        if (result.value("success").toBool() && !projectUid.isEmpty())
        {
            auto project = loadPtr(projectUid);
            if (project && project->connector() == NativeConnector::CONNECTOR_NAME)
            {
                auto head = Utils::httpGetHead(App::instance()->networkAccessManager(), finalWebUpdateUrl);
                project->set_webUpdateUrl(finalWebUpdateUrl);
                project->set_webUpdateETag(head.etag);
            }
        }
    }
    else if (App::instance()->offlineMapManager()->canInstallPackage(filePath))
    {
        auto head = Utils::httpGetHead(App::instance()->networkAccessManager(), finalWebUpdateUrl);
        result = App::instance()->offlineMapManager()->installPackage(filePath, QString(), finalWebUpdateUrl, head.etag).toMap();
    }
    else
    {
        // Text/JSON definition file (Classic or Native metadata).
        auto file = QFile(filePath);
        auto data = file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
        file.close();

        if (QString(data).startsWith("3."))
        {
            result = bootstrap(ClassicConnector::CONNECTOR_NAME, QVariantMap {{ "webUpdateUrl", finalWebUpdateUrl }}, false);
        }
        else if (Utils::variantMapFromJson(data).contains("ctVersion"))
        {
            result = bootstrap(NativeConnector::CONNECTOR_NAME, QVariantMap {{ "webUpdateUrl", finalWebUpdateUrl }}, false);
        }
        else
        {
            result = ApiResult::ErrorMap(tr("Bad definition format"));
        }
    }

    if (result.value("success").toBool())
    {
        emit App::instance()->showToast(tr("Install successful"));
    }
    else if (result.value("expected").toBool())
    {
        emit App::instance()->showToast(result.value("errorString").toString());
    }
    else
    {
        emit App::instance()->showError(result.value("errorString").toString());
    }

    return result;
}

QVariantMap ProjectManager::bootstrap(const QString& connectorName, const QVariantMap& params, bool showToasts)
{
    auto hasUpdateLock = HasUpdate::Lock();
    auto uploadQueueLock = UploadQueue::Lock();

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

bool ProjectManager::canLogin(const QString& projectUid) const
{
    auto project = loadPtr(projectUid);

    // Override for webUpdateUrl.
    auto connector = project->webUpdateUrl().isEmpty() ? project->connector() : NativeConnector::CONNECTOR_NAME;

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
    auto connector = project->webUpdateUrl().isEmpty() ? project->connector() : NativeConnector::CONNECTOR_NAME;

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
    auto hasUpdateLock = HasUpdate::Lock();
    auto uploadQueueLock = UploadQueue::Lock();

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
        if (projectUid[0] == '_')
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

        // Skip connectors which fail to create.
        if (!connector)
        {
            result[projectUid] = ApiResult::Error(tr("Not supported")).toMap();
            continue;
        }

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
            connector = App::instance()->createConnectorPtr(NativeConnector::CONNECTOR_NAME);
        }

        // If the version is 0, then we are bootstrapping.
        auto bootstrap = project->version() == 0;

        // Start the updates.
        auto updateResult = connector->update(project.get());

        // If the update was successful, update the Project.qml file with the connection settings.
        if (updateResult.success)
        {
            auto updateProject = std::unique_ptr<Project>(qobject_cast<Project*>(Utils::loadQmlFromFile(App::instance()->qmlEngine(), updateFolder + "/Project.qml")));
            if (updateProject)
            {
                // Project was updated, so clear out unreferenced cached Qml files.
                Schema::garbageCollectCache();

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
            auto updateProject = std::unique_ptr<Project>(qobject_cast<Project*>(Utils::loadQmlFromFile(App::instance()->qmlEngine(), updateFolder + "/Project.qml")));
            auto offlineMapUrl = updateProject ? updateProject->offlineMapUrl() : project->offlineMapUrl();
            if (!offlineMapUrl.isEmpty())
            {
                mapUpdateResult = App::instance()->offlineMapManager()->installOrUpdatePackage(offlineMapUrl, projectUid);
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

                // Discard SMART zips, since they contain keys.
                QFile::remove(projectFolderOld + "/SMART.zip");

                // Update the form last modified date so we can detect changes made by the user to the project folder.
                // The user can then edit the form in the project directory and changes will be detected.
                project->set_formLastModified(QFileInfo(connector->getFormFilePath(project.get())).lastModified().toMSecsSinceEpoch());

                // Clear the hash cache.
                project->set_hashCache(QVariantMap());

                // Migrate the project: one time operation which gives the connector a chance
                // to finalize the project.
                connector->migrateProject(project.get());
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

        if (bootstrap)
        {
            emit projectInstalled(projectUid, updateResult.toMap());
        }
        else
        {
            emit projectUpdated(projectUid, updateResult.toMap());
        }
    }

    if (okayCount > 0)
    {
        emit projectsChanged();
    }

    return result;
}

void ProjectManager::modify(const QString& projectUid, const std::function<void(Project*)>& callback)
{
    auto project = loadPtr(projectUid);
    callback(project.get());
    project->saveToQmlFile(getFilePath(projectUid, "Project.qml"));
}

void ProjectManager::reset(const QString& projectUid, bool keepData)
{
    auto hasUpdateLock = HasUpdate::Lock();
    auto uploadQueueLock = UploadQueue::Lock();

    // Connector clean up.
    auto project = loadPtr(projectUid);
    auto connector = App::instance()->createConnectorPtr(project->connector());
    connector->reset(project.get());

    // Cleanup all project state, but keep the values minus provider state.
    auto projectValues = QVariantMap();
    m_database->loadProject(projectUid, &projectValues);
    projectValues.remove("providerState");

    if (keepData)
    {
        // Only clear the form state.
        m_database->deleteFormState(projectUid, "*");
        connector->deleteState(project.get());
    }
    else
    {
        // Remove and re-add the project: this clears all state.
        m_database->removeProject(projectUid);
        m_database->addProject(projectUid);
        m_database->saveProject(projectUid, projectValues);
        connector->deleteState(project.get());
        connector->deleteData(project.get());
        QFile::remove(App::instance()->tracksPath() + "/" + projectUid + ".db");
        project->set_locationStreamingEnabled(false);
    }

    // Cleanup references.
    App::instance()->garbageCollectMedia();
    App::instance()->garbageCollectArchive();
    App::instance()->garbageCollectStreamers();

    // Cleanup project global state.
    auto settings = App::instance()->settings();
    if (settings->kioskModeProjectUid() == projectUid)
    {
        settings->set_kioskModeProjectUid("");
    }

    if (settings->patrolProjectUid() == projectUid)
    {
        settings->set_patrolProjectUid("");
    }

    // Remove working track file.
    QFile::remove(App::instance()->tracksPath() + "/" + projectUid + ".db");

    // In all cases, the streamers are removed.
    project->set_hasStreamers(false);
    emit projectSettingChanged(projectUid, "hasStreamers", false);
}

void ProjectManager::resetArchive()
{
    m_database->removeProject(PROJECT_ARCHIVE);
    App::instance()->garbageCollectMedia();
    App::instance()->garbageCollectArchive();

    emit archiveReset();
}

void ProjectManager::remove(const QString& projectUid)
{
    auto hasUpdateLock = HasUpdate::Lock();
    auto uploadQueueLock = UploadQueue::Lock();

    // Connector clean up.
    auto project = loadPtr(projectUid);
    if (project)
    {
        App::instance()->createConnectorPtr(project->connector())->remove(project.get());
        emit projectDeleted(project->title());
    }

    // Remove the project.
    auto projectDir = getFilePath(projectUid);
    qFatalIf(!QDir(projectDir).removeRecursively(), "Failed to delete project folder");
    m_database->removeProject(projectUid);

    // Remove settings related to this project.
    auto settings = App::instance()->settings();
    if (settings->autoLaunchProjectUid() == projectUid)
    {
        settings->set_autoLaunchProjectUid("");
    }

    if (settings->kioskModeProjectUid() == projectUid)
    {
        settings->set_kioskModeProjectUid("");
    }

    if (settings->patrolProjectUid() == projectUid)
    {
        settings->set_patrolProjectUid("");
    }

    // Remove working track file.
    QFile::remove(App::instance()->tracksPath() + "/" + projectUid + ".db");

    // Cleanup tokens.
    App::instance()->vault()->setAccessToken(projectUid, "");
    App::instance()->vault()->setRefreshToken(projectUid, "");

    // Cleanup offline maps.
    App::instance()->offlineMapManager()->deletePackagesByProjectUid(projectUid);

    // Cleanup references.
    App::instance()->garbageCollectMedia();
    App::instance()->garbageCollectArchive();
    App::instance()->garbageCollectStreamers();

    emit projectsChanged();
}

} // namespace CyberTracker
