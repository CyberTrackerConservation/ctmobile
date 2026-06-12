#include "pch.h"
#include "TrackFile.h"
#include "App.h"

namespace CyberTracker {

TrackFile::TrackFile(QObject* parent): QObject(parent)
{
}

TrackFile::TrackFile(const QString& filePath, QObject* parent): QObject(parent)
{
    m_connectionName = Utils::createUuid();
    m_filePath = filePath;

    init();
}

TrackFile::~TrackFile()
{
    close();
}

void TrackFile::init()
{
    auto fileExists = QFile::exists(m_filePath);

    auto db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(m_filePath);
    qFatalIf(!db.open(), "Cannot open sqlite database: " + m_filePath);

    // Migrate old timestamp TEXT column to t INTEGER.
    if (fileExists)
    {
        migrateTimestamp(db);
    }

    QSqlQuery query = QSqlQuery(db);

    // Create info table.
    auto success = query.prepare("CREATE TABLE IF NOT EXISTS info (data TEXT)");
    qFatalIf(!success, "Failed to create info table 1");
    success = query.exec();
    qFatalIf(!success, "Failed to create info table 2");

    // Create initial info blob.
    success = query.prepare("SELECT EXISTS(SELECT 1 FROM info)");
    qFatalIf(!success, "Failed to init info 1");
    success = query.exec() && query.next();
    qFatalIf(!success, "Failed to init info 2 (during init) " + query.lastError().text());
    auto exists = query.value(0).toInt() == 1;

    if (!exists)
    {
        success = query.prepare("INSERT INTO info (data) VALUES (:data)");
        qFatalIf(!success, "Failed to init info 3");
        query.bindValue(":data", Utils::variantMapToJson(infoToMap(TrackFileInfo())));
        success = query.exec();
        qFatalIf(!success, "Failed to init info 4");
    }

    // Create location table.
    success = query.prepare("CREATE TABLE IF NOT EXISTS data (uid TEXT NOT NULL UNIQUE, t INTEGER, x REAL, y REAL, z REAL, accuracy REAL, direction REAL, speed REAL, counter INTEGER)");
    qFatalIf(!success, "Failed to create location table 1");
    success = query.exec();
    qFatalIf(!success, "Failed to create location table 2");
}

void TrackFile::migrateTimestamp(QSqlDatabase& db)
{
    auto query = QSqlQuery(db);

    // Check if old timestamp column exists.
    auto success = query.prepare("PRAGMA table_info(data)");
    qFatalIf(!success, "Failed to check table info");
    success = query.exec();
    qFatalIf(!success, "Failed to exec table info");

    auto hasTimestamp = false;
    while (query.next())
    {
        if (query.value(1).toString() == "timestamp")
        {
            hasTimestamp = true;
            break;
        }
    }

    if (!hasTimestamp)
    {
        return;
    }

    qDebug() << "TrackFile: migrating timestamp TEXT to t INTEGER";

    // Add t column if missing.
    query.prepare("ALTER TABLE data ADD COLUMN t INTEGER");
    if (query.exec())
    {
        // Populate t from timestamp.
        db.transaction();

        success = query.prepare("SELECT rowid, timestamp FROM data");
        qFatalIf(!success, "Failed to read timestamps for migration");
        success = query.exec();
        qFatalIf(!success, "Failed to exec timestamps for migration");

        auto updateQuery = QSqlQuery(db);
        success = updateQuery.prepare("UPDATE data SET t = :t WHERE rowid = :rowid");
        qFatalIf(!success, "Failed to prepare migration update");

        auto count = 0;
        while (query.next())
        {
            auto rowid = query.value(0).toLongLong();
            auto timestamp = query.value(1).toString();
            updateQuery.bindValue(":t", Utils::decodeTimestampMSecs(timestamp));
            updateQuery.bindValue(":rowid", rowid);
            updateQuery.exec();
            count++;
        }

        db.commit();
        qDebug() << "TrackFile: migrated" << count << "rows";
    }

    // Drop old timestamp column.
    query.prepare("ALTER TABLE data DROP COLUMN timestamp");
    query.exec();
    qDebug() << "TrackFile: migration complete";
}

void TrackFile::close()
{
    QSqlDatabase::database(m_connectionName).close();
    QSqlDatabase::removeDatabase(m_connectionName);

    m_batchQuery = nullptr;
}

void TrackFile::reset()
{
    close();

    QFile::remove(m_filePath);
    init();
}

QString TrackFile::moveToMedia()
{
    close();

    auto result = App::instance()->moveToMedia(m_filePath);

    init();

    return result;
}

void TrackFile::compact()
{
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);
    auto success = query.prepare("VACUUM");
    qFatalIf(!success, "Failed compact 1");
    success = query.exec();
    qFatalIf(!success, "Failed compact 2");
}

