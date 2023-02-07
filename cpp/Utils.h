#pragma once
#include "pch.h"

inline static void qFatalIf(bool faultIfTrue, const QString& message)
{
    if (faultIfTrue)
    {
        qFatal("%s", message.toStdString().c_str());
    }
}

inline static void dumpResources()
{
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        qDebug() << it.next();
    }
}

struct ApiResult
{
    bool success = false;
    QString errorString;
    bool expected = false;

    QVariantMap toMap() const
    {
        return QVariantMap {{ "success", success }, { "errorString", errorString }, { "expected", expected }};
    }

    static ApiResult fromMap(const QVariantMap& value)
    {
        return make(value["success"].toBool(), value["errorString"].toString(), value["expected"].toBool());
    }

    static ApiResult make(bool success, const QString& errorString, bool expected)
    {
        return ApiResult { success, errorString, expected };
    }

    static ApiResult Success()
    {
        return make(true, "", true);
    }

    static ApiResult Error(const QString& errorString)
    {
        return make(false, errorString, false);
    }

    static ApiResult Expected(const QString& errorString)
    {
        return make(false, errorString, true);
    }

    static QVariantMap SuccessMap()
    {
        return ApiResult { true, "", false }.toMap();
    }

    static QVariantMap ErrorMap(const QString& errorString)
    {
        return ApiResult { false, errorString, false }.toMap();
    }

    static QVariantMap ExpectedMap(const QString& errorString)
    {
        return ApiResult { false, errorString, true }.toMap();
    }
};

inline bool operator==(const ApiResult& lhs, const ApiResult& rhs)
{
    return lhs.success == rhs.success && lhs.expected == rhs.expected && lhs.errorString == rhs.errorString;
}

