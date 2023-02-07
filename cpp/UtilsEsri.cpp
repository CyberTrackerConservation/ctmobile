#include "pch.h"
#include "App.h"

namespace Utils
{

HttpResponse esriDecodeResponse(const HttpResponse& response)
{
    auto result = response;

    if (response.success)
    {
        auto errorMap = QJsonDocument::fromJson(response.data).object().toVariantMap();
        if (errorMap.contains("error"))
        {
            errorMap = errorMap.value("error").toMap();
            result = HttpResponse { false, errorMap.value("code", -1).toInt(), errorMap.value("message").toString() };
        }
    }

    if (!result.success)
    {
        qDebug() << "ESRI error: " << result.toMap();
    }

    return result;
}

HttpResponse esriAcquireToken(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& password, QString* outToken)
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

    auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, server, query));
    if (!response.success)
    {
        return response;
    }

    *outToken = QJsonDocument::fromJson(response.data).object().value("token").toString();

    return HttpResponse::ErrorIf(outToken->isEmpty(), "Unknown error");
}

HttpResponse esriAcquireOAuthToken(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& password, QString* outAccessToken, QString* outRefreshToken)
{
    *outAccessToken = "";
    *outRefreshToken = "";

    auto clientId = App::instance()->config()["esriClientId"].toString();
    auto clientSecret = App::instance()->config()["esriClientSecret"].toString();
    auto redirectUri = "http://localhost/cybertracker";
    auto authCode = QString();

    // Get the authorization code.
    auto response = Utils::esriDecodeResponse(Utils::httpGet(networkAccessManager, "https://www.arcgis.com/sharing/rest/oauth2/authorize?f=json&response_type=code&client_id=" + clientId + "&redirect_uri=" + redirectUri));
    if (response.success)
    {
        auto oauthState = Utils::extractText(QString(response.data), "\"oauth_state\":\"", "\",\"client_id\"");

        auto query = QUrlQuery();
        query.addQueryItem("user_orgkey", "");
        query.addQueryItem("oauth_state", oauthState);
        query.addQueryItem("username", username);
        query.addQueryItem("password", password);

        response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, "https://www.arcgis.com/sharing/oauth2/signin", query));
        if (response.status == 302)
        {
            authCode = extractText(response.location, "code=", "");
        }
    }

    if (authCode.isEmpty())
    {
        return HttpResponse::Error("Authorization failed");
    }

    // Get the access and refresh tokens.
    auto query = QUrlQuery();
    query.addQueryItem("code", authCode);
    query.addQueryItem("grant_type", "authorization_code");
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("client_secret", clientSecret);
    query.addQueryItem("redirect_uri", redirectUri);
    query.addQueryItem("expiration", "-1");

    response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, "https://www.arcgis.com/sharing/rest/oauth2/token", query));
    if (response.success)
    {
        auto jsonObj = QJsonDocument::fromJson(response.data).object().toVariantMap();
        *outAccessToken = jsonObj.value("access_token").toString();
        *outRefreshToken = jsonObj.value("refresh_token").toString();
    }

    return HttpResponse::ErrorIf(outAccessToken->isEmpty() || outRefreshToken->isEmpty(), "Unknown error");
}

HttpResponse esriFetchContent(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& folderId, const QString& token, QVariantMap* outContent)
{
    outContent->clear();

    auto server = "https://www.arcgis.com/sharing/rest/content/users/" + username + "/" + folderId;

    auto query = QUrlQuery();
    query.addQueryItem("f", "json");
    query.addQueryItem("start", "1");
    query.addQueryItem("num", "100");
    query.addQueryItem("token", token);

    auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, server, query));
    if (!response.success)
    {
        return response;
    }

    *outContent = QJsonDocument::fromJson(response.data).object().toVariantMap();

    return HttpResponse::ErrorIf(outContent->isEmpty(), "Unknown error");
}

HttpResponse esriFetchPortal(QNetworkAccessManager* networkAccessManager, const QString& token, QVariantMap* outPortal)
{
    outPortal->clear();

    auto server = "https://www.arcgis.com/sharing/rest/portals/self";

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("token", token);

    auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, server, query));
    if (!response.success)
    {
        return response;
    }

    *outPortal = QJsonDocument::fromJson(response.data).object().toVariantMap();

    return HttpResponse::ErrorIf(outPortal->isEmpty(), "Unknown error");
}

