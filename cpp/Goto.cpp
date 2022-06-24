#include "Goto.h"
#include "App.h"

#include "jlcompress.h"
#include "quazip.h"
#include "quazipfile.h"

constexpr bool CACHE_ENABLE = true;

GotoManager::GotoManager(QObject* parent): QObject(parent)
{
    m_gotoHeading = 0;
    m_gotoPathIndex = 0;
    m_symbolScale = App::instance()->guiApplication()->primaryScreen()->physicalDotsPerInch() / 96;

    loadState();
}

GotoManager::~GotoManager()
{
}

void GotoManager::loadState()
{
    auto state = App::instance()->settings()->gotoManager();
    m_gotoTitle = state.value("title", QString()).toString();
    m_gotoPath = state.value("path", QVariantList()).toList();
    m_gotoPathIndex = state.value("pathIndex", 0).toInt();
}

void GotoManager::saveState()
{
    App::instance()->settings()->set_gotoManager(QVariantMap
    {
        { "title", m_gotoTitle },
        { "path", m_gotoPath },
        { "pathIndex", m_gotoPathIndex }
    });
}

void GotoManager::setTarget(const QString& title, const QVariantList& path, int pathIndex)
{
    pathIndex = std::min(pathIndex, path.count() - 1);
    pathIndex = std::max(pathIndex, 0);

    update_gotoTitle(title);
    update_gotoPath(path);
    update_gotoPathIndex(pathIndex);

    saveState();

    emit gotoTargetChanged(title);
}

QVariantList GotoManager::findPath(int id)
{
    qFatalIf(id < 0 || id >= m_paths.count(), "Bad path index");
    return m_paths[id].toList();
}

bool GotoManager::bootstrap(const QString& zipFilePath)
{
    auto zipData = QVariantMap();
    if (!getZipContents(zipFilePath, zipData))
    {
        return false;
    }

    auto zipTargetFilePath = App::instance()->gotoPath() + "/" + zipData["filename"].toString();

    auto jsonData = QJsonDocument(QJsonObject::fromVariantMap(zipData)).toJson();
    Utils::writeJsonToFile(zipTargetFilePath, jsonData);

    QFile::remove(zipFilePath);

    m_cache.clear();
    m_paths.clear();

    return true;
}

QVariantList& GotoManager::getLayers()
{
    if (m_cache.count() == 0 || !CACHE_ENABLE)
    {
        m_cache.clear();
        m_paths.clear();

        auto dir = QDir(App::instance()->gotoPath());

        auto availableFiles = QFileInfoList(dir.entryInfoList(QStringList({"*.json"}), QDir::Files));
        for (auto it = availableFiles.constBegin(); it != availableFiles.constEnd(); it++)
        {
            auto fileData = Utils::variantMapFromJsonFile(it->filePath());
            m_cache.append(transform(fileData));
        }
    }

    return m_cache;
}

void GotoManager::removeLayer(const QString& id)
{
    auto filePath = App::instance()->gotoPath() + "/" + id;
    QFile::remove(filePath);
    m_cache.clear();
}

void GotoManager::install(const QVariantMap& layer)
{
    auto targetFilePath = App::instance()->gotoPath() + "/" + layer["filename"].toString();
    auto jsonData = QJsonDocument(QJsonObject::fromVariantMap(layer)).toJson();
    Utils::writeJsonToFile(targetFilePath, jsonData);
    m_cache.clear();
}

bool GotoManager::getZipContents(const QString& zipFile, QVariantMap& contents)
{
    QuaZip zip(zipFile);
    if (!zip.open(QuaZip::mdUnzip))
    {
        return false;
    }

    // Load project.json
    zip.setCurrentFile("project.json", QuaZip::csInsensitive);
    if (!zip.hasCurrentFile())
    {
        return false;
    }

    QuaZipFile projectFile(&zip);
    if (!projectFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Bad project.json";
        return false;
    }

    QJsonParseError jsonErrorCode;
    auto projectMap = QJsonDocument::fromJson(projectFile.readAll(), &jsonErrorCode).object().toVariantMap();
    if (jsonErrorCode.error != QJsonParseError::NoError)
    {
        qDebug() << "Bad project.json";
        return false;
    }

    // Must have these fields to be considered a valid SMART zip.
    if (!projectMap.contains("decoder") ||
        !projectMap.contains("projectName") ||
        !projectMap.contains("definition"))
    {
        qDebug() << "Missing fields from project.json";
        return false;
    }

    // Load ct_profile.json.
    zip.setCurrentFile(projectMap["definition"].toString(), QuaZip::csInsensitive);
    if (!zip.hasCurrentFile())
    {
        return false;
    }

    QuaZipFile dataFile(&zip);
    if (!dataFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Missing definition json";
        return false;
    }

    contents = QJsonDocument::fromJson(dataFile.readAll(), &jsonErrorCode).object().toVariantMap();
    if (jsonErrorCode.error != QJsonParseError::NoError)
    {
        qDebug() << "Bad definition json";
        return false;
    }

    contents["filename"] = projectMap["definition"].toString();
    contents["decoder"] = projectMap["decoder"].toString();
    contents["name"] = projectMap["projectName"].toString();

    return true;
}

