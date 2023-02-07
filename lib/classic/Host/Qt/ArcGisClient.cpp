#include "ArcGisClient.h"
#include "App.h"
#include "Utils.h"

ArcGisClient::ArcGisClient(QObject* parent): QObject(parent)
{
    auto app = App::instance();
    m_clientId = app->config()["esriClientId"].toString();
    m_clientSecret = app->config()["esriClientSecret"].toString();
    m_redirectUri = "http://localhost/cybertracker";
}

void ArcGisClient::reset()
{
    m_accessToken.clear();
    m_refreshToken.clear();
    m_authorizationCode.clear();
}

QString ArcGisClient::extractText(const QString& value, const QString& prefix, const QString& suffix)
{
    auto index1 = value.indexOf(prefix);
    auto index2 = suffix.length() == 0 ? value.length() : value.indexOf(suffix, index1 + prefix.length());
    return value.mid(index1 + prefix.length(), index2 - index1 - prefix.length());
}

int ArcGisClient::httpPost(const QString& url, const QUrlQuery& query, QByteArray* outResponseBody, QString* outResponseLocation)
{
    QNetworkAccessManager networkAccessManager;
    QNetworkReply* reply;
    QNetworkRequest request;
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setUrl(url);
    auto payload = query.toString(QUrl::FullyEncoded).toUtf8();
    reply = networkAccessManager.post(request, payload);

    QEventLoop eventLoop;
    int responseCode;
    QObject::connect(reply, &QNetworkReply::finished, this, [&]()
    {
        responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << reply->error();
        if (reply->error() == QNetworkReply::NoError)
        {
            if (outResponseBody)
            {
                *outResponseBody = reply->readAll();
            }

            if (outResponseLocation)
            {
                *outResponseLocation = reply->header(QNetworkRequest::LocationHeader).toString();
            }
        }

        reply->deleteLater();
        eventLoop.exit();
    });

    eventLoop.exec();

    return responseCode;
}

bool ArcGisClient::authorizeOAuth()
{
    m_authorizationCode.clear();
    m_accessToken.clear();
    m_refreshToken.clear();

    // Get the authorization code.
    QNetworkAccessManager networkAccessManager;
    auto response = Utils::httpGet(&networkAccessManager, "https://www.arcgis.com/sharing/rest/oauth2/authorize?f=json&response_type=code&client_id=" + m_clientId + "&redirect_uri=" + m_redirectUri);
    if (response.success)
    {
        auto oauthState = extractText(QString(response.data), "\"oauth_state\":\"", "\",\"client_id\"");

        auto query = QUrlQuery();
        query.addQueryItem("user_orgkey", "");
        query.addQueryItem("oauth_state", oauthState);
        query.addQueryItem("username", m_username);
        query.addQueryItem("password", m_password);

        auto responseLocation = QString();
        auto responseCode = httpPost("https://www.arcgis.com/sharing/oauth2/signin", query, nullptr, &responseLocation);
        if (responseCode == 302)
        {
            m_authorizationCode = extractText(responseLocation, "code=", "");
        }
    }

    if (m_authorizationCode.isEmpty())
    {
        qDebug() << "Authorization failed";
        return false;
    }

    // Get the access and refresh tokens.
    auto query = QUrlQuery();
    query.addQueryItem("code", m_authorizationCode);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("client_id", m_clientId);
    query.addQueryItem("client_secret", m_clientSecret);
    query.addQueryItem("redirect_uri", m_redirectUri);

    auto responseBody = QByteArray();
    auto responseCode = httpPost("https://www.arcgis.com/sharing/rest/oauth2/token/", query, &responseBody);

    if (responseCode == 200)
    {
        auto jsonObj = QJsonDocument::fromJson(responseBody).object().toVariantMap();
        m_accessToken = jsonObj.value("access_token").toString();
        m_refreshToken = jsonObj.value("refresh_token").toString();
    }
    else
    {
        m_accessToken.clear();
        m_refreshToken.clear();
    }

    return !m_accessToken.isEmpty();
}

bool ArcGisClient::authorizeToken()
{
    m_authorizationCode.clear();
    m_accessToken.clear();
    m_refreshToken.clear();

    auto server = QUrl("https://www.arcgis.com/sharing/rest/generateToken");
    auto username = m_username;
    auto password = m_password;

    // Retrieve the access token.
    QNetworkRequest request;
    request.setUrl(server);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept","application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");

    auto query = QUrlQuery();
    query.addQueryItem("f", "json");
    query.addQueryItem("clientid", m_clientId);
    query.addQueryItem("request", "gettoken");
    query.addQueryItem("username", m_username);
    query.addQueryItem("password", m_password);
    query.addQueryItem("expiration", QString::number(15 * 24 * 60));

    QNetworkAccessManager networkAccessManager;
    QNetworkReply* reply = networkAccessManager.post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    QEventLoop eventLoop;
    connect(reply, &QNetworkReply::finished, this, [&]()
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            auto data = reply->readAll();
            auto jsonDoc = QJsonDocument::fromJson(data);

            if (jsonDoc.isObject())
            {
                auto jsonObj = jsonDoc.object();
                m_accessToken = jsonObj.value("token").toString();
            }
        }
        else
        {
            qDebug() << "Network reply error 0: " << reply->errorString();
        }

        reply->deleteLater();
        eventLoop.exit();
    });

    eventLoop.exec();

    return !m_accessToken.isEmpty();
}