HttpResponse esriFetchSurveys(QNetworkAccessManager* networkAccessManager, const QString& token, QVariantMap* outContent)
{
    outContent->clear();

    auto response = esriFetchPortal(networkAccessManager, token, outContent);
    if (!response.success)
    {
        return response;
    }

    auto user = outContent->value("user").toMap();
    if (user.isEmpty())
    {
        return HttpResponse::Error("User not found");
    }

    auto orgId = user.value("orgId").toString();
    if (orgId.isEmpty())
    {
        return HttpResponse::Error("orgId not found");
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

    response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, server, query));
    if (!response.success)
    {
        return response;
    }

    *outContent = QJsonDocument::fromJson(response.data).object().toVariantMap();

    return HttpResponse::ErrorIf(outContent->isEmpty(), "Unknown error");
}

HttpResponse esriFetchSurvey(QNetworkAccessManager* networkAccessManager, const QString& surveyId, const QString& token, QVariantMap* outSurvey)
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

    auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, server, query));
    if (!response.success)
    {
        return response;
    }

    auto results = QJsonDocument::fromJson(response.data).object().toVariantMap().value("results").toList();
    if (results.count() != 1)
    {
        return HttpResponse::Error("Survey not found");
    }

    *outSurvey = results[0].toMap();
    if (outSurvey->value("id") != surveyId)
    {
        return HttpResponse::Error("Survey mismatch");
    }

    return HttpResponse::Success();
}

HttpResponse esriFetchSurveyServiceUrl(QNetworkAccessManager* networkAccessManager, const QString& surveyId, const QString& token, QString* outServiceUrl)
{
    outServiceUrl->clear();

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("token", token);
    query.addQueryItem("relationshipType", "Survey2Service");

    auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, "https://www.arcgis.com/sharing/rest/content/items/" + surveyId + "/relatedItems", query));
    if (!response.success)
    {
        return response;
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
            return HttpResponse::ErrorIf(outServiceUrl->isEmpty(), "Unknown error");
        }
    }

    return HttpResponse::Error("Service not found");
}

HttpResponse esriFetchServiceMetadata(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& accessToken, QVariantMap* outServiceInfo)
{
    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("token", accessToken);

    auto postResult = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, serviceUrl, query));
    if (postResult.status != 200)
    {
        return postResult;
    }

    auto serviceInfo = QJsonDocument::fromJson(postResult.data).object().toVariantMap();
    if (serviceInfo.isEmpty())
    {
        return HttpResponse::Error("Service info not found");
    }

    auto tables = serviceInfo["tables"].toList();
    for (auto i = 0; i < tables.count(); i++)
    {
        auto table = tables[i].toMap();
        auto postResult = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, serviceUrl + "/" + table["id"].toString(), query));
        if (postResult.status != 200)
        {
            return postResult;
        }

        table["serviceInfo"] = QJsonDocument::fromJson(postResult.data).object().toVariantMap();
        tables[i] = table;
    }
    serviceInfo["tables"] = tables;

    auto layers = serviceInfo["layers"].toList();
    for (auto i = 0; i < layers.count(); i++)
    {
        auto layer = layers[i].toMap();
        auto postResult = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, serviceUrl + "/" + layer["id"].toString(), query));
        if (postResult.status != 200)
        {
            return postResult;
        }

        layer["serviceInfo"] = QJsonDocument::fromJson(postResult.data).object().toVariantMap();
        layers[i] = layer;
    }
    serviceInfo["layers"] = layers;

    *outServiceInfo = serviceInfo;

    return HttpResponse::ErrorIf(serviceInfo.isEmpty(), "Unknown error");
}

HttpResponse esriCreateService(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& payload, const QString& token, QVariantMap* infoOut)
{
    infoOut->clear();

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("token", token);
    query.addQueryItem("outputType", "featureService");
    query.addQueryItem("createParameters", payload);

    auto url = QString("%1%2%3").arg("https://www.arcgis.com/sharing/rest/content/users/", username, "/createService");

    auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, url, query));
    if (!response.success)
    {
        return response;
    }

    auto responseMap = QJsonDocument::fromJson(response.data).object().toVariantMap();
    if (!responseMap.value("success").toBool())
    {
        return HttpResponse::Error("Internal error");
    }

    if (responseMap.value("serviceItemId").toString().isEmpty())
    {
        return HttpResponse::Error("Service item missing");
    }

    if (responseMap.value("serviceurl").toString().isEmpty())
    {
        return HttpResponse::Error("Service url missing");
    }

    *infoOut = responseMap;

    return HttpResponse::Success();
}

