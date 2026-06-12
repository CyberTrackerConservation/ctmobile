#include "UtilsMap.h"

#include "Envelope.h"
#include "TileInfo.h"
#include "LevelOfDetail.h"
#include "ImageTiledLayer.h"
#include "GraphicsOverlay.h"
#include "LinearUnit.h"
#include "GeometryEngine.h"
#include "PictureMarkerSymbol.h"
#include "FeatureCollection.h"
#include "FeatureCollectionTable.h"
#include "FeatureCollectionLayer.h"
#include "SimpleRenderer.h"
#include "SimpleMarkerSymbol.h"
#include "SimpleLineSymbol.h"
#include "Geometry.h"
#include "Polyline.h"
#include "PolylineBuilder.h"
#include "SpatialReference.h"
#include "Point.h"
#include "TileKey.h"
#include "Graphic.h"
#include "GraphicListModel.h"
#include "SymbolTypes.h"
#include "Feature.h"
#include "FeatureCollectionTableListModel.h"
#include "AttributeListModel.h"
#include "HeatmapRenderer.h"
#include "TaskWatcher.h"
#include "PolygonBuilder.h"
#include "Polygon.h"
#include "Field.h"
#include "PartCollection.h"
#include "Part.h"

using namespace Esri::ArcGISRuntime;

#include "ECW.h"
#include <QtConcurrent/QtConcurrent>
#include <QtEndian>

namespace CyberTracker {
namespace Utils {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MBTilesLayer

class MBTilesLayer: public ImageTiledLayer
{
public:
    MBTilesLayer(const QString& filePath, const TileInfo& tileInfo, const Envelope& fullExtent, QObject* parent):
        ImageTiledLayer(tileInfo, fullExtent, parent)
    {
        setNoDataTileBehavior((NoDataTileBehavior)0);   // NoDataTileBehavior::UpSample

        m_filePath = filePath;
        m_connectionName = Utils::createUuid();

        auto db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
        db.setDatabaseName(filePath);

        qFatalIf(!db.open(), "Failed to open mbtile database");

        connect(this, &MBTilesLayer::tileRequest, this, &MBTilesLayer::getTile);
    }

    ~MBTilesLayer() override
    {
        QSqlDatabase::database(m_connectionName).close();
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    void getTile(const TileKey& tileKey)
    {
        // TMS requires inverting the y.
        auto level = tileKey.level();
        auto row = static_cast<int>((qPow(2, level) - 1) - tileKey.row());
        auto col = tileKey.column();

        // OSM mbtiles generator creates two zoom levels. Ignore the zero level which is just the world.
        if (level == 0)
        {
            setNoDataTile(tileKey);
            return;
        }

        auto db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen())
        {
            qDebug() << "Error: connection" << m_connectionName << "is not open";
            setNoDataTile(tileKey);
            return;
        }

        QSqlQuery query = QSqlQuery(db);
        query.prepare("SELECT tile_data FROM tiles WHERE (zoom_level = (:level)) AND (tile_column = (:column)) AND (tile_row = (:row))");
        query.bindValue(":level", level);
        query.bindValue(":row", row);
        query.bindValue(":column", col);

        if (!query.exec() || !query.next())
        {
            setNoDataTile(tileKey);
            return;
        }

        setTileData(tileKey, query.value(0).toByteArray());
    }

private:
    QString m_filePath;
    QString m_connectionName;
};

Layer* createMBTilesLayer(const QString& filePath, QObject* parent)
{
    auto levelsOfDetail = QList<LevelOfDetail>();
    auto fileExtent = Envelope(-20037508.34, +20037508.34, -20037508.34, +20037508.34, SpatialReference::webMercator());
    auto origin = Point(-20037508.34, +20037508.34, SpatialReference::webMercator());

    auto connectionName = Utils::createUuid();

    auto build = [&]() -> bool
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(filePath);

        if (!db.open())
        {
            qDebug() << "Error: failed to open: " << filePath;
            return false;
        }

        auto query = QSqlQuery(db);

        query.prepare("SELECT name, value FROM metadata");
        if (!query.exec())
        {
            qDebug() << "Error: No metadata found in " << filePath;
            return false;
        }

        auto metadata = QVariantMap();
        while (query.next())
        {
            metadata.insert(query.value(0).toString(), query.value(1).toString());
        }

        auto bounds = metadata.value("bounds").toString().split(',');
        if (bounds.count() == 4)
        {
            fileExtent = Envelope(bounds[0].toDouble(), bounds[1].toDouble(), bounds[2].toDouble(), bounds[3].toDouble(), SpatialReference::wgs84());
        }

        // Lookup metrics.
        query.prepare("SELECT zoom_level, tile_column, tile_row FROM tiles");
        if (!query.exec())
        {
            qDebug() << "Error: No tiles found in " << filePath;
            return false;
        }

        auto levelMin = INT_MAX;
        auto levelMax = INT_MIN;
        while (query.next())
        {
            auto level = query.value(0).toInt();
            levelMax = qMax(level, levelMax);
            levelMin = qMin(level, levelMin);
        }

        double resolution = (20037508.34 * 2) / 256;
        double scale = resolution * 96 * 39.37;

        for (auto i = 0; i <= 24; i++)
        {
            if ((i >= levelMin) && (i <= levelMax))
            {
                levelsOfDetail.append(LevelOfDetail(i, resolution, scale));
            }

            resolution /= 2;
            scale /= 2;
        }

        return true;
    };

