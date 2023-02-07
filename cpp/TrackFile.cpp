#include "pch.h"
#include "TrackFile.h"
#include "App.h"
#include <jlcompress.h>

namespace  {
QThreadStorage<QPair<QSqlDatabase, int>> s_databases;
}

TrackFile::TrackFile(QObject* parent): QObject(parent)
{
}

TrackFile::TrackFile(const QString& filePath, QObject* parent): QObject(parent)
{
    m_filePath = filePath;
    m_connectionName = "ct_db_track_" + QString::number((quint64)QThread::currentThread(), 16);

    auto db = s_databases.localData();
    if (!db.first.isValid())
    {
        db.first = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
        db.first.setDatabaseName(filePath);
        db.second = 0;
        qFatalIf(!db.first.open(), "Cannot open sqlite database: " + filePath);
    }

    db.second++;
    s_databases.setLocalData(db);

    m_db = db.first;

    QSqlQuery query = QSqlQuery(m_db);

    // Create info table.
    auto success = query.prepare("CREATE TABLE IF NOT EXISTS info (data BLOB)");
    qFatalIf(!success, "Failed to create info table 1");
    success = query.exec();
    qFatalIf(!success, "Failed to create info table 2");

    // Create initial info blob.
    success = query.prepare("SELECT EXISTS(SELECT 1 FROM info)");
    qFatalIf(!success, "Failed to init info 1");
    success = query.exec() && query.next();
    qFatalIf(!success, "Failed to init info 2");
    auto exists = query.value(0).toInt() == 1;

    if (!exists)
    {
        success = query.prepare("INSERT INTO info (data) VALUES (:data)");
        qFatalIf(!success, "Failed to init info 3");
        query.bindValue(":data", Utils::variantMapToBlob(infoToMap(TrackFileInfo())));
        success = query.exec();
        qFatalIf(!success, "Failed to init info 4");
    }

    // Create location table.
    success = query.prepare("CREATE TABLE IF NOT EXISTS location (uid TEXT, data BLOB)");
    qFatalIf(!success, "Failed to create location table 1");
    success = query.exec();
    qFatalIf(!success, "Failed to create location table 2");


}

TrackFile::~TrackFile()
{
    auto db = s_databases.localData();
    qFatalIf(!db.first.isValid(), "Bad database reference 1");
    qFatalIf(db.second <= 0, "Bad database reference 2");
    db.second--;

    if (db.second == 0)
    {
        m_db.close();
        m_db = db.first = QSqlDatabase::database();
        s_databases.setLocalData(db);   // Must occur before removeDatabase.
        QSqlDatabase::removeDatabase(m_connectionName);
    }
    else
    {
        s_databases.setLocalData(db);
    }
}

void TrackFile::verifyThread() const
{
    auto connectionName = "ct_db_track_" + QString::number((quint64)QThread::currentThread(), 16);
    qFatalIf(connectionName != m_connectionName, "Wrong thread");
}

void TrackFile::compact()
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("VACUUM");
    qFatalIf(!success, "Failed compact 1");
    success = query.exec();
    qFatalIf(!success, "Failed compact 2");
}

QVariantMap TrackFile::infoToMap(const TrackFileInfo& info) const
{
    return QVariantMap
    {
        { "deviceId", info.deviceId },
        { "username", info.username },
        { "count", info.count },
        { "extent", info.extent },
        { "speedMin", info.speedMin },
        { "speedMax", info.speedMax },
        { "speedAvg", info.speedAvg },
        { "speedTotal", info.speedTotal },
        { "accuracyMin", info.accuracyMin },
        { "accuracyMax", info.accuracyMax },
        { "accuracyAvg", info.accuracyAvg },
        { "accuracyTotal", info.accuracyTotal },
        { "timeStart", info.timeStart },
        { "timeStop", info.timeStop },
        { "timeTotal", info.timeTotal },
        { "distance", info.distance },
    };
}

TrackFileInfo TrackFile::mapToInfo(const QVariantMap& map) const
{
    auto result = TrackFileInfo();
    result.deviceId = map["deviceId"].toString();
    result.username = map["username"].toString();
    result.count = map["count"].toInt();
    result.extent = map["extent"].toRectF();
    result.speedMin = map["speedMin"].toDouble();
    result.speedMax = map["speedMax"].toDouble();
    result.speedAvg = map["speedAvg"].toDouble();
    result.speedTotal = map["speedTotal"].toDouble();
    result.accuracyMin = map["accuracyMin"].toDouble();
    result.accuracyMax = map["accuracyMax"].toDouble();
    result.accuracyAvg = map["accuracyAvg"].toDouble();
    result.accuracyTotal = map["accuracyTotal"].toDouble();
    result.timeStart = map["timeStart"].toString();
    result.timeStop = map["timeStop"].toString();
    result.timeTotal = map["timeTotal"].toInt();
    result.distance = map["distance"].toDouble();

    return result;
}