HttpResponse esriAddToDefinition(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& payload, const QString& token, QVariantMap* infoOut)
{
    infoOut->clear();

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("token", token);
    query.addQueryItem("addToDefinition", payload);

    auto url = QString("%1%2").arg(serviceUrl, "/addToDefinition");
    if (!url.contains("/admin/"))
    {
        url.replace("/rest/services/", "/rest/admin/services/");
    }

    auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, url, query));
    if (!response.success)
    {
        return response;
    }

    auto responseMap = QJsonDocument::fromJson(response.data).object().toVariantMap();
    if (!responseMap.value("success").toBool())
    {
        return HttpResponse::Error("Internal error");
    }

    *infoOut = responseMap;

    return HttpResponse::Success();
}

HttpResponse esriShareItemWithOrg(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& item, const QString& token)
{
    auto url = "https://cybertracker.maps.arcgis.com/sharing/rest/content/users/" + username + "/shareItems";

    auto query = QUrlQuery();
    query.addQueryItem("f", "pjson");
    query.addQueryItem("token", token);
    query.addQueryItem("items", item);
    query.addQueryItem("org", "true");
    query.addQueryItem("everyone", "false");
    query.addQueryItem("confirmItemControl", "true");

    auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, url, query));
    if (!response.success)
    {
        return response;
    }

    auto responseMap = QJsonDocument::fromJson(response.data).object().toVariantMap().value("results").toList().constFirst().toMap();
    if (!responseMap.value("success").toBool())
    {
        return HttpResponse::Error("Internal error");
    }

    return response;
}

HttpResponse esriBuildService(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& serviceDefinition, const QString& layersDefinition, const QString& token, QString* serviceUrlOut)
{
    // Create the service.
    auto serviceInfo = QVariantMap();
    auto response = esriCreateService(networkAccessManager, username, serviceDefinition, token, &serviceInfo);
    if (!response.success)
    {
        return response;
    }

    auto serviceUrl = serviceInfo["serviceurl"].toString();
    if (serviceUrl.isEmpty())
    {
        return HttpResponse::Error("Service url not found");
    }

    // Share with the org.
    auto serviceItemId = serviceInfo["serviceItemId"].toString();
    response = esriShareItemWithOrg(networkAccessManager, username, serviceItemId, token);
    if (!response.success)
    {
        return response;
    }

    // Create the layers.
    auto layerInfo = QVariantMap();
    response = esriAddToDefinition(networkAccessManager, serviceUrl, layersDefinition, token, &layerInfo);
    if (!response.success)
    {
        return response;
    }

    // Success.
    *serviceUrlOut = serviceUrl;

    return response;
}

HttpResponse esriInitLocationTracking(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& token, QVariantMap* esriLocationServiceStateOut)
{
    esriLocationServiceStateOut->clear();

    // Retrieve portal data.
    auto portalData = QVariantMap();
    auto response = Utils::esriFetchPortal(networkAccessManager, token, &portalData);
    if (!response.success)
    {
        return response;
    }

    // Retrieve username and fullName.
    auto user = portalData.value("user").toMap();
    auto username = user.value("username").toString();
    auto fullName = user.value("fullName").toString();
    if (username.isEmpty() || fullName.isEmpty())
    {
        return HttpResponse::Error("Failed to read user data");
    }

    // Build deviceid.
    auto deviceId = Utils::addBracesToUuid(App::instance()->settings()->deviceId());

    // Lookup last location.
    auto query = QUrlQuery();
    query.addQueryItem("created_user", "'" + username + "'");
    query.addQueryItem("deviceid", "'" + deviceId + "'");
    query.addQueryItem("outFields", "globalid");
    query.addQueryItem("returnGeometry", "false");
    query.addQueryItem("f", "json");
    query.addQueryItem("token", token);

    response = Utils::esriDecodeResponse(Utils::httpGet(networkAccessManager, serviceUrl + "/1/query?where=" + query.toString()));
    if (!response.success)
    {
        return response;
    }

    auto locationTracking = QVariantMap
    {
        { "serviceUrl", serviceUrl },
        { "fullName", fullName },
    };

    // Lookup last feature.
    auto features = QJsonDocument::fromJson(response.data).object().toVariantMap().value("features").toList();
    auto lastFeature = features.isEmpty() ? QVariantMap() : features.constLast().toMap().value("attributes").toMap();
    if (!lastFeature.isEmpty()) // Last feature already exists.
    {
        locationTracking.insert("globalid", lastFeature.value("globalid"));
        *esriLocationServiceStateOut = locationTracking;
        return HttpResponse::Success();
    }

    // No last feature, so create one.
    auto feature = QVariantMap
    {
        { "attributes", QVariantMap
            {
                { "globalid", Utils::addBracesToUuid(QUuid::createUuid().toString(QUuid::Id128)) },
                { "deviceid", deviceId },
                { "timestamp", Utils::decodeTimestampMSecs(App::instance()->timeManager()->currentDateTimeISO()) },
                { "full_name", fullName }
            }
        }
    };

    auto results = QVariantList();
    response = esriApplyEdits(networkAccessManager, serviceUrl, "adds", 1, feature, token, &results);
    if (!response.success)
    {
        return response;
    }

    // If successful, then run this again and pick up the ids.
    if (!results.isEmpty())
    {
        auto addResults = results.constFirst().toMap().value("addResults").toList();
        if (addResults.length() == 1 && addResults.constFirst().toMap().value("success").toBool())
        {
            return esriInitLocationTracking(networkAccessManager, serviceUrl, token, esriLocationServiceStateOut);
        }
    }

    // Failed to initialize.
    return HttpResponse::Error("Last known location failed");
}

