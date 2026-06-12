#include "pch.h"
#include <zxing-cpp/core/src/MultiFormatWriter.h>
#include <zxing-cpp/core/src/BitMatrix.h>
#include <exiv2/exiv2.hpp>
#include "zonedetect/zonedetect.h"

namespace CyberTracker {

namespace Utils {

bool isMainThread()
{
    return QThread::currentThread() == QGuiApplication::instance()->thread();
}

void setStreamUtf8(QTextStream* stream)
{
    stream->setEncoding(QStringConverter::Encoding::Utf8);
}

QJsonDocument jsonDocumentFromFile(const QString& filePath)
{
    QFile file(filePath);

    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        return QJsonDocument::fromJson(file.readAll());
    }
    else
    {
        return QJsonDocument();
    }
}

QVariantMap variantMapFromJsonFile(const QString& filePath)
{
    auto jsonDocument = jsonDocumentFromFile(filePath);
    if (jsonDocument.isEmpty())
    {
        return QVariantMap();
    }

    auto jsonObject = jsonDocument.object();
    return jsonObject.toVariantMap();
}

QByteArray variantMapToJson(const QVariantMap& map, QJsonDocument::JsonFormat format)
{
    auto jsonObject = QJsonObject::fromVariantMap(map);
    QJsonDocument jsonDocument(jsonObject);
    return jsonDocument.toJson(format);
}

void variantMapToJsonFile(const QString& filePath, const QVariantMap& map)
{
    writeJsonToFile(filePath, variantMapToJson(map));
}

QByteArray variantListToJson(const QVariantList& list, QJsonDocument::JsonFormat format)
{
    auto jsonArray = QJsonArray::fromVariantList(list);
    QJsonDocument jsonDocument(jsonArray);
    return jsonDocument.toJson(format);
}

void variantListToJsonFile(const QString& filePath, const QVariantList& list, QJsonDocument::JsonFormat format)
{
    writeJsonToFile(filePath, variantListToJson(list, format));
}

QString variantMapToString(const QVariantMap& map)
{
    return QString(variantMapToJson(map));
}

QVariantMap variantMapFromJson(const QByteArray& json)
{
    auto jsonDocument = QJsonDocument::fromJson(json);
    return jsonDocument.object().toVariantMap();
}

QByteArray variantMapToBlob(const QVariantMap& map)
{
    QByteArray byteArray;
    QDataStream dataStream(&byteArray, QIODevice::WriteOnly);
    dataStream << map;
    return byteArray;
}

QVariantMap variantMapFromBlob(const QByteArray& byteArray)
{
    QDataStream dataStream(byteArray);
    QVariantMap map;
    dataStream >> map;
    return map;
}

QByteArray variantListToBlob(const QVariantList& list)
{
    QByteArray byteArray;
    QDataStream dataStream(&byteArray, QIODevice::WriteOnly);
    dataStream << list;
    return byteArray;
}

QVariantList variantListFromBlob(const QByteArray& byteArray)
{
    QDataStream dataStream(byteArray);
    QVariantList list;
    dataStream >> list;
    return list;
}

QVariantList variantListFromJsonFile(const QString& filePath)
{
    auto jsonDocument = jsonDocumentFromFile(filePath);
    if (jsonDocument.isEmpty())
    {
        return QVariantList();
    }

    auto jsonArray = jsonDocument.array();
    return jsonArray.toVariantList();
}

void writeJsonToFile(const QString& filePath, const QByteArray& data)
{
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);

    QTextStream streamFileOut(&file);
    Utils::setStreamUtf8(&streamFileOut);
    streamFileOut.setGenerateByteOrderMark(true);
    streamFileOut << data;
}

void writeDataToFile(const QString& filePath, const QByteArray& data)
{
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(data);
}

QByteArray readDataFromFile(const QString& filePath)
{
    QFile file(filePath);

    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        return file.readAll();
    }
    else
    {
        return QByteArray();
    }
}