    auto result = build();

    QSqlDatabase::database(connectionName).close();
    QSqlDatabase::removeDatabase(connectionName);

    if (!result)
    {
        return nullptr;
    }

    // Success.
    auto tileInfo = TileInfo(96, (TileImageFormat)3 /* TileImageFormat::PNG */, levelsOfDetail, SpatialReference::webMercator(), origin, 256, 256);
    auto fullExtent = Envelope(GeometryEngine::project(fileExtent, SpatialReference::webMercator()));
    return new MBTilesLayer(filePath, tileInfo, fullExtent, parent);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ECWLayer

static std::once_flag s_ecwInitFlag;

constexpr double ECW_EARTH_RADIUS = 6378137.0;
constexpr double ECW_EARTH_CIRC = ECW_EARTH_RADIUS * 2.0 * M_PI;
constexpr double ECW_EARTH_HALF_CIRC = ECW_EARTH_RADIUS * M_PI;

// Describes how to convert between an ECW file's native coordinate space and WGS84.
struct ECWProjection
{
    enum Type { Geodetic, UTM, VirtualEarth };

    Type type = Geodetic;
    SpatialReference fileSR = SpatialReference::wgs84();

    // VirtualEarth-specific.
    double veArc = 0;
    double veK = 0;

    // Convert file-native coordinates to WGS84 (lon, lat).
    void toWgs84(double x, double y, double* lon, double* lat) const
    {
        if (type == VirtualEarth)
        {
            double metersX = x * veArc - ECW_EARTH_HALF_CIRC;
            *lon = (180.0 / M_PI) * (metersX / ECW_EARTH_RADIUS);

            double flippedY = veK - y;
            double metersY = ECW_EARTH_HALF_CIRC - flippedY * veArc;
            double a = exp(metersY * 2.0 / ECW_EARTH_RADIUS);
            *lat = (180.0 / M_PI) * asin((a - 1.0) / (a + 1.0));
        }
        else if (type == UTM)
        {
            auto projected = Point(x, y, fileSR);
            auto wgs84Point = Point(GeometryEngine::project(projected, SpatialReference::wgs84()));
            *lon = wgs84Point.x();
            *lat = wgs84Point.y();
        }
        else
        {
            *lon = x;
            *lat = y;
        }
    }