TrackFileInfo TrackFile::getInfo() const
{
    verifyThread();

    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT data FROM info");
    qFatalIf(!success, "Failed to read metadata 1");
    success = query.exec() && query.next();

    if (success)
    {
        auto dataBlob = query.value(0).toByteArray();
        if (!dataBlob.isEmpty())
        {
            auto data = Utils::variantMapFromBlob(dataBlob);
            return mapToInfo(data);
        }
    }

    return TrackFileInfo();
}

void TrackFile::setInfo(const TrackFileInfo& info)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("UPDATE info SET data = :data");
    qFatalIf(!success, "Failed to save info 1");
    query.bindValue(":data", Utils::variantMapToBlob(infoToMap(info)));
    success = query.exec();
    qFatalIf(!success, "Failed to set info 2");
}

void TrackFile::batchAddInit(const QString& deviceId, const QString& username)
{
    verifyThread();

    m_batchInfo = TrackFileInfo();
    m_batchInfo.deviceId = deviceId;
    m_batchInfo.username = username;
}

void TrackFile::batchAddLocation(const QString& uid, const QVariantMap& locationMap)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("INSERT INTO location (uid, data) VALUES (:uid, :data)");
    qFatalIf(!success, "Failed to add location 1");
    query.bindValue(":uid", uid);
    query.bindValue(":data", Utils::variantMapToBlob(locationMap));
    success = query.exec();
    qFatalIf(!success, "Failed to add location 2");

    Location location(locationMap);

    if (m_batchInfo.count == 0)
    {
        m_batchInfo.timeStart = location.timestamp();
        m_batchLastX = location.x();
        m_batchLastY = location.y();
    }
    else
    {
        m_batchInfo.distance += location.distanceTo(m_batchLastX, m_batchLastY);
    }

    m_batchInfo.timeStop = location.timestamp();
    m_batchInfo.speedTotal += location.speed();
    m_batchInfo.speedMin = std::min(m_batchInfo.speedMin, location.speed());
    m_batchInfo.speedMax = std::max(m_batchInfo.speedMax, location.speed());
    m_batchInfo.accuracyTotal += location.accuracy();
    m_batchInfo.accuracyMin = std::min(m_batchInfo.accuracyMin, location.accuracy());
    m_batchInfo.accuracyMax = std::max(m_batchInfo.accuracyMax, location.accuracy());

    m_batchInfo.extent = m_batchInfo.extent.united(Utils::pointToRect(location.x(), location.y()));

    m_batchInfo.count++;
}

int TrackFile::batchAddDone()
{
    if (m_batchInfo.count == 0)
    {
        return 0;
    }

    m_batchInfo.speedAvg = m_batchInfo.speedTotal / m_batchInfo.count;
    m_batchInfo.accuracyAvg = m_batchInfo.accuracyTotal / m_batchInfo.count;
    m_batchInfo.timeTotal = Utils::decodeTimestampSecs(m_batchInfo.timeStop) - Utils::decodeTimestampSecs(m_batchInfo.timeStart);

    setInfo(m_batchInfo);

    compact();

    return m_batchInfo.count;
}

void TrackFile::enumLocations(const std::function<void(const QString& uid, const QVariantMap& locationMap)>& callback) const
{
    verifyThread();

    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT uid, data FROM location ORDER BY rowid");
    qFatalIf(!success, "Failed to enum locations 1");
    success = query.exec();
    qFatalIf(!success, "Failed to enum locations 2");

    while (query.next())
    {
        auto uid = query.value(0).toString();

        auto dataBlob = query.value(1).toByteArray();
        qFatalIf(dataBlob.isEmpty(), "Failed to read location");
        auto locationMap = Utils::variantMapFromBlob(dataBlob);
        callback(uid, locationMap);
    }
}

