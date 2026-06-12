#include "EsriConnector.h"
#include "EsriProvider.h"
#include "App.h"
#include "XlsForm.h"

namespace CyberTracker {

EsriConnector::EsriConnector(QObject *parent) : Connector(parent)
{
    update_name(CONNECTOR_NAME);
    update_icon("qrc:/Esri/logo.svg");
}

EsriConnector::~EsriConnector()
{
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
        { "connector", CONNECTOR_NAME },
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
        result.insert("accessToken", "?");
        result.insert("refreshToken", project->refreshToken());
    }

    if (project->defaultLocationServiceUrl().isEmpty() && !project->locationServiceUrl().isEmpty())
    {
        result.insert("serviceUrl", project->locationServiceUrl());
        result.insert("pointInterval", project->locationPointInterval());
        result.insert("trackInterval", project->locationTrackInterval());
        result.insert("trackDistance", project->locationTrackDistance());
    }

    return result;
}

bool EsriConnector::canLogin(Project* /*project*/) const
{
    return true;
}

bool EsriConnector::loggedIn(Project* project) const
{
    return !project->refreshToken().isEmpty();
}

bool EsriConnector::login(Project* project, const QString& /*server*/, const QString& username, const QString& password) const
{
    auto accessToken = QString();
    auto refreshToken = QString();

    auto result = Utils::esriAcquireOAuthToken(m_networkAccessManager, username, password, &accessToken, &refreshToken);
    if (!result.success)
    {
        qDebug() << "Login failed: " << result.errorString;
        return false;
    }

    project->set_accessToken(accessToken);
    project->set_refreshToken(refreshToken);
    project->set_username(username);

    return true;
}

void EsriConnector::logout(Project* project) const
{
    project->set_accessToken("");
    project->set_refreshToken("");
}

QString EsriConnector::getFormFilePath(Project* project) const
{
    return m_projectManager->getFilePath(project->uid(), project->getConnectorParam("formName").toString());
}

ApiResult EsriConnector::bootstrap(const QVariantMap& params) const
{
    auto surveyId = params["surveyId"].toString();
    auto projectUid = "ESRI_" + surveyId;
    auto username = params["username"].toString();
    auto password = params["password"].toString();
    auto accessToken = params.value("accessToken").toString();
    auto refreshToken = params.value("refreshToken").toString();
    auto serviceUrl = params.value("serviceUrl").toString();

    if (m_projectManager->exists(projectUid))
    {
        return AlreadyConnected();
    }

    if (!m_projectManager->init(projectUid, EsriProvider::PROVIDER_NAME, QVariantMap(), CONNECTOR_NAME, QVariantMap {{ "surveyId", surveyId }}))
    {
        return Failure(QString(tr("Failed to create %1")).arg(App::instance()->alias_project()));
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

        if (!serviceUrl.isEmpty())
        {
            project->set_locationServiceUrl(serviceUrl);
            project->set_locationPointInterval(params.value("pointInterval").toInt());
            project->set_locationTrackInterval(params.value("trackInterval").toInt());
            project->set_locationTrackDistance(params.value("trackDistance").toInt());
        }
    });

    return bootstrapUpdate(projectUid);
}

ApiResult EsriConnector::refreshAccessToken(Project* project, QString* oldRefreshTokenOut, QString* newAccessTokenOut, QString* newRefreshTokenOut)
{
    auto oldRefreshToken = project->refreshToken();
    if (oldRefreshToken.isEmpty())
    {
        return Unauthorized();
    }

    auto clientId = App::instance()->config()["esriClientId"].toString();
    auto serverUrl = QString("https://www.arcgis.com/sharing/rest/oauth2/token");

    auto query = QUrlQuery();
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", oldRefreshToken);
    query.addQueryItem("client_id", clientId);
    auto payload = query.toString(QUrl::FullyEncoded).toUtf8();

    auto request = Utils::httpCreateRequest(serverUrl);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");
    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->post(request, payload));

    Utils::httpWait(reply.get());

    auto response = Utils::esriDecodeResponse(Utils::HttpResponse::fromReply(serverUrl, reply.get()));
    if (!response.success)
    {
        *oldRefreshTokenOut = oldRefreshToken;
        return Failure(response);
    }

    auto jsonObj = QJsonDocument::fromJson(response.data).object();
    auto accessToken = jsonObj["access_token"].toString();
    auto refreshToken = jsonObj["refresh_token"].toString();

    if (!accessToken.isEmpty() && refreshToken.isEmpty())
    {
        refreshToken = oldRefreshToken;
    }

    if (accessToken.isEmpty() || refreshToken.isEmpty())
    {
        *oldRefreshTokenOut = oldRefreshToken;
        return Unauthorized();
    }

    *oldRefreshTokenOut = oldRefreshToken;
    *newAccessTokenOut = accessToken;
    *newRefreshTokenOut = refreshToken;

    return Success();
}