bool compareFiles(const QString& filePath1, const QString& filePath2)
{
    QFile file1(filePath1);
    if (!file1.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QFile file2(filePath2);
    if (!file2.open(QIODevice::ReadOnly))
    {
        return false;
    }

    auto data1 = file1.readAll();
    auto data2 = file2.readAll();

    if (data1.size() != data2.size())
    {
        return false;
    }

    return data1.compare(data2) == 0;
}

bool compareVariantMaps(const QVariantMap& map1, const QVariantMap& map2)
{
    auto m1 = variantMapToBlob(map1);
    auto m2 = variantMapToBlob(map2);
    return m1.compare(m2) == 0;
}

QVariant variantFromJSValue(const QVariant& value)
{
    auto result = value;
    if (QString(value.typeName()) == "QJSValue")
    {
        auto jsValue = qvariant_cast<QJSValue>(value);
        result = jsValue.toVariant();
    }

    return result;
}

void insertParameter(QVariantMap* params, const QString& name, const QString& value)
{
    auto valueText = value.trimmed();
    if (valueText.isEmpty())
    {
        return;
    }

    // Convert "TRUE", "FALSE", "YES", "NO" to lowercase boolean values.
    auto fixups = QMap<QString, QString> { { "TRUE", "true" }, { "FALSE", "false" }, { "YES", "true" }, { "NO", "false" } };
    valueText = fixups.value(valueText.toUpper(), valueText);

    // Try a JSON type.
    auto valueJson = "{ \"v\": " + valueText + " }";
    auto v = QJsonDocument::fromJson(valueJson.toLatin1()).object().toVariantMap().value("v");
    if (!v.isValid() || v.isNull())
    {
        // object, array, bool, integer and double failed -> must be text.
        v = valueText;
    }

    // 'parameters' must be an object.
    if (name == "parameters")
    {
        params->insert(v.toMap());
        return;
    }

    // Check for full attribute value.
    auto parts = name.split('.');

    // attribute is full name.
    if (parts.length() == 1)
    {
        params->insert(name, v);
        return;
    }

    // attribute is a.b format, e.g. content.style.
    if (parts.length() == 2)
    {
        auto mapName = parts.constFirst();
        auto mapValue = parts.constLast();

        auto paramMap = params->value(mapName).toMap();
        paramMap[mapValue] = v;
        params->insert(mapName, paramMap);
        return;
    }

    // Format not supported.
    qDebug() << "Invalid attribute name: " << name;
}

QVariantMap qtSerialize(const QObject* obj)
{
    auto result = QJsonObject();

    auto metaObj = obj->metaObject();
    for (int i = 0; i < metaObj->propertyCount(); ++i)
    {
        const QString propertyName = metaObj->property(i).name();
        if (propertyName == "objectName")
        {
            continue;
        }

        auto data = obj->property(propertyName.toLatin1().data());
        auto type = static_cast<QMetaType::Type>(data.type());
        auto value = QJsonValue();

        if (type == QMetaType::Bool || type == QMetaType::Double || type == QMetaType::Float ||
            type == QMetaType::Int || type == QMetaType::UInt || type == QMetaType::LongLong ||
            type == QMetaType::ULongLong || type == QMetaType::QString || type == QMetaType::QByteArray)
        {
            value = QJsonValue::fromVariant(data);
        }
        else if (type == QMetaType::QVariantMap)
        {
            auto obj = QJsonObject::fromVariantMap(data.value<QVariantMap>());
            value = obj;
        }
        else if (type == QMetaType::QStringList)
        {
            auto array = QJsonArray::fromVariantList(data.value<QVariantList>());
            value = array;
        }
        else if (type == QMetaType::QVariantList)
        {
            auto array = QJsonArray::fromVariantList(data.value<QVariantList>());
            value = array;
        }
        else if (data.canConvert<QString>())
        {
            value = QJsonValue::fromVariant(data);
        }
        else
        {
            qWarning() << "Unable to convert property <" + propertyName + "> to JSON - unkown/unsupported type ";
        }

        if (!value.isNull())
        {
            result.insert(propertyName, value);
        }
    }

    return result.toVariantMap();
}

void qtDeserialize(const QVariantMap& data, QObject* obj)
{
    auto jsonObj = QJsonObject::fromVariantMap(data);

    for (auto it = jsonObj.constBegin(); it != jsonObj.constEnd(); it++)
    {
        auto key = it.key();
        auto value = it.value();

        if (value.isBool() || value.isDouble() || value.isString())
        {
            obj->setProperty(key.toLatin1().data(), value.toVariant());
        }
        else if (value.isObject())
        {
            obj->setProperty(key.toLatin1().data(), value.toObject().toVariantMap());
        }
        else if (value.isArray())
        {
            obj->setProperty(key.toLatin1().data(), value.toArray().toVariantList());
        }
        else
        {
            qDebug() << "Unsupported JSON type (" << value.type() << ") for key:" << key;
        }
    }
}

QObject* loadQmlFromFile(QQmlEngine* engine, const QString& filePath)
{
    qFatalIf(QThread::currentThread() != QCoreApplication::instance()->thread(), "Main thread only");

    auto file = QFile(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed to read QML file: " << filePath << file.errorString();
        return nullptr;
    }

    return loadQmlFromData(engine, file.readAll());
}

QObject* loadQmlFromData(QQmlEngine* engine, const QByteArray& data)
{
    qFatalIf(QThread::currentThread() != QCoreApplication::instance()->thread(), "Main thread only");

    auto component = QQmlComponent(engine);
    component.setData(data, QUrl());

    auto result = component.create();
    if (!result)
    {
        qDebug() << component.errorString();
    }

    engine->setObjectOwnership(result, QQmlEngine::CppOwnership);

    return result;
}

void writeQml(QTextStream& stream, int depth, const QString& text)
{
    for (int i = 0; i < depth; i++)
    {
        stream << "    ";
    }

    stream << text + '\n';
}

void writeQml(QTextStream& stream, int depth, const QString& name, const QString& value)
{
    if (value.isEmpty())
    {
        return;
    }

    for (int i = 0; i < depth; i++)
    {
        stream << "    ";
    }

    stream << name << ": " << encodeJsonStringLiteral(value).toUtf8() << "\n";
}

void writeQml(QTextStream& stream, int depth, const QString& name, const QJsonValue& value)
{
    if (value.isObject())
    {
        writeQml(stream, depth, name, value.toObject());
        return;
    }

    if (value.isString())
    {
        writeQml(stream, depth, name, value.toString());
        return;
    }

    if (value.isDouble())
    {
        writeQml(stream, depth, name, value.toDouble());
        return;
    }

    if (value.isBool())
    {
        writeQml(stream, depth, name, value.toBool(), false);
        return;
    }

    auto valueText = value.toString();
    if (valueText.isEmpty())
    {
        return;
    }

    for (int i = 0; i < depth; i++)
    {
        stream << "    ";
    }

    stream << name + ": ";
    stream << valueText;

    stream << '\n';
}

void writeQml(QTextStream& stream, int depth, const QString& name, const QJsonObject& value)
{
    if (value.isEmpty())
    {
        return;
    }

    for (int i = 0; i < depth; i++)
    {
        stream << "    ";
    }

    stream << name + ": ";

    QJsonDocument doc(value);
    stream << doc.toJson(QJsonDocument::Compact);

    stream << '\n';
}

void writeQml(QTextStream& stream, int depth, const QString& name, const QJsonArray& value)
{
    if (value.isEmpty())
    {
        return;
    }

    for (int i = 0; i < depth; i++)
    {
        stream << "    ";
    }

    stream << name + ": ";

    QJsonDocument doc(value);
    stream << doc.toJson(QJsonDocument::Compact);

    stream << '\n';
}

void writeQml(QTextStream& stream, int depth, const QString& name, const QVariantMap& value)
{
    writeQml(stream, depth, name, QJsonObject::fromVariantMap(value));
}

void writeQml(QTextStream& stream, int depth, const QString& name, const QVariantList& value)
{
    writeQml(stream, depth, name, QJsonArray::fromVariantList(value));
}

void writeQml(QTextStream& stream, int depth, const QString& name, const QVariant& value)
{
    if (value.isValid())
    {
        writeQml(stream, depth, name, value.toJsonValue());
    }
}

void writeQml(QTextStream& stream, int depth, const QString& name, const bool value, const bool defaultValue)
{
    if (value == defaultValue)
    {
        return;
    }

    for (int i = 0; i < depth; i++)
    {
        stream << "    ";
    }

    stream << name + ": ";
    stream << (value ? "true" : "false");
    stream << "\n";
}

void writeQml(QTextStream& stream, int depth, const QString& name, const int value, const int defaultValue)
{
    if (value == defaultValue)
    {
        return;
    }

    for (int i = 0; i < depth; i++)
    {
        stream << "    ";
    }

    stream << name + ": ";
    stream << value;
    stream << "\n";
}

void writeQml(QTextStream& stream, int depth, const QString& name, const double value)
{
    for (int i = 0; i < depth; i++)
    {
        stream << "    ";
    }

    stream << name + ": ";
    stream << value;
    stream << "\n";
}

void writeQml(QTextStream& stream, int depth, const QString& name, const QStringList& value)
{
    if (value.isEmpty())
    {
        return;
    }

    for (int i = 0; i < depth; i++)
    {
        stream << "    ";
    }

    stream << name + ": ";

    QJsonDocument doc(QJsonArray::fromStringList(value));
    stream << doc.toJson(QJsonDocument::Compact);

    stream << '\n';
}

bool ensurePath(const QString& path, bool mediaScanOnSuccess)
{
    auto result = QDir().mkpath(path);

    if (result && mediaScanOnSuccess)
    {
        mediaScan(path);
    }

    return result;
}

QString urlToLocalFile(const QString& filePathUrl)
{
    return filePathUrl.startsWith("file:") ? QUrl(filePathUrl).toLocalFile() : filePathUrl;
}

bool mediaScan(const QString& filePath)
{
#if defined(Q_OS_ANDROID)
    QJniObject jsPath = QJniObject::fromString(filePath);
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("mediaScan", "(Ljava/lang/String;)V", jsPath.object<jstring>());
    return true;
#else
    Q_UNUSED(filePath)
    return true;
#endif
}

bool locationValid(const QVariantMap& locationMap)
{
    return !locationMap.isEmpty() &&
           (locationMap["x"].toDouble() != 0.0 || locationMap["y"].toDouble() != 0.0);
}

double distanceInMeters(double lat1, double lon1, double lat2, double lon2)
{
    return QGeoCoordinate(lat1, lon1).distanceTo(QGeoCoordinate(lat2, lon2));
}

int headingInDegrees(double lat1, double lon1, double lat2, double lon2)
{
    return QGeoCoordinate(lat1, lon1).azimuthTo(QGeoCoordinate(lat2, lon2));
}

QString headingToText(int heading)
{
    struct Heading
    {
        int start, end;
        QString caption;
    };

    static const Heading headings[] =
    {
        {   0 , +15, "N"  },
        {  75 , 105, "E"  },
        { 165 , 195, "S"  },
        { 255 , 285, "W"  },
        {  15 ,  75, "NE" },
        { 105 , 165, "SE" },
        { 195 , 255, "SW" },
        { 285 , 345, "NW" },
        { 345 , 360, "N"  }
    };

    auto headingText = QString();

    heading %= 360;
    for (int i = 0; i < 9; i++)
    {
        if ((heading >= headings[i].start) && (heading <= headings[i].end))
        {
            headingText = headings[i].caption;
            break;
        }
   }

    return headingText;
}

QRectF pointToRect(double x, double y)
{
    return QRectF(x - 0.0005, y - 0.0005, 0.0010, 0.0010);
}

void extractResource(const QString& resource, const QString& targetPath)
{
    auto targetFilePath = targetPath + resource.right(resource.length() - resource.lastIndexOf("/"));
    if (!QFile::exists(targetFilePath))
    {
        auto result = QFile::copy(resource, targetFilePath);
        if (result)
        {
            auto permissions = QFile::permissions(targetFilePath);
            permissions |= QFileDevice::WriteOther;
            QFile::setPermissions(targetFilePath, permissions);
        }
        qFatalIf(!result, "Failed to extract " + resource + " to " + targetFilePath);
    }
}

bool copyPath(const QString& src, const QString& dst, const QStringList& excludes)
{
    auto dir = QDir(src);
    if (!dir.exists())
    {
        qDebug() << "src path does not exist: " << src;
        return false;
    }

    foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        auto dstPath = dst + QDir::separator() + d;
        if (dir.mkpath(dstPath))
        {
            copyPath(src+ QDir::separator() + d, dstPath, excludes);
        }
        else
        {
            qDebug() << "Failed to copy path: " << dstPath;
        }
    }

    if (!dir.mkpath(dst))
    {
        qDebug() << "dst path failed to create: " << dst;
        return false;
    }

    foreach (QString f, dir.entryList(QDir::Files))
    {
        if (excludes.contains(f, Qt::CaseInsensitive))
        {
            qDebug() << "Excluded file: " << f;
            continue;
        }

        if (!QFile::copy(src + QDir::separator() + f, dst + QDir::separator() + f))
        {
            qDebug() << "Failed to copy file: " << f;
            return false;
        }
    }

    return true;
}

QString removeTrailingSlash(const QString& string)
{
    if (!string.isEmpty() && (string.back() == '/' || string.back() == '\\'))
    {
        return removeTrailingSlash(string.left(string.length() - 1));
    }
    else
    {
        return string;
    }
}

QStringList searchStandardFolders(const QString& fileSpec)
{
    auto searchPaths = QStringList();
    searchPaths.append(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation));
    searchPaths.append(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation));

    // Add extra paths for Android and iOS.
#if defined(Q_OS_ANDROID)
    searchPaths.append("/sdcard/Download");
    searchPaths.append(Utils::androidGetExternalFilesDir());
    searchPaths.append(Utils::androidGetExternalFilesDir() + "/Download");
#elif defined(Q_OS_IOS)
    searchPaths.append(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation));
#endif

    auto result = QStringList();
    auto filterExtensions = QStringList {{ fileSpec }};
    for (auto it = searchPaths.constBegin(); it != searchPaths.constEnd(); it++)
    {
        auto files = QDir(*it).entryInfoList(filterExtensions, QDir::Files);
        for (auto it = files.constBegin(); it != files.constEnd(); it++)
        {
            result.append(it->filePath());
        }
    }

    return result;
}

bool isAndroid()
{
    return
    #if defined(Q_OS_ANDROID)
        true;
    #else
        false;
    #endif
}

QString trimUrl(const QString& url)
{
    auto resultUrl = url.trimmed();
    while (resultUrl.endsWith('/'))
    {
        resultUrl.chop(1);
    }

    return resultUrl;
}

