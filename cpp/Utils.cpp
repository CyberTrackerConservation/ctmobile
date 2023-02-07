#include "pch.h"

#include <GeographicLib/PolygonArea.hpp>
#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/Constants.hpp>
#include <GeographicLib/GeoCoords.hpp>
#include <QZXing.h>
#include <jlcompress.h>

namespace Utils
{

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

QByteArray variantListToJson(const QVariantList& list, QJsonDocument::JsonFormat format)
{
    auto jsonArray = QJsonArray::fromVariantList(list);
    QJsonDocument jsonDocument(jsonArray);
    return jsonDocument.toJson(format);
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
    streamFileOut.setCodec("UTF-8");
    streamFileOut.setGenerateByteOrderMark(true);
    streamFileOut << data;
}

void writeDataToFile(const QString& filePath, const QByteArray& data)
{
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write(data);
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

QString unpackZip(const QString& targetPath, const QString& targetFolder, const QString& zipFilePath)
{
    auto path = QDir::cleanPath(targetPath + "/" + targetFolder);
    QDir(path).removeRecursively();
    qFatalIf(!Utils::ensurePath(path), "Failed to create target folder");

    auto extractedFiles = JlCompress::extractDir(zipFilePath, path);

    if (extractedFiles.isEmpty())
    {
        QDir(path).removeRecursively();
        return QString();
    }

    return path;
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

QObject* loadQmlFromFile(const QString& filePath)
{
    QQmlEngine engine;
    return loadQmlFromFile(&engine, filePath);
}

QObject* loadQmlFromFile(QQmlEngine* engine, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
       return nullptr;
    }

    QQmlComponent component(engine);
    component.loadUrl(QUrl::fromLocalFile(filePath));

    auto result = component.create();
    if (!result)
    {
        qDebug() << component.errorString();
    }

    return result;
}

QObject* loadQmlFromData(const QByteArray& data)
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(data, QUrl());

    auto result = component.create();
    if (!result)
    {
        qDebug() << component.errorString();
    }

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
    QAndroidJniObject jsPath = QAndroidJniObject::fromString(filePath);
    QtAndroid::androidActivity().callMethod<void>("mediaScan", "(Ljava/lang/String;)V", jsPath.object<jstring>());
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

HttpResponse httpGetHead(QNetworkAccessManager* networkAccessManager, const QString& url)
{
    auto result = HttpResponse();

    QNetworkRequest request;
    QEventLoop eventLoop;

    auto finalUrl = url;
    for (;;)
    {
        request.setUrl(finalUrl);
        auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->head(request));
        reply->ignoreSslErrors();

        QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();

        auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirectUrl.isValid())
        {
            finalUrl = redirectUrl.toString();
            continue;
        }

        result.success = reply->error() == QNetworkReply::NoError;
        result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        result.etag = reply->header(QNetworkRequest::ETagHeader).toString();
        result.location = reply->header(QNetworkRequest::LocationHeader).toString();
        result.url = finalUrl;
        result.errorString = reply->errorString();
        result.data = reply->isOpen() ? QByteArray(reply->readAll()) : QByteArray();

        if (!result.success)
        {
            qDebug() << "HTTP error: " << result.errorString;
            qDebug() << "HTTP error: " << result.data;
        }

        break;
    }

    return result;
}

HttpResponse httpGet(QNetworkAccessManager* networkAccessManager, const QString& url, const QString& username, const QString& password)
{
    auto result = HttpResponse();

    QNetworkRequest request;
    QEventLoop eventLoop;

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
        reply->ignoreSslErrors();

        QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();

        auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirectUrl.isValid())
        {
            finalUrl = redirectUrl.toString();
            continue;
        }

        result.success = reply->error() == QNetworkReply::NoError;
        result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        result.etag = reply->header(QNetworkRequest::ETagHeader).toString();
        result.location = reply->header(QNetworkRequest::LocationHeader).toString();
        result.url = finalUrl;
        result.errorString = reply->errorString();
        result.data = reply->isOpen() ? QByteArray(reply->readAll()) : QByteArray();

        if (!result.success)
        {
            qDebug() << "HTTP error: " << result.errorString;
            qDebug() << "HTTP error: " << result.data;
        }

        break;
    }

    return result;
}

HttpResponse httpGetWithToken(QNetworkAccessManager* networkAccessManager, const QString& url, const QString& token, const QString& tokenType)
{
    auto result = HttpResponse();

    QNetworkRequest request;
    QEventLoop eventLoop;

    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Authorization", QString(tokenType + " " + token).toUtf8());

    auto finalUrl = url;
    for (;;)
    {
        request.setUrl(finalUrl);
        auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->get(request));
        reply->ignoreSslErrors();

        QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();

        auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirectUrl.isValid())
        {
            finalUrl = redirectUrl.toString();
            continue;
        }

        result.success = reply->error() == QNetworkReply::NoError;
        result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result.reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
        result.etag = reply->header(QNetworkRequest::ETagHeader).toString();
        result.location = reply->header(QNetworkRequest::LocationHeader).toString();
        result.url = finalUrl;
        result.errorString = reply->errorString();
        result.data = reply->isOpen() ? QByteArray(reply->readAll()) : QByteArray();

        if (!result.success)
        {
            qDebug() << "HTTP error: " << result.errorString;
            qDebug() << "HTTP error: " << result.data;
        }

        break;
    }

    return result;
}