namespace Utils
{

QJsonDocument jsonDocumentFromFile(const QString& filePath);
QVariantMap variantMapFromJsonFile(const QString& filePath);
QByteArray variantMapToJson(const QVariantMap& map, QJsonDocument::JsonFormat format = QJsonDocument::Indented);
QByteArray variantListToJson(const QVariantList& list, QJsonDocument::JsonFormat format = QJsonDocument::Indented);
QString variantMapToString(const QVariantMap& map);
QVariantMap variantMapFromJson(const QByteArray& json);
QByteArray variantMapToBlob(const QVariantMap& map);
QVariantMap variantMapFromBlob(const QByteArray& byteArray);
QByteArray variantListToBlob(const QVariantList& list);
QVariantList variantListFromBlob(const QByteArray& byteArray);
QVariantList variantListFromJsonFile(const QString& filePath);
void writeJsonToFile(const QString& filePath, const QByteArray& data);
void writeDataToFile(const QString& filePath, const QByteArray& data);
bool compareFiles(const QString& filePath1, const QString& filePath2);
bool compareVariantMaps(const QVariantMap& map1, const QVariantMap& map2);

QVariant variantFromJSValue(const QVariant& value);

QString unpackZip(const QString& targetPath, const QString& targetFolder, const QString& zipFilePath);

QVariantMap qtSerialize(const QObject* obj);
void qtDeserialize(const QVariantMap& data, QObject* obj);

QObject* loadQmlFromFile(const QString& filePath);
QObject* loadQmlFromFile(QQmlEngine* engine, const QString& filePath);
QObject* loadQmlFromData(const QByteArray& data);

void writeQml(QTextStream& stream, int depth, const QString& text);
void writeQml(QTextStream& stream, int depth, const QString& name, const QString& value);
void writeQml(QTextStream& stream, int depth, const QString& name, const QJsonValue& value);
void writeQml(QTextStream& stream, int depth, const QString& name, const QJsonObject& value);
void writeQml(QTextStream& stream, int depth, const QString& name, const QJsonArray& value);
void writeQml(QTextStream& stream, int depth, const QString& name, const QVariantMap& value);
void writeQml(QTextStream& stream, int depth, const QString& name, const QVariantList& value);
void writeQml(QTextStream& stream, int depth, const QString& name, const QVariant& value);
void writeQml(QTextStream& stream, int depth, const QString& name, const bool value, const bool defaultValue);
void writeQml(QTextStream& stream, int depth, const QString& name, const int value, const int defaultValue = 0);
void writeQml(QTextStream& stream, int depth, const QString& name, const double value);
void writeQml(QTextStream& stream, int depth, const QString& name, const QStringList& value);

void latLonToUTMXY(double lat, double lon, double* xOut, double* yOut, int* zoneOut, bool* northOut);
void UTMXYToLatLon(double x, double y, int zone, bool north, double* latOut, double* lonOut);

bool locationValid(const QVariantMap& locationMap);
double distanceInMeters(double lat1, double lon1, double lat2, double lon2);
double distanceInMeters(const QVariantList& points);
double areaInSquareMeters(const QVariantList& points);
int headingInDegrees(double lat1, double lon1, double lat2, double lon2);
QString headingToText(int heading);
QRectF pointToRect(double x, double y);
QVariantMap computeExtent(const QVariantList& points);

QVariantList pointsFromXml(const QString& xml);
QString pointsToXml(const QVariantList& points);

bool ensurePath(const QString& path, bool mediaScanOnSuccess = false);
QString urlToLocalFile(const QString& filePathUrl);

bool mediaScan(const QString& filePath);

void extractResource(const QString& resource, const QString& targetPath);
bool copyPath(const QString& src, const QString& dst, const QStringList& excludes = QStringList());
QString removeTrailingSlash(const QString& string);

QStringList searchStandardFolders(const QString& fileSpec);
void enforceFolderQuota(const QString& folder, int maxFileCount);

bool isAndroid();

QString trimUrl(const QString& url);
QString redirectOnlineDriveUrl(const QString& url);
bool pingUrl(QNetworkAccessManager* networkAccessManager, const QString& url);
bool networkAvailable(QNetworkAccessManager* networkAccessManager);

QString encodeFilePath(const QString& filePath);
QString decodeFilePath(const QString& filePath);

struct HttpResponse
{
    bool success = false;
    int status = 0;
    QString errorString = "Unknown error";
    QString reason;
    QString etag = 0;
    QString location;
    QString url;
    QByteArray data = {};

    static HttpResponse Success()
    {
        return HttpResponse { true, 0, "" };
    }

    static HttpResponse Error(const QString& errorString)
    {
        return HttpResponse { false, 0, errorString };
    }

    static HttpResponse ErrorIf(bool condition, const QString& errorString)
    {
        return condition ? Error(errorString) : Success();
    }

    QVariantMap toMap()
    {
        return QVariantMap
        {
            { "success", success },
            { "status",  status },
            { "reason", reason },
            { "etag", etag },
            { "errorString", errorString },
            { "data", data },
        };
    }