QString redirectOnlineDriveUrl(const QString& url)
{
    auto resultUrl = QUrl(url);

    // Google Drive.
    if (resultUrl.scheme() == "https" && resultUrl.host() == "drive.google.com")
    {
        auto pathList = resultUrl.path().split('/');
        if (pathList.count() >= 4 && pathList[1] == "file" && pathList[2] == "d")
        {
            auto fileId = pathList[3];
            if (!fileId.isEmpty())
            {
                resultUrl.setPath("/uc");
                resultUrl.setQuery(QUrlQuery { {"export", "download"}, {"id", fileId} } );
            }
        }
    }
    // Dropbox.
    else if (resultUrl.scheme() == "https" && resultUrl.host() == "www.dropbox.com")
    {
        resultUrl.setHost("dl.dropboxusercontent.com");
    }

    return resultUrl.toString();
}

bool pingUrl(QNetworkAccessManager* /*networkAccessManager*/, const QString& urlString)
{
//    return httpGet(networkAccessManager, url).success;
    QUrl url(urlString);
    QTcpSocket socket;
    socket.connectToHost(url.host(), url.port(80));

    if (socket.waitForConnected())
    {
        socket.write("HEAD " + url.path().toUtf8() + " HTTP/1.1\r\nHost: " + url.host().toUtf8() + "\r\n\r\n");
        if (socket.waitForReadyRead())
        {
            QByteArray bytes = socket.readAll();
            if (bytes.contains("HTTP"))
            {
                return true;
            }
        }
    }

    return false;
}

bool networkAvailable(QNetworkAccessManager* networkAccessManager)
{
    // Quick & dirty since QNetworkConfiguration's isOnline is deprecated.
    return pingUrl(networkAccessManager, "https://google.com");
}

QByteArray makeMultiPartBoundary()
{
    return (QString("------" + QString::number(QDateTime::currentMSecsSinceEpoch())).toLatin1());
}

QNetworkRequest httpCreateRequest(const QString& url)
{
    auto request = QNetworkRequest(url);
    //request.setTransferTimeout(120000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    return request;
}

static bool s_shutdown = false;

void httpShutdown()
{
    s_shutdown = true;
}

void httpWait(QNetworkReply* reply)
{
    auto thread = QThread::currentThread();

    while (!reply->isFinished())
    {
        if (thread->isInterruptionRequested() || QCoreApplication::closingDown() || s_shutdown)
        {
            reply->abort();
            return;
        }

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

HttpResponse httpGetHead(QNetworkAccessManager* networkAccessManager, const QString& url)
{
    auto result = HttpResponse();

    auto request = httpCreateRequest();

    auto finalUrl = url;
    for (;;)
    {
        request.setUrl(finalUrl);
        auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->head(request));

        httpWait(reply.get());

        auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirectUrl.isValid())
        {
            finalUrl = redirectUrl.toString();
            continue;
        }

        result = HttpResponse::fromReply(finalUrl, reply.get());
        if (!result.success)
        {
            qDebug() << "HTTP error: " << result.status << url << result.errorString << result.data;
        }

        break;
    }

    return result;
}

HttpResponse httpGet(QNetworkAccessManager* networkAccessManager, const QString& url, const QString& username, const QString& password)
{
    auto result = HttpResponse();

    auto request = httpCreateRequest();

    if (!username.isEmpty() && !password.isEmpty())
    {
        auto auth = "Basic " + (username + ":" + password).toLocal8Bit().toBase64();
        request.setRawHeader("Authorization", auth);
    }

    auto finalUrl = url;
    for (;;)
    {
        request.setUrl(finalUrl);
        auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->get(request));

        httpWait(reply.get());

        auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirectUrl.isValid())
        {
            finalUrl = redirectUrl.toString();
            continue;
        }

        result = HttpResponse::fromReply(finalUrl, reply.get());
        if (!result.success)
        {
            qDebug() << "HTTP error: " << result.status << url << result.errorString << result.data;
        }

        break;
    }

    return result;
}

HttpResponse httpGetWithToken(QNetworkAccessManager* networkAccessManager, const QString& url, const QString& token, const QString& tokenType)
{
    auto result = HttpResponse();

    auto request = httpCreateRequest();
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Authorization", QString(tokenType + " " + token).toUtf8());

    auto finalUrl = url;
    for (;;)
    {
        request.setUrl(finalUrl);
        auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->get(request));

        httpWait(reply.get());

        auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirectUrl.isValid())
        {
            finalUrl = redirectUrl.toString();

            // Cannot have both token authorization as well as Amazon S3 authorization. Strip the token auth.
            // This is needed because ODK Central stores its forms on S3.
            if (finalUrl.contains("X-Amz-Algorithm"))
            {
                request.setRawHeader("Authorization", QByteArray());
            }

            if (finalUrl.startsWith("http:"))
            {
                qDebug() << "Failed to redirect to: " << finalUrl;
                return HttpResponse::Error("Bad redirector");
            }

            continue;
        }

        result = HttpResponse::fromReply(finalUrl, reply.get());
        if (!result.success)
        {
            qDebug() << "HTTP error: " << result.status << url << result.errorString << result.data;
        }

        break;
    }

    return result;
}

HttpResponse httpPostJson(QNetworkAccessManager* networkAccessManager, const QString& url, const QVariantMap& payloadJson, const QString& token, const QString& tokenType, bool deflate)
{
    auto request = httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!token.isEmpty())
    {
        request.setRawHeader("Authorization", QString(tokenType + " " + token).toUtf8());
    }

    auto payload = QJsonDocument::fromVariant(payloadJson).toJson();

    auto compressedPayload = QByteArray();
    if (deflate)
    {
        request.setRawHeader("Content-Encoding", "deflate");
        compressedPayload = qCompress(payload);
        compressedPayload.remove(0, 4);    // Remove the first 4 bytes which are just a header.
    }

    auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->post(request, deflate ? compressedPayload : payload));

    httpWait(reply.get());

    auto result = HttpResponse::fromReply(url, reply.get());
    if (!result.success)
    {
        qDebug() << "HTTP error: " << result.status << url << result.errorString << result.data;
    }

    return result;
}

HttpResponse httpPostData(QNetworkAccessManager* networkAccessManager, const QString& url, const QByteArray& payload, const QString& token, const QString& tokenType, bool deflate)
{
    auto request = httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");
    if (!token.isEmpty())
    {
        request.setRawHeader("Authorization", QString(tokenType + " " + token).toUtf8());
    }

    auto compressedPayload = QByteArray();
    if (deflate)
    {
        request.setRawHeader("Content-Encoding", "deflate");
        compressedPayload = qCompress(payload);
        compressedPayload.remove(0, 4);    // Remove the first 4 bytes which are just a header.
    }

    auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->post(request, deflate ? compressedPayload : payload));

    httpWait(reply.get());

    auto result = HttpResponse::fromReply(url, reply.get());
    if (!result.success)
    {
        qDebug() << "HTTP error: " << result.status << url << result.errorString << result.data;
    }

    return result;
}

HttpResponse httpPostQuery(QNetworkAccessManager* networkAccessManager, const QString& url, const QUrlQuery& query)
{
    auto request = httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");

    auto payload = query.toString(QUrl::FullyEncoded).toUtf8();

    auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->post(request, payload));

    httpWait(reply.get());

    auto result = HttpResponse::fromReply(url, reply.get());
    if (!result.success)
    {
        qDebug() << "HTTP error: " << result.status << url << result.errorString << result.data;
    }

    return result;
}

HttpResponse httpPostQuery(QNetworkAccessManager* networkAccessManager, const QString& url, const QVariantMap& queryMap)
{
    auto query = QUrlQuery();
    for (auto it = queryMap.constKeyValueBegin(); it != queryMap.constKeyValueEnd(); it++)
    {
        query.addQueryItem(it->first, it->second.toString());
    }

    return httpPostQuery(networkAccessManager, url, query);
}

HttpResponse httpPostMultiPart(QNetworkAccessManager* networkAccessManager, const QString& url, QHttpMultiPart* multiPart, const QString& token, const QString& tokenType)
{
    multiPart->setBoundary(Utils::makeMultiPartBoundary());

    auto request = httpCreateRequest(url);
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + multiPart->boundary());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    if (!token.isEmpty())
    {
        request.setRawHeader("Authorization", (tokenType + " " + token).toLocal8Bit());
    }

    auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->post(request, multiPart));

    httpWait(reply.get());

    auto result = HttpResponse::fromReply(url, reply.get());
    if (!result.success)
    {
        qDebug() << "HTTP error: " << result.status << url << result.errorString << result.data;
    }

    return result;
}

HttpResponse httpAcquireOAuthToken(QNetworkAccessManager* networkAccessManager, const QString& server, const QString& clientId, const QString& username, const QString& password, QString* outAccessToken, QString* outRefreshToken)
{
    auto query = QUrlQuery();
    query.addQueryItem("grant_type", "password");
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("username", username);
    query.addQueryItem("password", password);

    auto response = httpPostQuery(networkAccessManager, trimUrl(server) + "/oauth2/token", query);
    if (!response.success)
    {
        return response;
    }

    auto jsonObj = QJsonDocument::fromJson(response.data).object();
    *outAccessToken = jsonObj["access_token"].toString();
    *outRefreshToken = jsonObj["refresh_token"].toString();

    response.success = !outAccessToken->isEmpty() && !outRefreshToken->isEmpty();
    response.errorString = "Unknown error";

    return response;
}