QVariantMap TrackFile::infoToMap(const TrackFileInfo& info) const
{
    return QVariantMap
    {
        { "deviceId", info.metadata.deviceId },
        { "username", info.metadata.username },
        { "interval", info.metadata.interval },
        { "distanceFilter", info.metadata.distanceFilter },
        { "accuracyFilter", info.metadata.accuracyFilter },
        { "count", info.count },
        { "extent", Utils::rectFToMap(info.extent) },
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
    result.metadata.deviceId = map["deviceId"].toString();
    result.metadata.username = map["username"].toString();
    result.metadata.interval = map["interval"].toInt();
    result.metadata.distanceFilter = map["distanceFilter"].toInt();
    result.metadata.accuracyFilter = map["accuracyFilter"].toInt();
    result.count = map["count"].toInt();
    result.extent = Utils::rectFFromMap(map["extent"].toMap());
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

QString TrackFile::summary() const
{
    auto info = getInfo();
    auto app = App::instance();

    return QString("%1, %2, %3 %4")
        .arg(app->getDistanceText(info.distance), app->getTimeText(info.timeTotal))
        .arg(info.count).arg(tr("points"));
}

QVariantList TrackFile::getPoints() const
{
    auto result = QVariantList();
    enumLocations([&](double x, double y, double /*z*/)
    {
        result.append(QVariant(QVariantList { x, y }));
    });

    return result;
}

TrackFileInfo TrackFile::getInfo() const
{
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);
    auto success = query.prepare("SELECT data FROM info");
    qFatalIf(!success, "Failed to read metadata 1");
    success = query.exec() && query.next();

    if (success)
    {
        auto dataJson = query.value(0).toByteArray();
        if (!dataJson.isEmpty())
        {
            auto data = Utils::variantMapFromJson(dataJson);
            return mapToInfo(data);
        }
    }

    return TrackFileInfo();
}

void TrackFile::setInfo(const TrackFileInfo& info)
{
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);
    auto success = query.prepare("UPDATE info SET data = :data");
    qFatalIf(!success, "Failed to save info 1");
    query.bindValue(":data", Utils::variantMapToJson(infoToMap(info)));
    success = query.exec();
    qFatalIf(!success, "Failed to set info 2 (during setInfo) " + query.lastError().text());
}

TrackFileInfo TrackFile::rebuildInfo(const TrackFileMetadata& metadata)
{
    auto info = TrackFileInfo();
    info.metadata = metadata;

    double lastX, lastY;

    enumLocations([&](const QString& /*uid*/, Location* location)
    {
        info.timeStop = location->timestamp();
        info.speedTotal += location->speed();
        info.speedMin = std::min(info.speedMin, location->speed());
        info.speedMax = std::max(info.speedMax, location->speed());
        info.accuracyTotal += location->accuracy();
        info.accuracyMin = std::min(info.accuracyMin, location->accuracy());
        info.accuracyMax = std::max(info.accuracyMax, location->accuracy());
        info.extent = info.extent.united(Utils::pointToRect(location->x(), location->y()));

        if (info.count == 0)
        {
            info.timeStart = location->timestamp();
        }
        else
        {
            info.distance += location->distanceTo(lastX, lastY);
        }

        lastX = location->x();
        lastY = location->y();

        info.count++;
    });

    if (info.count > 0)
    {
        info.speedAvg = info.speedTotal / info.count;
        info.accuracyAvg = info.accuracyTotal / info.count;
        info.timeTotal = Utils::decodeTimestampSecs(info.timeStop) - Utils::decodeTimestampSecs(info.timeStart);
    }

    setInfo(info);

    return info;
}