ApiResult EsriConnector::hasUpdate(Project* project)
{
    auto accessToken = project->accessToken();
    if (accessToken.isEmpty())
    {
        return Unauthorized();
    }

    auto surveyId = project->getConnectorParam("surveyId").toString();
    auto currentVersion = project->getConnectorParam("modified").toLongLong();
    auto url = QString("https://www.arcgis.com/sharing/rest/search");

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("num", "100");
    query.addQueryItem("q", "id:" + surveyId);
    query.addQueryItem("sortField", "modified");
    query.addQueryItem("sortOrder", "desc");
    query.addQueryItem("start", "1");
    query.addQueryItem("token", accessToken);

    auto request = Utils::httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");
    auto reply = std::unique_ptr<QNetworkReply>(m_networkAccessManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8()));

    Utils::httpWait(reply.get());

    auto response = Utils::esriDecodeResponse(Utils::HttpResponse::fromReply(url, reply.get()));
    if (!response.success)
    {
        return Failure(response);
    }

    auto results = QJsonDocument::fromJson(response.data).object().toVariantMap().value("results").toList();
    if (results.count() != 1)
    {
        return Failure(tr("Survey not found"));
    }

    auto surveyMap = results.constFirst().toMap();
    auto deployedVersion = surveyMap.value("modified").toLongLong();
    if (deployedVersion == currentVersion)
    {
        return AlreadyUpToDate();
    }

    return UpdateAvailable();
}