HttpResponse httpRefreshOAuthToken(QNetworkAccessManager* networkAccessManager, const QString& server, const QString& clientId, const QString& refreshToken, QString* outAccessToken, QString* outRefreshToken)
{
    auto response = HttpResponse();

    if (refreshToken.isEmpty())
    {
        qDebug() << "Refresh token not found";
        return response;
    }

    auto oldRefreshToken = refreshToken;

    auto query = QUrlQuery();
    query.addQueryItem("grant_type", "refresh_token");
    query.addQueryItem("refresh_token", refreshToken);
    query.addQueryItem("client_id", clientId);

    response = httpPostQuery(networkAccessManager, trimUrl(server) + "/oauth2/token", query);
    if (!response.success)
    {
        qDebug() << "Refresh auth token failed: " << response.status << " " << response.data;
        return response;
    }

    auto jsonObj = QJsonDocument::fromJson(response.data).object();
    *outAccessToken = jsonObj["access_token"].toString();
    *outRefreshToken = jsonObj["refresh_token"].toString();

    // Keep the old refresh token if we do not have a new one.
    if (!outAccessToken->isEmpty() && outRefreshToken->isEmpty())
    {
        *outRefreshToken = oldRefreshToken;
    }

    response.success = !outAccessToken->isEmpty() && !outRefreshToken->isEmpty();
    response.errorString = "Unknown error";

    return response;
}

void enforceFolderQuota(const QString& folder, int maxFileCount)
{
    auto dir = QDir(folder);
    auto fileInfos = dir.entryInfoList(QDir::Files, QDir::Time);

    while (fileInfos.count() > maxFileCount)
    {
        QFile::remove(fileInfos.last().filePath());
        fileInfos.removeLast();
    }
}

QString createUuid()
{
    return QUuid::createUuid().toRfc4122().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QUuid combineUuidsToUuid(const QString& namespaceUuid, const QString& nameUuid)
{
    // Create a SHA-1 hash of the namespace and name.
    auto crypto = QCryptographicHash(QCryptographicHash::Sha1);
    crypto.addData(namespaceUuid.toLatin1());
    crypto.addData(nameUuid.toLatin1());
    auto hash = crypto.result();
    hash[6] = (hash[6] & 0x0F) | 0x50; // version 5
    hash[8] = (hash[8] & 0x3F) | 0x80; // variant is RFC 4122

    // Return the leftmost 16 bytes.
    return QUuid::fromRfc4122(hash.left(16));
}

QString combineUuids(const QString& namespaceUuid, const QString& nameUuid)
{
    return combineUuidsToUuid(namespaceUuid, nameUuid).toString(QUuid::Id128);
}

QString addBracesToUuid(const QString& uuid)
{
    auto result = uuid;
    result.insert(0, '{');
    result.insert(9, '-');
    result.insert(14, '-');
    result.insert(19, '-');
    result.insert(24, '-');
    result.append('}');

    return result;
}

QString encodeJsonStringLiteral(const QString& value)
{
    return QString(QJsonDocument(QJsonArray() << value).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
}

struct ImageMetadata
{
    bool empty = true;
    Exiv2::ExifData exifData;
    Exiv2::IptcData iptcData;
    QString comments;
};

ImageMetadata loadExifData(const QString& filePath)
{
    auto result = ImageMetadata();

    try
    {
        auto image = Exiv2::ImageFactory::open((const char*)(QFile::encodeName(filePath)).constData());
        image->readMetadata();

        result.exifData = image->exifData();
        result.iptcData = image->iptcData();
        result.comments = image->comment().data();

        result.empty = result.exifData.empty() && result.iptcData.empty() && result.comments.isEmpty();
    }
    catch (Exiv2::Error& e)
    {
        qDebug() << "Failed to load exif data" << filePath << e.what();
    }

    return result;
}

bool saveExifData(const QString& filePath, const ImageMetadata& imageMetadata)
{
    auto result = false;
    try
    {
        auto image = Exiv2::ImageFactory::open((const char*)(QFile::encodeName(filePath)).constData());
        image->setExifData(imageMetadata.exifData);
        image->setIptcData(imageMetadata.iptcData);
        image->setComment(imageMetadata.comments.toStdString());
        image->writeMetadata();

        result = true;
    }
    catch (Exiv2::Error& e)
    {
        qDebug() << "Failed to save exif data" << filePath << e.what();
    }

    return result;
}

bool resizeImageFile(const QString& filePath, int resolutionX, int resolutionY)
{
    auto imageReader = QImageReader(filePath);
    imageReader.setAutoTransform(true);

    auto image = imageReader.read();
    auto pixmap = QPixmap::fromImage(image);

    if (resolutionY == -1)
    {
        resolutionY = resolutionX;
    }

    // Unspecified means 4k.
    if (resolutionX == -1)
    {
        resolutionX = 3840;
        resolutionY = 2160;
    }

    auto longEdge = std::max(pixmap.width(), pixmap.height());
    if (resolutionX > 0 && longEdge > resolutionX)
    {
        pixmap = pixmap.scaledToWidth(resolutionX, Qt::SmoothTransformation);
    }
    else if (resolutionY > 0 && longEdge > resolutionY)
    {
        pixmap = pixmap.scaledToHeight(resolutionY, Qt::SmoothTransformation);
    }

    auto exifData = loadExifData(filePath);

    auto result = pixmap.save(filePath);
    if (result)
    {
        // Auto-transform re-oriented the image, but the metadata has the old value.
        exifData.exifData["Exif.Image.Orientation"] = uint16_t(1);
        if (!saveExifData(filePath, exifData))
        {
            qDebug() << "Failed to write jpeg exif header";
        }
    }

    return result;
}

bool reorientImageFile(const QString& filePath, int resolutionX, int resolutionY, bool isBackFace, int correctionAngle)
{
    QImageReader imageReader(filePath);
    QImage image;
    QPixmap pixmap;

#if defined(Q_OS_IOS)
    {
        imageReader.setAutoTransform(false);
        image = imageReader.read();
        pixmap = QPixmap::fromImage(image);

        auto screen = QGuiApplication::primaryScreen();
        auto screenAngle = screen->angleBetween(screen->nativeOrientation(), screen->orientation());
        auto rotation = 0;

        if (isBackFace)
        {
            rotation = (360 - correctionAngle + screenAngle) % 360;
        }
        else
        {
            rotation = (correctionAngle - screenAngle + 180) % 360;
        }

        if (rotation != 0)
        {
            QTransform transform;
            transform.rotate(rotation);
            pixmap = pixmap.transformed(transform);
        }
    }
#else
    Q_UNUSED(isBackFace)
    Q_UNUSED(correctionAngle)

    imageReader.setAutoTransform(true);
    image = imageReader.read();
    pixmap = QPixmap::fromImage(image);
#endif

    if (resolutionY == -1)
    {
        resolutionY = resolutionX;
    }

    auto longEdge = std::max(pixmap.width(), pixmap.height());
    if (resolutionX > 0 && longEdge > resolutionX)
    {
        pixmap = pixmap.scaledToWidth(resolutionX, Qt::SmoothTransformation);
    }
    else if (resolutionY > 0 && longEdge > resolutionY)
    {
        pixmap = pixmap.scaledToHeight(resolutionY, Qt::SmoothTransformation);
    }

    // Horizontal mirror.
    if (!isBackFace)
    {
        pixmap = pixmap.transformed(QTransform().scale(-1, 1));
    }

    auto exifData = loadExifData(filePath);
    auto result = pixmap.save(filePath);
    if (result)
    {
        exifData.exifData["Exif.Image.Orientation"] = static_cast<uint16_t>(1); // ORIENTATION_NORMAL

        if (!saveExifData(filePath, exifData))
        {
            qDebug() << "Failed to write jpeg exif header";
        }
    }

    return result;
}

QString convertImageToJpeg(const QString& filePath)
{
    auto fileInfo = QFileInfo(filePath);
    auto suffix = fileInfo.suffix().toLower();
    if (suffix == "jpg" || suffix == "jpeg")
    {
        // No change.
        return filePath;
    }

    QImageReader imageReader(filePath);
    if (!imageReader.canRead())
    {
        qDebug() << "Failed to convert to jpeg: " << filePath;
        return filePath;
    }

    auto image = imageReader.read();
    auto pixmap = QPixmap::fromImage(image);

    auto exifData = loadExifData(filePath);

    QString filePathJpg = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".jpg";
    if (!pixmap.save(filePathJpg))
    {
        qDebug() << "Failed to write jpeg: " << filePathJpg;
        return "";
    }

    if (!exifData.empty && !saveExifData(filePathJpg, exifData))
    {
        qDebug() << "Failed to write exif to jpeg: " << filePathJpg;
        // Fall through.
    }

    // Remove non-jpeg.
    QFile::remove(filePath);

    // Return jpeg filename.
    return filePathJpg;
}

QStringList packTags(const QVariantMap& tags, const QStringList& allTags)
{
    auto result = QStringList();

    for (auto tagIt = allTags.constBegin(); tagIt != allTags.constEnd(); tagIt++)
    {
        if (tags.value(*tagIt).toBool())
        {
            result.append(*tagIt);
        }
    }

    return result;
}

QVariantMap unpackTags(const QStringList& tags, const QStringList& allTags)
{
    auto result = QVariantMap();

    for (auto tagIt = allTags.constBegin(); tagIt != allTags.constEnd(); tagIt++)
    {
        result.insert(*tagIt, tags.contains(*tagIt));
    }

    return result;
}

constexpr char IMAGE_TAGS_PREFIX[] = "TAGS=";

QStringList getImageTags(const QString& filePath)
{
    auto result = QStringList();

    if (!QFile::exists(filePath))
    {
        return result;
    }

    try
    {
        auto exifData = loadExifData(filePath);

        auto comments = exifData.comments.split('\n');
        comments.removeAll("");
        comments.removeAll(IMAGE_TAGS_PREFIX);

        for (auto lineIt = comments.constBegin(); lineIt != comments.constEnd(); lineIt++)
        {
            if (lineIt->startsWith(IMAGE_TAGS_PREFIX))
            {
                result = lineIt->mid(QString(IMAGE_TAGS_PREFIX).length()).split(',');
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        qDebug() << e.what();
        return result;
    }

    return result;
}

bool setImageTags(const QString& filePath, const QStringList& tags)
{
    auto result = false;

    if (!QFile::exists(filePath))
    {
        return false;
    }

    try
    {
        auto exifData = loadExifData(filePath);
        auto comments = exifData.comments.split('\n');
        comments.removeAll("");

        for (auto lineIt = comments.constBegin(); lineIt != comments.constEnd(); lineIt++)
        {
            if (lineIt->startsWith(IMAGE_TAGS_PREFIX))
            {
                comments.removeOne(*lineIt);
                break;
            }
        }

        if (!tags.isEmpty())
        {
            comments.append(IMAGE_TAGS_PREFIX + tags.join(','));
        }

        exifData.comments = comments.join('\n').toUtf8();
        result = saveExifData(filePath, exifData);
    }
    catch (const std::exception& e)
    {
        qDebug() << e.what();
        return false;
    }

    return result;
}

QColor colorFromString(const QString& value)
{
    return value.startsWith('#') ? QColor(value) : QColor('#' + value);
}

bool renderMapCallout(const QString& iconFilePath, const QString& targetFilePath, const QColor& color, const QColor& outlineColor, int size)
{
    if (!QFile::exists(iconFilePath))
    {
        qDebug() << "renderIcon: file not found";
        return false;
    }

    auto spacer = size / 8;
    auto iconSize = size - spacer;

    auto image = QImage(QSize(size, size), QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);

    QPainterPath path;
    auto iconRect = QRect(spacer / 2, 0, iconSize, iconSize);
    path.moveTo(iconRect.topLeft());
    path.lineTo(iconRect.topRight());
    path.lineTo(iconRect.bottomRight());
    path.lineTo(iconRect.center().x() + spacer, iconRect.bottom());
    path.lineTo(iconRect.center().x(), size - 1);
    path.lineTo(iconRect.center().x() - spacer, iconRect.bottom());
    path.lineTo(iconRect.bottomLeft());
    path.lineTo(iconRect.topLeft());
    path.closeSubpath();

    painter.setPen(outlineColor);
    painter.setBrush(color);
    painter.drawPath(path);

    auto icon = QPixmap();
    iconRect.adjust(2, 2, -2, -2);

    renderSquarePixmap(&icon, iconFilePath, iconRect.width(), iconRect.height());

    auto targetRect = icon.rect();
    targetRect.moveCenter(iconRect.center());
    painter.drawPixmap(targetRect, icon);

    return image.save(targetFilePath);
}

bool renderMapMarker(const QString& url, const QString& targetFilePath, const QColor& color, const QColor& outlineColor, int width, int height)
{
    QFile file(url);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    auto svgData = QString(file.readAll());
    svgData.replace("fill:#000000", "fill:" + color.name(QColor::HexRgb));
    svgData.replace("stroke:#ffffff", "stroke:" + outlineColor.name(QColor::HexRgb));
    QSvgRenderer renderer(svgData.toLatin1());

    auto image = QImage(QSize(width, width), QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    renderer.render(&painter, QRect(0, 0, width, height));

    return image.save(targetFilePath);
}

QImage renderSvgToImage(const QString& url, int width, int height)
{
    QFile file(url);
    if (!file.open(QFile::ReadOnly))
    {
        return QImage();
    }

    QSvgRenderer renderer(file.readAll());
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 0));

    QPainter painter(&image);
    renderer.render(&painter);

    return image;
}

QString renderSvgToPng(const QString& url, int width, int height)
{
    QFile file(url);
    if (!file.open(QFile::ReadOnly))
    {
        return QString();
    }

    QSvgRenderer renderer(file.readAll());
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 0));

    QPainter painter(&image);
    renderer.render(&painter);

    QBuffer buffer;
    image.save(&buffer);

    return buffer.data();
}

bool renderSvgToPngFile(const QString& url, const QString& targetFilePath, int width, int height)
{
    QFile file(url);
    if (!file.open(QFile::ReadOnly))
    {
        return false;
    }

    QSvgRenderer renderer(file.readAll());
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(QColor(0, 0, 0, 0));

    QPainter painter(&image);
    renderer.render(&painter);

    image.save(targetFilePath);

    return true;
}

bool renderSquarePixmap(QPixmap* pixmap, const QString& filePath, int width, int height, int angle)
{
    if (pixmap->width() != width || pixmap->height() != height)
    {
        *pixmap = QPixmap::fromImage(QImage(width, height, QImage::Format_ARGB32));
    }

    QImageReader imageReader(filePath);
    imageReader.setAutoTransform(true);
    auto scaledSize = imageReader.size().scaled(QSize(width, height), Qt::KeepAspectRatio);
    auto targetRect = QRectF((width - scaledSize.width()) / 2, (height - scaledSize.height()) / 2, scaledSize.width(), scaledSize.height());

    pixmap->fill(Qt::transparent);
    QPainter painter(pixmap);

    if (angle != 0)
    {
        QTransform transform;
        transform.translate(width / 2, height / 2);
        transform.rotate(angle);
        transform.translate(-width / 2, -height / 2);
        painter.setTransform(transform);
    }

    auto suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == "svg")
    {
        QFile file(filePath);
        if (!file.open(QFile::ReadOnly))
        {
            return false;
        }

        QSvgRenderer renderer(file.readAll());
        renderer.render(&painter, targetRect);
    }
    else
    {
        auto pixmap = QPixmap::fromImage(imageReader.read());

        if (pixmap.width() > pixmap.height())
        {
            pixmap = pixmap.scaledToWidth(width, Qt::SmoothTransformation);
        }
        else
        {
            pixmap = pixmap.scaledToHeight(height, Qt::SmoothTransformation);
        }

        painter.drawPixmap(targetRect.toRect(), pixmap);
    }

    return true;
}