HttpResponse esriApplyEdits(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& operation, int layerIndex, const QVariantMap& feature, const QString& token, QVariantList* resultsOut)
{
    return esriApplyEdits(networkAccessManager, serviceUrl, operation, layerIndex, QVariantList { feature }, token, resultsOut);
}

HttpResponse esriApplyEdits(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& operation, int layerIndex, const QVariantList& features, const QString& token, QVariantList* resultsOut)
{
    resultsOut->clear();

    auto payload = QVariantList { QVariantMap {{ operation, features }, { "id", layerIndex }}};
    auto payloadJson = QJsonDocument(QJsonArray::fromVariantList(payload)).toJson(QJsonDocument::Compact);

    auto query = QUrlQuery();
    query.addQueryItem("f", "json");
    query.addQueryItem("edits", payloadJson);
    query.addQueryItem("useGlobalIds", "true");
    query.addQueryItem("rollbackOnFailure", "true");
    query.addQueryItem("token", token);

    auto response = Utils::esriDecodeResponse(Utils::httpPostQuery(networkAccessManager, serviceUrl + "/applyEdits", query));
    if (response.success)
    {
        *resultsOut = QJsonDocument::fromJson(response.data).array().toVariantList();
    }

    return response;
}

auto s_defaultColor = "ff0000";
auto s_defaultPointSize = 6;
auto s_defaultLineWidth = 1.5;

QVariantMap makePointSymbol(const QVariantMap& style, double symbolScale, const QColor& outlineColor)
{
    auto outlineSymbol = QVariantMap
    {
        { "symbolType", "SimpleLineSymbol" },
        { "color", QVariantList { outlineColor.red(), outlineColor.green(), outlineColor.blue(), outlineColor.alpha() } },
        { "style", "esriSLSSolid" },
        { "type", "esriSLS" },
        { "width", 0.5 * symbolScale },
    };

    auto styleColor = QColor('#' + style.value("marker-color", s_defaultColor).toString());
    auto styleMap = QVariantMap
    {
        { "circle", "esriSMSCircle" },
        { "cross", "esriSMSCross" },
        { "diamond", "esriSMSDiamond" },
        { "square", "esriSMSSquare" },
        { "triangle", "esriSMSTriangle" },
        { "x", "esriSMSX" },
    };

    return QVariantMap
    {
        { "symbolType", "SimpleMarkerSymbol" },
        { "color", QVariantList { styleColor.red(), styleColor.green(), styleColor.blue(), 200 } },
        { "style", styleMap[style.value("marker-style", "circle").toString()] },
        { "type", "esriSMS" },
        { "size", style.value("marker-size", s_defaultPointSize).toDouble() * symbolScale },
        { "outline", outlineSymbol },
        { "angle", 0.0 },
        { "yoffset", 0.0 },
    };
}

QVariantMap makeLineSymbol(const QVariantMap& style, double symbolScale)
{
    auto styleColor = QColor('#' + style.value("stroke-color", s_defaultColor).toString());

    auto styleMap = QVariantMap
    {
        { "solid", "esriSLSSolid" },
        { "dash", "esriSLSDash"  },
        { "dashdot", "esriSLSDashDot" },
        { "dashdotdot", "esriSLSDashDotDot" },
        { "dot", "esriSLSDot" },
    };

    return QVariantMap
    {
        { "symbolType", "SimpleLineSymbol" },
        { "color", QVariantList { styleColor.red(), styleColor.green(), styleColor.blue(), 200 } },
        { "style", styleMap[style.value("stroke-style", "solid").toString()] },
        { "type", "esriSLS" },
        { "width", style.value("stroke-size", s_defaultLineWidth).toDouble() * symbolScale },
    };
}

