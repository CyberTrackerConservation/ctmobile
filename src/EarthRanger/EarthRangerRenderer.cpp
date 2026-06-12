#include "EarthRangerRenderer.h"
#include "EarthRangerSchema.h"

namespace CyberTracker {

EarthRangerRenderer::EarthRangerRenderer(QObject* parent): Renderer(parent)
{
}

QString EarthRangerRenderer::getSightingSummaryText(Sighting* sighting) const
{
    auto reportElementUid = sighting->getRootFieldValue("reportUid").toString();
    if (reportElementUid.isEmpty())
    {
        return QString();
    }

    return sighting->schema()->getElementName(reportElementUid, sighting->languageCode());
}

QString EarthRangerRenderer::getSightingSummaryIcon(Sighting* sighting) const
{
    auto reportElementUid = sighting->getRootFieldValue("reportUid").toString();
    if (reportElementUid.isEmpty())
    {
        return QString();
    }

    return sighting->schema()->getElementIcon(reportElementUid);
}

QString EarthRangerRenderer::getSightingStatusIcon(Sighting* /*sighting*/, Database::DataFlags flags) const
{
    if (flags.testFlag(Database::DataFlag::DB_UPLOADED_FLAG))
    {
        return "qrc:/icons/cloud_done.svg";
    }

    return "qrc:/icons/cloud_queue.svg";
}

QByteArray EarthRangerRenderer::getSightingWebhook(Project* project, Sighting* sighting) const
{
    auto reportElementUid = sighting->getRootFieldValue("reportUid").toString();
    auto reportElement = sighting->schema()->getElement(reportElementUid);
    auto filterFieldUids = reportElement->fieldUids();

    static auto includedFieldUids = QStringList {
        "reportUid",
        EarthRangerSchema::FIELD_LOCATION,
        EarthRangerSchema::FIELD_PHOTOS,
        EarthRangerSchema::FIELD_NOTES,
    };

    return sightingToJson(project, sighting, [&](const FieldValue& fieldValue) -> bool
    {
        if (!fieldValue.isValid() || !fieldValue.isRelevant() || !fieldValue.isVisible())
        {
            return false;
        }

        auto fieldUid = fieldValue.fieldUid();
        if (includedFieldUids.contains(fieldUid))
        {
            return true;
        }

        if (filterFieldUids.contains(fieldUid))
        {
            return true;
        }

        return false;
    });
}

} // namespace CyberTracker