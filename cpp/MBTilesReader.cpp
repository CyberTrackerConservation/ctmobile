#include "MBTilesReader.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MBTilesReader

MBTilesReader::MBTilesReader(QObject* parent): QObject(parent)
{
}

MBTilesReader::~MBTilesReader()
{
    reset();
}

void MBTilesReader::reset()
{
    while (m_cachePaths.count() > 0)
    {
        close(m_cachePaths.firstKey());
    }

    m_cachePaths.clear();
}

QString MBTilesReader::getQml(const QString& filePath)
{
    // Cleanup previous instances.
    close(filePath);

    // Open the database.
    m_dbs[filePath] = QSqlDatabase::addDatabase("QSQLITE", filePath);
    auto db = &m_dbs[filePath];
    db->setDatabaseName(filePath);
    if (!db->open())
    {
        return QString();
    }

    QSqlQuery queryMetadata = QSqlQuery(*db);
    queryMetadata.prepare("SELECT name, value FROM metadata");
    if (!queryMetadata.exec())
    {
        return QString();
    }

    auto metadata = QVariantMap();
    while (queryMetadata.next())
    {
        metadata.insert(queryMetadata.value(0).toString(), queryMetadata.value(1).toString());
    }

    // Lookup metrics.
    QSqlQuery query = QSqlQuery(*db);
    query.prepare("SELECT zoom_level, tile_column, tile_row FROM tiles");
    if (!query.exec())
    {
        return QString();
    }

    auto levelMin = INT_MAX;
    auto levelMax = INT_MIN;
    auto bestScaleLevel = INT_MAX;
    while (query.next())
    {
        auto level = query.value(0).toInt();
        levelMax = qMax(level, levelMax);
        levelMin = qMin(level, levelMin);

        auto lodBounds = m_lodBounds[filePath].value(level, LodBounds { INT_MAX, INT_MIN, INT_MAX, INT_MIN });

        auto col = query.value(1).toInt();
        lodBounds.colMax = qMax(col, lodBounds.colMax);
        lodBounds.colMin = qMin(col, lodBounds.colMin);

        auto row = query.value(2).toInt();
        lodBounds.rowMax = qMax(row, lodBounds.rowMax);
        lodBounds.rowMin = qMin(row, lodBounds.rowMin);

        m_lodBounds[filePath][level] = lodBounds;

        if (level > 0 && level < bestScaleLevel)
        {
            bestScaleLevel = level;
        }
    }

    QFileInfo fileInfo(filePath);

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(fileInfo.fileName().toLatin1());
    hash.addData(QString::number(fileInfo.lastModified().toMSecsSinceEpoch()).toLatin1());
    m_cachePaths[filePath] = QDir::cleanPath(m_cachePath + "/" + hash.result().toHex());

    garbageCollect();

    auto bounds = metadata.value("bounds").toString().split(',');
    if (bounds.count() != 4)
    {
        bounds = QStringList { "-20037508.34", "+20037508.34", "-20037508.34", "+20037508.34" };
    }

    // Generate QML.
    QStringList l;

    l << "import QtQuick 2.12";
    l << "import Esri.ArcGISRuntime 100.14";
    l << "ImageTiledLayer {";

    l << "    Component.onDestruction: MBTilesReader.close(\"" + filePath + "\")";

    l << "    SpatialReference { id: wgs84; wkid: 4326 }";
    l << "    SpatialReference { id: webMercator; wkid: 3857 }";
    l << "    Envelope { id: fileExtent; xMin: " + bounds[0] + "; yMin: " + bounds[3] + "; xMax: " + bounds[2] + "; yMax: " + bounds[1] + "; spatialReference: wgs84 }";

    l << "    fullExtent: createFullExtent()";
    l << "    tileInfo: createTileInfo()";
    l << "    tileCallback: function(level, row, column) { return MBTilesReader.getTile(\"" + filePath + "\", level, row, column).toString(); }";

    l << "    function createFullExtent() {";
    l << "        let e = GeometryEngine.project(fileExtent, webMercator)";
    l << "        return ArcGISRuntimeEnvironment.createObject(\"Envelope\", { \"json\": e.json })";
    l << "    }";

    l << "    function createTileInfo() {";
    l << "        return ArcGISRuntimeEnvironment.createObject(\"TileInfo\", { origin: createOrigin(), dpi: 96, tileWidth: 256, tileHeight: 256, spatialReference: webMercator, levelsOfDetail: createLods() })";
    l << "    }";

    l << "    function createOrigin() {";
    l << "        return ArcGISRuntimeEnvironment.createObject(\"Point\", { x: -20037508.34, y: +20037508.34, spatialReference: webMercator })";
    l << "    }";

    l << "    function createLods() {";
    l << "        let lods = []";

    double resolution = (20037508.34 * 2) / 256;
    double scale = resolution * 96 * 39.37;
    auto bestScale = QString();

    for (auto i = 0; i <= 24; i++)
    {
        if ((i >= levelMin) && (i <= levelMax))
        {
            l << "        lods.push(ArcGISRuntimeEnvironment.createObject(\"LevelOfDetail\", {level: " + QString::number(i) + ", resolution: " + QString::number(resolution, 'g', 20) + ", scale: " + QString::number(scale, 'g', 20) + "}))";

            if (i == bestScaleLevel)
            {
                bestScale = QString::number(scale, 'g', 20);
            }
        }

        resolution /= 2;
        scale /= 2;
    }

    l << "        return lods";
    l << "    }";
    l << "    property double mbMaxScale: " + (!bestScale.isEmpty() ? bestScale : "undefined");
    l << "}";

//    auto result = l.join('\n');
//    {
//        QFile qmlFile("C:/Users/Justin/Desktop/MapTileLoader.qml");
//        qmlFile.open(QIODevice::WriteOnly);

//        QTextStream streamFileOut(&qmlFile);
//        streamFileOut.setCodec("UTF-8");
//        streamFileOut.setGenerateByteOrderMark(true);
//        streamFileOut << result;
//    }
//    return result;

    return l.join('\n');
}