HttpResponse httpPostJson(QNetworkAccessManager* networkAccessManager, const QString& url, const QVariantMap& payloadJson, const QString& token, const QString& tokenType, bool deflate)
{
    auto result = HttpResponse();

    QNetworkRequest request;
    QEventLoop eventLoop;

    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(url);
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
    reply->ignoreSslErrors();

    QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    result.success = reply->error() == QNetworkReply::NoError;
    result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    result.etag = reply->header(QNetworkRequest::ETagHeader).toString();
    result.location = reply->header(QNetworkRequest::LocationHeader).toString();
    result.url = url;
    result.errorString = reply->errorString();
    result.data = reply->isOpen() ? QByteArray(reply->readAll()) : QByteArray();

    return result;
}

HttpResponse httpPostData(QNetworkAccessManager* networkAccessManager, const QString& url, const QByteArray& payload, const QString& token, const QString& tokenType, bool deflate)
{
    auto result = HttpResponse();

    QNetworkRequest request;
    QEventLoop eventLoop;
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");
    request.setUrl(url);
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
    reply->ignoreSslErrors();

    QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    result.success = reply->error() == QNetworkReply::NoError;
    result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    result.etag = reply->header(QNetworkRequest::ETagHeader).toString();
    result.location = reply->header(QNetworkRequest::LocationHeader).toString();
    result.url = url;
    result.errorString = reply->errorString();
    result.data = reply->isOpen() ? QByteArray(reply->readAll()) : QByteArray();

    if (!result.success)
    {
        qDebug() << "HTTP error: " << result.errorString;
        qDebug() << "HTTP error: " << result.data;
    }

    return result;
}