ApiResult EsriConnector::uploadData(Project* project, const Database::DbQueueItem& queueItem, QVariantMap* state, const CommitState& commitState)
{
    // LAST_LOCATION.
    if (queueItem.type == UPLOAD_LAST_LOCATION)
    {
        auto results = QVariantList();
        auto feature = Utils::variantMapFromJson(queueItem.payload);

        Utils::esriApplyEdits(m_networkAccessManager, queueItem.url, "updates", 1, feature, project->accessToken(), &results);

        // Always succeed for pings.
        return Success();
    }

    // TRACK.
    if (queueItem.type == UPLOAD_TRACK)
    {
        // Locations.
        {
            auto filename = queueItem.attachments[0];
            auto filePath = App::instance()->getMediaFilePath(filename);
            auto featureList = Utils::variantListFromJsonFile(filePath);
            auto results = QVariantList();
            for (auto i = 0; i < featureList.length(); i++)
            {
                auto batchId = QString("batch_%1").arg(i);
                if (state->contains(batchId))
                {
                    continue;
                }

                auto features = featureList[i].toList();
                auto response = Utils::esriApplyEdits(m_networkAccessManager, queueItem.url, "adds", 0, features, project->accessToken(), &results);
                if (!response.success)
                {
                    return ResultFromResponse(response);
                }

                state->insert(batchId, results);
                commitState();
            }
        }

        // Lines.
        if (!state->contains("lines"))
        {
            auto filename = queueItem.attachments[1];
            auto filePath = App::instance()->getMediaFilePath(filename);
            auto features = Utils::variantListFromJsonFile(filePath);
            auto results = QVariantList();
            auto response = Utils::esriApplyEdits(m_networkAccessManager, queueItem.url, "adds", 2, features, project->accessToken(), &results);
            if (!response.success)
            {
                return ResultFromResponse(response);
            }

            state->insert("lines", results);
            commitState();
        }

        return ApiResult::Success();
    }

    // DATA.
    if (queueItem.type == UPLOAD_DATA)
    {
        auto payload = Utils::variantMapFromJson(queueItem.payload);

        // Sighting if not already sent.
        if (!state->contains("results"))
        {
            auto features = QJsonDocument(QJsonArray::fromVariantList(payload["features"].toList())).toJson(QJsonDocument::Compact);
            auto serviceUrl = queueItem.url + "/applyEdits";

            auto query = QUrlQuery();
            query.addQueryItem("f", "json");
            query.addQueryItem("edits", features);
            query.addQueryItem("useGlobalIds", "true");
            query.addQueryItem("rollbackOnFailure", "true");
            query.addQueryItem("token", project->accessToken());

            auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(m_networkAccessManager, serviceUrl, query));
            if (!response.success)
            {
                return ResultFromResponse(response);
            }

            auto results = QJsonDocument::fromJson(response.data).array().toVariantList();
            state->insert("results", results);
            commitState();
        }

        // Attachments.
        auto attachments = payload["attachments"].toMap();
        if (!attachments.isEmpty())
        {
            auto lookupObjectId = [&](const QString& recordUid, int* serviceIdOut, qint64* objectIdOut) -> bool
            {
                auto globalId = Utils::addBracesToUuid(recordUid);
                auto resultsList = state->value("results").toList();

                for (auto resultsMapIt = resultsList.constBegin(); resultsMapIt != resultsList.constEnd(); resultsMapIt++)
                {
                    auto resultsMap = resultsMapIt->toMap();
                    auto addResults = resultsMap["addResults"].toList();
                    for (auto resultMapIt = addResults.constBegin(); resultMapIt != addResults.constEnd(); resultMapIt++)
                    {
                        auto resultMap = resultMapIt->toMap();
                        if (resultMap["globalId"] != globalId)
                        {
                            continue;
                        }

                        *serviceIdOut = resultsMap["id"].toInt();
                        *objectIdOut = resultMap["objectId"].toLongLong();
                        return true;
                    }
                }

                return false;
            };

            for (auto attachmentIt = attachments.constKeyValueBegin(); attachmentIt != attachments.constKeyValueEnd(); attachmentIt++)
            {
                auto recordUid = attachmentIt->first;
                auto filenames = attachmentIt->second.toStringList();

                for (auto filenameIt = filenames.constBegin(); filenameIt != filenames.constEnd(); filenameIt++)
                {
                    auto filename = *filenameIt;

                    // Skip if already sent.
                    if (state->contains(filename))
                    {
                        continue;
                    }

                    auto serviceId = (int)-1;
                    auto objectId = (qint64)0;

                    if (!lookupObjectId(recordUid, &serviceId, &objectId))
                    {
                        qDebug() << "Error: lost attachment - no object found";
                        continue; // Skip this attachment.
                    }

                    auto filePath = App::instance()->getMediaFilePath(filename);
                    auto file = QFile(filePath);
                    qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to open attachment");

                    auto serviceUrl = queueItem.url + '/' + QString::number(serviceId) + '/' + QString::number(objectId) + "/addAttachment";

                    auto multipart = QHttpMultiPart(QHttpMultiPart::FormDataType);

                    auto part1 = QHttpPart();
                    part1.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"f\"");
                    part1.setBody("json");
                    multipart.append(part1);

                    auto part2 = QHttpPart();
                    part2.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"token\"");
                    part2.setBody(project->accessToken().toUtf8());
                    multipart.append(part2);

                    auto part3 = QHttpPart();
                    part3.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"attachment\"");
                    part3.setBody(filename.toUtf8());
                    multipart.append(part3);

                    auto part4 = QHttpPart();
                    part4.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; token=\"" + project->accessToken() + "\"; name=\"attachment\"; filename=\"" + filename + "\"");
                    part4.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, Utils::detectMimeType(filePath));
                    part4.setBodyDevice(&file);
                    multipart.append(part4);

                    auto response = Utils::esriDecodeResponse(Utils::httpPostMultiPart(m_networkAccessManager, serviceUrl, &multipart));
                    if (!response.success)
                    {
                        return ResultFromResponse(response);
                    }

                    state->insert(filename, QJsonDocument::fromJson(response.data).object().toVariantMap().value("addAttachmentResult").toMap());
                    commitState();
                }
            }
        }
    }

    return Success();
}