QUrl MBTilesReader::getTile(const QString& filePath, int level, int row, int col)
{
    // TMS requires inverting the y.
    row = static_cast<int>((qPow(2, level) - 1) - row);

    // Fast lookup to see if out of bounds.
    auto lodBounds = m_lodBounds[filePath][level];
    if ((row < lodBounds.rowMin) || (row > lodBounds.rowMax) ||
        (col < lodBounds.colMin) || (col > lodBounds.colMax))
    {
        return QUrl();
    }

    // OSM mbtiles generator creates two zoom levels. Ignore the zero level which is just
    // the world.
    if (level == 0 && m_lodBounds[filePath].count() == 2)
    {
        return QUrl();
    }

    //qFatalIf(!m_dbs.contains(filename), "DB not loaded 1");
    auto db = &m_dbs[filePath];

    //qFatalIf(!m_cachePaths.contains(filename), "DB not loaded 2");
    auto cachePath = m_cachePaths[filePath] + "_" + QString::number(level) + "_" + QString::number(row) + "_" + QString::number(col) + ".png";

    QFile file(cachePath);
    if (file.exists())
    {
        return QUrl::fromLocalFile(cachePath);
    }

    QSqlQuery query = QSqlQuery(*db);
    query.prepare("SELECT tile_data FROM tiles WHERE (zoom_level = (:level)) AND (tile_column = (:column)) AND (tile_row = (:row))");
    query.bindValue(":level", level);
    query.bindValue(":row", row);
    query.bindValue(":column", col);

    if (!query.exec() || !query.next())
    {
        return QUrl();
    }

    auto data = query.value(0).toByteArray();

    // Hack start: remove white pixels for SMART 6.
    auto image = QImage::fromData(data);
    if (image.width() > 0 && image.height() > 0 && image.bitPlaneCount() == 24 && image.bytesPerLine() == 1024)
    {
        image = image.convertToFormat(QImage::Format_ARGB32);
        auto bits = reinterpret_cast<uint*>(image.bits());

        for (auto i = 0; i < image.sizeInBytes() / 4; i++)
        {
            auto color = *bits;
            if (color == 0xffffffff)
            {
                color = 0x00000000;
            }
            else
            {
                auto r = 3 * ((color & 0xFF0000) >> 16) >> 2;
                auto g = 3 * ((color & 0x00FF00) >> 8) >> 2;
                auto b = 3 * ((color & 0x0000FF)) >> 2;
                color = 0xA0000000 | (r << 16 | g << 8 | b);
            }
            *bits = color;
            bits++;
        }

        file.open(QIODevice::WriteOnly);
        image.save(&file);
    } // Hack end.
    else
    {
        file.open(QIODevice::WriteOnly);
        file.write(data);
    }

    m_gcCounter++;
    if (m_gcCounter % 16 == 0)
    {
        garbageCollect();
    }

    return QUrl::fromLocalFile(cachePath);
}

void MBTilesReader::close(const QString& filePath)
{
    m_cachePaths.remove(filePath);

    if (m_dbs.contains(filePath))
    {
        m_dbs.remove(filePath);
        QSqlDatabase::removeDatabase(filePath);
    }
}

void MBTilesReader::garbageCollect()
{
    Utils::enforceFolderQuota(m_cachePath, 1024);
}