    QVariantMap toApiResult()
    {
        return ApiResult::make(success, errorString, false).toMap();
    }
};

QByteArray makeMultiPartBoundary();
HttpResponse httpGetHead(QNetworkAccessManager* networkAccessManager, const QString& url);
HttpResponse httpGet(QNetworkAccessManager* networkAccessManager, const QString& url, const QString& username = QString(), const QString& password = QString());
HttpResponse httpGetWithToken(QNetworkAccessManager* networkAccessManager, const QString& url, const QString& token, const QString& tokenType = "Bearer");
HttpResponse httpPostJson(QNetworkAccessManager* networkAccessManager, const QString& url, const QVariantMap& data, const QString& token = QString(), const QString& tokenType = "Bearer", bool deflate = false);
HttpResponse httpPostData(QNetworkAccessManager* networkAccessManager, const QString& url, const QByteArray& payload, const QString& token = QString(), const QString& tokenType = "Bearer", bool deflate = false);
HttpResponse httpPostQuery(QNetworkAccessManager* networkAccessManager, const QString& url, const QUrlQuery& query);
HttpResponse httpPostQuery(QNetworkAccessManager* networkAccessManager, const QString& url, const QVariantMap& queryMap);
HttpResponse httpPostMultiPart(QNetworkAccessManager* networkAccessManager, const QString& url, QHttpMultiPart* multiPart);

HttpResponse httpAcquireOAuthToken(QNetworkAccessManager* networkAccessManager, const QString& server, const QString& clientId, const QString& username, const QString& password, QString* accessTokenOut, QString* refreshTokenOut);
HttpResponse httpRefreshOAuthToken(QNetworkAccessManager* networkAccessManager, const QString& server, const QString& clientId, const QString& refreshToken, QString* accessTokenOut, QString* refreshTokenOut);

HttpResponse esriDecodeResponse(const HttpResponse& response);
HttpResponse esriAcquireToken(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& password, QString* tokenOut);
HttpResponse esriAcquireOAuthToken(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& password, QString* accessTokenOut, QString* refreshTokenOut);
HttpResponse esriFetchContent(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& folderId, const QString& token, QVariantMap* contentOut);
HttpResponse esriFetchSurveys(QNetworkAccessManager* networkAccessManager, const QString& token, QVariantMap* contentOut);
HttpResponse esriFetchSurvey(QNetworkAccessManager* networkAccessManager, const QString& surveyId, const QString& token, QVariantMap* surveyOut);
HttpResponse esriFetchSurveyServiceUrl(QNetworkAccessManager* networkAccessManager, const QString& surveyId, const QString& token, QString* serviceUrlOut);
HttpResponse esriFetchPortal(QNetworkAccessManager* networkAccessManager, const QString& token, QVariantMap* portalOut);
HttpResponse esriFetchServiceMetadata(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& accessToken, QVariantMap* serviceInfoOut);
HttpResponse esriCreateService(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& payload, const QString& token, QVariantMap* infoOut);
HttpResponse esriAddToDefinition(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& payload, const QString& token, QVariantMap* infoOut);
HttpResponse esriBuildService(QNetworkAccessManager* networkAccessManager, const QString& username, const QString& serviceDefinition, const QString& layersDefinition, const QString& token, QString* serviceUrlOut);
HttpResponse esriInitLocationTracking(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& token, QVariantMap* esriLocationServiceStateOut);
HttpResponse esriApplyEdits(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& operation, int layerIndex, const QVariantMap& feature, const QString& token, QVariantList* resultsOut);
HttpResponse esriApplyEdits(QNetworkAccessManager* networkAccessManager, const QString& serviceUrl, const QString& operation, int layerIndex, const QVariantList& features, const QString& token, QVariantList* resultsOut);

QVariantMap esriLayerFromGeoJson(const QString& filename, double symbolScale, const QColor& outlineColor, QVariantList* paths);

QString addBracesToUuid(const QString& uuid);
QString encodeJsonStringLiteral(const QString& value);

bool renderMapCallout(const QString& iconFilePath, const QString& targetFilePath, const QColor& color, const QColor& outlineColor, int size);
bool renderMapMarker(const QString& url, const QString& targetFilePath, const QColor& color, const QColor& outlineColor, int width, int height);

QString renderSvgToPng(const QString& url, int width, int height);
bool renderSvgToPngFile(const QString& url, const QString& targetFilePath, int width, int height);
bool renderSquarePixmap(QPixmap* pixmap, const QString& filePath, int width, int height);
QString renderPointsToSvg(const QVariantList& points, const QColor& color, int penWidth, bool fill = false);
QString renderSketchToSvg(const QVariant& sketch, QColor penColor = Qt::black, int penWidth = 2);
bool renderSketchToPng(const QVariant& sketch, const QString& filePath, QColor penColor = Qt::black, int penWidth = 2);
QVariant sketchToVariant(const QList<QPolygon>& sketch);
QList<QPolygon> variantToSketch(const QVariant& sketchVariant);

QString renderQRCodeToPng(const QString& data, int width, int height);

QList<QStringList> buildListCombinations(const QList<QStringList>& lists);

bool reorientImageFile(const QString& filePath, int resolutionX, int resolutionY, bool isBackFace, int cameraOrientation);

QString extractText(const QString& value, const QString& prefix, const QString& suffix);
QString lastPathComponent(const QString& path);
bool compareLastPathComponents(const QString& path1, const QString& path2);

QVariant getParam(const QVariantMap& params, const QString& key, const QVariant& defaultValue = QVariant());

QString androidGetExternalFilesDir();
void androidHideNavigationBar();

QString classicRoot();

QString encodeTimestamp(const QDateTime& dateTime);
QDateTime decodeTimestamp(const QString& isoDateTime);
qint64 decodeTimestampSecs(const QString& isoDateTime);
qint64 decodeTimestampMSecs(const QString& isoDateTime);
qint64 timestampDeltaMs(const QString& startISODateTime, const QString& stopISODateTime);

QMimeDatabase* mimeDatabase();

void enumFiles(const QString& folder, std::function<void(const QFileInfo& fileInfo)> callback);
void enumFolders(const QString& folder, std::function<void(const QFileInfo& fileInfo)> callback);
QString computeFileHash(const QString& filePath);
QString computeFolderHash(const QString& folder);

bool isMapLayerSuffix(const QString& suffix);
bool isMapLayerVector(const QString& suffix);
bool isMapLayerRaster(const QString& suffix);

QStringList buildMapLayerList(const QString& folder);

class Api : public QObject
{
    Q_OBJECT
    QML_WRITABLE_AUTO_PROPERTY (QNetworkAccessManager*, networkAccessManager)

public:
    Api(QObject* parent = nullptr): QObject(parent)
    {}