QVariantMap GotoManager::transform(const QVariantMap& source)
{
    auto result = QVariantMap();

    result["id"] = source["filename"];
    result["name"] = source["name"];

    auto extentRect = QRectF();
    result["polylines"] = buildLayerPolylines(source, &extentRect);
    result["points"] = buildLayerPoints(source, &extentRect);
    result["extent"] = QVariantMap
    {
        { "spatialReference", QVariantMap {{ "wkid", 4326 }} },
        { "xmin", extentRect.left() },
        { "ymin", extentRect.top() },
        { "xmax", extentRect.right() },
        { "ymax", extentRect.bottom() }
    };

    return result;
}

QVariantMap GotoManager::makePointSymbol(const QVariantMap& style)
{
    auto outlineSymbol = QVariantMap
    {
        { "symbolType", "SimpleLineSymbol" },
        { "color", QVariantList { 0, 0, 0, 255 } },
        { "style", "esriSLSSolid" },
        { "type", "esriSLS" },
        { "width", 0.5 * m_symbolScale },
    };

    auto styleColor = QColor('#' + style["marker-color"].toString());
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
        { "style", styleMap[style["marker-style"].toString()] },
        { "type", "esriSMS" },
        { "size", style["marker-size"].toDouble() * m_symbolScale },
        { "outline", outlineSymbol },
        { "angle", 0.0 },
        { "yoffset", 0.0 },
    };
}

QVariantMap GotoManager::buildLayerPoints(const QVariantMap& source, QRectF* extentOut)
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
            { "pathId", m_paths.count() },
            { "symbol", symbol },
        });

        m_paths.append(pathPoints);

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
            addPoint(name, coordinates[0].toDouble(), coordinates[1].toDouble(), makePointSymbol(style), pathPoints);
            continue;
        }

        // Add a point for the front and back of a line-string.
        if (geometry.value("type") == "LineString")
        {
            pathPoints = geometry.value("coordinates").toList();
            auto symbol = makePointSymbol(QVariantMap { { "marker-color", style["stroke-color"] }, { "marker-style", "circle" }, { "marker-size", 6 } });

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
        { "geometryType", "Points" },
        { "geometry", points },
        { "source", source["name"] }
    };
}

QVariantMap GotoManager::makeLineSymbol(const QVariantMap& style)
{
    auto styleColor = QColor('#' + style["stroke-color"].toString());

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
        { "style", styleMap[style["stroke-style"].toString()] },
        { "type", "esriSLS" },
        { "width", style["stroke-size"].toDouble() * m_symbolScale },
    };
}

QVariantList GotoManager::buildLayerPolylines(const QVariantMap& source, QRectF* extentOut)
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
            { "name",  featureMap.value("properties").toMap().value("id").toString() },
            { "geometryType", "Polyline" },
            { "geometry", QVariantMap { { "spatialReference", QVariantMap {{ "wkid", 4326 }} }, { "paths", paths }} },
            { "symbol", makeLineSymbol(featureMap.value("style").toMap()) },
        });
    }

    return result;
}

void GotoManager::recalculate(const QVariantMap& lastLocation)
{
    if (m_gotoPath.isEmpty() || lastLocation.isEmpty())
    {
        auto text = m_gotoPath.isEmpty() ? "-" : tr("No fix");
        update_gotoTitle(text);
        update_gotoPath(QVariantList());
        update_gotoTitleText(text);
        update_gotoLocationText(text);
        update_gotoDistanceText(text);
        update_gotoHeadingText(text);
        update_gotoHeading(0);
        saveState();

        return;
    }

    auto gotoPoint = m_gotoPath[m_gotoPathIndex].toList();
    auto gotoX = gotoPoint[0].toDouble();
    auto gotoY = gotoPoint[1].toDouble();
    auto currX = lastLocation["x"].toDouble();
    auto currY = lastLocation["y"].toDouble();
    auto distance = static_cast<int>(Utils::distanceInMeters(currY, currX, gotoY, gotoX));

    auto titleText = m_gotoTitle;
    if (m_gotoPath.count() > 1)
    {
        // Advance to the next point.
        if (distance < 40 && m_gotoPathIndex < m_gotoPath.count() - 1)
        {
            update_gotoPathIndex(m_gotoPathIndex++);
            recalculate(lastLocation);
            return;
        }

        titleText += " (" + QString::number(m_gotoPathIndex + 1) + "/" + QString::number(m_gotoPath.count()) + ")";
    }

    // Update the public properties.
    update_gotoTitleText(titleText);
    update_gotoLocationText(App::instance()->getLocationText(gotoX, gotoY));
    update_gotoDistanceText(App::instance()->getDistanceText(distance));
    update_gotoLocation(QVariantMap {{"x", gotoX}, {"y", gotoY}});

    auto heading = Utils::headingInDegrees(currY, currX, gotoY, gotoX);
    update_gotoHeadingText(Utils::headingToText(heading));
    update_gotoHeading((360 + heading - lastLocation["d"].toInt()) % 360);

    saveState();
}
