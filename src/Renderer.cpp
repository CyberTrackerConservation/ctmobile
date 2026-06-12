#include "Renderer.h"
#include "TrackFile.h"
#include "App.h"

namespace CyberTracker {

Renderer::Renderer(QObject* parent): QObject(parent)
{
}

QString Renderer::getSightingGroupText(Sighting* /*sighting*/) const
{
    return QString();
}

QString Renderer::getSightingSummaryText(Sighting* sighting) const
{
    return sighting->trackOnly() ? tr("Track") : sighting->summaryText(false);
}

QString Renderer::getSightingSummaryIcon(Sighting* sighting) const
{
    return sighting->summaryIcon();
}

QString Renderer::getSightingStatusIcon(Sighting* sighting, Database::DataFlags flags) const
{
    return sighting->statusIcon(flags);
}

QStringList Renderer::getSightingIcons(Sighting* sighting) const
{
    return sighting->referencedIcons();
}

bool Renderer::getSightingCanEdit(Sighting* /*sighting*/, Database::DataFlags flags) const
{
    return !flags.testFlag(Database::DataFlag::DB_READONLY_FLAG);
}

bool Renderer::getSightingCanDelete(Sighting* /*sighting*/, Database::DataFlags /*flags*/) const
{
    return true;
}

QByteArray Renderer::getSightingWebhook(Project* project, Sighting* sighting) const
{
    return sightingToJson(project, sighting,
        [&](const FieldValue& fieldValue) -> bool {
            return fieldValue.isValid() && fieldValue.isRelevant() && fieldValue.isVisible();
        });
}

Renderer::MapSymbol Renderer::getSightingMapSymbol(Sighting* sighting) const
{
    auto result = Renderer::MapSymbol();
    result.projectTitle = sighting->schema()->title();
    result.filename = getSightingSummaryIcon(sighting);
    result.url = ":/icons/mark-circle.svg";
    result.color = Qt::darkRed;

    return result;
}

void Renderer::buildRecordModel(Sighting* sighting, QVariantList* recordModel, const std::function<bool(const FieldValue& fieldValue, int depth)>& filter) const
{
    auto depth = -1;

    auto getFieldIcon = [&](const BaseField* field) -> QString
    {
        auto hash = sighting->getFieldIconHash(field->uid());
        return hash.isEmpty() ? QString() : "file:///" + App::instance()->getArchiveFilePath(hash);
    };

    sighting->recordManager()->enumFieldValues(sighting->rootRecordUid(), true,
        [&](const Record& /*record*/, bool* /*skipRecordOut*/)
        {
            depth++;
        },
        [&](const FieldValue& fieldValue)
        {
            auto field = fieldValue.field();

            // Skip hidden fields.
            if (field->hidden())
            {
                return;
            }

            // Handle RecordField.
            if (fieldValue.isRecordField())
            {
                // Append a title of the form "n records" for complex (not group) records.
                if (filter(fieldValue, depth))
                {
                    auto map = QVariantMap {
                        { "delegate", "field" },
                        { "fieldUid", fieldValue.fieldUid() },
                        { "fieldType", fieldValue.type() },
                        { "fieldName", fieldValue.name() },
                        { "fieldIcon", getFieldIcon(field) },
                        { "fieldValue", fieldValue.displayValue() },
                        { "value", fieldValue.value() },
                        { "recordUid", fieldValue.recordUid() },
                        { "depth", depth },
                        { "group", RecordField::fromField(field)->group() }
                    };

                    if (!map.value("fieldName").toString().isEmpty() && !map.value("fieldValue").toString().isEmpty())
                    {
                        recordModel->append(map);
                    }
                }

                return;
            }

            // Determine whether to keep this field.
            if (!filter(fieldValue, depth))
            {
                return;
            }

            // Handle all other fields.
            auto displayValue = fieldValue.displayValue();
            if (!displayValue.isEmpty())
            {
                auto map = QVariantMap {
                    { "delegate", "field" },
                    { "fieldUid", fieldValue.fieldUid() },
                    { "fieldType", fieldValue.type() },
                    { "fieldName", fieldValue.name() },
                    { "fieldIcon", getFieldIcon(field) },
                    { "fieldValue", displayValue },
                    { "value", fieldValue.value() },
                    { "recordUid", fieldValue.recordUid() },
                    { "depth", depth } };

                // Add photo tags.
                auto photoField = qobject_cast<PhotoField*>(fieldValue.field());
                if (photoField)
                {
                    map["photoTags"] = photoField->decodeTags(fieldValue.value(), sighting->languageCode());
                }

                recordModel->append(map);
                return;
            }
        },
        [&](const Record& /*recordUid*/)
        {
            depth--;
        }
    );
}

QVariantList Renderer::buildSightingModel(Sighting* sighting) const
{
    auto recordModel = QVariantList();
    auto modelIndex = 1;

    // Add timestamp.
    recordModel.append(QVariantMap {
        { "delegate", "timestamp" },
        { "createTime", sighting->createTime() } });
    modelIndex++;

    // Add top-level location.
    auto locationFieldValue = sighting->locationMap();
    if (!locationFieldValue.isEmpty())
    {
        recordModel.append(QVariantMap {
            { "delegate", "field" },
            { "fieldType", "LocationField" },
            { "value", locationFieldValue } });
        modelIndex++;
    }

    // Add a title.
    if (!sighting->trackOnly())
    {
        recordModel.append(QVariantMap {
            { "delegate", "group" },
            { "caption", tr("Data") }
        });
        modelIndex++;
    }

    buildRecordModel(sighting, &recordModel, [&](const FieldValue& fieldValue, int /*depth*/) -> bool
    {
        // Skip location fields in the root, we already have this at the top.
        if (fieldValue.recordUid() == sighting->rootRecordUid() && qobject_cast<const LocationField*>(fieldValue.field()))
        {
            return false;
        }

        return true;
    });

    // Fancy logic to add headers and dividers to make complex records more readable.
    while (modelIndex <= recordModel.count() - 1)
    {
        auto lastIndex = modelIndex - 1;
        auto lastMap = recordModel.at(lastIndex).toMap();
        auto lastDepth = lastMap.value("depth", 0).toInt();
        auto lastRecordUid = lastMap.value("recordUid").toString();

        auto currMap = recordModel.at(modelIndex).toMap();
        auto currDepth = currMap.value("depth", 0).toInt();
        auto currRecordUid = currMap.value("recordUid").toString();

        auto addDivider = false;
        addDivider = addDivider || (currDepth > 0 && lastMap.value("depth") != currMap.value("depth"));
        addDivider = addDivider || (currDepth > 0 && lastMap.value("recordUid") != currMap.value("recordUid"));
        addDivider = addDivider || (lastDepth > currDepth && lastDepth > 0);
        if (addDivider)
        {
            lastMap["divider"] = true;
            recordModel.replace(lastIndex, lastMap);
        }

        auto addHeader = currDepth > 0 && (lastRecordUid != currRecordUid);
        if (addHeader)
        {
            // For groups, remove the record counter.
            if (lastMap.value("group").toBool())
            {
                recordModel.removeAt(lastIndex);
                modelIndex--;

                if (modelIndex > 0)
                {
                    auto lastMap = recordModel.at(modelIndex - 1).toMap();
                    lastMap["divider"] = true;
                    recordModel.replace(modelIndex - 1, lastMap);
                }
            }

            recordModel.insert(modelIndex, QVariantMap {
                { "delegate", "recordHeader" },
                { "text", sighting->getRecord(currRecordUid)->name() },
                { "divider", true } });

            modelIndex++;
        }

        modelIndex++;
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

QByteArray Renderer::sightingToJson(Project* project, Sighting* sighting, const std::function<bool(const FieldValue& fieldValue)>& filter) const
{
    auto records = QVariantList();

    auto recordUids = sighting->recordManager()->recordUids();
    for (auto recordIt = recordUids.constBegin(); recordIt != recordUids.constEnd(); recordIt++)
    {
        auto record = sighting->recordManager()->getRecord(*recordIt);
        auto values = QVariantList();

        record->enumFieldValues([&](const FieldValue& fieldValue, bool* stopOut)
        {
            if (!filter(fieldValue))
            {
                return;
            }

            auto field = fieldValue.field();
            auto value = fieldValue.value();
            if (qobject_cast<StringField*>(field))
            {
                value = fieldValue.displayValue();
            }
            else if (qobject_cast<LocationField*>(field))
            {
                value = Location(fieldValue.value().toMap()).toPoint();
            }
            else if (qobject_cast<CheckField*>(field) && !field->listElementUid().isEmpty())
            {
                auto checkedList = QStringList();
                auto elementUids = QStringList();
                field->appendElementUids(value, &elementUids);
                if (elementUids.isEmpty())
                {
                    return;
                }

                for (auto elementIt = elementUids.constBegin(); elementIt != elementUids.constEnd(); elementIt++)
                {
                    checkedList.append(sighting->schema()->getElementName(*elementIt, "system"));
                }

                value = checkedList;
            }

            if (value.typeId() == QMetaType::QString && value.toString().isEmpty())
            {
                return;
            }

            auto v = QVariantMap {{ "name", fieldValue.name() }, { "value", value }};

            auto exportUid = fieldValue.exportUid();
            if (!exportUid.isEmpty())
            {
                v.insert("id", exportUid);
            }

            values.append(v);
        });

        if (!values.isEmpty())
        {
            records.append(QVariantMap {{ "uid", record->recordUid() }, { "values", values }, { "summary", record->summary() }});
        }
    }

    auto result = QVariantMap
    {
        { "uid", sighting->rootRecordUid() },
        { "deviceId", sighting->deviceId() },
        { "sessionId", sighting->sessionId() },
        { "createTime", sighting->createTime() },
        { "updateTime", sighting->updateTime() },
        { "trackOnly", sighting->trackOnly() },
        { "summary", sighting->summaryText() },
        { "username", App::instance()->settings()->username() },
        { "project", project->summary() },
        { "records", records },
    };

    auto location = sighting->locationPtr();
    if (location && location->isValid())
    {
        result.insert("location", location->toPoint());
    }

    return Utils::variantMapToJson(result, QJsonDocument::Compact);
}

} // namespace CyberTracker