    Q_INVOKABLE QString generateUuid()
    {
        return QUuid::createUuid().toString(QUuid::Id128);
    }

    Q_INVOKABLE QString getCurrentTimeAsString(const QString& format)
    {
        return QDateTime::currentDateTime().toString(format);
    }

    Q_INVOKABLE bool mediaScan(const QString& filePath)
    {
        return Utils::mediaScan(filePath);
    }

    Q_INVOKABLE bool copyFile(const QString& srcFilePath, const QString& dstFilePath)
    {
        if (QFile::exists(dstFilePath))
        {
            QFile::remove(dstFilePath);
        }

        return QFile::copy(srcFilePath, dstFilePath);
    }

    Q_INVOKABLE bool removeFile(const QString& filePath)
    {
        return !filePath.isEmpty() && QFile().remove(filePath);
    }

    Q_INVOKABLE int lightness(QColor color)
    {
        return color.lightness();
    }

    Q_INVOKABLE QColor lighter(QColor color, int factor = 150)
    {
        return color.lighter(factor);
    }

    Q_INVOKABLE QColor changeAlpha(QColor color, int newAlpha)
    {
        color.setAlpha(newAlpha);
        return color;
    }

    Q_INVOKABLE bool reorientImageFile(const QString& filePath, int resolutionX = -1, int resolutionY = -1, bool isBackFace = true, int cameraOrientation = 0)
    {
        return Utils::reorientImageFile(filePath, resolutionX, resolutionY, isBackFace, cameraOrientation);
    }

    Q_INVOKABLE QVariantMap loadJsonFile(const QString& filePath)
    {
        return variantMapFromJsonFile(filePath);
    }

    Q_INVOKABLE QString makeUrlPath(const QString& urlString, const QString& newPath)
    {
        auto url = QUrl(urlString);
        url.setPath(newPath);
        return url.toString();
    }

    Q_INVOKABLE bool pingUrl(const QString& url)
    {
        return Utils::pingUrl(m_networkAccessManager, url);
    }

    Q_INVOKABLE bool networkAvailable()
    {
        return Utils::networkAvailable(m_networkAccessManager);
    }