    // Convert WGS84 (lon, lat) to file-native coordinates.
    void fromWgs84(double lon, double lat, double* x, double* y) const
    {
        if (type == VirtualEarth)
        {
            double metersX = ECW_EARTH_RADIUS * (M_PI / 180.0) * lon;
            *x = (metersX + ECW_EARTH_HALF_CIRC) / veArc;

            double sinLat = sin((M_PI / 180.0) * lat);
            double metersY = (ECW_EARTH_RADIUS / 2.0) * log((1.0 + sinLat) / (1.0 - sinLat));
            *y = veK - (ECW_EARTH_HALF_CIRC - metersY) / veArc;
        }
        else if (type == UTM)
        {
            auto wgs84Point = Point(lon, lat, SpatialReference::wgs84());
            auto projected = Point(GeometryEngine::project(wgs84Point, fileSR));
            *x = projected.x();
            *y = projected.y();
        }
        else
        {
            *x = lon;
            *y = lat;
        }
    }
};

static ECWProjection ecwParseProjection(const char* szProjection, const char* szDatum)
{
    ECWProjection proj;
    char projectionCode[64] = {};
    int projectionValue = 0;

    if (sscanf(szProjection, "%63s %d", projectionCode, &projectionValue) == 2)
    {
        if (strcmp(projectionCode, "UTM") == 0)
        {
            proj.type = ECWProjection::UTM;
            proj.fileSR = SpatialReference(32600 + projectionValue);
        }
        else if (strcmp(projectionCode, "VEA") == 0 || strcmp(projectionCode, "VEF") == 0)
        {
            proj.type = ECWProjection::VirtualEarth;
            proj.veArc = ECW_EARTH_CIRC / ((1 << projectionValue) * 256.0);
            proj.veK = 256.0 * (1 << projectionValue);
        }
    }
    else if (strncmp(szProjection, "NUTM", 4) == 0 && sscanf(szProjection + 4, "%d", &projectionValue) == 1)
    {
        proj.type = ECWProjection::UTM;
        proj.fileSR = SpatialReference(32600 + projectionValue);
    }

    Q_UNUSED(szDatum);

    return proj;
}

// Header fields parsed from the first 89 bytes of an ECW v2+ file. This is
// cheap to obtain (one disk read) and is everything we need to compute the
// layer's extent and LOD list — no SDK call required.
struct ECWHeader
{
    bool ok = false;
    int imageW = 0;
    int imageH = 0;
    int bands = 0;
    double originX = 0, originY = 0;
    double cellIncrX = 0, cellIncrY = 0;
    QByteArray projection;  // ER Mapper projection name, e.g. "GEODETIC", "NUTM33"
    QByteArray datum;       // ER Mapper datum name, e.g. "WGS84", "RAW"
};

// Parse the ECW header without invoking the SDK. The format is:
//   off  0   uint8   magic 'e'
//   off  1   uint8   version (must be >= 2 for georeferencing)
//   off  6   uint32  image width  (big-endian)
//   off 10   uint32  image height (big-endian)
//   off 14   uint16  band count   (big-endian)
//   off 25   ieee8   cell increment X (little-endian)
//   off 33   ieee8   cell increment Y (little-endian)
//   off 41   ieee8   origin X         (little-endian)
//   off 49   ieee8   origin Y         (little-endian)
//   off 57   char[16] datum (null-padded)
//   off 73   char[16] projection (null-padded)
// See erw_decompress_open in 3rdparty/ecwlib/NCSEcw.cpp for the canonical
// reader; sread_int*/sread_ieee8 confirm the per-field byte order.
static ECWHeader ecwReadHeader(const QString& filePath)
{
    ECWHeader out;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
    {
        return out;
    }
    auto buf = f.read(89);
    if (buf.size() < 89)
    {
        return out;
    }

    auto* p = reinterpret_cast<const uchar*>(buf.constData());
    if (p[0] != 'e' || p[1] < 2)
    {
        return out;
    }

    out.imageW = (int)qFromBigEndian<quint32>(p + 6);
    out.imageH = (int)qFromBigEndian<quint32>(p + 10);
    out.bands  = (int)qFromBigEndian<quint16>(p + 14);

    // Doubles are stored little-endian on disk; all our targets are LE so a
    // direct memcpy yields the correct value.
    memcpy(&out.cellIncrX, p + 25, 8);
    memcpy(&out.cellIncrY, p + 33, 8);
    memcpy(&out.originX,   p + 41, 8);
    memcpy(&out.originY,   p + 49, 8);

    auto extractCStr = [](const uchar* src, int max) {
        QByteArray s((const char*)src, max);
        int n = s.indexOf('\0');
        if (n >= 0) s.truncate(n);
        return s;
    };
    out.datum      = extractCStr(p + 57, 16);
    out.projection = extractCStr(p + 73, 16);

    if (out.imageW <= 0 || out.imageH <= 0 || out.cellIncrX == 0 || out.cellIncrY == 0)
    {
        return out;
    }

    out.ok = true;
    return out;
}

// Builds an ECWLayer's TileInfo from the file's native resolution, restricting
// LODs to those that don't oversample relative to the source image.
static TileInfo ecwBuildTileInfo(double nativeMetersPerPixel)
{
    QList<LevelOfDetail> lods;
    double resolution = (20037508.34 * 2) / 256.0;
    double scale = resolution * 96 * 39.37;

    for (int i = 0; i <= 24; i++)
    {
        if (resolution >= nativeMetersPerPixel * 0.5)
        {
            lods.append(LevelOfDetail(i, resolution, scale));
        }
        resolution /= 2;
        scale /= 2;
    }

    auto origin = Point(-20037508.34, 20037508.34, SpatialReference::webMercator());
    return TileInfo(96, (TileImageFormat)3 /* PNG */, lods, SpatialReference::webMercator(), origin, 256, 256);
}

// Opens the ECW file view (the expensive call: builds the QMF pyramid and
// reads the offset table). Safe to call from a worker thread.
static void* ecwOpenFileView(const QString& filePath)
{
    std::call_once(s_ecwInitFlag, []() { NCSecwInit(); });

    void* fileView = nullptr;
    auto path = filePath.toStdString();
    int eError = NCScbmOpenFileView(const_cast<char*>(path.c_str()), &fileView);
    if (eError != 0 || !fileView)
    {
        qDebug() << "ECW: failed to open" << filePath;
        return nullptr;
    }
    return fileView;
}

class ECWLayer: public ImageTiledLayer
{
public:
    ECWLayer(const QString& filePath, const ECWHeader& header,
             const TileInfo& tileInfo, const Envelope& fullExtent, QObject* parent):
        ImageTiledLayer(tileInfo, fullExtent, parent),
        m_filePath(filePath),
        m_imageW(header.imageW), m_imageH(header.imageH), m_bands(header.bands),
        m_originX(header.originX), m_originY(header.originY),
        m_cellIncrX(header.cellIncrX), m_cellIncrY(header.cellIncrY),
        m_proj(ecwParseProjection(header.projection.constData(), header.datum.constData()))
    {
        setNoDataTileBehavior((NoDataTileBehavior)0); // UpSample

        connect(this, &ECWLayer::tileRequest, this, &ECWLayer::getTile);

        // Heavy SDK open runs on the thread pool. When several ECW layers are
        // constructed back-to-back, their opens run in parallel.
        m_fileViewFuture = QtConcurrent::run(ecwOpenFileView, filePath);
    }

    ~ECWLayer() override
    {
        m_fileViewFuture.waitForFinished();

        QMutexLocker lock(&m_mutex);
        if (auto* fv = m_fileViewFuture.result())
        {
            NCScbmCloseFileView(fv);
        }
    }