HttpResponse httpPostQuery(QNetworkAccessManager* networkAccessManager, const QString& url, const QUrlQuery& query)
{
    auto result = HttpResponse();

    QNetworkRequest request;
    QEventLoop eventLoop;
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "application/json, text/plain, */*");
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");
    request.setUrl(url);

    auto payload = query.toString(QUrl::FullyEncoded).toUtf8();

    auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->post(request, payload));
    reply->ignoreSslErrors();

    QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    result.success = reply->error() == QNetworkReply::NoError;
    result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    result.etag = reply->header(QNetworkRequest::ETagHeader).toString();
    result.location = reply->header(QNetworkRequest::LocationHeader).toString();
    result.url = url;
    result.errorString = reply->errorString();
    result.data = reply->isOpen() ? QByteArray(reply->readAll()) : QByteArray();

    if (!result.success)
    {
        qDebug() << "HTTP error: " << result.errorString;
        qDebug() << "HTTP error: " << result.data;
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

HttpResponse httpPostMultiPart(QNetworkAccessManager* networkAccessManager, const QString& url, QHttpMultiPart* multiPart)
{
    auto result = HttpResponse();

    multiPart->setBoundary(Utils::makeMultiPartBoundary());

    QNetworkRequest request;
    QEventLoop eventLoop;
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Content-Type", "multipart/form-data; boundary=" + multiPart->boundary());
    request.setUrl(url);

    auto reply = std::unique_ptr<QNetworkReply>(networkAccessManager->post(request, multiPart));
    reply->ignoreSslErrors();

    QObject::connect(reply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    result.success = reply->error() == QNetworkReply::NoError;
    result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    result.etag = reply->header(QNetworkRequest::ETagHeader).toString();
    result.location = reply->header(QNetworkRequest::LocationHeader).toString();
    result.url = url;
    result.errorString = reply->errorString();
    result.data = reply->isOpen() ? QByteArray(reply->readAll()) : QByteArray();

    if (!result.success)
    {
        qDebug() << "HTTP error: " << result.errorString;
        qDebug() << "HTTP error: " << result.data;
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

bool reorientImageFile(const QString& filePath, int resolutionX, int resolutionY, bool isBackFace, int cameraOrientation)
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
            rotation = (360 - cameraOrientation + screenAngle) % 360;
        }
        else
        {
            rotation = (cameraOrientation - screenAngle + 180) % 360;
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
    Q_UNUSED(cameraOrientation)

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
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    if (!isBackFace)
    {
        pixmap = pixmap.transformed(QTransform().scale(-1, 1));
    }
#endif

    return pixmap.save(filePath);
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

    QImage icon(iconFilePath);
    iconRect.adjust(2, 2, -2, -2);
    auto iconScaled = icon.scaled(iconRect.width(), iconRect.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    auto targetRect = iconScaled.rect();
    targetRect.moveCenter(iconRect.center());
    painter.drawImage(targetRect, iconScaled);

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

bool renderSquarePixmap(QPixmap* pixmap, const QString& filePath, int width, int height)
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

QString renderQRCodeToPng(const QString& data, int width, int height)
{
    auto image = QZXing::encodeData(data, QZXing::EncoderFormat_QR_CODE, QSize(width, height), QZXing::EncodeErrorCorrectionLevel_L, true, false);

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
        auto androidContext = QtAndroid::androidContext();
        QAndroidJniObject dir = QAndroidJniObject::fromString(QString(""));
        QAndroidJniObject path = androidContext.callObjectMethod("getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;", dir.object());
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
    QtAndroid::runOnAndroidThread([=]()
    {
        QAndroidJniObject window = QtAndroid::androidActivity().callObjectMethod("getWindow", "()Landroid/view/Window;");
        QAndroidJniObject decorView = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
        decorView.callMethod<void>("setSystemUiVisibility", "(I)V", 0x2 /*SYSTEM_UI_FLAG_HIDE_NAVIGATION*/);
    });
#endif
}

QString classicRoot()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    return QStandardPaths::writableLocation(QStandardPaths::StandardLocation::DesktopLocation) + "/CT";
#elif defined(Q_OS_IOS)
    return QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation) + "/CTClassic";
#elif defined(Q_OS_ANDROID)
    return androidGetExternalFilesDir() + "/cybertracker";
#endif
}

QString encodeTimestamp(const QDateTime& dateTime)
{
    return dateTime.toString(Qt::DateFormat::ISODateWithMs);
}

QDateTime decodeTimestamp(const QString& isoDateTime)
{
    return QDateTime::fromString(isoDateTime, Qt::DateFormat::ISODateWithMs);
}

qint64 decodeTimestampSecs(const QString& isoDateTime)
{
    return QDateTime::fromString(isoDateTime, Qt::DateFormat::ISODateWithMs).toSecsSinceEpoch();
}

qint64 decodeTimestampMSecs(const QString& isoDateTime)
{
    return QDateTime::fromString(isoDateTime, Qt::DateFormat::ISODateWithMs).toMSecsSinceEpoch();
}

qint64 timestampDeltaMs(const QString& startISODateTime, const QString& stopISODateTime)
{
    return decodeTimestampMSecs(stopISODateTime) - decodeTimestampMSecs(startISODateTime);
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

double areaInSquareMeters(const QVariantList& points)
{
    auto perimeter = 0.0;
    auto area = 0.0;

    try
    {
        GeographicLib::Geodesic geod(GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f());
        GeographicLib::PolygonArea poly(geod);

        for (auto it = points.constBegin(); it != points.constEnd(); it++)
        {
            auto coords = it->toList();
            poly.AddPoint(coords[1].toDouble(), coords[0].toDouble());
        }

        poly.Compute(false, true, perimeter, area);
    }
    catch (const std::exception& e)
    {
        qDebug() << e.what();
        return 0.0;
    }

    return std::abs(area);
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
            QString::number(y, 'f', 8),
            QString::number(x, 'f', 8),
            QString::number(z, 'f', 8),
            QString::number(std::isnan(a) ? 0 : a, 'f', 2)));
    }

    return result.join(';');
}

void latLonToUTMXY(double lat, double lon, double* xOut, double* yOut, int* zoneOut, bool* northOut)
{
    try
    {
        GeographicLib::GeoCoords coords(lat, lon);
        *xOut = coords.Easting();
        *yOut = coords.Northing();
        *zoneOut = coords.Zone();
        *northOut = coords.Northp();
    }
    catch (const std::exception& e)
    {
        qDebug() << "Exception converting to UTM: " << e.what();
    }
}

void UTMXYToLatLon(double x, double y, int zone, bool north, double* latOut, double* lonOut)
{
    try
    {
        GeographicLib::GeoCoords coords(zone, north, x, y);
        *latOut = coords.Latitude();
        *lonOut = coords.Longitude();
    }
    catch (const std::exception& e)
    {
        qDebug() << "Exception converting from UTM: " << e.what();
    }
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

static QMimeDatabase* s_mimeDatabase;

QMimeDatabase* mimeDatabase()
{
    if (s_mimeDatabase == nullptr)
    {
        s_mimeDatabase = new QMimeDatabase();
    }

    return s_mimeDatabase;
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
    auto s_rasterExtensions = QStringList { "tpk", "vtpk", "img", "i12", "dt0", "dt1", "dt2", "tc2", "geotiff", "tif", "tiff", "hr1", "jpg", "jpeg", "jp2", "ntf", "png", "i21", "mbtiles", "wms" };
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

}