QString renderPointsToSvg(const QVariantList& points, const QColor& color, int penWidth, bool fill)
{
    auto boundingRect = QRectF();
    for (auto it = points.constBegin(); it != points.constEnd(); it++)
    {
        auto coord = it->toList();
        auto x = coord[0].toDouble();
        auto y = -coord[1].toDouble();
        boundingRect = boundingRect.united(QRectF(x, y, 0.000000001, 0.000000001));
    }

    auto w = boundingRect.width();
    auto h = boundingRect.height();
    if (w == 0 || h == 0)
    {
        return QString();
    }

    auto border = 8;
    auto scale = std::min((256.0 - border * 2) / w, (256.0 - border * 2)  / h);

    auto left = boundingRect.left();
    auto top = boundingRect.top();
    auto width = boundingRect.width() * scale + border * 2;
    auto height = boundingRect.height() * scale + border * 2;

    QBuffer buffer;
    QSvgGenerator generator;
    generator.setOutputDevice(&buffer);
    generator.setSize(QSize(width, height));
    generator.setViewBox(QRect(0, 0, width, height));
    generator.setTitle("CyberTracker points");

    QPainter painter;
    painter.begin(&generator);
    painter.setPen(QPen(color, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(QColor(color.red(), color.green(), color.blue(), 64)));
    painter.setRenderHint(QPainter::Antialiasing, true);

    auto polygon = QPolygonF();
    for (auto it = points.constBegin(); it != points.constEnd(); it++)
    {
        auto coord = it->toList();
        auto x = (coord[0].toDouble() - left) * scale + border;
        auto y = -(coord[1].toDouble() + top) * scale + border;
        polygon.append(QPointF(x, y));
    }

    if (fill)
    {
        painter.drawPolygon(polygon);
    }
    else
    {
        painter.drawPolyline(polygon);
    }

    painter.end();

    return buffer.data();
}

QString renderSketchToSvg(const QVariant& sketch, QColor penColor, int penWidth)
{
    auto lines = variantToSketch(sketch);
    auto boundingRect = QRect();
    for (auto it = lines.constBegin(); it != lines.constEnd(); it++)
    {
        boundingRect = boundingRect.united(it->boundingRect());
    }

    auto border = 8;
    auto width = boundingRect.width() + border * 2;
    auto height = boundingRect.height() + border * 2;

    QBuffer buffer;
    QSvgGenerator generator;
    generator.setOutputDevice(&buffer);
    generator.setSize(QSize(width, height));
    generator.setViewBox(QRect(0, 0, width, height));
    generator.setTitle("CyberTracker sketch");

    QPainter painter;
    painter.begin(&generator);
    painter.setPen(QPen(penColor, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (auto it = lines.begin(); it != lines.end(); it++)
    {
        it->translate(-boundingRect.left() + border, -boundingRect.top() + border);
        painter.drawPolyline(*it);
    }
    painter.end();

    return buffer.data();
}

bool renderSketchToPng(const QVariant& sketch, const QString& filePath, QColor penColor, int penWidth)
{
    auto lines = variantToSketch(sketch);
    auto boundingRect = QRect();
    for (auto it = lines.constBegin(); it != lines.constEnd(); it++)
    {
        boundingRect = boundingRect.united(it->boundingRect());
    }

    auto border = 8;
    auto width = boundingRect.width() + border * 2;
    auto height = boundingRect.height() + border * 2;

    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setPen(QPen(penColor, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (auto it = lines.begin(); it != lines.end(); it++)
    {
        it->translate(-boundingRect.left() + border, -boundingRect.top() + border);
        painter.drawPolyline(*it);
    }

    return image.save(filePath);
}

QVariant sketchToVariant(const QList<QPolygon>& sketch)
{
    auto result = QVariantList();

    for (auto lineIt = sketch.constBegin(); lineIt != sketch.constEnd(); lineIt++)
    {
        auto part = QVariantList();
        for (auto itPoint = lineIt->constBegin(); itPoint != lineIt->constEnd(); itPoint++)
        {
            part.append(QVariant(QVariantList { itPoint->x(), itPoint->y() }));
        }

        result.append(QVariant(part));
    }

    return result;
}

QList<QPolygon> variantToSketch(const QVariant& sketchVariant)
{
    QList<QPolygon> result;
    auto sketch = sketchVariant.toList();

    for (auto lineIt = sketch.constBegin(); lineIt != sketch.constEnd(); lineIt++)
    {
        auto part = QPolygon();
        auto line = lineIt->toList();
        for (auto itPoint = line.constBegin(); itPoint != line.constEnd(); itPoint++)
        {
            auto pointArray = itPoint->toList();
            part.append(QPoint(pointArray[0].toDouble(), pointArray[1].toDouble()));
        }

        result.append(part);
    }

    return result;
}

QImage renderQRCode(const QString& data, int width, int height, QColor backgroundColor, QColor foregroundColor)
{
    if (data.isEmpty())
    {
        return QImage();
    }

    try
    {
        ZXing::MultiFormatWriter writer = ZXing::MultiFormatWriter(ZXing::BarcodeFormat::QRCode);
        writer.setMargin(1);
        writer.setEccLevel(8);

        auto qrCodeMatrix = writer.encode(data.toStdString(), width, height);

        auto image = QImage(width, height, QImage::Format_ARGB32);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (qrCodeMatrix.get(x, y))
                {
                    image.setPixelColor(x, y, foregroundColor);
                }
                else
                {
                    image.setPixelColor(x, y, backgroundColor);
                }
            }
        }

        return image;
    }
    catch (const std::exception& e)
    {
        qDebug() << e.what();
    }
    catch (...)
    {
    }

    return QImage();
}

QString renderQRCodeToPng(const QString& data, int width, int height)
{
    auto image = renderQRCode(data, width, height);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    return buffer.data().toBase64();
}

void buildListCombinationsInternal(const QList<QStringList>& lists, int listIndex, QStringList vector, QList<QStringList>& result)
{
    if (listIndex >= lists.count())
    {
        result.append(vector);
        return;
    }

    auto list = lists[listIndex];
    for (auto it = list.constBegin(); it != list.constEnd(); it++)
    {
        auto newVector = QStringList(vector);
        newVector.append(*it);
        buildListCombinationsInternal(lists, listIndex + 1, newVector, result);
    }
}

QList<QStringList> buildListCombinations(const QList<QStringList>& lists)
{
    auto result = QList<QStringList>();
    buildListCombinationsInternal(lists, 0, QStringList(), result);
    return result;
}

QString extractText(const QString& value, const QString& prefix, const QString& suffix)
{
    auto index1 = value.indexOf(prefix);
    auto index2 = suffix.length() == 0 ? value.length() : value.indexOf(suffix, index1 + prefix.length());
    return value.mid(index1 + prefix.length(), index2 - index1 - prefix.length());
}

QString lastPathComponent(const QString& path)
{
    auto slashIndex = path.lastIndexOf('/');
    if (slashIndex == -1)
    {
        return path;
    }
    else
    {
        return path.right(path.length() - slashIndex - 1);
    }
}

bool compareLastPathComponents(const QString& path1, const QString& path2)
{
    if (path1 == path2)
    {
        return true;
    }

    auto view1 = QStringView(path1);
    auto slashIndex1 = view1.lastIndexOf('/');
    if (slashIndex1 != -1)
    {
        view1 = view1.right(view1.length() - slashIndex1 - 1);
    }

    auto view2 = QStringView(path2);
    auto slashIndex2 = view2.lastIndexOf('/');
    if (slashIndex2 != -1)
    {
        view2 = view2.right(view2.length() - slashIndex2 - 1);
    }

    return view1 == view2;
}

QVariant getParam(const QVariantMap& params, const QString& key, const QVariant& defaultValue)
{
    if (!key.contains('.'))
    {
        return params.value(key, defaultValue);
    }

    // Drill through objects, so keys can be of the form X.Y.Z = v.
    auto curr = params;
    auto keys = key.split('.');
    for (auto i = 0; i < keys.count() - 1; i++)
    {
        curr = curr[keys[i]].toMap();
        if (curr.isEmpty())
        {
            return defaultValue;
        }
    }

    return curr.value(keys.last(), defaultValue);
}

QString androidGetExternalFilesDir()
{
#if defined (Q_OS_ANDROID)
    static QString s_externalFilesDir;
    if (s_externalFilesDir.isEmpty())
    {
        auto androidContext = QJniObject(QNativeInterface::QAndroidApplication::context());
        QJniObject dir = QJniObject::fromString(QString(""));
        QJniObject path = androidContext.callObjectMethod("getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;", dir.object());
        s_externalFilesDir = path.toString();
    }

    return s_externalFilesDir;
#else
    qFatalIf(true, "Android only API called on non-Android platform");
    return "";
#endif
}

void androidHideNavigationBar()
{
#if defined (Q_OS_ANDROID)
    //QtAndroid::runOnAndroidThread([=]()
    //{
        QJniObject window = QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("getWindow", "()Landroid/view/Window;");
        QJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
        decorView.callMethod<void>("setSystemUiVisibility", "(I)V", 0x2 /*SYSTEM_UI_FLAG_HIDE_NAVIGATION*/);
    //});
#endif
}

int androidSdkVersion()
{
#if defined (Q_OS_ANDROID)
    return QJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");
#else
    return 0;
#endif
}

QString classicRoot()
{
#if defined(Q_OS_IOS)
    return QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation) + "/CTClassic";
#elif defined(Q_OS_ANDROID)
    return androidGetExternalFilesDir() + "/cybertracker";
#else
    return QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DesktopLocation) + "/CT";
#endif
}

QTimeZone detectTimeZone(double x, double y)
{
    static ZoneDetect* s_zoneDb = nullptr;

    if (!s_zoneDb)
    {
        static QFile s_zoneFile(":/timezone21.bin");
        static uchar* s_zoneFileMap = nullptr;

        if (s_zoneFile.open(QIODevice::ReadOnly))
        {
            auto fileSize = static_cast<uint>(s_zoneFile.size());
            s_zoneFileMap = s_zoneFile.map(0, fileSize);
            s_zoneDb = ZDOpenDatabaseFromMemory(s_zoneFileMap, fileSize);
        }
    }

    qFatalIf(!s_zoneDb, "Failed to open time zone db");

    // Cache: if the point is within the safe zone of the last lookup, the timezone is the same.
    static float s_lastLat = 0, s_lastLon = 0, s_lastSafeZone = 0;
    static QTimeZone s_lastTimeZone;

    if (s_lastSafeZone > 0)
    {
        auto dLat = static_cast<float>(y) - s_lastLat;
        auto dLon = static_cast<float>(x) - s_lastLon;
        if (dLat * dLat + dLon * dLon < s_lastSafeZone * s_lastSafeZone)
        {
            return s_lastTimeZone;
        }
    }

    auto safeZone = static_cast<float>(0);
    auto results = ZDLookup(s_zoneDb, static_cast<float>(y), static_cast<float>(x), &safeZone);
    auto result = results[0];
    auto ianaId = QString();

    if ((result.lookupResult != ZD_LOOKUP_END) && (result.numFields >= 2))
    {
        auto zonePrefix = QString(result.data[0]);
        auto zoneId = QString(result.data[1]);
        ianaId = zonePrefix + zoneId;
    }
    else
    {
        ianaId = QDateTime::currentDateTime().timeZone().id();
    }

    ZDFreeResults(results);

    s_lastLat = static_cast<float>(y);
    s_lastLon = static_cast<float>(x);
    s_lastSafeZone = safeZone;
    s_lastTimeZone = QTimeZone(ianaId.toLatin1());

    return s_lastTimeZone;
}

QDateTime currentDateTime()
{
    auto now = QDateTime::currentDateTime();
    now.setTimeZone(now.timeZone());
    return now;
}

QString encodeTimestamp(const QDateTime& dateTime)
{
    return dateTime.toString(Qt::DateFormat::ISODateWithMs);
}

QDateTime decodeTimestamp(const QString& isoDateTime)
{
    if (isoDateTime.contains("T"))
    {
        return QDateTime::fromString(isoDateTime, Qt::DateFormat::ISODateWithMs);
    }
    else
    {
        return QDateTime::fromString(isoDateTime + "T00:00:00Z", Qt::DateFormat::ISODateWithMs);
    }
}

qint64 decodeTimestampSecs(const QString& isoDateTime)
{
    return decodeTimestamp(isoDateTime).toSecsSinceEpoch();
}

qint64 decodeTimestampMSecs(const QString& isoDateTime)
{
    return decodeTimestamp(isoDateTime).toMSecsSinceEpoch();
}

qint64 timestampDeltaMs(const QString& startISODateTime, const QString& stopISODateTime)
{
    return decodeTimestampMSecs(stopISODateTime) - decodeTimestampMSecs(startISODateTime);
}

QString makeUniqueFilename(const QString& prefix, const QDateTime& dateTime, const QString& suffix)
{
    return QString("%1%2%3").arg(prefix, dateTime.toString("yyyy-MM-ddThh_mm_ss_zzz"), suffix);
}

QString makeUniqueFilename(const QString& prefix, const QString& isoDateTime, const QString& suffix)
{
    return makeUniqueFilename(prefix, Utils::decodeTimestamp(isoDateTime), suffix);
}

QStringList stringToList(const QString& string)
{
    return string.isEmpty() ? QStringList() : string.split(';');
}

QString listToString(const QStringList& list)
{
    auto listTemp = list;
    listTemp.removeAll("");
    return listTemp.join(';');
}

double distanceInMeters(const QVariantList& points)
{
    auto result = 0.0;

    auto lastX = 0.0;
    auto lastY = 0.0;
    for (auto it = points.constBegin(); it != points.constEnd(); it++)
    {
        auto coords = it->toList();
        auto x = coords[0].toDouble();
        auto y = coords[1].toDouble();

        if (it != points.constBegin())
        {
            result += distanceInMeters(y, x, lastY, lastX);
        }

        lastX = x;
        lastY = y;
    }

    return result;
}

QVariantMap computeExtent(const QVariantList& points)
{
    auto extentRect = QRectF();
    for (auto it = points.constBegin(); it != points.constEnd(); it++)
    {
        auto coord = it->toList();
        auto x = coord[0].toDouble();
        auto y = coord[1].toDouble();
        extentRect = extentRect.united(pointToRect(x, y));
    }

    return QVariantMap
    {
        { "spatialReference", QVariantMap {{ "wkid", 4326 }} },
        { "xmin", extentRect.left() },
        { "ymin", extentRect.top() },
        { "xmax", extentRect.right() },
        { "ymax", extentRect.bottom() }
    };
}

QVariantMap rectFToMap(const QRectF& rect)
{
    return QVariantMap { { "xmin", rect.left() }, { "ymin", rect.top() }, { "xmax", rect.right() }, { "ymax", rect.bottom() }};
}

QRectF rectFFromMap(const QVariantMap& map)
{
    auto xmin = map["xmin"].toDouble();
    auto ymin = map["ymin"].toDouble();
    auto xmax = map["xmax"].toDouble();
    auto ymax = map["ymax"].toDouble();

    return QRectF(QPointF(xmin, ymin), QPointF(xmax, ymax));
}

QVariantList pointsFromXml(const QString& xml)
{
    auto points = xml.split(';');
    auto result = QVariantList();

    if (!xml.isEmpty())
    {
        for (auto it = points.constBegin(); it != points.constEnd(); it++)
        {
            auto point = it->split(' ');
            auto x = point[1].toDouble();
            auto y = point[0].toDouble();
            auto z = points.count() > 2 ? point[2].toDouble() : 0.0;
            auto a = points.count() > 3 ? point[3].toDouble() : 0.0;

            result.append(QVariant(QVariantList { x, y, z, a }));
        }
    }

    return result;
}

QString pointsToXml(const QVariantList& points)
{
    auto result = QStringList();

    for (auto it = points.constBegin(); it != points.constEnd(); it++)
    {
        auto point = it->toList();
        auto x = point[0].toDouble();
        auto y = point[1].toDouble();
        auto z = point[2].toDouble();
        auto a = point[3].toDouble();

        result.append(QString("%1 %2 %3 %4").arg(
            QString::number(y, 'g', 10),
            QString::number(x, 'g', 10),
            QString::number(z, 'g', 10),
            QString::number(std::isnan(a) ? 0.0 : a, 'f', 2)));
    }

    return result.join(';');
}

QVariantMap pointsToGeoJsonLine(const QVariantList& points)
{
    auto coords = QVariantList {};
    for (auto pointIt = points.constBegin(); pointIt != points.constEnd(); pointIt++)
    {
        auto point = pointIt->toList();
        auto x = point[0].toDouble();
        auto y = point[1].toDouble();
        coords.append(QVariant(QVariantList { x, y }));
    }

    return QVariantMap {
        { "type", "Feature" },
        { "properties", QVariantMap {{ "source", "gps" }} },
        { "geometry", QVariantMap {{ "type", "LineString" }, { "coordinates", coords }} }
    };
}

QVariantMap pointsToGeoJsonArea(const QVariantList& points)
{
    auto coords = QVariantList {};
    for (auto pointIt = points.constBegin(); pointIt != points.constEnd(); pointIt++)
    {
        auto point = pointIt->toList();
        auto x = point[0].toDouble();
        auto y = point[1].toDouble();
        coords.append(QVariant(QVariantList { x, y }));
    }

    // Close the loop.
    if (coords.constFirst() != coords.constLast())
    {
        coords.append(coords.constFirst());
    }

    return QVariantMap {
        { "type", "Feature" },
        { "properties", QVariantMap {{ "source", "gps"}} },
        { "geometry", QVariantMap {
            { "type", "Polygon" },
            { "coordinates", QVariantList { QVariant(coords) }} }
        }
    };
}

static auto FILE_PATH_MAP = QMap<QString, QString>
{
    { QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DesktopLocation), "%DESKTOP%" },
    { QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation), "%APPDATA%" },
    { QStandardPaths::writableLocation(QStandardPaths::StandardLocation::CacheLocation), "%CACHE%" },
    { QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DownloadLocation), "%DOWNLOAD%" },
    { QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DocumentsLocation), "%DOCUMENTS%" },
    { QStandardPaths::writableLocation(QStandardPaths::StandardLocation::PicturesLocation), "%PICTURES%" },
    #if defined (Q_OS_ANDROID)
        { androidGetExternalFilesDir(), "%ANDROID_EXTERNAL%" }
    #endif
};