    void getTile(const TileKey& tileKey)
    {
        constexpr int TILE_SIZE = 256;

        // Wait for the background NCScbmOpenFileView. Blocks the first time
        // around; afterwards the future is already finished.
        void* fileView = m_fileViewFuture.result();
        if (!fileView)
        {
            setNoDataTile(tileKey);
            return;
        }

        // Compute tile extent in Web Mercator.
        auto level = tileKey.level();
        auto row = tileKey.row();
        auto col = tileKey.column();

        double worldSize = 20037508.34 * 2.0;
        double tileSize = worldSize / qPow(2, level);

        double tileXMin = -20037508.34 + col * tileSize;
        double tileYMax = 20037508.34 - row * tileSize;
        double tileXMax = tileXMin + tileSize;
        double tileYMin = tileYMax - tileSize;

        // Convert Web Mercator tile corners to WGS84.
        auto wmEnv = Envelope(tileXMin, tileYMin, tileXMax, tileYMax, SpatialReference::webMercator());
        auto wgs84Env = Envelope(GeometryEngine::project(wmEnv, SpatialReference::wgs84()));
        if (wgs84Env.isEmpty())
        {
            setNoDataTile(tileKey);
            return;
        }

        // Convert WGS84 corners to ECW file coordinates.
        double fxMin, fyMin, fxMax, fyMax;
        m_proj.fromWgs84(wgs84Env.xMin(), wgs84Env.yMax(), &fxMin, &fyMax); // top-left
        m_proj.fromWgs84(wgs84Env.xMax(), wgs84Env.yMin(), &fxMax, &fyMin); // bottom-right

        // Full (unclamped) source rect in ECW image pixel space. cellIncrY is
        // usually negative (file y increases northward, image row increases
        // southward), so fyMax (north edge) becomes the smaller pixel row.
        double fullLeft   = (fxMin - m_originX) / m_cellIncrX;
        double fullRight  = (fxMax - m_originX) / m_cellIncrX;
        double fullTop    = (fyMax - m_originY) / m_cellIncrY;
        double fullBottom = (fyMin - m_originY) / m_cellIncrY;

        if (fullLeft > fullRight) std::swap(fullLeft, fullRight);
        if (fullTop > fullBottom) std::swap(fullTop, fullBottom);

        double fullW = fullRight - fullLeft;
        double fullH = fullBottom - fullTop;
        if (fullW <= 0 || fullH <= 0)
        {
            setNoDataTile(tileKey);
            return;
        }

        // Intersect with the image bounds.
        double clampedLeft   = qMax(0.0, fullLeft);
        double clampedTop    = qMax(0.0, fullTop);
        double clampedRight  = qMin((double)m_imageW, fullRight);
        double clampedBottom = qMin((double)m_imageH, fullBottom);

        if (clampedLeft >= clampedRight || clampedTop >= clampedBottom)
        {
            setNoDataTile(tileKey);
            return;
        }

        // Map the clamped source region to the destination tile's pixel grid.
        // Without this, a tile that only partially overlaps the image would
        // stretch the (clamped) source across the full 256x256 output.
        int dstLeft   = (int)qRound(TILE_SIZE * (clampedLeft   - fullLeft) / fullW);
        int dstRight  = (int)qRound(TILE_SIZE * (clampedRight  - fullLeft) / fullW);
        int dstTop    = (int)qRound(TILE_SIZE * (clampedTop    - fullTop)  / fullH);
        int dstBottom = (int)qRound(TILE_SIZE * (clampedBottom - fullTop)  / fullH);

        dstLeft   = qBound(0, dstLeft,   TILE_SIZE);
        dstRight  = qBound(0, dstRight,  TILE_SIZE);
        dstTop    = qBound(0, dstTop,    TILE_SIZE);
        dstBottom = qBound(0, dstBottom, TILE_SIZE);

        int dstW = dstRight - dstLeft;
        int dstH = dstBottom - dstTop;
        if (dstW <= 0 || dstH <= 0)
        {
            setNoDataTile(tileKey);
            return;
        }

        // Integer source rect. NCScbmSetFileView uses inclusive bottom-right.
        int srcLeft   = qBound(0, (int)qFloor(clampedLeft),       m_imageW - 1);
        int srcTop    = qBound(0, (int)qFloor(clampedTop),        m_imageH - 1);
        int srcRight  = qBound(0, (int)qCeil(clampedRight)  - 1,  m_imageW - 1);
        int srcBottom = qBound(0, (int)qCeil(clampedBottom) - 1,  m_imageH - 1);

        if (srcLeft > srcRight || srcTop > srcBottom)
        {
            setNoDataTile(tileKey);
            return;
        }

        QMutexLocker lock(&m_mutex);

        uint32_t nBands = m_bands < 3 ? 1 : 3;
        uint32_t bandList[3] = { 0, 1, 2 };

        int eError = NCScbmSetFileView(fileView, nBands, bandList,
                                        srcLeft, srcTop, srcRight, srcBottom,
                                        dstW, dstH);
        if (eError != 0)
        {
            setNoDataTile(tileKey);
            return;
        }

        QImage image(TILE_SIZE, TILE_SIZE, QImage::Format_RGBA8888);
        image.fill(Qt::transparent);

        auto buffer = std::vector<uint32_t>(dstW);

        for (int line = 0; line < dstH; line++)
        {
            NCSEcwReadStatus status = NCScbmReadViewLineRGBA(fileView, buffer.data());
            if (status != NCSECW_READ_OK) break;

            auto dst = reinterpret_cast<uint32_t*>(image.scanLine(dstTop + line)) + dstLeft;
            for (int x = 0; x < dstW; x++)
            {
                uint32_t pixel = buffer[x];
                // ECW RGBA is BGRA order with alpha=0. Swap R and B, set alpha to 255.
                uint8_t b = (pixel >> 0) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t r = (pixel >> 16) & 0xFF;
                dst[x] = qRgba(r, g, b, 255);
            }
        }

        QByteArray pngData;
        QBuffer pngBuffer(&pngData);
        pngBuffer.open(QIODevice::WriteOnly);
        image.save(&pngBuffer, "PNG");

        setTileData(tileKey, pngData);
    }

private:
    QString m_filePath;
    int m_imageW, m_imageH, m_bands;
    double m_originX, m_originY;
    double m_cellIncrX, m_cellIncrY;
    ECWProjection m_proj;
    QFuture<void*> m_fileViewFuture;
    QMutex m_mutex;
};

Layer* createECWLayer(const QString& filePath, QObject* parent)
{
    auto header = ecwReadHeader(filePath);
    if (!header.ok)
    {
        qDebug() << "ECW: failed to read header" << filePath;
        return nullptr;
    }

    auto proj = ecwParseProjection(header.projection.constData(), header.datum.constData());

    // Compute WGS84 extent by unprojecting the file's corners, then project to
    // Web Mercator (the layer's spatial reference).
    double lon1, lat1, lon2, lat2;
    proj.toWgs84(header.originX, header.originY, &lon1, &lat1);  // top-left
    double xMax = header.originX + header.imageW * header.cellIncrX;
    double yMin = header.originY + header.imageH * header.cellIncrY;
    proj.toWgs84(xMax, yMin, &lon2, &lat2);                       // bottom-right

    auto wgs84Extent = Envelope(qMin(lon1, lon2), qMin(lat1, lat2),
                                qMax(lon1, lon2), qMax(lat1, lat2),
                                SpatialReference::wgs84());
    auto wmExtent = Envelope(GeometryEngine::project(wgs84Extent, SpatialReference::webMercator()));

    double nativeMetersPerPixel = (wmExtent.xMax() - wmExtent.xMin()) / header.imageW;
    auto tileInfo = ecwBuildTileInfo(nativeMetersPerPixel);
    if (tileInfo.levelsOfDetail().isEmpty())
    {
        qDebug() << "ECW: no valid LODs" << filePath;
        return nullptr;
    }

    return new ECWLayer(filePath, header, tileInfo, wmExtent, parent);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GeoJsonLayer

Layer* createGeoJsonLayer(const QString& /*filePath*/, QObject* parent)
{
    // Create a FeatureCollection and a FeatureCollectionLayer
    auto featureCollection = new FeatureCollection(parent);
    auto featureCollectionLayer = new FeatureCollectionLayer(featureCollection, parent);
    //map->operationalLayers()->append(featureCollectionLayer);

    // Create a FeatureCollectionTable for points with appropriate fields
    FeatureCollectionTable* pointsTable = new FeatureCollectionTable(QList<Field>(), GeometryType::Point, SpatialReference::wgs84(), parent);

    // Define a simple marker symbol for the points
    SimpleMarkerSymbol* pointSymbol = new SimpleMarkerSymbol(SimpleMarkerSymbolStyle::Circle, QColor("red"), 10.0, parent);
    pointsTable->setRenderer(new SimpleRenderer(pointSymbol, parent));

    // Create and add a point feature
    Feature* pointFeature = pointsTable->createFeature(parent);
    pointFeature->setGeometry(Point(-117.195800, 34.056295));
    pointsTable->addFeature(pointFeature);

    // Create a FeatureCollectionTable for polylines
    FeatureCollectionTable* linesTable = new FeatureCollectionTable(QList<Field>(), GeometryType::Polyline, SpatialReference::wgs84(), parent);

    // Define a simple line symbol for the lines
    SimpleLineSymbol* lineSymbol = new SimpleLineSymbol(SimpleLineSymbolStyle::Solid, QColor("blue"), 3.0, parent);
    linesTable->setRenderer(new SimpleRenderer(lineSymbol, parent));

    // Create and add a line feature
    QList<Point> linePoints;
    linePoints.append(Point(-117.195800, 34.056295));
    linePoints.append(Point(-117.918976, 33.812092));

    PolylineBuilder* builder = new PolylineBuilder(SpatialReference::wgs84(), parent);
    builder->addPoint(-117.0, 34.0);
    builder->addPoint(-118.0, 35.0);

    Feature* lineFeature = linesTable->createFeature(parent);
    lineFeature->setGeometry(builder->toPolyline());
    linesTable->addFeature(lineFeature);

    // Add the tables to the feature collection
    featureCollection->tables()->append(pointsTable);
    featureCollection->tables()->append(linesTable);

    return featureCollectionLayer;
}

void appendPointsToOverlay(GraphicsOverlay* overlay, const QVariantMap& layer)
{
    auto points = layer.value("geometry").toList();
    for (auto pointIt = points.constBegin(); pointIt != points.constEnd(); pointIt++)
    {
        auto point = pointIt->toMap();
        auto symbol = Symbol::fromJson(Utils::variantMapToJson(point["symbol"].toMap()));
        auto graphic = new Graphic(Point::fromJson(Utils::variantMapToJson(point)), symbol);
        auto attributes = point;
        attributes.insert("source", layer.value("source", ""));
        graphic->attributes()->setAttributesMap(attributes);
        overlay->graphics()->append(graphic);
    }
}

void appendLinesToOverlay(GraphicsOverlay* overlay, const QVariantMap& layer)
{
    auto symbol = Symbol::fromJson(Utils::variantMapToJson(layer["symbol"].toMap()));
    auto geometry1 = Polyline::fromJson(Utils::variantMapToJson(layer["geometry"].toMap()));
    auto geometry2 = GeometryEngine::densifyGeodetic(geometry1, 1, LinearUnit(LinearUnitId::Kilometers), GeodeticCurveType::Geodesic);
    auto graphic = new Graphic(geometry2, symbol);
    overlay->graphics()->append(graphic);
}

void appendLineToOverlay(Esri::ArcGISRuntime::GraphicsOverlay* overlay, double x1, double y1, double x2, double y2, double width, const QColor& color)
{
    auto symbol = new SimpleLineSymbol(SimpleLineSymbolStyle::ShortDot, color, width);
    auto polylineBuilder = PolylineBuilder(SpatialReference::wgs84());
    polylineBuilder.addPoint(Point(x1, y1));
    polylineBuilder.addPoint(Point(x2, y2));

    auto geometry1 = polylineBuilder.toPolyline();
    auto geometry2 = GeometryEngine::densifyGeodetic(geometry1, 1, LinearUnit(LinearUnitId::Kilometers), GeodeticCurveType::Geodesic);

    auto graphic = new Graphic(geometry2, symbol);
    overlay->graphics()->append(graphic);
}

void appendPointToOverlay(Esri::ArcGISRuntime::GraphicsOverlay* overlay, double x, double y, int size, const QString& url)
{
    auto point = Point(x, y, SpatialReference::wgs84());

    auto symbol = new PictureMarkerSymbol(url);
    symbol->setHeight(size);
    symbol->setWidth(size);
    symbol->setOffsetY(size / 2);
    symbol->setAngle(0);

    auto graphic = new Graphic(point, symbol);
    overlay->graphics()->append(graphic);
}

void processProjections()
{
    // Process the projections and make sure that they are valid according to ESRI.
    auto l = Utils::variantListFromJsonFile("D:/github/ct/tools/projections/concatenated_filtered_data.json");
    auto o = QVariantList();
    for (auto it = l.constBegin(); it != l.constEnd(); it++)
    {
        auto e = it->toMap();
        auto wkid = e["i"].toInt();

        if (wkid == 4326)
        {
            continue;
        }

        auto sr = SpatialReference(wkid);
        if (sr.wkid() == wkid)
        {
            o.append(e);
        }
    }

    o.insert(0, QVariantMap {{ "i", 4326 }, { "d", "tr:Decimal degrees" }});

    Utils::writeJsonToFile("d:/github/ct/config/projections.json", variantListToJson(o, QJsonDocument::Compact));
}

QString getLocationText(const QVariantMap& projection, double x, double y)
{
    auto projected = projectLocation(projection, x, y);
    if (projected.value("error").toBool())
    {
        return "??";
    }

    return projected["text"].toString();
}

QString decimalDegreesToString(double y, double x)
{
    auto absY = qAbs(y);
    auto absX = qAbs(x);
    auto symbol = QChar(0x00B0);   // degrees symbol
    auto yStr = QString::number(absY, 'f', 6) + symbol;
    auto xStr = QString::number(absX, 'f', 6) + symbol;

    // now add the "-" to the start, or append the hemisphere char
    if (y < 0)
    {
        yStr.append(" S");
    }
    else if (y > 0)
    {
        yStr.append(" N");
    }

    if (x < 0)
    {
        xStr.append(" W");
    }
    else if (x > 0)
    {
        xStr.append(" E");
    }

    return QString("%1, %2").arg(yStr, xStr);
}

QVariantMap projectLocation(const QVariantMap& projection, double x, double y)
{
    auto wkId = projection.value("i", 4326).toInt();
    auto aId = projection.value("a").toString();
    auto bId = projection.value("b").toString();
    auto a = (double)0.0;
    auto b = (double)0.0;

    try
    {
        auto point = Point(x, y, SpatialReference::wgs84());
        auto sr = SpatialReference(wkId);
        auto projectedPoint = GeometryEngine::project(point, sr);
        a = projectedPoint.extent().center().x();
        b = projectedPoint.extent().center().y();

        if (std::isnan(a) || std::isnan(b))
        {
            throw std::invalid_argument("Invalid coordinates");
        }

        auto invert = aId == "" || aId == "N" || aId == "Y"; // Swap if y is first.
        if (invert)
        {
            std::swap(a, b);
        }
    }
    catch (...)
    {
        return QVariantMap {{ "error", true }};
    }

    auto aText = QString::number(a, 'f', aId.isEmpty() ? 6 : 1);
    auto bText = QString::number(b, 'f', bId.isEmpty() ? 6 : 1);

    auto text = QString();
    if (aId.isEmpty() || bId.isEmpty())
    {
        text = decimalDegreesToString(y, x);
    }
    else
    {
        text = QString("%1%2, %3%4").arg(
            aText,
            aId != "X" && aId != "Y" ? aId : "",
            bText,
            bId != "X" && bId != "Y" ? bId : "");
    }

    return QVariantMap {
        { "x", x }, { "xText", QString::number(x, 'f', 6) },
        { "y", y }, { "yText", QString::number(y, 'f', 6) },
        { "aId", aId }, { "a", a }, { "aText", aText },
        { "bId", bId }, { "b", b }, { "bText", QString::number(b, 'f', bId.isEmpty() ? 6 : 1) },
        { "text", text },
        { "error", false }
    };
}

QVariantMap unprojectLocation(const QVariantMap& projection, double a, double b)
{
    auto wkId = projection.value("i", 4326).toInt();
    auto aId = projection.value("a").toString();

    auto invert = aId == "" || aId == "N" || aId == "Y"; // Swap if y is first.
    auto x = invert ? b : a;
    auto y = invert ? a : b;

    try
    {
        auto sr = SpatialReference(wkId);
        auto point = Point(x, y, sr);
        auto projectedPoint = GeometryEngine::project(point, SpatialReference::wgs84());
        x = projectedPoint.extent().center().x();
        y = projectedPoint.extent().center().y();

        if (std::isnan(x) || std::isnan(y))
        {
            throw std::invalid_argument("Invalid coordinates");
        }        
    }
    catch (...)
    {
        return QVariantMap {{ "error", true }};
    }

    return QVariantMap {{ "x", x }, { "y", y }, { "error", false }};
}

Symbol* createPictureMarkerSymbol(const QString& svg, int size, QObject* parent)
{
    auto result = new PictureMarkerSymbol(Utils::renderSvgToImage(svg, size, size), parent);
    result->setWidth(size);
    result->setHeight(size);

    return result;
}

Symbol* createTrackArrowSymbol(int lineWidth, int arrowSize, const QColor& color, QObject* parent)
{
    static constexpr char SYMBOL_TEXT[] = R"({
        "type": "CIMSymbolReference",
        "symbol": {
          "type": "CIMLineSymbol",
          "symbolLayers": [{
            "type": "CIMSolidStroke",
            "effects": [{
              "type": "CIMGeometricEffectDashes",
              "dashTemplate": [0.1, 5]
            }],
            "enable": "true",
            "width": _lineWidth,
            "color": [_colorR, _colorG, _colorB, _colorA]
          },
          {
            "type": "CIMVectorMarker",
            "enable": "true",
            "size": _arrowSize,
            "markerPlacement": {
              "type": "CIMMarkerPlacementAlongLineSameSize",
              "endings": "WithMarkers",
              "placementTemplate": [59.5]
            },
            "frame": {
              "xmin": -5,
              "ymin": -5,
              "xmax": 5,
              "ymax": 5
            },
            "markerGraphics": [{
              "type": "CIMMarkerGraphic",
              "geometry": {
                "rings": [
                  [
                    [-8, -4],
                    [-6,  0],
                    [-8,  4],
                    [ 2,  0],
                    [-8, -4]
                  ]
                ]
              },
              "symbol": {
                "type": "CIMPolygonSymbol",
                "symbolLayers": [{
                  "type": "CIMSolidFill",
                  "enable": "true",
                  "color": [_colorR, _colorG, _colorB, _colorA]
                }]
              }
            }]
          }]
        }})";

