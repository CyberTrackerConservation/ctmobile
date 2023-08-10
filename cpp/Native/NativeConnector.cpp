#include "NativeConnector.h"
#include "NativeProvider.h"
#include "App.h"
#include <jlcompress.h>

NativeConnector::NativeConnector(QObject *parent) : Connector(parent)
{
    update_name(NATIVE_CONNECTOR);
}

bool NativeConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool NativeConnector::canShareAuth(Project* /*project*/) const
{
    return false;
}

QVariantMap NativeConnector::getShareData(Project* project, bool /*auth*/) const
{
    if (project->webUpdateUrl().isEmpty())
    {
        return QVariantMap();
    }

    return QVariantMap
    {
        { "connector", NATIVE_CONNECTOR },
        { "webUpdateUrl", project->webUpdateUrl() },
        { "projectUid", project->uid() },
        { "launch", true }
    };
}

bool NativeConnector::canLogin(Project* /*project*/) const
{
    return false;
}

ApiResult NativeConnector::bootstrap(const QVariantMap& params)
{
    // Get update url.
    auto webUpdateUrl = params.value("webUpdateUrl").toString();
    if (webUpdateUrl.isEmpty())
    {
        return Failure(tr("Install kind not found"));
    }

    // Load the metadata and check for a matching project.
    auto response = Utils::httpGet(App::instance()->networkAccessManager(), webUpdateUrl);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto metadata = Utils::variantMapFromJson(response.data);

    auto projectUid = metadata.value("projectUid").toString();
    if (projectUid.isEmpty())
    {
        return Failure(tr("Missing projectUid"));
    }

    auto projects = m_projectManager->buildList();
    for (auto it = projects.constBegin(); it != projects.constEnd(); it++)
    {
        auto project = *it;
        if (project->uid() == projectUid)
        {
            return AlreadyConnected();
        }

        if (project->webUpdateUrl().compare(webUpdateUrl, Qt::CaseInsensitive) == 0)
        {
            qDebug() << "Project uids are different, but another project matches the webUpdateUrl";
            return AlreadyConnected();
        }
    }

    // Bootstrap the project.
    m_projectManager->init(projectUid, NATIVE_PROVIDER, QVariantMap(), NATIVE_CONNECTOR, QVariantMap());
    m_projectManager->modify(projectUid, [&](Project* project)
    {
        project->set_webUpdateUrl(webUpdateUrl);
    });

    return bootstrapUpdate(projectUid);
}

ApiResult NativeConnector::hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project)
{
    auto projectUid = project->uid();
    auto webUpdateUrl = project->webUpdateUrl();

    // Get the update metadata file.
    auto response = Utils::httpGet(networkAccessManager, webUpdateUrl);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto metadata = Utils::variantMapFromJson(response.data);
    if (metadata.value("projectUid") != projectUid)
    {
        return Failure(tr("Package mismatch"));
    }

    auto currentVersion = project->webUpdateMetadata().value("projectVersion", 0).toInt();
    auto metadataVersion = metadata.value("projectVersion").toInt();

    // Check for up to date.
    if (currentVersion >= metadataVersion)
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

bool NativeConnector::canUpdate(Project* project) const
{
    return !project->webUpdateUrl().isEmpty();
}

ApiResult NativeConnector::update(Project* project)
{
    auto projectUid = project->uid();
    auto webUpdateUrl = project->webUpdateUrl();

    // Get the update metadata file.
    auto response = Utils::httpGet(App::instance()->networkAccessManager(), webUpdateUrl);
    if (!response.success)
    {
        return Failure(response.errorString);
    }

    auto metadata = Utils::variantMapFromJson(response.data);
    if (metadata.value("projectUid") != projectUid)
    {
        return Failure(tr("Package mismatch"));
    }

    auto currentVersion = project->webUpdateMetadata().value("projectVersion", 0).toInt();
    auto metadataVersion = metadata.value("projectVersion").toInt();

    // Check for up to date.
    if (currentVersion >= metadataVersion)
    {
        return AlreadyUpToDate();
    }

    // Online version is newer, so download the package.
    auto zipFilePath = App::instance()->downloadFile(metadata.value("packageUrl").toString());
    if (zipFilePath.isEmpty())
    {
        return Failure(tr("Download failed"));
    }

    // Unpack the package and sanity check it.
    auto packagePath = Utils::unpackZip(App::instance()->tempPath(), "Package", zipFilePath);
    QFile::remove(zipFilePath); // Always cleanup downloaded file.
    if (packagePath.isEmpty())
    {
        return Failure(tr("Bad package"));
    }

    auto projectJson = Utils::variantMapFromJsonFile(packagePath + "/project.json");
    if (!projectJson.contains("uid"))
    {
        return Failure(tr("Not a package"));
    }

    // Copy the project files to the update folder.
    auto projectPath = packagePath + "/project";
    if (!QDir(projectPath).exists())
    {
        return Failure(QString(tr("No %1 in package")).arg(App::instance()->alias_project()));
    }

    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);
    Utils::copyPath(projectPath, updateFolder);

    // Set up the new project.
    Project newProject;
    newProject.set_uid(projectUid);
    projectJson["uid"] = projectUid; // Ignore project uid from package.

    // Load settings for new projects (version == 0).
    newProject.load(projectJson, project->version() == 0);

    // Store the url and metadata.
    newProject.set_webUpdateUrl(webUpdateUrl);
    newProject.set_webUpdateMetadata(metadata);

    // Commit the new project file.
    newProject.saveToQmlFile(updateFolder + "/Project.qml");

    return Success();
}