    Q_INVOKABLE QVariantMap httpGet(const QString& url, const QString& username = QString(), const QString& password = QString())
    {
        return Utils::httpGet(m_networkAccessManager, url, username, password).toMap();
    }

    Q_INVOKABLE QVariantMap httpGetWithToken(const QString& url, const QString& token, const QString& tokenType = "Bearer")
    {
        return Utils::httpGetWithToken(m_networkAccessManager, url, token, tokenType).toMap();
    }

    Q_INVOKABLE QVariantMap httpPostJson(const QString& url, const QVariantMap& data, const QString& token = QString(), const QString& tokenType = "Bearer")
    {
        return Utils::httpPostJson(m_networkAccessManager, url, data, token, tokenType).toMap();
    }

    Q_INVOKABLE QVariantMap httpPostQuery(const QString& url, const QVariantMap& queryMap)
    {
        return Utils::httpPostQuery(m_networkAccessManager, url, queryMap).toMap();
    }

    Q_INVOKABLE QVariantMap decodeTimestamp(const QString& isoDateTime)
    {
        auto dateTime = Utils::decodeTimestamp(isoDateTime);
        auto date = dateTime.date();
        auto time = dateTime.time();

        return QVariantMap
        {
            { "year", date.year() },
            { "month", date.month() },
            { "day", date.day() },
            { "h", time.hour() },
            { "m", time.minute() },
            { "s", time.second() },
        };
    }

    Q_INVOKABLE QString changeDate(const QString& isoDateTime, int year, int month, int day)
    {
        auto dateTime = Utils::decodeTimestamp(isoDateTime);
        auto date = dateTime.date();
        date.setDate(year, month, day);
        dateTime.setDate(date);
        return Utils::encodeTimestamp(dateTime);
    }

    Q_INVOKABLE QString changeTime(const QString& isoDateTime, int h, int m, int s)
    {
        auto dateTime = Utils::decodeTimestamp(isoDateTime);
        auto time = dateTime.time();
        time.setHMS(h, m, s);
        dateTime.setTime(time);
        return Utils::encodeTimestamp(dateTime);
    }

    Q_INVOKABLE QUrl urlFromLocalFile(const QString& filePath)
    {
        return QUrl::fromLocalFile(filePath);
    }

    Q_INVOKABLE QString urlToLocalFile(const QString& filePathUrl)
    {
        return Utils::urlToLocalFile(filePathUrl);
    }

    Q_INVOKABLE QVariantMap locationToUTM(double lat, double lon)
    {
        auto ux = 0.0;
        auto uy = 0.0;
        auto zone = 0;
        auto north = false;
        Utils::latLonToUTMXY(lat, lon, &ux, &uy, &zone, &north);

        return QVariantMap { { "ux", qFloor(qFabs(ux) + 0.5) }, { "uy", qFloor(qFabs(uy) + 0.5) }, { "zone", zone }, { "hemi", north ? "N" : "S" } };
    }

    Q_INVOKABLE QVariantMap locationFromUTM(double x, double y, int zone, const QString& hemi)
    {
        auto lat = 0.0;
        auto lon = 0.0;
        UTMXYToLatLon(x, y, zone, hemi == "N", &lat, &lon);

        return QVariantMap { { "x", lon }, { "y", lat }, { "spatialReference", QVariantMap { { "wkid", 4326 } } } };
    }

    Q_INVOKABLE QVariantMap esriAcquireToken(const QString& username, const QString& password)
    {
        auto token = QString();

        auto result = Utils::esriAcquireToken(m_networkAccessManager, username, password, &token).toApiResult();
        result["token"] = token;

        return result;
    }

    Q_INVOKABLE QVariantMap esriAcquireOAuthToken(const QString& username, const QString& password)
    {
        auto accessToken = QString();
        auto refreshToken = QString();
        auto result = Utils::esriAcquireOAuthToken(m_networkAccessManager, username, password, &accessToken, &refreshToken).toApiResult();
        result["accessToken"] = accessToken;
        result["refreshToken"] = refreshToken;

        return result;
    }