    auto symbolText = QString(SYMBOL_TEXT);
    symbolText.replace("_lineWidth", QString::number(lineWidth));
    symbolText.replace("_arrowSize", QString::number(arrowSize));
    symbolText.replace("_colorR", QString::number(color.red()));
    symbolText.replace("_colorG", QString::number(color.green()));
    symbolText.replace("_colorB", QString::number(color.blue()));
    symbolText.replace("_colorA", QString::number(color.alpha()));

    return Symbol::fromJson(symbolText.toLatin1(), parent);
}

HeatmapRenderer* createHeatmapRenderer(int blurRadius, double minIntensity, double maxIntensity, QObject* parent)
{
    static constexpr char RENDERER_TEXT[] = R"({
        "type": "heatmap",
        "blurRadius": _blurRadius,
        "maxPixelIntensity": _maxIntensity,
        "minPixelIntensity": _minIntensity,
        "colorStops": [
            { "ratio": 0.0000, "color": [ 133, 193, 200,   0 ] },
            { "ratio": 0.0100, "color": [ 133, 193, 200, 255 ] },
            { "ratio": 0.0925, "color": [ 156, 129, 132, 255 ] },
            { "ratio": 0.1750, "color": [ 156, 129, 132, 255 ] },
            { "ratio": 0.2575, "color": [ 167,  97, 170, 255 ] },
            { "ratio": 0.3400, "color": [ 175,  73, 128, 255 ] },
            { "ratio": 0.4225, "color": [ 184,  48,  85, 255 ] },
            { "ratio": 0.5050, "color": [ 192,  24,  42, 255 ] },
            { "ratio": 0.5875, "color": [ 200,   0,   0, 255 ] },
            { "ratio": 0.6700, "color": [ 211,  51,   0, 255 ] },
            { "ratio": 0.7525, "color": [ 222, 102,   0, 255 ] },
            { "ratio": 0.8350, "color": [ 233, 153,   0, 255 ] },
            { "ratio": 0.9175, "color": [ 244, 204,   0, 255 ] },
            { "ratio": 1.0000, "color": [ 255, 255,   0, 255 ] }
        ]
    })";

    auto rendererText = QString(RENDERER_TEXT);
    rendererText.replace("_blurRadius", QString::number(blurRadius));
    rendererText.replace("_maxIntensity", QString::number(maxIntensity));
    rendererText.replace("_minIntensity", QString::number(minIntensity));

    return qobject_cast<HeatmapRenderer*>(HeatmapRenderer::fromJson(rendererText.toLatin1(), parent));
}

