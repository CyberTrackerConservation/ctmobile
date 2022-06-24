#include "EsriConnector.h"
#include "EsriProvider.h"
#include "App.h"
#include "XlsForm.h"
#include "jlcompress.h"

EsriConnector::EsriConnector(QObject *parent) : Connector(parent)
{
    update_name(ESRI_CONNECTOR);
    update_icon("qrc:/Esri/logo.svg");
}

bool EsriConnector::canSharePackage(Project* /*project*/) const
{
    return true;
}

bool EsriConnector::canShareAuth(Project* project) const
{
    return loggedIn(project);
}

QVariantMap EsriConnector::getShareData(Project* project, bool auth) const
{
    auto result = QVariantMap
    {
        { "connector", ESRI_CONNECTOR },
        { "surveyId", project->connectorParams()["surveyId"] },
        { "launch", true }
    };

    if (auth || !loggedIn(project))
    {
        result.insert("auth", true);
    }
    else
    {
        result.insert("username", project->username());
        result.insert("accessToken", project->accessToken());
    }

    return result;
}

bool EsriConnector::loggedIn(Project* project) const
{
    return !project->accessToken().isEmpty();
}

bool EsriConnector::login(Project* project, const QString& /*server*/, const QString& username, const QString& password)
{
    auto accessToken = QString();
    auto refreshToken = QString();
    auto errorString = QString();

    auto result = Utils::esriAcquireOAuthToken(App::instance()->networkAccessManager(), username, password, &accessToken, &refreshToken);
    if (!result.success)
    {
        qDebug() << "Login failed: " << errorString;
        return false;
    }

    project->set_accessToken(accessToken);
    project->set_refreshToken(refreshToken);
    project->set_username(username);

    return true;
}

void EsriConnector::logout(Project* project)
{
    project->set_accessToken("");
    project->set_refreshToken("");
}

ApiResult EsriConnector::bootstrap(const QVariantMap& params)
{
    auto surveyId = params["surveyId"].toString();
    auto projectUid = "ESRI_" + surveyId;
    auto username = params["username"].toString();
    auto password = params["password"].toString();
    auto accessToken = params.value("accessToken").toString();
    auto refreshToken = params.value("refreshToken").toString();

    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    if (!m_projectManager->init(projectUid, ESRI_PROVIDER, QVariantMap(), ESRI_CONNECTOR, QVariantMap {{ "surveyId", surveyId }}))
    {
        return Failure(tr("Failed to create project"));
    }

    m_projectManager->modify(projectUid, [&](Project* project)
    {
        if (accessToken.isEmpty() && !username.isEmpty() && !password.isEmpty())
        {
            login(project, "", username, password);
        }
        else
        {
            project->set_username(username);
            project->set_accessToken(accessToken);
            project->set_refreshToken(refreshToken);
        }
    });

    return bootstrapUpdate(projectUid);
}

