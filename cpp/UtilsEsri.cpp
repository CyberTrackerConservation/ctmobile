#include "pch.h"
#include "App.h"

namespace Utils
{

ApiResult esriAcquireToken(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& password, QString* outToken)
{
    outToken->clear();

    auto server = "https://www.arcgis.com/sharing/rest/generateToken";

    auto query = QUrlQuery();
    query.addQueryItem("f", "json");
    query.addQueryItem("clientid", App::instance()->config()["esriClientId"].toString());
    query.addQueryItem("request", "gettoken");
    query.addQueryItem("username", username);
    query.addQueryItem("password", password);
    query.addQueryItem("expiration", QString::number(15 * 24 * 60));

    auto response = Utils::httpPostQuery(networkAccessManager, server, query);
    if (!response.success)
    {
        return ApiResult::Error(response.errorString);
    }

    *outToken = QJsonDocument::fromJson(response.data).object().value("token").toString();

    return ApiResult::make(!outToken->isEmpty(), "Unknown error");
}

ApiResult esriAcquireOAuthToken(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& password, QString* outAccessToken, QString* outRefreshToken)
{
    *outAccessToken = "";
    *outRefreshToken = "";

    auto clientId = App::instance()->config()["esriClientId"].toString();
    auto clientSecret = App::instance()->config()["esriClientSecret"].toString();
    auto redirectUri = "http://localhost/cybertracker";
    auto authCode = QString();

    // Get the authorization code.
    auto response = Utils::httpGet(networkAccessManager, "https://www.arcgis.com/sharing/rest/oauth2/authorize?f=json&response_type=code&client_id=" + clientId + "&redirect_uri=" + redirectUri);
    if (response.success)
    {
        auto oauthState = Utils::extractText(QString(response.data), "\"oauth_state\":\"", "\",\"client_id\"");

        auto query = QUrlQuery();
        query.addQueryItem("user_orgkey", "");
        query.addQueryItem("oauth_state", oauthState);
        query.addQueryItem("username", username);
        query.addQueryItem("password", password);

        response = Utils::httpPostQuery(networkAccessManager, "https://www.arcgis.com/sharing/oauth2/signin", query);
        if (response.status == 302)
        {
            authCode = extractText(response.location, "code=", "");
        }
    }

    if (authCode.isEmpty())
    {
        return ApiResult::Error("Authorization failed");
    }

    // Get the access and refresh tokens.
    auto query = QUrlQuery();
    query.addQueryItem("code", authCode);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("client_secret", clientSecret);
    query.addQueryItem("redirect_uri", redirectUri);
    query.addQueryItem("expiration", "-1");

    response = Utils::httpPostQuery(networkAccessManager, "https://www.arcgis.com/sharing/rest/oauth2/token", query);
    if (response.success)
    {
        auto jsonObj = QJsonDocument::fromJson(response.data).object().toVariantMap();
        *outAccessToken = jsonObj.value("access_token").toString();
        *outRefreshToken = jsonObj.value("refresh_token").toString();
    }

    return ApiResult::make(!outAccessToken->isEmpty() && !outRefreshToken->isEmpty(), "Unknown error");
}

ApiResult esriRefreshOAuthToken(QNetworkAccessManager* networkAccessManager, const QString& refreshToken, QString* outAccessToken, QString* outRefreshToken)
{
    auto clientId = App::instance()->config()["esriClientId"].toString();

    return ApiResult::make(Utils::httpRefreshOAuthToken(networkAccessManager, "https://www.arcgis.com/sharing/rest", clientId, refreshToken, outAccessToken, outRefreshToken), "Unknown error");
}

ApiResult esriFetchContent(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& folderId, const QString& token, QVariantMap* outContent)
{
    outContent->clear();

    auto server = "https://www.arcgis.com/sharing/rest/content/users/" + username + "/" + folderId;

    auto query = QUrlQuery();
    query.addQueryItem("f", "json");
    query.addQueryItem("start", "1");
    query.addQueryItem("num", "100");
    query.addQueryItem("token", token);

    auto response = Utils::httpPostQuery(networkAccessManager, server, query);
    if (!response.success)
    {
        return ApiResult::Error(response.errorString);
    }

    *outContent = QJsonDocument::fromJson(response.data).object().toVariantMap();

    return ApiResult::make(!outContent->isEmpty(), "Unknown error");
}

ApiResult esriFetchPortal(QNetworkAccessManager* networkAccessManager, const QString& token, QVariantMap* outPortal)
{
    auto server = "https://www.arcgis.com/sharing/rest/portals/self";

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("token", token);

    auto response = Utils::httpPostQuery(networkAccessManager, server, query);
    if (!response.success)
    {
        return ApiResult::Error(response.errorString);
    }

    *outPortal = QJsonDocument::fromJson(response.data).object().toVariantMap();

    return ApiResult::make(!outPortal->isEmpty(), "Unknown error");
}

ApiResult esriFetchSurveys(QNetworkAccessManager* networkAccessManager, const QString& token, QVariantMap* outContent)
{
    outContent->clear();

    auto result = esriFetchPortal(networkAccessManager, token, outContent);
    if (!result.success)
    {
        return result;
    }

    auto user = outContent->value("user").toMap();
    if (user.isEmpty())
    {
        return ApiResult::Error("User not found");
    }

    auto orgId = user.value("orgId").toString();
    if (orgId.isEmpty())
    {
        return ApiResult::Error("OrgId not found");
    }

    auto server = "https://www.arcgis.com/sharing/rest/search";

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("num", "100");
    query.addQueryItem("q", "((NOT access:public) OR orgid:" + orgId + ") AND ((type:Form AND NOT tags:\"draft\" AND NOT typekeywords:draft) OR (type:\"Code Sample\" AND typekeywords:XForms AND tags:\"xform\"))");
    query.addQueryItem("sortField", "modified");
    query.addQueryItem("sortOrder", "desc");
    query.addQueryItem("start", "1");
    query.addQueryItem("token", token);

    auto response = Utils::httpPostQuery(networkAccessManager, server, query);
    if (!response.success)
    {
        return ApiResult::Error(response.errorString);
    }

    *outContent = QJsonDocument::fromJson(response.data).object().toVariantMap();

    return ApiResult::make(!outContent->isEmpty(), "Unknown error");
}

ApiResult esriFetchSurvey(QNetworkAccessManager* networkAccessManager, const QString& surveyId, const QString& token, QVariantMap* outSurvey)
{
    outSurvey->clear();

    auto server = "https://www.arcgis.com/sharing/rest/search";

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("num", "100");
    query.addQueryItem("q", "id:" + surveyId);
    query.addQueryItem("sortField", "modified");
    query.addQueryItem("sortOrder", "desc");
    query.addQueryItem("start", "1");
    query.addQueryItem("token", token);

    auto response = Utils::httpPostQuery(networkAccessManager, server, query);
    if (!response.success)
    {
        return ApiResult::Error(response.errorString);
    }

    auto results = QJsonDocument::fromJson(response.data).object().toVariantMap().value("results").toList();
    if (results.count() != 1)
    {
        return ApiResult::Error("Survey not found");
    }

    *outSurvey = results[0].toMap();
    if (outSurvey->value("id") != surveyId)
    {
        return ApiResult::Error("Survey mismatch");
    }

    return ApiResult::Success();
}

ApiResult esriFetchSurveyServiceUrl(QNetworkAccessManager* networkAccessManager, const QString& surveyId, const QString& token, QString* outServiceUrl)
{
    outServiceUrl->clear();

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("token", token);
    query.addQueryItem("relationshipType", "Survey2Service");

    auto response = Utils::httpPostQuery(networkAccessManager, "https://www.arcgis.com/sharing/rest/content/items/" + surveyId + "/relatedItems", query);
    if (!response.success)
    {
        return ApiResult::Error(response.errorString);
    }

    auto responseMap = QJsonDocument::fromJson(response.data).object().toVariantMap();
    auto relatedItems = responseMap["relatedItems"].toList();
    auto serviceUrl = QString();
    for (auto relatedItem: relatedItems)
    {
        auto relatedItemMap = relatedItem.toMap();
        if (relatedItemMap["type"] == "Feature Service")
        {
            *outServiceUrl = relatedItemMap["url"].toString();
            return ApiResult::Success();
        }
    }

    return ApiResult::Error("Service not found");
}

ApiResult esriFetchServiceMetadata(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& accessToken, QVariantMap* outServiceInfo)
{
    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("token", accessToken);

    auto postResult = Utils::httpPostQuery(networkAccessManager, serviceUrl, query);
    if (postResult.status != 200)
    {
        return ApiResult::Error(postResult.errorString);
    }

    auto serviceInfo = QJsonDocument::fromJson(postResult.data).object().toVariantMap();
    if (serviceInfo.isEmpty())
    {
        return ApiResult::Error("Service info not found");
    }

    auto tables = serviceInfo["tables"].toList();
    for (auto i = 0; i < tables.count(); i++)
    {
        auto table = tables[i].toMap();
        auto postResult = Utils::httpPostQuery(networkAccessManager, serviceUrl + "/" + table["id"].toString(), query);
        if (postResult.status != 200)
        {
            return ApiResult::Error(postResult.errorString);
        }

        table["serviceInfo"] = QJsonDocument::fromJson(postResult.data).object().toVariantMap();
        tables[i] = table;
    }
    serviceInfo["tables"] = tables;

    auto layers = serviceInfo["layers"].toList();
    for (auto i = 0; i < layers.count(); i++)
    {
        auto layer = layers[i].toMap();
        auto postResult = Utils::httpPostQuery(networkAccessManager, serviceUrl + "/" + layer["id"].toString(), query);
        if (postResult.status != 200)
        {
            return ApiResult::Error(postResult.errorString);
        }

        layer["serviceInfo"] = QJsonDocument::fromJson(postResult.data).object().toVariantMap();
        layers[i] = layer;
    }
    serviceInfo["layers"] = layers;

    *outServiceInfo = serviceInfo;

    return ApiResult::Success();
}

}