double areaInSquareMeters(const QVariantList& points)
{
    auto wgs84 = SpatialReference::wgs84();
    auto polygonBuilder = PolygonBuilder(wgs84);
    for (auto it = points.constBegin(); it != points.constEnd(); it++)
    {
        auto coords = it->toList();
        auto point = Point(coords[0].toDouble(), coords[1].toDouble(), wgs84);
        polygonBuilder.addPoint(point);
    }

    auto polygon = polygonBuilder.toPolygon();
    auto projectedPolygon = GeometryEngine::project(polygon, SpatialReference::webMercator());

    auto area = GeometryEngine::area(projectedPolygon);

    return std::abs(area);
}

bool testPointInsidePolygon(const Point& point, const SpatialReference& spatialReference, const QJsonArray& rings)
{
    auto parts = PartCollection(spatialReference);

    for (auto ringsIt = rings.constBegin(); ringsIt != rings.constEnd(); ringsIt++)
    {
        auto part = new Part(spatialReference);

        auto points = ringsIt->toArray();
        for (auto pointsIt = points.constBegin(); pointsIt != points.constEnd(); pointsIt++)
        {
            auto coord = pointsIt->toArray();
            if (coord.count() != 2)
            {
                break;
            }

            part->addPoint(coord[0].toDouble(), coord[1].toDouble());
        }

        parts.addPart(part);
    }

    auto polygonBuilder = PolygonBuilder(spatialReference);
    polygonBuilder.setParts(&parts);

    auto polygon = polygonBuilder.toPolygon();
    if (GeometryEngine::contains(polygon, point))
    {
        return true;
    }

    return false;
}