void TrackFile::addLocation(const QString& uid, Location* location)
{
    if (m_batchQuery)
    {
        m_batchQuery->bindValue(":uid", uid);
        m_batchQuery->bindValue(":t", location->t());
        m_batchQuery->bindValue(":x", location->x());
        m_batchQuery->bindValue(":y", location->y());
        m_batchQuery->bindValue(":z", location->z());
        m_batchQuery->bindValue(":accuracy", location->accuracy());
        m_batchQuery->bindValue(":direction", location->direction());
        m_batchQuery->bindValue(":speed", location->speed());
        m_batchQuery->bindValue(":counter", location->counter());

        if (!m_batchQuery->exec())
        {
            qDebug() << "batch addLocation failed: " << m_batchQuery->lastError().text();
        }

        return;
    }

    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);
    auto success = query.prepare("INSERT INTO data (uid, t, x, y, z, accuracy, direction, speed, counter) VALUES (:uid, :t, :x, :y, :z, :accuracy, :direction, :speed, :counter)");
    qFatalIf(!success, "Failed to prepare query: " + query.lastError().text());

    query.bindValue(":uid", uid);
    query.bindValue(":t", location->t());
    query.bindValue(":x", location->x());
    query.bindValue(":y", location->y());
    query.bindValue(":z", location->z());
    query.bindValue(":accuracy", location->accuracy());
    query.bindValue(":direction", location->direction());
    query.bindValue(":speed", location->speed());
    query.bindValue(":counter", location->counter());

    if (!query.exec())
    {
        qDebug() << "addLocation failed: " << query.lastError().text();
    }
}

bool TrackFile::addLocationBatch(std::function<void()> callback)
{
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);
    auto success = query.prepare("INSERT INTO data (uid, t, x, y, z, accuracy, direction, speed, counter) VALUES (:uid, :t, :x, :y, :z, :accuracy, :direction, :speed, :counter)");
    qFatalIf(!success, "Failed to prepare batch query");

    db.transaction();
    m_batchQuery = &query;
    callback();
    m_batchQuery = nullptr;

    return db.commit();
}

void TrackFile::enumLocations(const QString& filePath, std::function<void(qint64 t, double x, double y, int counter)> callback)
{
    static auto s_connectionName = "TrackFileReader";

    // Ensure everything goes out of scope before removing the connection.
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", s_connectionName);
        db.setDatabaseName(filePath);
        qFatalIf(!db.open(), "Cannot open sqlite database: " + filePath);

        migrateTimestamp(db);

        auto query = QSqlQuery(db);
        auto success = query.prepare("SELECT t, x, y, counter FROM data ORDER BY rowid");
        qFatalIf(!success, "Failed to enum locations 1");
        success = query.exec();
        qFatalIf(!success, "Failed to enum locations 2");

        while (query.next())
        {
            callback(query.value(0).toLongLong(), query.value(1).toDouble(), query.value(2).toDouble(), query.value(3).toInt());
        }
    }

    QSqlDatabase::database(s_connectionName).close();
    QSqlDatabase::removeDatabase(s_connectionName);
}

void TrackFile::enumLocations(const std::function<void(const QString& uid, Location* location)>& callback) const
{
    auto info = getInfo();
    auto location = Location();
    location.set_accuracyFilter(info.metadata.accuracyFilter);
    location.set_distanceFilter(info.metadata.distanceFilter);
    location.set_interval(info.metadata.interval);

    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);
    auto success = query.prepare("SELECT uid, t, x, y, z, accuracy, direction, speed, counter FROM data ORDER BY rowid");
    qFatalIf(!success, "Failed to enum locations 1");
    success = query.exec();
    qFatalIf(!success, "Failed to enum locations 2");

    while (query.next())
    {
        auto uid = query.value(0).toString();

        location.set_t(query.value(1).toLongLong());
        location.set_x(query.value(2).toDouble());
        location.set_y(query.value(3).toDouble());
        location.set_z(query.value(4).toDouble());
        location.set_accuracy(query.value(5).toDouble());
        location.set_direction(query.value(6).toDouble());
        location.set_speed(query.value(7).toDouble());
        location.set_counter(query.value(8).toDouble());

        callback(uid, &location);
    }
}

void TrackFile::enumLocations(std::function<void(double x, double y, double z)> callback) const
{
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);
    auto success = query.prepare("SELECT x, y, z FROM data ORDER BY rowid");
    qFatalIf(!success, "Failed to enum locations 1");
    success = query.exec();
    qFatalIf(!success, "Failed to enum locations 2");

    while (query.next())
    {
        callback(query.value(0).toDouble(), query.value(1).toDouble(), query.value(2).toDouble());
    }
}

