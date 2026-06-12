#include "SMARTRenderer.h"
#include "SMARTProvider.h"
#include "SMARTSchema.h"
#include "App.h"
#include "TrackFile.h"

namespace CyberTracker {

SMARTRenderer::SMARTRenderer(QObject *parent): Renderer(parent)
{
}

QString SMARTRenderer::getPatrolText(bool surveyMode) const
{
    return surveyMode ? tr("Survey") : tr("Patrol");
}

QString SMARTRenderer::getSightingTypeText(const QString& observationType, bool surveyMode) const
{
    if (observationType == SMARTProvider::VALUE_NEW_PATROL)
    {
        return surveyMode ? tr("Start survey") : tr("Start patrol");
    }
    else if (observationType == SMARTProvider::VALUE_STOP_PATROL)
    {
        return surveyMode ? tr("End survey") : tr("End patrol");
    }
    else if (observationType == SMARTProvider::VALUE_PAUSE_PATROL)
    {
        return surveyMode ? tr("Pause survey") : tr("Pause patrol");
    }
    else if (observationType == SMARTProvider::VALUE_RESUME_PATROL)
    {
        return surveyMode ? tr("Resume survey") : tr("Resume patrol");
    }
    else if (observationType == SMARTProvider::VALUE_CHANGE_PATROL)
    {
        return surveyMode ? tr("Change survey") : tr("Change patrol");
    }
    else if (observationType == SMARTProvider::VALUE_OBSERVATION)
    {
        return getPatrolText(surveyMode);
    }
    else
    {
        return "";
    }
}

QString SMARTRenderer::getSightingTypeIcon(const QString& observationType) const
{
    if (observationType == SMARTProvider::VALUE_NEW_PATROL)
    {
        return "qrc:/icons/play.svg";
    }
    else if (observationType == SMARTProvider::VALUE_STOP_PATROL)
    {
        return "qrc:/icons/stop.svg";
    }
    else if (observationType == SMARTProvider::VALUE_PAUSE_PATROL)
    {
        return "qrc:/icons/pause.svg";
    }
    else if (observationType == SMARTProvider::VALUE_RESUME_PATROL)
    {
        return "qrc:/icons/step_forward.svg";
    }
    else if (observationType == SMARTProvider::VALUE_CHANGE_PATROL)
    {
        return "qrc:/icons/account_edit.svg";
    }
    else
    {
        return QString();
    }
}

QString SMARTRenderer::getSightingGroupText(Sighting* sighting) const
{
    if (sighting->stateSpace() != SMARTSchema::TYPE_PATROL)
    {
        return Renderer::getSightingGroupText(sighting);
    }

    auto surveyMode = sighting->schema()->getSetting("surveyMode").toBool();

    auto rootRecordUid = sighting->rootRecordUid();
    auto startDate = QDate::fromString(sighting->getFieldValue(rootRecordUid, SMARTSchema::FIELD_PATROL_START_DATE).toString(), "yyyy/MM/dd");
    auto startTime = QTime::fromString(sighting->getFieldValue(rootRecordUid, SMARTSchema::FIELD_PATROL_START_TIME).toString(), "HH:mm:ss");

    auto startDateTime = QDateTime(startDate, startTime);
    return getPatrolText(surveyMode) + " - " + App::instance()->formatDateTime(startDateTime);
}

QString SMARTRenderer::getSightingSummaryText(Sighting* sighting) const
{
    auto recordUids = sighting->getRootFieldValue(SMARTSchema::FIELD_MODEL).toStringList();
    auto surveyMode = sighting->schema()->getSetting("surveyMode").toBool();
    if (recordUids.isEmpty())
    {
        return getSightingTypeText(sighting->getRootFieldValue(SMARTSchema::FIELD_OBSERVATION_TYPE).toString(), surveyMode);
    }

    auto results = QStringList();

    for (auto it = recordUids.constBegin(); it != recordUids.constEnd(); it++)
    {
        auto modelElementUid = sighting->getFieldValue(*it, SMARTSchema::FIELD_MODEL_TREE, "").toString();
        if (modelElementUid.isEmpty())
        {
            continue;
        }

        auto name = sighting->schema()->getElementName(modelElementUid, sighting->languageCode());
        if (name.isEmpty())
        {
            continue;
        }

        results.append(name);
    }

    return results.join(", ");
}

QString SMARTRenderer::getSightingSummaryIcon(Sighting* sighting) const
{
    auto recordUids = sighting->getRootFieldValue(SMARTSchema::FIELD_MODEL).toStringList();
    if (recordUids.isEmpty())
    {
        return getSightingTypeIcon(sighting->getRootFieldValue(SMARTSchema::FIELD_OBSERVATION_TYPE).toString());
    }

    for (auto it = recordUids.constBegin(); it != recordUids.constEnd(); it++)
    {
        auto modelElementUid = sighting->getFieldValue(*it, SMARTSchema::FIELD_MODEL_TREE).toString();
        if (modelElementUid.isEmpty())
        {
            continue;
        }

        auto icon = sighting->schema()->getElementIcon(modelElementUid, true);
        if (icon.isEmpty())
        {
            continue;
        }

        return icon;
    }

    return QString();
}

QString SMARTRenderer::getSightingStatusIcon(Sighting* sighting, Database::DataFlags flags) const
{
    if (sighting->schema()->getSetting("manualUpload").toBool())
    {
        return QString();
    }

    if (flags.testFlag(Database::DataFlag::DB_UPLOADED_FLAG) || flags.testFlag(Database::DataFlag::DB_EXPORTED_FLAG))
    {
        return "qrc:/icons/cloud_done.svg";
    }

    return "qrc:/icons/cloud_none.svg";
}

QStringList SMARTRenderer::getSightingIcons(Sighting* sighting) const
{
    auto result = QStringList();

    auto recordUids = sighting->getRootFieldValue(SMARTSchema::FIELD_MODEL).toStringList();

    for (auto it = recordUids.constBegin(); it != recordUids.constEnd(); it++)
    {
        auto modelElementUid = sighting->getFieldValue(*it, SMARTSchema::FIELD_MODEL_TREE).toString();
        if (modelElementUid.isEmpty())
        {
            continue;
        }

        auto icon = sighting->schema()->getElementIcon(modelElementUid, true);
        if (icon.isEmpty())
        {
            continue;
        }

        result.append(icon);
    }

    return result;
}

bool SMARTRenderer::getSightingCanEdit(Sighting* sighting, Database::DataFlags flags) const
{
    // Can only edit observations, not metadata.
    auto observationType = sighting->getRootFieldValue(SMARTSchema::FIELD_OBSERVATION_TYPE).toString();
    if (observationType != SMARTProvider::VALUE_OBSERVATION)
    {
        return false;
    }

    // Check if editing is disabled.
    if (sighting->schema()->getSetting("disableEditing", false).toBool())
    {
        return false;
    }

    // Check if marked read only.
    return !flags.testFlag(Database::DataFlag::DB_READONLY_FLAG);
}

bool SMARTRenderer::getSightingCanDelete(Sighting* sighting, Database::DataFlags flags) const
{
    return getSightingCanEdit(sighting, flags);
}

QByteArray SMARTRenderer::getSightingWebhook(Project* project, Sighting* sighting) const
{
    auto rootRecordUid = sighting->rootRecordUid();

    static auto rootFieldUids = QStringList
        {
            SMARTSchema::FIELD_GROUPS,
            SMARTSchema::FIELD_LOCATION,
            SMARTSchema::FIELD_DISTANCE,
            SMARTSchema::FIELD_BEARING,
            SMARTSchema::FIELD_PHOTOS,
            SMARTSchema::FIELD_MODEL,
        };
    auto filterFieldUids = QMap<QString, QStringList>();

    return sightingToJson(project, sighting, [&](const FieldValue& fieldValue) -> bool
    {
        auto record = fieldValue.record();
        auto recordUid = fieldValue.recordUid();
        auto field = fieldValue.field();
        auto fieldUid = fieldValue.fieldUid();

        if (recordUid == rootRecordUid)
        {
            return fieldValue.isValid() &&
                   (rootFieldUids.indexOf(fieldUid) != -1 || field->getTagValue("metadata").toBool());
        }

        if (record->recordField()->uid() == SMARTSchema::FIELD_MODEL && !filterFieldUids.contains(recordUid))
        {
            auto modelElementUid = record->getFieldValue(SMARTSchema::FIELD_MODEL_TREE).toString();
            auto modelElement = sighting->schema()->getElement(modelElementUid);
            auto fieldUids = modelElement->fieldUids();
            fieldUids.append(SMARTSchema::FIELD_SIGHTING_GROUP_ID);
            filterFieldUids.insert(recordUid, fieldUids);
        }

        if (!filterFieldUids.contains(recordUid))
        {
            return true;
        }

        if (filterFieldUids[recordUid].indexOf(field->uid()) != -1)
        {
            return fieldValue.isValid() && fieldValue.isRelevant() && fieldValue.isVisible();
        }

        return false;
    });
}

Renderer::MapSymbol SMARTRenderer::getSightingMapSymbol(Sighting* sighting) const
{
    static auto iconSelect = QMap<QString, QString> {
        { SMARTProvider::VALUE_NEW_PATROL, ":/icons/mark-play.svg" },
        { SMARTProvider::VALUE_STOP_PATROL, ":/icons/mark-stop.svg" },
        { SMARTProvider::VALUE_PAUSE_PATROL, ":/icons/mark-pause.svg" },
        { SMARTProvider::VALUE_RESUME_PATROL, ":/icons/mark-resume.svg" },
        { SMARTProvider::VALUE_CHANGE_PATROL, ":/icons/mark-change.svg" },
    };

    static auto colorSelect = QMap<QString, QColor> {
        { SMARTSchema::TYPE_PATROL, Qt::darkBlue },
        { SMARTSchema::TYPE_INCIDENT, Qt::darkRed },
        { SMARTSchema::TYPE_COLLECT, Qt::darkGreen },
    };

    auto result = Renderer::MapSymbol();
    result.projectTitle = sighting->schema()->title();
    result.color = colorSelect.value(sighting->stateSpace(), Qt::darkCyan);

    auto observationType = sighting->getRootFieldValue(SMARTSchema::FIELD_OBSERVATION_TYPE).toString();
    if (iconSelect.contains(observationType))
    {
        result.url = iconSelect[observationType];
        return result;
    }

    // Model icons.
    auto modelIcon = getSightingSummaryIcon(sighting);
    if (!modelIcon.isEmpty())
    {
        result.filename = modelIcon;
        return result;
    }

    // Default marker.
    result.url = ":/icons/mark-circle.svg";
    return result;
}

QVariantList SMARTRenderer::buildSightingModel(Sighting* sighting) const
{
    auto recordModel = QVariantList();

    auto groupList = sighting->getRootFieldValue(SMARTSchema::FIELD_GROUPS).toStringList();
    auto recordUids = sighting->getRootFieldValue(SMARTSchema::FIELD_MODEL).toStringList();
    auto rootRecordUid = sighting->rootRecordUid();

    auto schema = sighting->schema();
    auto surveyMode = schema->getSetting("surveyMode", false) == true;
    auto useDistanceAndBearing = schema->getSetting("useDistanceAndBearing", false) == true;
    auto useGroupUI = schema->getSetting("useGroupUI", false) == true;
    auto useObserver = schema->getSetting("useObserver", false) == true;

    // Add timestamp.
    recordModel.append(QVariantMap {
        { "delegate", "timestamp" },
        { "createTime", sighting->createTime() } });

    // Add location.
    auto locationFieldValue = sighting->getRootFieldValue(SMARTSchema::FIELD_LOCATION);
    if (locationFieldValue.isValid())
    {
        recordModel.append(QVariantMap {
            { "delegate", "field" },
            { "fieldType", "LocationField" },
            { "value", locationFieldValue } });
    }

    // Add observer.
    if (useObserver)
    {
        auto observer = sighting->getRootFieldValue(SMARTSchema::FIELD_OBSERVER).toString();
        if (!observer.isEmpty())
        {
            recordModel.append(QVariantMap {
                { "delegate", "field" },
                { "fieldName", sighting->getRecord(rootRecordUid)->getFieldName(SMARTSchema::FIELD_OBSERVER)  },
                { "fieldIcon", "" },
                { "fieldValue", sighting->getRecord(rootRecordUid)->getFieldDisplayValue(SMARTSchema::FIELD_OBSERVER) },
                { "recordUid", rootRecordUid },
                { "depth", 1 } });
        }
    }

    // Add distance and bearing.
    if (useDistanceAndBearing)
    {
        auto distance = sighting->getRootFieldValue(SMARTSchema::FIELD_DISTANCE);
        auto bearing = sighting->getRootFieldValue(SMARTSchema::FIELD_BEARING);
        if (distance.isValid() && bearing.isValid())
        {
            recordModel.append(QVariantMap {
                { "delegate", "field" },
                { "fieldName", sighting->getRecord(rootRecordUid)->getFieldName(SMARTSchema::FIELD_DISTANCE) },
                { "fieldIcon", "" },
                { "fieldValue", distance },
                { "recordUid", rootRecordUid },
                { "depth", 1 } });

            recordModel.append(QVariantMap {
                { "delegate", "field" },
                { "fieldName", sighting->getRecord(rootRecordUid)->getFieldName(SMARTSchema::FIELD_BEARING) },
                { "fieldIcon", "" },
                { "fieldValue", bearing },
                { "recordUid", rootRecordUid },
                { "depth", 1 } });
        }
    }

    // Build records group at a time.
    auto modelStart = recordModel.count();

    if (!groupList.isEmpty())
    {
        for (auto groupIt = groupList.constBegin(); groupIt != groupList.constEnd(); groupIt++)
        {
            auto groupUid = *groupIt;

            recordModel.append(QVariantMap {
                { "delegate", "group" },
                { "caption", useGroupUI ? tr("Group") + " " + QString::number(groupList.indexOf(groupUid) + 1) : tr("Observation") }
            });

            auto modelFieldUids = QStringList();

            buildRecordModel(sighting, &recordModel, [&](const FieldValue& fieldValue, int depth) -> bool
            {
                if (depth == 0)
                {
                    return false;
                }

                // Skip records from other groups.
                auto groupRecordUid = depth == 1 ? fieldValue.recordUid() : fieldValue.record()->parentRecordUid();
                if (sighting->getRecord(groupRecordUid)->getFieldValue(SMARTSchema::FIELD_SIGHTING_GROUP_ID) != groupUid)
                {
                    return false;
                }

                // Skip group id.
                if (fieldValue.fieldUid() == SMARTSchema::FIELD_SIGHTING_GROUP_ID)
                {
                    return false;
                }

                // Special handling for the model row.
                if (fieldValue.fieldUid() == SMARTSchema::FIELD_MODEL_TREE)
                {
                    auto elementUid = fieldValue.value().toString();

                    recordModel.append(QVariantMap {
                        { "delegate", "element" },
                        { "elementUid", elementUid },
                        { "caption", fieldValue.displayValue() },
                        { "icon", App::instance()->resolveArchiveUrl(sighting->getElementIconHash(elementUid)) }
                    });
                    modelFieldUids = schema->getElement(elementUid)->fieldUids();
                    return false;
                }

                // Skip fields which are not part of the selected model. This happens when defaults are specified.
                if (depth == 1 && modelFieldUids.indexOf(fieldValue.fieldUid()) == -1)
                {
                    return false;
                }

                if (depth == 2 && modelFieldUids.indexOf(fieldValue.parentFieldUid()) == -1)
                {
                    return false;
                }

                return true;
            });
        }
    }

    // Fancy logic to add headers and dividers to make complex records more readable.
    auto modelIndex = modelStart;
    while (modelIndex <= recordModel.count() - 1)
    {
       auto lastMap = recordModel.at(modelIndex - 1).toMap();
       auto lastDepth = lastMap.value("depth", 0).toInt();
       auto lastRecordUid = lastMap.value("recordUid").toString();
       auto currMap = recordModel.at(modelIndex).toMap();
       auto currDepth = currMap.value("depth", 0).toInt();
       auto currRecordUid = currMap.value("recordUid").toString();

       auto addDivider = false;
       addDivider = addDivider || (currDepth > 1 && lastMap.value("depth") != currMap.value("depth"));
       addDivider = addDivider || (currDepth > 1 && lastMap.value("recordUid") != currMap.value("recordUid"));
       addDivider = addDivider || (lastDepth > currDepth && lastDepth > 1);
       if (addDivider)
       {
           lastMap["divider"] = true;
           recordModel.replace(modelIndex - 1, lastMap);
       }

       auto addHeader = currDepth > 1 && lastRecordUid != currRecordUid;
       if (addHeader)
       {
           recordModel.insert(modelIndex, QVariantMap {
               { "delegate", "recordHeader" },
               { "text", sighting->getRecord(currRecordUid)->name() },
               { "divider", true } });
           modelIndex++;
       }

       modelIndex++;
    }

    // Add patrol metadata.
    if (sighting->stateSpace() == SMARTSchema::TYPE_PATROL)
    {
        // Add title.
        auto map = QVariantMap();
        map["delegate"] = "group";
        auto observationType = sighting->getRootFieldValue(SMARTSchema::FIELD_OBSERVATION_TYPE).toString();
        map["caption"] = getSightingTypeText(observationType, surveyMode);
        recordModel.append(map);

        // Add leg data if on patrol.
        auto legText = "#" + sighting->getRootFieldValue(SMARTSchema::FIELD_PATROL_LEG_COUNTER).toString() + " - " +
            App::instance()->getDistanceText(sighting->getRootFieldValue(SMARTSchema::FIELD_PATROL_LEG_DISTANCE, 0.0).toDouble());

        recordModel.append(QVariantMap {
            { "delegate", "field" },
            { "fieldName", tr("Leg") },
            { "fieldIcon", "" },
            { "fieldValue", legText },
            { "recordUid", rootRecordUid },
            { "depth", 1 } });

        buildRecordModel(sighting, &recordModel, [&](const FieldValue& fieldValue, int depth) -> bool
        {
            if (depth != 0)
            {
                return false;
            }

            if (fieldValue.fieldUid() == SMARTSchema::FIELD_LOCATION)
            {
                return false;
            }

            if (fieldValue.recordUid() != rootRecordUid)
            {
                return false;
            }

            if (!fieldValue.field()->getTagValue("metadata").toBool())
            {
                return false;
            }

            return true;
        });
    }

    // Add TrackFile.
    if (!sighting->trackFile().isEmpty())
    {
        // Add a title.
        recordModel.append(QVariantMap {
            { "delegate", "group" },
            { "caption", tr("Track") }
        });
        modelIndex++;

        auto trackFile = TrackFile(App::instance()->getMediaFilePath(sighting->trackFile()));
        recordModel.append(QVariantMap {
            { "delegate", "trackFile" },
            { "points", trackFile.getPoints() },
            { "summary", trackFile.summary() }});
        modelIndex++;
    }

    return recordModel;
}

} // namespace CyberTracker