bool EsriConnector::canUpdate(Project* project) const
{
    return loggedIn(project);
}

ApiResult EsriConnector::update(Project* project) const
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
//    auto sightingUids = QStringList();
//    m_database->getData(project->uid(), "", Database::DataFlag::DB_SIGHTING_FLAG, &sightingUids);
//    if (sightingUids.count() > 0)
//    {
//        return Failure(tr("Unsent data"));
//    }

    // Update.

    // Refresh the access token.
    // BUGBUG: expireToken
    // if (!refreshAccessToken(networkAccessManager, project))
    // {
    //     return Failure(tr("Login required"));
    // }

    // Fetch the survey metadata.
    auto accessToken = project->accessToken();
    auto connectorParams = project->connectorParams();
    auto surveyId = connectorParams["surveyId"].toString();
    auto surveyMap = QVariantMap();

    auto surveyResult = Utils::esriFetchSurvey(m_networkAccessManager, surveyId, accessToken, &surveyMap);
    if (!surveyResult.success)
    {
        return Failure(tr("Could not retrieve survey"));
    }

    // Check if anything has changed.
    if (surveyMap.value("modified").toLongLong() == connectorParams["modified"].toLongLong())
    {
        return AlreadyUpToDate();
    }

    // Lookup the service url.
    auto serviceUrl = QString();
    auto serviceResult = Utils::esriFetchSurveyServiceUrl(m_networkAccessManager, surveyId, accessToken, &serviceUrl);
    if (!serviceResult.success)
    {
        return Failure(serviceResult.errorString);
    }

    auto updateFolder = m_projectManager->getUpdateFolder(projectUid);

    // Pick up metadata about the service.
    auto serviceInfo = QVariantMap();
    auto metadataResult = Utils::esriFetchServiceMetadata(m_networkAccessManager, serviceUrl, accessToken, &serviceInfo);
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

    auto extractedFiles = Utils::zipExtractToDir(formZipFilePath, updateFolder);
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

    auto formName = "esriinfo/" + formInfo["name"].toString();
    if (QFile::exists(updateFolder + "/" + formName + ".xlsx"))
    {
        formName += ".xlsx";
    }
    else if (QFile::exists(updateFolder + "/" + formName + ".xls"))
    {
        formName += ".xls";
    }
    else
    {
        return Failure(tr("Form not found"));
    }

    // Update.
    auto iconPath = "esriInfo/media/logo.svg";
    if (QFile::exists(updateFolder + "/" + iconPath))
    {
        thumbnail = iconPath;
    }

    // Finalize connection parameters.
    project->set_name(surveyMap["title"].toString());
    project->set_subtitle(App::instance()->formatDateTime(surveyMap["modified"].toLongLong()));
    project->set_icon(thumbnail.isEmpty() ? "qrc:/Esri/Survey123.svg" : thumbnail);
    project->setConnectorParam("modified", surveyMap.value("modified").toLongLong());
    project->setConnectorParam("serviceUrl", serviceUrl);
    project->setConnectorParam("formName", formName);

    // Reset the project.
    m_projectManager->reset(projectUid, true);

    // Prepare the project.
    auto result = buildSchema(project, updateFolder);

    // Finalize location tracking.
    if (result.success)
    {
        auto locationServiceUrl = project->locationServiceUrl();
        if (!locationServiceUrl.isEmpty())
        {
            auto esriLocationServiceState = QVariantMap();
            auto response = Utils::esriInitLocationTracking(m_networkAccessManager, locationServiceUrl, accessToken, &esriLocationServiceState);
            if (!response.success)
            {
                return Failure(response);
            }

            project->set_esriLocationServiceState(esriLocationServiceState);
            project->set_requireBackgroundLocation(true);
        }
        else
        {
            project->set_locationServiceUrl("");
            project->set_esriLocationServiceState(QVariantMap());
        }
    }

    return result;
}