QString encodeFilePath(const QString& filePath)
{
    for (auto it = FILE_PATH_MAP.constKeyValueBegin(); it != FILE_PATH_MAP.constKeyValueEnd(); it++)
    {
        if (filePath.startsWith(it->first))
        {
            auto result = filePath;
            return result.replace(it->first, it->second);
        }
    }

    return filePath;
}

QString decodeFilePath(const QString& filePath)
{
    for (auto it = FILE_PATH_MAP.constKeyValueBegin(); it != FILE_PATH_MAP.constKeyValueEnd(); it++)
    {
        if (filePath.startsWith(it->second))
        {
            auto result = filePath;
            return result.replace(it->second, it->first);
        }
    }

    // Hack to recover file paths on iOS.
    if (!QFile::exists(filePath))
    {
        for (auto it = FILE_PATH_MAP.constKeyValueBegin(); it != FILE_PATH_MAP.constKeyValueEnd(); it++)
        {
            if (filePath.length() <= it->first.length())
            {
                continue;
            }

            auto testFilePath = QString(filePath).replace(0, it->first.length(), it->first);
            if (QFile::exists(testFilePath))
            {
                qDebug() << "Decoding '" << filePath << "' as '" << testFilePath << "'";
                return testFilePath;
            }
        }
    }

    // No decoding.
    return filePath;
}