QVariantMap buildPoints(const QVariantMap& source, double symbolScale, const QColor& outlineColor, QRectF* extentOut, QVariantList* paths)
{
    auto points = QVariantList();

    auto addPoint = [&](const QString& name, double x, double y, const QVariantMap& symbol, const QVariant& pathPoints)
    {
        points.append(QVariantMap
        {
            { "x", x },
            { "y", y },
            { "spatialReference", QVariantMap {{ "wkid", 4326 }} },
            { "name", name },
            { "pathIndex", 0 },
            { "pathId", paths->count() },
            { "symbol", symbol },
        });

        paths->append(pathPoints);

        *extentOut = extentOut->united(Utils::pointToRect(x, y));
    };

    auto features = source["features"].toList();
    for (auto it = features.constBegin(); it != features.constEnd(); it++)
    {
        auto featureMap = it->toMap();
        auto style = featureMap["style"].toMap();
        auto geometry = featureMap.value("geometry").toMap();
        auto pathPoints = QVariantList();
        auto name = featureMap.value("properties").toMap().value("id").toString();

        // Add a single point.
        if (geometry.value("type") == "Point")
        {
            pathPoints.append(geometry.value("coordinates"));   // Append the point as tuple.

            auto coordinates = pathPoints.first().toList();
            addPoint(name, coordinates[0].toDouble(), coordinates[1].toDouble(), makePointSymbol(style, symbolScale, outlineColor), pathPoints);
            continue;
        }

        // Add a point for the front and back of a line-string.
        if (geometry.value("type") == "LineString")
        {
            pathPoints = geometry.value("coordinates").toList();
            auto symbol = makePointSymbol(
                QVariantMap { { "marker-color", style.value("stroke-color", s_defaultColor) }, { "marker-style", "circle" }, { "marker-size", s_defaultPointSize } },
                symbolScale, outlineColor);

            // Store the forward path with the first point.
            auto coordinates = pathPoints.first().toList();
            addPoint(name, coordinates[0].toDouble(), coordinates[1].toDouble(), symbol, pathPoints);

            // Store the reverse path with the last point.
            std::reverse(pathPoints.begin(), pathPoints.end());
            coordinates = pathPoints.first().toList();
            addPoint(name, coordinates[0].toDouble(), coordinates[1].toDouble(), symbol, pathPoints);
            continue;
        }

        qFatalIf(true, "Bad geometry type");
    }

    return points.isEmpty() ? QVariantMap() : QVariantMap
    {
        { "source", source["name"] },
        { "geometryType", "Points" },
        { "geometry", points },
    };
}

QVariantList buildLines(const QVariantMap& source, double symbolScale, QRectF* extentOut)
{
    auto result = QVariantList();

    auto features = source["features"].toList();
    for (auto it = features.constBegin(); it != features.constEnd(); it++)
    {
        auto featureMap = it->toMap();
        auto featureGeometry = featureMap.value("geometry").toMap();
        if (featureGeometry.value("type") != "LineString")
        {
            continue;
        }

        auto paths = QVariantList();
        auto pathPart = QVariantList();
        auto coords = featureGeometry.value("coordinates").toList();
        for (auto it = coords.constBegin(); it != coords.constEnd(); it++)
        {
            auto coord = it->toList();
            auto x = coord[0].toDouble();
            auto y = coord[1].toDouble();
            *extentOut = extentOut->united(Utils::pointToRect(x, y));
            pathPart.append(*it);
        }

        paths.append(QVariant(pathPart));

        result.append(QVariantMap
        {
            { "source", source["name"] },
            { "name",  featureMap.value("properties").toMap().value("id", "?").toString() },
            { "geometryType", "Polyline" },
            { "geometry", QVariantMap { { "spatialReference", QVariantMap {{ "wkid", 4326 }} }, { "paths", paths }} },
            { "symbol", makeLineSymbol(featureMap.value("style").toMap(), symbolScale) },
        });
    }

    return result;
}

QVariantMap esriLayerFromGeoJson(const QString& filePath, double symbolScale, const QColor& outlineColor, QVariantList* paths)
{
    auto source = Utils::variantMapFromJsonFile(filePath);
    source["name"] = source.value("name", QFileInfo(filePath).baseName());

    auto extentRect = QRectF();
    auto points = buildPoints(source, symbolScale, outlineColor, &extentRect, paths);
    auto polylines = buildLines(source, symbolScale, &extentRect);

    return QVariantMap
    {
        { "name", source["name"] },
        { "polylines", polylines },
        { "points", points },
        { "extent", QVariantMap {
            { "spatialReference", QVariantMap {{ "wkid", 4326 }} },
            { "xmin", extentRect.left() },
            { "ymin", extentRect.top() },
            { "xmax", extentRect.right() },
            { "ymax", extentRect.bottom() }
        }}
    };
}

}