void setFieldTags(const QString& serviceInfoFilePath, Schema* schema)
{
    auto serviceInfo = Utils::variantMapFromJsonFile(serviceInfoFilePath);

    auto processFields = [&](const QVariantMap& serviceInfo)
    {
        auto serviceId = serviceInfo["id"].toInt();

        auto fields = serviceInfo["fields"].toList();
        auto fieldMap = QMap<QString, int>();
        for (auto i = 0; i < fields.count(); i++)
        {
            fieldMap.insert(fields[i].toMap().value("name").toString(), i);
        }

        auto recordField = qobject_cast<RecordField*>(serviceId == 0 ? schema->rootField() : schema->getField(serviceInfo["name"].toString()));
        if (!recordField)
        {
            qDebug() << "EsriProvider - skipping table: " << serviceInfo["name"];
            return;
        }

        recordField->set_tag(QVariantMap {{ "serviceId", serviceId }, { "geometryType", serviceInfo.value("geometryType", "") } });
        auto recordFields = QList<RecordField*>{ recordField };

        while (!recordFields.isEmpty())
        {
            recordFields.first()->enumFields([&](BaseField* field)
            {
                // Handle child records.
                auto childRecordField = qobject_cast<RecordField*>(field);
                if (childRecordField)
                {
                    if (childRecordField->group())
                    {
                        recordFields.append(childRecordField);
                    }

                    return;
                }

                auto fieldIndex = fieldMap.value(Utils::lastPathComponent(field->uid()), -1);
                if (fieldIndex == -1)
                {
                    return;
                }

                auto fieldMeta = fields[fieldIndex].toMap();

                // Skip parentglobalid.
                if (fieldMeta["name"] == "parentglobalid")
                {
                    return;
                }

                // Skip read-only fields.
                if (!fieldMeta["editable"].toBool())
                {
                    return;
                }
                fieldMeta.remove("editable");

                // Remove domain tag, because it repeats list elements and makes the qml needlessly large.
                fieldMeta.remove("domain");

                field->set_tag(fieldMeta);
            });

            recordFields.removeFirst();
        }
    };

    auto tables = serviceInfo["tables"].toList();
    for (auto i = 0; i < tables.count(); i++)
    {
        processFields(tables[i].toMap()["serviceInfo"].toMap());
    }

    auto layers = serviceInfo["layers"].toList();
    for (auto i = 0; i < layers.count(); i++)
    {
        processFields(layers[i].toMap()["serviceInfo"].toMap());
    }
}

ApiResult EsriConnector::buildSchema(Project* project, const QString& path) const
{
    // Parse the form.
    auto schema = Schema();
    auto errorString = QString();

    auto formFilePath = path + "/" + project->getConnectorParam("formName").toString();

    // Legacy from when we copied the form to the root.
    if (!QFile::exists(formFilePath))
    {
        formFilePath = path + "/form.xlsx";
    }

    if (!XlsFormParser::parse(formFilePath, &schema, &errorString))
    {
        return Failure(tr("Bad form"));
    }

    setFieldTags(path + "/serviceInfo.json", &schema);

    XlsFormParser::configureProject(project, &schema);

    project->set_version(1);
    project->set_provider(EsriProvider::PROVIDER_NAME);
    project->set_defaultWizardMode(true);

    // Commit the project and schema file.
    project->saveToQmlFile(path + "/Project.qml");
    schema.setProjectInfo(project->title(), project->subtitle(), project->icon());
    schema.saveToQmlFile(path + "/Form.qml");

    return Success();
}

void EsriConnector::reset(Project* project) const
{
    QFile::remove(m_projectManager->getFilePath(project->uid(), "Form.qml"));
}

} // namespace CyberTracker