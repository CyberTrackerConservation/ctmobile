#include "Place.h"
#include "Utils.h"
#include "App.h"

namespace CyberTracker {

static QString blockSignature(const QVariantList& blocks)
{
    return Utils::variantListToJson(blocks, QJsonDocument::Compact);
}

// PlaceRow.
void PlaceRow::load(const QVariantMap& source)
{
    path.clear();
    auto pathArray = source["path"].toList();
    for (auto it = pathArray.constBegin(); it != pathArray.constEnd(); it++)
    {
        path.append(it->toString());
    }

    auto view = source["view"].toMap();
    name = view["name"].toString();
    title = view["title"].toString();
    subtitle = view["subtitle"].toString();
    blocks = view["blocks"].toList();
}

QString PlaceRow::hash() const
{
    auto text = path.join('\x1f') + '\x1f' + title + '\x1f' + subtitle + '\x1f' + blockSignature(blocks);
    return QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5).toHex();
}

// Place.
Place::Place(QObject *parent): QObject(parent)
{
}

void Place::clear()
{
    m_items.clear();
    m_rows.clear();
    m_packageLanguage.clear();
    m_defaultLocale.clear();
}

bool Place::isEmpty() const
{
    return m_rows.isEmpty();
}

void Place::merge(const QString& filePath, const QStringList& featureIds)
{
    auto root = Utils::variantMapFromJsonFile(filePath);

    auto package = root["package"].toMap();
    if (m_packageLanguage.isEmpty())
    {
        m_packageLanguage = package["language"].toString();
        m_defaultLocale = package["defaultLocale"].toString();
    }

    auto datasets = root["datasets"].toList();
    for (auto datasetIt = datasets.constBegin(); datasetIt != datasets.constEnd(); datasetIt++)
    {
        auto dataset = datasetIt->toMap();

        auto filter = dataset["filter"].toMap();

        auto datasetRefs = filter["featureIds"].toList();
        auto matched = false;
        for (auto refIt = datasetRefs.constBegin(); refIt != datasetRefs.constEnd(); refIt++)
        {
            auto ref = refIt->toString();
            if (ref == "*" || featureIds.contains(ref))
            {
                matched = true;
                break;
            }
        }

        if (!matched)
        {
            continue;
        }

        if (filter.contains("when"))
        {
            auto when = filter["when"].toMap();
            auto now = QDateTime::currentDateTime();

            auto startStr = when["start"].toString();
            if (!startStr.isEmpty())
            {
                auto start = QDateTime::fromString(startStr, Qt::ISODate);
                if (start.isValid() && now < start)
                {
                    continue;
                }
            }

            auto endStr = when["end"].toString();
            if (!endStr.isEmpty())
            {
                auto end = QDateTime::fromString(endStr, Qt::ISODate);
                if (end.isValid() && now > end)
                {
                    continue;
                }
            }
        }

        auto rows = dataset["rows"].toList();
        for (auto rowIt = rows.constBegin(); rowIt != rows.constEnd(); rowIt++)
        {
            auto row = PlaceRow();
            row.load(rowIt->toMap());
            if (row.path.isEmpty() || row.title.isEmpty())
            {
                continue;
            }

            auto rowHash = row.hash();
            m_items.insert(rowHash, row);

            m_rows.append('/' + row.path.join('/') + '/' + rowHash);
        }
    }

    m_rows.sort();
    m_rows.removeDuplicates();
}

QVariantList Place::buildModel(const QStringList& stack)
{
    auto prefix = '/' + stack.join('/');
    if (!prefix.endsWith('/'))
    {
        prefix += '/';
    }

    auto result = QVariantList();
    auto strings = QStringList();

    for (auto rowIt = m_rows.constBegin(); rowIt != m_rows.constEnd(); rowIt++)
    {
        auto row = *rowIt;
        if (!row.startsWith(prefix))
        {
            continue;
        }

        row.remove(0, prefix.length());
        auto items = row.split('/');
        if (items.isEmpty())
        {
            qDebug() << "Bad row:" << *rowIt;
            continue;
        }

        if (items.count() > 1)
        {
            auto name = items.first();
            if (!strings.contains(name))
            {
                strings.append(name);
                result.append(QVariant(QVariantMap {{ "name", name }}));
            }

            continue;
        }

        auto item = m_items[items.first()];
        auto blocks = QVariantList();
        for (auto blockIt = item.blocks.constBegin(); blockIt != item.blocks.constEnd(); blockIt++)
        {
            blocks.append(*blockIt);
        }

        result.append(QVariant(QVariantMap
        {
            { "name", item.name },
            { "title", item.title },
            { "subtitle", item.subtitle },
            { "blocks", blocks },
        }));
    }

    std::sort(result.begin(), result.end(), [&](const QVariant& f1, const QVariant& f2) -> bool
    {
        return f1.toMap()["name"].toString() < f2.toMap()["name"].toString();
    });

    return result;
}

bool Place::search(double x, double y)
{
    clear();

    auto mapManager = App::instance()->offlineMapManager();
    const auto layers = mapManager->getLayers();

    for (const auto& layerId: layers)
    {
        auto layer = mapManager->getLayer(layerId);
        if (!layer.lookup)
        {
            continue;
        }

        if (!layer.active)
        {
            continue;
        }

        auto package = mapManager->getPackage(layer.packageId);
        auto filePath = mapManager->layersPath() + "/" + package.folder + "/" + layer.filename;
        auto fileInfo = QFileInfo(filePath);
        auto rootFilePath = fileInfo.dir().filePath(fileInfo.completeBaseName());

        auto matchedRefs = Utils::testPointInsidePolygonGeoJson(rootFilePath + ".geojson", x, y);
        if (matchedRefs.isEmpty())
        {
            continue;
        }

        auto metadataFilePath = rootFilePath + ".json";
        if (!QFile::exists(metadataFilePath))
        {
            qDebug() << "Error: metadata file does not exist: " << metadataFilePath;
            continue;
        }

        merge(metadataFilePath, matchedRefs);
    }

    return !m_rows.isEmpty();
}

} // namespace CyberTracker