bool TrackFile::exportKml(const QString& filePath) const
{
    QFile fileTemplate(":/app/trackTemplate.kml");
    if (!fileTemplate.open(QFile::ReadOnly | QFile::Text))
    {
        qFatalIf(true, "Failed to open track template");
        return false;
    }

    auto trackTemplate = QString(fileTemplate.readAll());

    QFile::remove(filePath);
    QFile fileOutput(filePath);
    fileOutput.open(QIODevice::WriteOnly);

    QTextStream streamFileOut(&fileOutput);
    streamFileOut.setCodec("UTF-8");
    streamFileOut.setGenerateByteOrderMark(true);

    trackTemplate.replace("{appid}", App::instance()->config()["title"].toString());

    auto parts = trackTemplate.split("<Placemarks />");
    streamFileOut << parts.constFirst();

    auto pointTemplate = QStringList
    {
        "<Placemark>",
        "<styleUrl>#point</styleUrl>", //<Style><IconStyle><heading>%5</heading></IconStyle></Style>",
        "<name>Point</name>",
        "<ExtendedData><SchemaData schemaUrl=\"#point\">",
        "<SimpleData name=\"uid\">%1</SimpleData>",
        "<SimpleData name=\"z\">%2</SimpleData>",
        "<SimpleData name=\"a\">%3</SimpleData>",
        "<SimpleData name=\"s\">%4</SimpleData>",
        "<SimpleData name=\"d\">%5</SimpleData>",
        "<SimpleData name=\"c\">%6</SimpleData>",
        "<SimpleData name=\"i\">%7</SimpleData>",
        "<SimpleData name=\"df\">%8</SimpleData>",
        "</SchemaData></ExtendedData>",
        "<Point><coordinates>%9,%10</coordinates></Point>",
        "</Placemark>"
    }.join("");

    auto points = QStringList();
    auto info = getInfo();
    points.reserve(info.count);
    enumLocations([&](const QString& uid, const QVariantMap& locationMap)
    {
        auto xs = locationMap["x"].toString();
        auto ys = locationMap["y"].toString();
        auto zs = locationMap["z"].toString();

        auto placeMark = QString(pointTemplate).arg(
            uid,
            zs,
            locationMap["a"].toString(),
            locationMap["s"].toString(),
            locationMap["d"].toString(),
            locationMap["c"].toString(),
            locationMap["interval"].toString(),
            locationMap["distanceFilter"].toString(),
            xs, ys);
        streamFileOut << placeMark;

        points.append(QString("%1,%2,%3").arg(xs, ys, zs));
    });

    auto lineTemplate = QStringList
    {
        "<Placemark>",
        "<name>Track</name>",
        "<ExtendedData><SchemaData schemaUrl=\"#line\">",
        "<SimpleData name=\"username\">%1</SimpleData>",
        "<SimpleData name=\"deviceId\">%2</SimpleData>",
        "<SimpleData name=\"distance\">%3</SimpleData>",
        "<SimpleData name=\"timeStart\">%4</SimpleData>",
        "<SimpleData name=\"timeStop\">%5</SimpleData>",
        "<SimpleData name=\"count\">%6</SimpleData>",
        "<SimpleData name=\"speedMin\">%7</SimpleData>",
        "<SimpleData name=\"speedMax\">%8</SimpleData>",
        "<SimpleData name=\"speedAvg\">%9</SimpleData>",
        "</SchemaData></ExtendedData>",
        "<LineString><coordinates>%10</coordinates></LineString>",
        "</Placemark>"
    }.join("");

    auto lineMark = QString(lineTemplate)
        .arg(info.username, info.deviceId)
        .arg(info.distance)
        .arg(info.timeStart, info.timeStop)
        .arg(info.count)
        .arg(info.speedMin)
        .arg(info.speedMax)
        .arg(info.speedAvg)
        .arg(points.join(' '));

    streamFileOut << lineMark;

    streamFileOut << parts.constLast();

    return true;
}

bool TrackFile::exportKmz(const QString& filePath) const
{
    auto workPath = App::instance()->tempPath() + "/kmz";
    QDir(workPath).removeRecursively();
    qFatalIf(!Utils::ensurePath(workPath), "Failed to create kmz working path");

    // Create the kml in a temporary location.
    if (!exportKml(workPath + "/doc.kml"))
    {
        qDebug() << "Failed to create kml file";
        return false;
    }

    // Zip the file.
    if (!JlCompress::compressDir(filePath, workPath, true))
    {
        qDebug() << "Failed to compress KMZ";
    }

    // Success.
    return true;
}