ApiResult EsriConnector::hasUpdate(QNetworkAccessManager* networkAccessManager, Project* project)
{
    // Fetch the survey metadata.
    auto connectorParams = project->connectorParams();
    auto surveyId = connectorParams["surveyId"].toString();
    auto surveyMap = QVariantMap();

    auto surveyResult = Utils::esriFetchSurvey(networkAccessManager, surveyId, project->accessToken(), &surveyMap);
    if (!surveyResult.success)
    {
        return Failure(tr("Could not retrieve survey"));
    }

    // Check modified.
    if (surveyMap.value("modified").toLongLong() == connectorParams["modified"].toLongLong())
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

bool EsriConnector::canUpdate(Project* project)
{
    return loggedIn(project);
}

ApiResult EsriConnector::update(Project *project)
{
    //
    // Check if we can update.
    //

    // Skip update if not logged in.
    auto projectUid = project->uid();
    if (!loggedIn(project))
    {
        return Failure(tr("Logged out"));
    }

    // Skip updates if there are any unsent sightings.
    auto sightingUids = QStringList();
    m_database->getSightings(project->uid(), "", Sighting::DB_SIGHTING_FLAG, &sightingUids);
    if (sightingUids.count() > 0)
    {
        return Failure(tr("Unsent data"));
    }

    auto networkAccessManager = App::instance()->networkAccessManager();
    auto accessToken = project->accessToken();

    // Fetch the survey metadata.
    auto connectorParams = project->connectorParams();
    auto surveyId = connectorParams["surveyId"].toString();
    auto surveyMap = QVariantMap();

    auto surveyResult = Utils::esriFetchSurvey(networkAccessManager, surveyId, accessToken, &surveyMap);
    if (!surveyResult.success)
    {
        return Failure(tr("Could not retrieve survey"));
    }

    // Check if anything has changed.
    if (surveyMap.value("modified").toLongLong() == connectorParams["modified"].toLongLong())
    {
        return AlreadyUpToDate();
    }

    connectorParams["modified"] = surveyMap.value("modified");

    // Lookup the service url.
    auto serviceUrl = QString();
    auto serviceResult = Utils::esriFetchSurveyServiceUrl(networkAccessManager, surveyId, accessToken, &serviceUrl);
    if (!serviceResult.success)
    {
        return Failure(serviceResult.errorString);
    }

    connectorParams["serviceUrl"] = serviceUrl;

    //
    // Update.
    //
    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);

    // Pick up metadata about the service.
    auto serviceInfo = QVariantMap();
    auto metadataResult = Utils::esriFetchServiceMetadata(networkAccessManager, serviceUrl, accessToken, &serviceInfo);
    if (!metadataResult.success)
    {
        return Failure(metadataResult.errorString);
    }

    Utils::writeJsonToFile(updateFolder + "/serviceInfo.json", (QJsonDocument(QJsonObject::fromVariantMap(serviceInfo)).toJson()));

    // Download the form.
    auto contentBaseUrl = "https://www.arcgis.com/sharing/rest/content/items/" + surveyId;

    // Download the thumbnail.
    auto thumbnail = surveyMap["thumbnail"].toString();
    if (!thumbnail.isEmpty())
    {
        auto iconTemp = App::instance()->downloadFile(contentBaseUrl + "/info/" + thumbnail + "?token=" + accessToken);
        auto iconName = thumbnail.split("/").last();
        QFile::rename(iconTemp, updateFolder + "/" + iconName);
        thumbnail = iconName;
    }

    // Download the form zip and unzip.
    auto formZipFilePath = App::instance()->downloadFile(contentBaseUrl + "/data?token=" + accessToken);
    if (formZipFilePath.isEmpty())
    {
        return Failure(tr("Form download failed"));
    }

    auto extractedFiles = JlCompress::extractDir(formZipFilePath, updateFolder);
    QFile::remove(formZipFilePath);
    if (extractedFiles.count() == 0)
    {
        return Failure(tr("Bad form zip"));
    }

    // Get the XLS form.
    auto formInfo = Utils::variantMapFromJsonFile(updateFolder + "/esriinfo/forminfo.json");
    if (formInfo.isEmpty())
    {
        return Failure(tr("Bad form"));
    }

    auto formPath = updateFolder + "/esriinfo/" + formInfo["name"].toString();
    QFile formFile(formPath + ".xlsx");
    if (!formFile.exists())
    {
        formFile.setFileName(formPath + ".xls");
        if (!formFile.exists())
        {
            return Failure(tr("Form not found"));
        }
    }

    if (!formFile.rename(formFile.fileName(), updateFolder + "/form.xlsx"))
    {
        return Failure(tr("Failed to copy form file"));
    }

    auto iconPath = "esriInfo/media/logo.svg";
    if (QFile::exists(updateFolder + "/" + iconPath))
    {
        thumbnail = iconPath;
    }

    Project updateProject;
    updateProject.set_uid(projectUid);
    updateProject.set_provider(ESRI_PROVIDER);
    updateProject.set_providerParams(project->providerParams());
    updateProject.set_connector(ESRI_CONNECTOR);
    updateProject.set_connectorParams(connectorParams);
    updateProject.set_version(1);
    updateProject.set_title(surveyMap["title"].toString());
    updateProject.set_subtitle(App::instance()->formatDateTime(surveyMap["modified"].toLongLong()));
    updateProject.set_icon(thumbnail.isEmpty() ? "qrc:/Esri/Survey123.svg" : thumbnail);
    updateProject.set_defaultWizardMode(true);

    QStringList androidPermissions = QStringList();
    androidPermissions << "CAMERA";
    androidPermissions << "RECORD_AUDIO";
    androidPermissions << "ACCESS_FINE_LOCATION";
    androidPermissions << "ACCESS_COARSE_LOCATION";
    updateProject.set_androidPermissions(androidPermissions);

    updateProject.saveToQmlFile(updateFolder + "/Project.qml");

    return Success();
}
