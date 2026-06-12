#include "EarthRangerSchema.h"
#include "EarthRangerRenderer.h"

namespace CyberTracker {

EarthRangerSchema::EarthRangerSchema(QObject *parent): QObject{parent}
{
}

bool EarthRangerSchema::parse(const QString& path, Schema* schema)
{
    schema->set_rendererName(EarthRangerRenderer::RENDERER_NAME);

    auto earthRangerSchema = EarthRangerSchema();
    earthRangerSchema.m_runningUid = 0;
    earthRangerSchema.m_path = path;
    earthRangerSchema.parseReportedBy(schema);
    earthRangerSchema.parseEventTypes(schema);

    return true;
}

void EarthRangerSchema::parseReportedBy(Schema* schema)
{
    auto schemaJson = Utils::variantMapFromJsonFile(m_path + "/schema.json");
    auto props = schemaJson["properties"].toMap();

    auto reportedByElement = new Element();
    reportedByElement->set_uid(ELEMENT_REPORTED_BY);
    reportedByElement->set_name("Reported by");
    schema->appendElement(reportedByElement);

    auto reportedByEnum = props["reported_by"].toMap()["enum_ext"].toList();
    for (auto itemIt = reportedByEnum.constBegin(); itemIt != reportedByEnum.constEnd(); itemIt++)
    {
        auto itemMap = itemIt->toMap();
        auto value = itemMap["value"].toMap();

        auto element = new Element();
        element->set_uid(value["id"].toString());
        element->set_name(itemMap["title"].toString());
        element->addTag("identity", value);
        reportedByElement->appendElement(element);
    }
}

void EarthRangerSchema::parseEventTypes(Schema* schema)
{
    auto eventTypesFilePath = m_path + "/eventtypes.json";
    QFile eventTypesFile(eventTypesFilePath);
    qFatalIf(!eventTypesFile.open(QFile::ReadOnly | QFile::Text), "Failed to read eventTypes.json from project folder");
    auto eventTypesList = QJsonDocument::fromJson(eventTypesFile.readAll()).array();

    auto rootElement = new Element();
    rootElement->set_uid("categories");
    rootElement->set_name(Schema::TR_CATEGORIES);
    schema->appendElement(rootElement);

    // Create Elements for each category.

    for (auto eventTypeIt = eventTypesList.constBegin(); eventTypeIt != eventTypesList.constEnd(); eventTypeIt++)
    {
        auto eventTypeObj = eventTypeIt->toObject();
        auto categoryObj = eventTypeObj.value("category").toObject();
        auto categoryUid = "category." + categoryObj.value("value").toString();
        auto categoryName = categoryObj.value("display").toString();
        auto categoryFlag = categoryObj.value("flag").toString();
        auto permissions = categoryObj.value("permissions").toArray();

        if (categoryName == "HIDDEN")
        {
            continue;
        }

        if (categoryFlag == "system")
        {
            continue;
        }

        if (!permissions.contains("create"))
        {
            continue;
        }

        if (schema->getElement(categoryUid) == nullptr)
        {
            auto categoryElement = new Element();
            categoryElement->set_uid(categoryUid);
            categoryElement->set_name(categoryName);
            rootElement->appendElement(categoryElement);
        }
    }

    // Location.
    auto locationField = new LocationField();
    locationField->set_uid(FIELD_LOCATION);
    locationField->set_name(Schema::TR_LOCATION);
    locationField->set_required(true);
    locationField->set_fixCount(4);
    locationField->set_relevant("LOCATION_VISIBLE");
    schema->appendField(locationField);

    // Photos.
    auto photoField = new PhotoField();
    photoField->set_uid(FIELD_PHOTOS);
    photoField->set_name(Schema::TR_PHOTOS);
    photoField->set_maxCount(16);
    photoField->set_showName(false);
    schema->appendField(photoField);

    // Create a synthetic field which represents the report uid.
    auto rootField = new StringField();
    rootField->set_uid("reportUid");
    rootField->set_name(Schema::TR_CATEGORIES);
    rootField->set_listElementUid("categories");
    schema->appendField(rootField);

    // Populate each category with reports.
    for (auto eventTypeIt = eventTypesList.constBegin(); eventTypeIt != eventTypesList.constEnd(); eventTypeIt++)
    {
        auto eventTypeObj = eventTypeIt->toObject();
        auto categoryObj = eventTypeObj.value("category").toObject();
        auto categoryUid = "category." + categoryObj.value("value").toString();

        auto geometryType = eventTypeObj.value("geometry_type").toString();
        if (geometryType != "Point")
        {
            qDebug() << "Not point geometry: " << eventTypeObj.value("display");
            continue;
        }

        auto categoryElement = schema->getElement(categoryUid);
        if (categoryElement == nullptr)
        {
            continue;
        }

        auto reportElement = new Element();
        reportElement->set_uid(eventTypeObj.value("value").toString());
        reportElement->set_name(eventTypeObj.value("display").toString());
        reportElement->set_icon(eventTypeObj.value("icon_id").toString() + ".svg");
        categoryElement->appendElement(reportElement);

        parseReport(schema, reportElement);
    }

    // Notes.
    auto notesField = new StringField();
    notesField->set_uid(FIELD_NOTES);
    notesField->set_name(Schema::TR_NOTES);
    notesField->set_multiLine(true);
    schema->appendField(notesField);
}

bool EarthRangerSchema::parseField(Schema* schema, const QString& reportUid, RecordField* recordField, const QJsonObject& jsonField, QString* keyOut)
{
    auto exportKey = jsonField.value("key").toString();
    auto key = reportUid + '.' + exportKey;

    auto type = jsonField.value("type").toString();
    auto fieldHtmlClass = jsonField.value("fieldHtmlClass").toString();
    auto format = jsonField.value("format").toString();
    auto title = jsonField.value("title").toString();

    // Create the field.
    BaseField* field = nullptr;
    if (format == "date")
    {
        auto dateField = new DateField();
        field = dateField;
    }
    else if (fieldHtmlClass.contains("date-time-picker"))
    {
        auto dateTimeField = new DateTimeField();
        field = dateTimeField;
    }
    else if (type == "string")
    {
        auto stringField = new StringField();
        stringField->set_multiLine(true);
        field = stringField;
    }
    else if (type == "number" || type == "integer")
    {
        auto numberField = new NumberField();

        auto restriction = jsonField.value("minimum");
        if (!restriction.isUndefined())
        {
            numberField->set_minValue(restriction.toInt());
        }

        restriction = jsonField.value("maximum");
        if (!restriction.isUndefined())
        {
            numberField->set_maxValue(restriction.toInt());
        }

        numberField->set_decimals(type == "integer" ? 0 : 4);

        field = numberField;
    }
    else if (type == "boolean" || type == "checkboxes")
    {
        auto checkField = new CheckField();
        field = checkField;
    }
    else if (type == "array")
    {
        auto jsonField2 = jsonField.value("items").toObject();
        if (jsonField2.isEmpty())
        {
            qDebug() << "No items for array type";
            return false;
        }

        if (jsonField2.value("type") != "object")
        {
            qDebug() << "Array type is not an object";
            return false;
        }

        auto props2 = jsonField2.value("properties").toObject();
        if (props2.isEmpty())
        {
            qDebug() << "No fields in child record";
            return false;
        }

        title = jsonField.value("title").toString();

        // Hack because JSON schema uses an object instead of an array, so the order is lost by the JSON object map.
        auto orderedKeys = QStringList(props2.keys());
        {
            QFile reportFile(m_path + "/" + reportUid + ".json");
            qFatalIf(!reportFile.open(QFile::ReadOnly | QFile::Text), "Failed to read report json");

            auto reportData = QString(reportFile.readAll());
            reportData.replace(' ', "");
            reportData.replace('\n', "");
            reportData.replace('\r', "");
            reportData.replace('\t', "");

            std::sort(orderedKeys.begin(), orderedKeys.end(),
                [&](const QString& k1, const QString& k2) -> bool
                {
                    auto offset1 = reportData.indexOf('"' + k1 + "\":{");
                    auto offset2 = reportData.indexOf('"' + k2 + "\":{");
                    return offset1 < offset2;
                });
        }

        auto recordField2 = new RecordField();
        for (auto key2It = orderedKeys.constBegin(); key2It != orderedKeys.constEnd(); key2It++)
        {
            auto key2 = *key2It;
            jsonField2 = props2.value(key2).toObject();
            jsonField2.insert("key", key2);
            auto key2temp = QString();
            parseField(schema, key, recordField2, jsonField2, &key2temp);
        }

        field = recordField2;
    }
    else
    {
        qDebug() << "Failed to find type " << type << " for " << key << " 3";
        return false;
    }

    // Create the Element for the field title.
    auto nameElement = new Element();
    nameElement->set_uid(key);
    nameElement->set_name(title);
    schema->appendElement(nameElement);

    // Create list elements.
    auto listItems = jsonField.value("enumNames");
    if (listItems.isObject())
    {
        auto iconItems = jsonField.value("enumImages").toObject().toVariantMap();
        auto listArray = jsonField.value("enum").toVariant().toStringList();
        auto listObject = listItems.toObject();

        for (auto listItemIt = listArray.constBegin(); listItemIt != listArray.constEnd(); listItemIt++)
        {
            auto listItem = *listItemIt;
            auto listElement = new Element();
            listElement->set_uid("id_" + QVariant(m_runningUid++).toString());

            auto name = listObject.value(listItem).toString();
            listElement->set_name(name);
            listElement->set_exportUid(listItem);

            if (iconItems.contains(listItem))
            {
                listElement->set_icon("choice_icons/" + iconItems[listItem].toString() + ".svg");
            }

            nameElement->appendElement(listElement);
        }
    }
    else if (jsonField.value("titleMap").isArray())
    {
        auto titleMapArray = jsonField.value("titleMap").toArray();
        for (auto titleItem: titleMapArray)
        {
            auto listElement = new Element();
            listElement->set_uid("id_" + QVariant(m_runningUid++).toString());

            auto titleItemMap = titleItem.toObject();
            listElement->set_name(titleItemMap.value("name").toString());
            listElement->set_exportUid(titleItemMap.value("value").toString());

            nameElement->appendElement(listElement);
        }
    }

    if (nameElement->elementCount() > 0)
    {
        field->set_listElementUid(key);
    }

    // Append the field.
    field->set_uid(key);
    field->set_exportUid(exportKey);
    field->set_nameElementUid(key);
    recordField->appendField(field);

    *keyOut = key;

    return true;
}

QJsonObject EarthRangerSchema::resolveForm(const QJsonValue& form, const QJsonObject& props)
{
    auto formJson = QJsonObject();
    if (form.isString())
    {
        auto key = form.toString();
        formJson = props.value(key).toObject();
        formJson["key"] = key;
    }
    else if (form.isObject())
    {
        auto formObj = form.toObject();

        // Start with key.
        auto key = formObj.value("key").toString();
        auto type = QString();

        if (props.contains(key))
        {
            formJson = props.value(key).toObject();
            type = formJson.value("type").toString();
        }

        // Overlay with type.
        auto ftype = formObj.value("type").toString();
        if (props.contains(ftype))
        {
            auto propsObj = props.value(ftype).toObject();
            for (auto propIt = propsObj.constBegin(); propIt != propsObj.constEnd(); propIt++)
            {
                formJson.insert(propIt.key(), propIt.value());
            }
            type = formJson.value("type").toString();
        }

        // Overlay with form.
        for (auto formIt = formObj.constBegin(); formIt != formObj.constEnd(); formIt++)
        {
            formJson.insert(formIt.key(), formIt.value());
        }

        // Get the final type.
        if (type.isEmpty())
        {
            type = formObj.value("type").toString();
        }

        formJson.insert("type", type);
    }
    else
    {
        qDebug() << "Unknown form type";
    }

    // Validate.
    {
        auto key = formJson.value("key").toString();
        auto success = !key.isEmpty();
        if (!success)
        {
            qDebug() << "Failed to find key " << key;
        }

        auto type = formJson.value("type").toString();
        success = success && !type.isEmpty();
        if (!success)
        {
            qDebug() << "Failed to find type " << type << " for " << key << " 1";
        }

        auto validTypes = QStringList { "string", "integer", "boolean", "number", "object", "checkboxes", "array" };
        success = success && (validTypes.indexOf(type) != -1);
        if (!success)
        {
            qDebug() << "Failed to find type " << type << " for " << key << " 2";
            formJson.empty();
        }
    }

    return formJson;
}

void EarthRangerSchema::parseReport(Schema* schema, Element* reportElement)
{
    QFile reportFile(m_path + "/" + reportElement->uid() + ".json");
    qFatalIf(!reportFile.open(QFile::ReadOnly | QFile::Text), "Failed to read eventTypes.json from project folder");

    auto reportJson = QJsonDocument::fromJson(reportFile.readAll());
    qFatalIf(!reportJson.isObject(), "Expected an object");

    auto forms = reportJson.object().value("definition").toArray();
    auto props = reportJson.object().value("schema").toObject().value("properties").toObject();

    reportElement->appendFieldUid(FIELD_PHOTOS);
    reportElement->appendFieldUid(FIELD_LOCATION);

    auto processedKeys = QVariantMap();

    for (auto formIt = forms.constBegin(); formIt != forms.constEnd(); formIt++)
    {
        auto schemaItem = resolveForm(*formIt, props);
        if (schemaItem.isEmpty())
        {
            continue;
        }

        auto schemaKey = schemaItem.value("key").toString();
        if (processedKeys.contains(schemaKey))
        {
            continue;
        }
        processedKeys.insert(schemaKey, true);

        auto key = QString();
        if (!parseField(schema, reportElement->uid(), schema->rootField(), schemaItem, &key))
        {
            continue;
        }

        reportElement->appendFieldUid(key);
    }

    reportElement->appendFieldUid(FIELD_NOTES);
}

} // namespace CyberTracker