QVariantList TrackFile::createEsriFeatures(const QString& globalId, const QString& fullName) const
{
    auto info = getInfo();

    // Match to fields in Location Service Track layer: See assets/Esri/CreateLayers.json.
    auto attributes = QVariantMap
    {
        { "globalid", Utils::addBracesToUuid(globalId) },
        { "deviceid", Utils::addBracesToUuid(info.deviceId) },
        { "appid", App::instance()->config()["title"].toString() },
        { "count", info.count },
        { "start_timestamp", Utils::decodeTimestampMSecs(info.timeStart) },
        { "stop_timestamp", Utils::decodeTimestampMSecs(info.timeStop) },
        { "full_name", fullName },
    };

    auto points = QVariantList();
    points.reserve(info.count);
    enumLocations([&](const QString& /*uid*/, const QVariantMap& locationMap)
    {
        points.append(QVariant(QVariantList { locationMap["x"], locationMap["y"], locationMap["z"] }));
    });

    auto geometry = QVariantMap
    {
        { "hasZ", true },
        { "hasM", false },
        { "geometryType", "esriGeometryPolyline" },
        { "spatialReference", QVariantMap {{ "wkid", 4326 }} },
        { "paths", QVariantList { QVariant(points) } },
    };

    return QVariantList { QVariantMap {{ "attributes", attributes }, { "geometry", geometry }} };
}

bool TrackFile::exportGeoJson(const QString& filePath) const
{
    QFile fileOutput(filePath);
    fileOutput.open(QIODevice::WriteOnly);

    QTextStream streamFileOut(&fileOutput);
    streamFileOut.setCodec("UTF-8");
    streamFileOut.setGenerateByteOrderMark(true);

    streamFileOut << "{ \"type\": \"FeatureCollection\", \"features\": [" << Qt::endl;

    auto info = getInfo();
    auto points = QVariantList();
    points.reserve(info.count);

    enumLocations([&](const QString& uid, const QVariantMap& locationMap)
    {
        Location location(locationMap);

        points.append(QVariant(QVariantList { locationMap["x"], locationMap["y"] }));

        auto feature = QJsonObject
        {
            { "type", "Feature" },
            { "geometry", QJsonObject
                {
                    { "type", "Point" },
                    { "coordinates", QJsonArray { location.x(), location.y() }},
                },
            },
            { "properties", QJsonObject
                {
                    { "name", "Point" },
                    { "uid", uid },
                    { "timestamp", location.timestamp() },
                    { "counter", location.counter() },
                    { "altitude", location.z() },
                    { "accuracy", location.accuracy() },
                    { "speed", location.speed() },
                    { "direction", location.direction() },
                    { "interval", location.interval() },
                    { "distanceFilter", location.distanceFilter() },
                }
            }
        };

        streamFileOut << QJsonDocument(feature).toJson(QJsonDocument::Compact) << ',';
    });

    auto lineProperties = QJsonObject
    {
        { "name", "Track" },
        { "username", info.username },
        { "deviceId", info.deviceId },
        { "distance", info.distance },
        { "timeStart", info.timeStart },
        { "timeStop", info.timeStop },
        { "count", info.count },
        { "speedMin", info.speedMin },
        { "speedMax", info.speedMax },
        { "speedAvg", info.speedAvg },
    };

    auto lineFeature = QJsonObject
    {
        { "type", "Feature" },
        { "properties", lineProperties },
        { "geometry", QJsonObject
            {
                { "type", "LineString" },
                { "coordinates", QJsonArray::fromVariantList(points) },
            },
        },
    };

    streamFileOut << QJsonDocument(lineFeature).toJson(QJsonDocument::Compact);

    streamFileOut << Qt::endl << "] }";

    return true;
}

QString TrackFile::exportFile(const QString& format, bool compress)
{
    auto workPath = App::instance()->tempPath() + "/trackFileExport";
    QDir(workPath).removeRecursively();
    Utils::ensurePath(workPath);

    auto filePath = workPath + "/track." + format;

    if (format == "kml")
    {
        exportKml(filePath);
    }
    else if (format == "kmz")
    {
        compress = false; // already compressed.
        exportKmz(filePath);
    }
    else if (format == "geojson")
    {
        exportGeoJson(filePath);
    }
    else
    {
        qDebug() << "Error: unknown track export format: " << format;
        return QString();
    }

    if (compress)
    {
        filePath = App::instance()->tempPath() + "/track.zip";
        QFile::remove(filePath);

        if (!JlCompress::compressDir(filePath, workPath, true))
        {
            qDebug() << "Failed to compress file";
        }
    }

    return filePath;
}
