#include "ClassicRenderer.h"
#include "App.h"
#include "TrackFile.h"

namespace CyberTracker {

ClassicRenderer::ClassicRenderer(QObject* parent): Renderer(parent)
{
}

QVariantList ClassicRenderer::buildSightingModel(Sighting* sighting) const
{
    auto attributes = sighting->classicData();

    // Fall back to default rendering for old sightings without classic data.
    if (attributes.isEmpty())
    {
        return Renderer::buildSightingModel(sighting);
    }

    auto result = QVariantList();
    auto schema = sighting->schema();
    auto languageCode = sighting->languageCode();

    // Add timestamp.
    result.append(QVariantMap {
        { "delegate", "timestamp" },
        { "createTime", sighting->createTime() } });

    // Add top-level location.
    auto locationFieldValue = sighting->locationMap();
    if (!locationFieldValue.isEmpty())
    {
        result.append(QVariantMap {
            { "delegate", "field" },
            { "fieldType", "LocationField" },
            { "value", locationFieldValue } });
    }

    // Add a title.
    if (!sighting->trackOnly())
    {
        result.append(QVariantMap {
            { "delegate", "group" },
            { "caption", tr("Data") }
        });
    }

    // Build field delegates from classic attributes.
    for (auto attributeIt = attributes.constBegin(); attributeIt != attributes.constEnd(); attributeIt++)
    {
        auto attribute = attributeIt->toMap();
        auto elementUid = attribute.value("elementUid").toString();
        auto value = attribute.value("fieldValue", attribute.value("value"));

        auto field = schema->getField(elementUid);
        if (!field || field->hidden())
        {
            continue;
        }

        // Skip location fields (already shown at top).
        if (qobject_cast<const LocationField*>(field))
        {
            continue;
        }

        auto fieldType = schema->getFieldType(elementUid);
        auto fieldName = schema->getFieldName(elementUid, languageCode);

        auto elementValue = schema->getElementName(value.toString(), languageCode);
        if (!elementValue.isEmpty())
        {
            value = elementValue;
        }

        auto fieldIcon = QString();
        auto iconHash = sighting->getFieldIconHash(elementUid);
        if (!iconHash.isEmpty())
        {
            fieldIcon = "file:///" + App::instance()->getArchiveFilePath(iconHash);
        }

        auto map = QVariantMap {
            { "delegate", "field" },
            { "fieldUid", elementUid },
            { "fieldType", fieldType },
            { "fieldName", fieldName },
            { "fieldIcon", fieldIcon },
            { "fieldValue", value },
            { "value", value },
            { "recordUid", sighting->rootRecordUid() },
            { "depth", 0 }
        };

        // Tag photo fields as classic photos.
        if (fieldType == "PhotoField")
        {
            map["classicPhoto"] = true;
        }

        result.append(map);
    }

    // Add TrackFile.
    if (sighting->trackFile().isEmpty())
    {
        if (attributes.empty())
        {
            result.append(QVariantMap {
                { "delegate", "text" },
                { "caption", tr("No data") }
            });
        }
    }
    else
    {
        result.append(QVariantMap {
            { "delegate", sighting->trackOnly() ? "nav" : "group" },
            { "caption", tr("Track") }
        });

        auto trackFile = TrackFile(App::instance()->getMediaFilePath(sighting->trackFile()));
        result.append(QVariantMap {
            { "delegate", "trackFile" },
            { "points", trackFile.getPoints() },
            { "summary", trackFile.summary() }});
    }

    return result;
}

QStringList ClassicRenderer::getSightingIcons(Sighting* sighting) const
{
    auto result = QStringList();

    auto attributes = sighting->classicData();

    for (auto it = attributes.constBegin(); it != attributes.constEnd(); it++)
    {
        auto elementUid = it->toMap()["elementUid"].toString();
        if (elementUid.isEmpty())
        {
            continue;
        }

        auto icon = sighting->schema()->getElementIcon(elementUid, false);
        if (!icon.isEmpty())
        {
            result.append(icon);
        }

        auto valueUid = it->toMap()["value"].toString();
        icon = sighting->schema()->getElementIcon(valueUid);
        if (!icon.isEmpty())
        {
            result.append(icon);
        }
    }

    return result;
}

bool ClassicRenderer::getSightingCanDelete(Sighting* /*sighting*/, Database::DataFlags flags) const
{
    return flags.testFlag(Database::DataFlag::DB_COMPLETED_FLAG);
}

} // namespace CyberTracker