QString getFeatureId(const QJsonObject& feature)
{
    // Pick up the feature id on demand.
    // feature.id = integer
    // feature.properties.fid = integer or string

    if (feature.contains("properties"))
    {
        const auto& properties = feature.value("properties").toObject();
        if (properties.contains("featureId"))
        {
            return properties.value("featureId").toString();
        }
        else if (properties.contains("fid"))
        {
            return properties.value("fid").toVariant().toString();
        }
    }

    if (feature.contains("id"))
    {
        return feature.value("id").toVariant().toString();
    }

    return "?";
}

QStringList testPointInsidePolygonGeoJson(const QString filePath, double x, double y)
{
    auto jsonDocument = jsonDocumentFromFile(filePath);
    if (jsonDocument.isEmpty())
    {
        return {};
    }

    auto result = QStringList();
    auto source = jsonDocument.object();
    auto wgs84 = SpatialReference::wgs84();
    auto point = Point(x, y, wgs84);

    auto features = source["features"].toArray();
    for (auto it = features.constBegin(); it != features.constEnd(); it++)
    {
        auto feature = it->toObject();

        auto featureGeometry = feature.value("geometry").toObject();
        auto type = featureGeometry.value("type").toString();
        if (type  == "MultiPolygon")
        {
            auto polygons = featureGeometry.value("coordinates").toArray();
            for (auto polygonIt = polygons.constBegin(); polygonIt != polygons.constEnd(); polygonIt++)
            {
                auto polygon = polygonIt->toArray();
                if (testPointInsidePolygon(point, wgs84, polygon))
                {
                    result += getFeatureId(feature);
                }
            }
        }
        else if (type == "Polygon")
        {
            auto polygon = featureGeometry.value("coordinates").toArray();
            if (testPointInsidePolygon(point, wgs84, polygon))
            {
                result += getFeatureId(feature);
            }
        }
    }

    return result;
}

} // namespace Utils

} // namespace CyberTracker