    Q_INVOKABLE QVariantMap esriFetchContent(const QString& username, const QString& folderId, const QString& token)
    {
        auto content = QVariantMap();
        auto result = Utils::esriFetchContent(m_networkAccessManager, username, folderId, token, &content).toApiResult();
        result["content"] = content;

        return result;
    }

    Q_INVOKABLE QVariantMap esriFetchSurveys(const QString& token)
    {
        auto content = QVariantMap();
        auto result = Utils::esriFetchSurveys(m_networkAccessManager, token, &content).toApiResult();
        result["content"] = content;

        return result;
    }

    Q_INVOKABLE QString renderPointsToSvg(const QVariantList& points, const QColor& color, int penWidth, bool fill = false)
    {
        return Utils::renderPointsToSvg(points, color, penWidth, fill);
    }

    Q_INVOKABLE QString renderSketchToSvg(const QVariant& sketch, QColor penColor, int penWidth)
    {
        return Utils::renderSketchToSvg(sketch, penColor, penWidth);
    }

    Q_INVOKABLE bool renderSketchToPng(const QVariant& sketch, const QString& filePath, QColor penColor = Qt::black, int penWidth = 2)
    {
        return Utils::renderSketchToPng(sketch, filePath, penColor, penWidth);
    }

    Q_INVOKABLE QString renderQRCodeToPng(const QString& data, int width, int height)
    {
        return Utils::renderQRCodeToPng(data, width, height);
    }

    Q_INVOKABLE double distanceInMeters(const QVariantList& points)
    {
        return Utils::distanceInMeters(points);
    }

    Q_INVOKABLE QVariantMap computeExtent(const QVariantList& points)
    {
        return Utils::computeExtent(points);
    }

    Q_INVOKABLE QString stripMarkdown(const QString& markdownText)
    {
        QTextDocument textDocument;
        textDocument.setMarkdown(markdownText, QTextDocument::MarkdownDialectCommonMark);
        return textDocument.toPlainText();
    }

    Q_INVOKABLE void androidHideNavigationBar()
    {
        return Utils::androidHideNavigationBar();
    }

    Q_INVOKABLE QStringList listFilenames(const QString& folder, const QString& filespec)
    {
        auto result = QStringList();

        auto files = QDir(folder).entryInfoList({ filespec }, QDir::Files, QDir::Time);
        for (auto it = files.constBegin(); it != files.constEnd(); it++)
        {
            result.append(it->fileName());
        }

        return result;
    }

    Q_INVOKABLE QString decodeBase64(const QString& data)
    {
        return QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64Encoding);
    }

    Q_INVOKABLE void writeTextToFile(const QString& filePath, const QString& data)
    {
        Utils::writeJsonToFile(filePath, data.toLatin1());
    }

    Q_INVOKABLE QStringList stringToList(const QString& string)
    {
        auto result = QStringList();
        auto items = string.split(' ');
        for (auto itemIt = items.constBegin(); itemIt != items.constEnd(); itemIt++)
        {
            auto s = itemIt->trimmed();
            if (!s.isEmpty())
            {
                result.append(s);
            }
        }

        return result;
    }

    Q_INVOKABLE bool isMimeTypeAnImage(const QString& mimeType)
    {
        return QImageReader::supportedMimeTypes().contains(mimeType.toLatin1());
    }

    Q_INVOKABLE QStringList buildFileDialogFilters(const QString& name, const QString& filter)
    {
        if (filter.isEmpty())
        {
            return QStringList { name + " (*.*)" };
        }

        auto dialogFilter = filter;
        dialogFilter.replace(".", "*.");

        return QStringList { name + "(" + dialogFilter + ")" };
    }

    Q_INVOKABLE QVariant getParam(const QVariantMap& params, const QString& key, const QVariant& defaultValue)
    {
        return Utils::getParam(params, key, defaultValue);
    }
};
}