bool ArcGisClient::refreshAccessToken()
{
    if (!m_refreshToken.isEmpty())
    {
        m_accessToken.clear();

        auto query = QUrlQuery();
        query.addQueryItem("grant_type", "refresh_token");
        query.addQueryItem("client_id", m_clientId);
        query.addQueryItem("refresh_token", m_refreshToken);

        auto responseBody = QByteArray();
        auto responseCode = httpPost("https://www.arcgis.com/sharing/rest/oauth2/token/", query, &responseBody);

        if (responseCode == 200)
        {
            auto jsonObj = QJsonDocument::fromJson(responseBody).object().toVariantMap();
            m_accessToken = jsonObj.value("access_token").toString();
        }
        else
        {
            qDebug() << "Failed to refresh access token";
        }
    }

    return !m_accessToken.isEmpty();
}

ArcGisClient::SendDataResponse ArcGisClient::sendData(const QString& serviceUrl, int layerId, const QString& filename)
{
    QFile file(filename);

    if (file.open(QFile::ReadOnly))
    {
        return sendData(serviceUrl, layerId, file.readAll());
    }

    return SendDataResponse{};
}

ArcGisClient::SendDataResponse ArcGisClient::sendData(const QString& serviceUrl, int layerId, const QByteArray& data)
{
    auto response = SendDataResponse {};
    if (!refreshAccessToken())
    {
        return response;
    }

    auto url = serviceUrl + "/FeatureServer/" + QString::number(layerId) + "/addFeatures";

    auto query = QUrlQuery();
    query.addQueryItem("f", "json");
    query.addQueryItem("token", m_accessToken);
    query.addQueryItem("features", data);

    auto responseBody = QByteArray();
    response.errorCode = httpPost(url, query, &responseBody);

    if (response.errorCode != 200)
    {
        qDebug() << "Failed to sendData: " << response.errorMessage;
        return response;
    }

    auto resultMap = QJsonDocument::fromJson(responseBody).object().toVariantMap();
    if (resultMap.contains("error"))
    {
        qDebug() << resultMap["error"];
        return response;
    }

    if (!resultMap.contains("addResults"))
    {
        qDebug() << "No addResults";
        return response;
    }

    auto addResultList = resultMap["addResults"].toList();
    response.success = addResultList.count() > 0;
    for (auto addResult: addResultList)
    {
        auto addResultMap = addResult.toMap();

        auto sendResult = SendResult {};
        sendResult.success = addResultMap.value("success").toBool();
        sendResult.objectId = addResultMap.value("objectId").toInt();
        response.results.append(sendResult);

        response.success = response.success && sendResult.success;
    }

    return response;
}

ArcGisClient::SendResponse ArcGisClient::sendAttachment(const QString& serviceUrl, int layerId, int objectId, const QString& filename)
{
    auto response = SendResponse {};

    // Ensure file exists.
    if (!QFile::exists(filename))
    {
        qDebug() << "Error - failed to read file: " + filename;
        return response;
    }

    // Refresh the token.
    if (!refreshAccessToken())
    {
        return response;
    }

    QFile file(filename);
    QFileInfo fileInfo(filename);

    if (!file.open(QIODevice::ReadOnly))
    {
        return response;
    }

    QHttpMultiPart multipart(QHttpMultiPart::FormDataType);
    multipart.setBoundary(Utils::makeMultiPartBoundary());

    auto part1 = QHttpPart();
    part1.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"f\"");
    part1.setBody("json");
    multipart.append(part1);

    auto part2 = QHttpPart();
    part2.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"token\"");
    part2.setBody(m_accessToken.toUtf8());
    multipart.append(part2);

    auto part3 = QHttpPart();
    part3.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"attachment\"");
    part3.setBody(fileInfo.fileName().toUtf8());
    multipart.append(part3);

    QMimeDatabase mimeDatabase;
    QMimeType mimeType;
    mimeType = mimeDatabase.mimeTypeForFile(fileInfo);

    auto part4 = QHttpPart();
    part4.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; token=\"" + m_accessToken + "\"; name=\"attachment\"; filename=\"" + fileInfo.fileName() + "\"");
    part4.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, mimeType.name());
    part4.setBodyDevice(&file);
    multipart.append(part4);

    QNetworkAccessManager networkAccessManager;
    QNetworkReply* reply;
    QNetworkRequest request;
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + multipart.boundary());
    auto url = serviceUrl + "/FeatureServer/" + QString::number(layerId) + "/" + QString::number(objectId) + "/addAttachment";
    request.setUrl(url);

    reply = networkAccessManager.post(request, &multipart);

    // Submit data.
    QEventLoop eventLoop;
    connect(reply, &QNetworkReply::finished, this, [&]()
    {
        response.errorCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        response.errorMessage = reply->errorString();
        if (reply->error() == QNetworkReply::NoError)
        {
            auto responseBody = reply->readAll();
            auto jsonObj = QJsonDocument::fromJson(responseBody).object().value("addAttachmentResult").toObject();
            response.success = jsonObj.value("success").toBool();
        }

        eventLoop.exit();
        reply->deleteLater();
    });

    eventLoop.exec();

    return response;
}