QMimeDatabase* mimeDatabase()
{
    static QMimeDatabase s_mimeDatabase;

    return &s_mimeDatabase;
}

QString detectMimeType(const QString& filePath)
{
    auto name = mimeDatabase()->mimeTypeForFile(filePath).name();
    if (name == "audio/vnd.wave")
    {
        name = "audio/wav";
    }

    return name;
}

QString detectFileSuffix(const QString& filePath)
{
    return mimeDatabase()->mimeTypeForFile(filePath).preferredSuffix().toLower();
}

QString sanitizeForFilename(const QString& input)
{
    QString sanitized = input.trimmed();

    // Remove invalid characters.
    auto invalidCharacters = QRegularExpression("[\\\\/:*?\"<>|]");
    sanitized.replace(invalidCharacters, "_");

    // Ensure it does not end with a period.
    while (sanitized.endsWith('.'))
    {
        sanitized.chop(1);
    }

    // Check for reserved names and append an underscore if it matches.
    auto reservedNames = QStringList { "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
    if (reservedNames.contains(sanitized, Qt::CaseInsensitive))
    {
        sanitized += "_";
    }

    return sanitized;
}

void enumFiles(const QString& folder, std::function<void(const QFileInfo& fileInfo)> callback)
{
    auto fileInfos = QDir(folder).entryInfoList(QDir::Files, QDir::Name);
    for (auto fileInfoIt = fileInfos.constBegin(); fileInfoIt != fileInfos.constEnd(); fileInfoIt++)
    {
        callback(*fileInfoIt);
    }
}

void enumFolders(const QString& folder, std::function<void(const QFileInfo& fileInfo)> callback)
{
    auto dirInfos = QDir(folder).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (auto dirInfoIt = dirInfos.constBegin(); dirInfoIt != dirInfos.constEnd(); dirInfoIt++)
    {
        callback(*dirInfoIt);
    }
}

QString computeBufferHash(const QByteArray& buffer)
{
    auto cryptHash = std::unique_ptr<QCryptographicHash>(new QCryptographicHash(QCryptographicHash::Md5));
    cryptHash->addData(buffer);

    return QString(cryptHash->result().toHex());
}

QString computeFileHash(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
    {
        qDebug() << "Failed to open file: " << file.errorString();
        return "";
    }

    auto cryptHash = std::unique_ptr<QCryptographicHash>(new QCryptographicHash(QCryptographicHash::Md5));
    while (!file.atEnd())
    {
        cryptHash->addData(file.read(1024 * 1024));
    }

    return QString(cryptHash->result().toHex());
}

QString computeFolderHash(const QString& folder)
{
    auto fileItems = QStringList();
    enumFiles(folder, [&](const QFileInfo& fileInfo)
    {
        fileItems.append(fileInfo.fileName().toLower());

        auto fileSize = fileInfo.size();
        if (fileSize < 65536)
        {
            fileItems.append(computeFileHash(fileInfo.filePath()));
        }
        else
        {
            fileItems.append(QString::number(fileSize));
        }
    });

    if (fileItems.isEmpty())
    {
        return QString();
    }

    return QString(QCryptographicHash::hash((fileItems.join('_').toLatin1()),QCryptographicHash::Md5).toHex());
}

bool isMapLayerSuffix(const QString& suffix)
{
    return isMapLayerVector(suffix) || isMapLayerRaster(suffix);
}

bool isMapLayerVector(const QString& suffix)
{
    auto s_vectorExtensions = QStringList { "shp", "kml", "geojson" };
    return s_vectorExtensions.contains(suffix.toLower());
}

bool isMapLayerRaster(const QString& suffix)
{
    auto s_rasterExtensions = QStringList { "tpk", "vtpk", "img", "i12", "dt0", "dt1", "dt2", "tc2", "geotiff", "tif", "tiff", "hr1", "jpg", "jpeg", "jp2", "ntf", "png", "i21", "mbtiles", "wms", "ecw" };
    return s_rasterExtensions.contains(suffix.toLower());
}

QStringList buildMapLayerList(const QString& folder)
{
    auto results = QStringList();

    enumFiles(folder, [&](const QFileInfo& fileInfo)
    {
        if (!isMapLayerSuffix(fileInfo.suffix()))
        {
            return;
        }

        results.append(fileInfo.filePath());
    });

    return results;
}

QVariantList getKoBoForms(QNetworkAccessManager* networkAccessManager, const QString& server, const QString& token, const QString& usernameFilter)
{
    auto result = QVariantList();

    auto response = Utils::httpGetWithToken(networkAccessManager, server + "/api/v2/assets/?q=asset_type%3Asurvey", token, "Token");
    if (response.status != 200)
    {
        qDebug() << "Error retrieving forms:" << response.errorString;
        return result;
    }

    auto forms = QJsonDocument::fromJson(response.data).object().toVariantMap()["results"].toList();
    if (forms.isEmpty())
    {
        qDebug() << "Error: No KoBo forms found";
        return result;
    }

    auto activeForms = QList<QVariantMap>();
    for (auto formIt = forms.constBegin(); formIt != forms.constEnd(); formIt++)
    {
        auto form = formIt->toMap();
        if (!form.value("deployment__active", false).toBool())
        {
            continue;
        }

        if (!usernameFilter.isEmpty())
        {
            if (form.value("owner__username").toString().compare(usernameFilter, Qt::CaseInsensitive) != 0)
            {
                continue;
            }
        }

        activeForms.append(form);
    }

    if (activeForms.isEmpty())
    {
        return result;
    }

    std::sort(activeForms.begin(), activeForms.end(), [&](const QVariantMap& f1, const QVariantMap& f2) -> bool
    {
        return f2["date_deployed"].toString() < f1["date_deployed"].toString();
    });

    // Retieve formhub uuid.
    static const auto s_v1_servers = QMap<QString, QString>
    {
        { "https://eu.kobotoolbox.org", "https://kc-eu.kobotoolbox.org" },
        { "https://kf.kobotoolbox.org", "https://kc.kobotoolbox.org" },
    };

    auto serverV1 = s_v1_servers.value(server, server);

    response = Utils::httpGetWithToken(networkAccessManager, serverV1 + "/api/v1/forms", token, "Token");
    if (!response.success)
    {
        qDebug() << "Error: Failed to retrieve v1 forms";
        return result;
    }

    auto v1forms = QJsonDocument::fromJson(response.data).array().toVariantList();

    for (auto formIt = activeForms.constBegin(); formIt != activeForms.constEnd(); formIt++)
    {
        auto form = *formIt;
        auto formUid = form["uid"].toString();
        auto formhubUuid = QString();

        for (auto v1formIt = v1forms.constBegin(); v1formIt != v1forms.constEnd(); v1formIt++)
        {
            auto v1form = v1formIt->toMap();

            auto assetUid = v1form["kpi_asset_uid"].toString();
            if (assetUid == formUid)
            {
                formhubUuid = v1form["uuid"].toString();
                break;
            }
        }

        auto finalForm = form;
        if (!formhubUuid.isEmpty())
        {
            finalForm.insert("formhubUuid", formhubUuid);
        }

        result.append(QVariant(finalForm));
    }

    return result;
}

QString replaceNonAlphanumeric(const QString& input)
{
    static auto s_regex = QRegularExpression("[^a-zA-Z0-9]");

    auto result = QString(input).replace(s_regex, ".");

    while (result.contains(".."))
    {
        result.replace("..", ".");
    }

    if (result.length() == 1)
    {
        result = "CT-" + result;
    }

    return result;
}

QStringList unionStrings(const QStringList& strings1, const QStringList& strings2)
{
    auto result = strings1;

    for (auto stringIt = strings2.constBegin(); stringIt != strings2.constEnd(); stringIt++)
    {
        if (!result.contains(*stringIt))
        {
            result.append(*stringIt);
        }
    }

    return result;
}

int computeDayOfWeekFontSize(const QLocale& locale, int width, const QString& fontFamily, bool bold)
{
    if (width == 0)
    {
        return 10;
    }

    auto font = QFont(fontFamily);
    font.setBold(bold);
    font.setPixelSize(32);
    auto fontMetrics = QFontMetrics(font);

    auto maxDayWidth = (int)0;
    auto maxDayName = QString();

    for (auto day = 0; day < 7; day++)
    {
        auto name = locale.dayName(day + 1, QLocale::ShortFormat);
        auto width = fontMetrics.horizontalAdvance(name);
        if (width > maxDayWidth)
        {
            maxDayWidth = width;
            maxDayName = name;
        }
    }

    auto pixelSize = font.pixelSize();
    while (pixelSize > 10 && fontMetrics.horizontalAdvance(maxDayName) > width / 8)
    {
        pixelSize--;
        font.setPixelSize(pixelSize);
        fontMetrics = QFontMetrics(font);
    }

    return pixelSize;
}

int computeNumberFontSize(int width, int number, const QString& fontFamily, bool bold)
{
    if (width == 0)
    {
        return 10;
    }

    auto font = QFont(fontFamily);
    font.setBold(bold);
    font.setPixelSize(40);
    auto fontMetrics = QFontMetrics(font);

    auto pixelSize = font.pixelSize();
    auto numberText = QString::number(number);
    while (pixelSize > 10 && fontMetrics.horizontalAdvance(numberText) >= width)
    {
        pixelSize--;
        font.setPixelSize(pixelSize);
        fontMetrics = QFontMetrics(font);
    }

    return pixelSize;
}

void parseSearchText(const QString& text, QStringList* includesOut, QStringList* excludesOut)
{
    auto str = text;

    // Regular expression to match phrases in double quotes with optional '+' or '-' prefix
    static auto s_phraseRegex = QRegularExpression(R"(([\-\+])?\"([^\"]+)\")");
    auto i = s_phraseRegex.globalMatch(str);

    // Process all matched phrases.
    while (i.hasNext())
    {
        auto match = i.next();
        auto fullMatch = match.captured(0);
        auto prefix = match.captured(1);
        auto phrase = match.captured(2);

        prefix == "-" ? excludesOut->append(phrase) : includesOut->append(phrase);

        // Remove the matched phrase from the input string.
        str.replace(fullMatch, "");
    }

    // Split the remaining string into words.
    static auto s_wordExpression = QRegularExpression("\\s+");
    auto words = str.split(s_wordExpression, Qt::SkipEmptyParts);

    // Process each word as a phrase of length one
    for (const QString& word: words)
    {
        auto cleanWord = word;
        auto prefix = QString();

        // Check for '+' or '-' prefix.
        if (word.startsWith("-") || word.startsWith("+"))
        {
            prefix = word.at(0);
            cleanWord = word.mid(1);
        }

        prefix == "-" ? excludesOut->append(cleanWord) : includesOut->append(cleanWord);
    }
}

bool transcribeAbortCallback(void* user_data)
{
    return ((QThread*)user_data)->isInterruptionRequested();
}

}

} // namespace CyberTracker