bool TrackFile::hasLocations() const
{
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);
    auto success = query.prepare("SELECT EXISTS(SELECT 1 FROM data)");
    qFatalIf(!success, "Failed to check locations 1");
    success = query.exec() && query.next();
    qFatalIf(!success, "Failed to check locations 2");
    return query.value(0).toInt() == 1;
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
    streamFileOut.setEncoding(QStringConverter::Encoding::Utf8);
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
        "</SchemaData></ExtendedData>",
        "<Point><coordinates>%9,%10</coordinates></Point>",
        "</Placemark>"
    }.join("");

    auto points = QStringList();
    auto info = getInfo();
    points.reserve(info.count);
    enumLocations([&](const QString& uid, Location* location)
    {
        auto xs = QString::number(location->x(), 'f', 10);
        auto ys = QString::number(location->y(), 'f', 10);
        auto zs = QString::number(location->z(), 'f', 10);

        auto placeMark = QString(pointTemplate).arg(
            uid,
            zs,
            QString::number(location->accuracy(), 'f', 2),
            QString::number(location->speed(), 'f', 2),
            QString::number(location->direction(), 'f', 2),
            QString::number(location->counter()),
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
        .arg(info.metadata.username, info.metadata.deviceId)
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
    if (!Utils::zipCompressDir(filePath, workPath))
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
        { "deviceid", Utils::addBracesToUuid(info.metadata.deviceId) },
        { "appid", App::instance()->config()["title"].toString() },
        { "count", info.count },
        { "start_timestamp", Utils::decodeTimestampMSecs(info.timeStart) },
        { "stop_timestamp", Utils::decodeTimestampMSecs(info.timeStop) },
        { "full_name", fullName },
    };

    auto points = QVariantList();
    points.reserve(info.count);
    enumLocations([&](double x, double y, double z)
    {
        points.append(QVariant(QVariantList { x, y, z }));
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
    streamFileOut.setEncoding(QStringConverter::Encoding::Utf8);
    streamFileOut.setGenerateByteOrderMark(true);

    streamFileOut << "{ \"type\": \"FeatureCollection\", \"features\": [" << Qt::endl;

    auto info = getInfo();
    auto points = QVariantList();
    points.reserve(info.count);

    enumLocations([&](const QString& uid, Location* location)
    {
        points.append(QVariant(QVariantList { location->x(), location->y() }));

        auto feature = QJsonObject
        {
            { "type", "Feature" },
            { "geometry", QJsonObject{
                { "type", "Point" },
                { "coordinates", QJsonArray { location->x(), location->y() }} },
            },
            { "properties", QJsonObject {
                { "name", "Point" },
                { "uid", uid },
                { "timestamp", location->timestamp() },
                { "counter", location->counter() },
                { "altitude", location->z() },
                { "accuracy", location->accuracy() },
                { "speed", location->speed() },
                { "direction", location->direction() },
                { "interval", info.metadata.interval },
                { "distanceFilter", info.metadata.distanceFilter } }
            }
        };

        streamFileOut << QJsonDocument(feature).toJson(QJsonDocument::Compact) << ',';
    });

    auto lineProperties = QJsonObject {
        { "name", "Track" },
        { "username", info.metadata.username },
        { "deviceId", info.metadata.deviceId },
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
        { "geometry", QJsonObject {
            { "type", "LineString" },
            { "coordinates", QJsonArray::fromVariantList(points) } },
        },
    };

    streamFileOut << QJsonDocument(lineFeature).toJson(QJsonDocument::Compact);

    streamFileOut << Qt::endl << "] }";

    return true;
}

QString TrackFile::exportFile(const QString& format, bool compress)
{
    auto workPath = App::instance()->tempPath() + "/TrackFileExport";
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
    else if (format.isEmpty())
    {
        exportGeoJson(filePath + "geojson");
    }
    else
    {
        qDebug() << "Failed to export track to format: " << format;
        return QString();
    }

    if (compress)
    {
        filePath = App::instance()->tempPath() + "/track.zip";
        QFile::remove(filePath);

        if (!Utils::zipCompressDir(filePath, workPath))
        {
            qDebug() << "Failed to compress file";
        }
    }

    return filePath;
}

} // namespace CyberTracker