#include "pch.h"
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>
#include <zlib.h>

namespace {
QString GGFORMS_SERVER_URL = QString("https://ggformserver-tvo77m5jea-ue.a.run.app");
QString s_clientId;
QString s_clientSecret;
QString s_redirectUri;
}

namespace Utils
{

void googleSetCredentials(const QVariantMap& credentials)
{
    s_clientId = credentials["clientId"].toString();
    s_clientSecret = credentials["clientSecret"].toString();
    s_redirectUri = credentials["redirectUri"].toString();
}

HttpResponse googleAcquireOAuthTokenFromCode(QNetworkAccessManager* networkAccessManager, const QString& code, QString* accessTokenOut, QString* refreshTokenOut)
{
    qFatalIf(s_clientId.isEmpty(), "Missing clientId");
    qFatalIf(s_clientSecret.isEmpty(), "Missing clientSecret");

    auto query = QUrlQuery();
    query.addQueryItem("client_id", s_clientId);
    query.addQueryItem("client_secret", s_clientSecret);
    query.addQueryItem("code", code);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("redirect_uri", "");

    auto response = httpPostQuery(networkAccessManager, "https://oauth2.googleapis.com/token", query);
    if (!response.success)
    {
        qDebug() << "Failed to acquire token from code: " << response.status << " " << response.data;
        return response;
    }

    auto jsonObj = QJsonDocument::fromJson(response.data).object();
    *accessTokenOut = jsonObj["access_token"].toString();
    *refreshTokenOut = jsonObj["refresh_token"].toString();

    response.success = !accessTokenOut->isEmpty() && !refreshTokenOut->isEmpty();
    response.errorString = "Unknown error";

    return response;
}

HttpResponse googleRefreshOAuthToken(QNetworkAccessManager* networkAccessManager, const QString& refreshToken, QString* accessTokenOut)
{
    qFatalIf(s_clientId.isEmpty(), "Missing clientId");
    qFatalIf(s_clientSecret.isEmpty(), "Missing clientSecret");

    auto response = HttpResponse();

    if (refreshToken.isEmpty())
    {
        qDebug() << "Refresh token not found";
        return response;
    }

    auto query = QUrlQuery();
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", refreshToken);
    query.addQueryItem("client_id", s_clientId);
    query.addQueryItem("client_secret", s_clientSecret);

    response = httpPostQuery(networkAccessManager, "https://oauth2.googleapis.com/token", query);
    if (!response.success)
    {
        qDebug() << "Refresh auth token failed: " << response.status << " " << response.data;
        return response;
    }

    auto jsonObj = QJsonDocument::fromJson(response.data).object();
    *accessTokenOut = jsonObj["access_token"].toString();

    response.success = !accessTokenOut->isEmpty();
    response.errorString = "Unknown error";

    return response;
}

HttpResponse googleFetchEmail(QNetworkAccessManager* networkAccessManager, const QString& accessToken, QString* emailOut)
{
    emailOut->clear();

    auto query = QUrlQuery();
    query.addQueryItem("access_token", accessToken);
    query.addQueryItem("fields", "email");

    auto url = QUrl("https://www.googleapis.com/oauth2/v2/userinfo");
    url.setQuery(query);

    auto response = httpGet(networkAccessManager, url.toString(QUrl::FullyEncoded));
    if (!response.success)
    {
        return response;
    }

    *emailOut = QJsonDocument::fromJson(response.data).object().toVariantMap()["email"].toString();

    return HttpResponse::ErrorIf(emailOut->isEmpty(), "Unknown error");
}

HttpResponse googleFetchForm(QNetworkAccessManager* networkAccessManager, const QString& formId, QVariantMap* contentOut)
{
    contentOut->clear();

    auto query = QUrlQuery();
    query.addQueryItem("formId", formId);

    auto url = QUrl(GGFORMS_SERVER_URL + "/api/form/def");
    url.setQuery(query);

    auto response = httpGet(networkAccessManager, url.toString());
    if (!response.success)
    {
        return response;
    }

    *contentOut = QJsonDocument::fromJson(response.data).object().toVariantMap();

    return HttpResponse::ErrorIf(contentOut->isEmpty(), "Unknown error");
}

HttpResponse googleFetchFormVersion(QNetworkAccessManager* networkAccessManager, const QString& formId, QString* versionOut)
{
    versionOut->clear();

    auto query = QUrlQuery();
    query.addQueryItem("formId", formId);

    auto url = QUrl(GGFORMS_SERVER_URL + "/api/form/ver");
    url.setQuery(query);

    auto response = httpGet(networkAccessManager, url.toString());
    if (response.success)
    {
        *versionOut = response.data;
    }

    return response;
}


HttpResponse googleUploadFile(QNetworkAccessManager* networkAccessManager, const QString& folderId, const QString& filePath, QString* urlOut)
{
    urlOut->clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return HttpResponse::Error("Error - failed to read file: " + filePath);
    }

    auto filename = QFileInfo(filePath).fileName();

    QHttpMultiPart multipart(QHttpMultiPart::FormDataType);
    multipart.setBoundary(Utils::makeMultiPartBoundary());

    auto part = QHttpPart();
    part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"" + filename + "\"");
    part.setBodyDevice(&file);
    multipart.append(part);

    auto response = Utils::httpPostMultiPart(networkAccessManager, GGFORMS_SERVER_URL + "/api/form/upload?folderId=" + folderId, &multipart);
    if (response.success)
    {
        *urlOut = response.data;
    }

    return response;
}

}
