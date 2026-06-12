#include "SMARTSchema.h"
#include "SMARTRenderer.h"

namespace CyberTracker {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SMARTModelHandler

class SMARTModelHandler //: public QXmlDefaultHandler
{
public:
    SMARTModelHandler(Schema* schema, RecordField* modelField);
    ~SMARTModelHandler();// override;

    bool startElement(const QString& qName, const QXmlStreamAttributes& attributes);
    bool endElement(const QString& qName);

private:
    Schema* m_schema = nullptr;
    RecordField* m_modelField = nullptr;

    QStringList m_stack;            // XML node stack.
    QList<Element*> m_nodeStack;    // Element node stack.
    QList<Element*> m_elementStack; // Current element stack.
    QVariantMap m_attribute;        // Current attribute.
    QStringList m_languages;        // Language list.
    QString m_defaultLanguage;      // Default language.
    QVariantMap m_lastAttribute;    // Last attribute for node.
    QMultiMap<QString, BaseField*> m_remapDefaultValue; // Remap list+tree dmUuid->uuid for default values

    void elementFromXml(Element* element, const QXmlStreamAttributes& attributes, const QString& exportUidPrefix = QString());
    bool matchParent(const QString& parentAttributeName);
    Element* getCurrentElement();
};

SMARTModelHandler::SMARTModelHandler(Schema* schema, RecordField* modelField)
{
    m_schema = schema;
    m_modelField = modelField;
}

SMARTModelHandler::~SMARTModelHandler()
{

}

bool SMARTModelHandler::matchParent(const QString& parentAttributeName)
{
    return m_stack.last().compare(parentAttributeName) == 0;
}

Element* SMARTModelHandler::getCurrentElement()
{
    return m_elementStack.last();
}

void SMARTModelHandler::elementFromXml(Element* element, const QXmlStreamAttributes& attributes, const QString& exportUidPrefix)
{
    auto tag = element->tag();
    auto id = QString();
    auto dmUuid = QString();

    for (const QXmlStreamAttribute &attr : attributes)
    {
        auto name = attr.name().toString();
        auto value = attr.value().toString();
        if (name == "id" || name == "uuid")
        {
            id = value;
            element->set_uid(value);
        }
        else if (name == "imageFile")
        {
            element->set_icon(value);
        }
        else if (name == "dmUuid" || name == "keyid")
        {
            dmUuid = value;
            if (!exportUidPrefix.isNull())
            {
                element->set_exportUid(exportUidPrefix + value);
            }
        }
        else if (name == "isActive")
        {
            element->set_hidden(value == "false");
        }
        else if (name.startsWith("photo"))
        {
            tag.insert(name, value);
        }
        else if (name == "collectMultipleObs" && value != "false")
        {
            tag.insert(name, value);
        }
    }

    element->set_tag(tag);

    // Default values for lists and trees are specified with a dmUuid, but we cannot use dmUuid as an
    // element id, because it is not unique. This code remaps the dmUuid to the uid for the default values
    // for lists and trees. Note that Elements are populated after all the fields have been specified.
    if (!id.isEmpty() && !dmUuid.isEmpty())
    {
        auto remaps = m_remapDefaultValue.values(dmUuid);
        if (remaps.count() > 0)
        {
            for (auto remapIt = remaps.begin(); remapIt != remaps.end(); remapIt++)
            {
                // Hack start: fix the default value in the check field list. The default value is specified as a comma-separated string.
                // This converts it to a map, so that this behavior (default value as string instead of a map) does not leak into the engine.
                auto checkField = qobject_cast<CheckField*>(*remapIt);
                if (checkField && !checkField->listElementUid().isEmpty())
                {
                    auto defaultValue = (*remapIt)->defaultValue();
                    auto defaultValueMap = defaultValue.toMap();

                    // Convert the default value to a map as needed.
                    if (!defaultValue.toString().isEmpty())
                    {
                        auto defaultValues = defaultValue.toString().split(',');
                        for (auto it = defaultValues.constBegin(); it != defaultValues.constEnd(); it++)
                        {
                            defaultValueMap.insert(*it, true);
                        }

                        (*remapIt)->set_defaultValue(defaultValueMap);
                    }

                    // Remap the current element.
                    defaultValueMap.remove(dmUuid);
                    defaultValueMap.insert(id, true);
                    (*remapIt)->set_defaultValue(defaultValueMap);

                    continue;
                }
                // Hack end.

                (*remapIt)->set_defaultValue(id);
            }

            m_remapDefaultValue.remove(dmUuid);
        }
    }
}

bool SMARTModelHandler::startElement(const QString& qName, const QXmlStreamAttributes& attributes)
{
    if (qName == "ConfigurableModel")
    {
        auto element = new Element(nullptr);
        elementFromXml(element, attributes);
        element->set_uid(SMARTSchema::FIELD_MODEL_TREE);
        m_schema->appendElement(element);
        m_elementStack.append(element);

        m_schema->setSetting("instantGPS", attributes.value("instantGps").toString() == "true");
        m_schema->setSetting("photoFirst", attributes.value("photoFirst").toString() == "true");
    }
    else if (qName == "language")
    {
        qFatalIf(!matchParent("languages"), "language has wrong parent");
        auto languageCode = attributes.value("code").toString();
        qFatalIf(languageCode.isEmpty(), "Missing language code");
        m_languages.append(languageCode);
        if (attributes.value("is_default").toString() == "true")
        {
            m_defaultLanguage = languageCode;
        }
    }
    else if (qName == "attachmentTags" && matchParent("ConfigurableModel"))
    {
        auto element = new Element(nullptr);
        element->set_uid(SMARTSchema::ATTACHMENT_TAGS);
        m_schema->rootElement()->appendElement(element);
        m_elementStack.append(element);

        m_schema->setSetting("hasAttachmentTags", true);
    }
    else if (qName == "AttachmentTagType" || qName == "attachmentTagType")
    {
        auto element = new Element(nullptr);
        elementFromXml(element, attributes, "");
        m_elementStack.last()->appendElement(element);
        m_elementStack.append(element);
    }
    else if ((qName == "attachmentTags" || qName == "attachmentTag") && matchParent("node"))
    {
        auto lastNode = m_nodeStack.last();
        auto attachmentTags = lastNode->getTagValue("attachmentTags", QStringList()).toStringList();
        attachmentTags.append(attributes.value("uuid").toString());
        lastNode->setTagValue("attachmentTags", attachmentTags);
    }
    else if (qName == "signatureType")
    {
        qFatalIf(!matchParent("signatures"), "signatureType has wrong parent");
        auto element = new Element(nullptr);
        auto uid = attributes.value("uuid").toString();
        element->set_uid(uid);
        m_schema->rootElement()->appendElement(element);
        m_elementStack.append(element);

        auto field = new SketchField();
        field->set_uid(uid);
        field->set_nameElementUid(uid);
        field->set_exportUid("SMART_Signature_" + attributes.value("keyid").toString());
        field->addTag("ENTER_ONCE", "END"); // Signatures can only be at the root level.
        m_modelField->appendField(field);

        m_schema->setSetting("hasSignatures", true);
    }
    else if (qName == "signature")
    {
        // Add this field to the current uid list.
        auto signatureUid = attributes.value("uuid").toString();
        m_nodeStack.last()->appendFieldUid(signatureUid);
    }
    else if (qName == "node")
    {
        qFatalIf(!matchParent("nodes") && !matchParent("node"), "node has wrong parent");
        auto element = new Element(nullptr);
        elementFromXml(element, attributes, "");
        m_elementStack.last()->appendElement(element);
        m_elementStack.append(element);
        m_nodeStack.append(element);
    }
    else if (qName == "name")
    {
        if (matchParent("node") || matchParent("ConfigurableModel") || matchParent("attributeConfig") ||
            matchParent("listItem") || matchParent("treeNode") || matchParent("children") || matchParent("attribute") ||
            matchParent("signatureType") || matchParent("AttachmentTagType") || matchParent("attachmentTagType"))
        {
            auto jsonObject = m_elementStack.last()->names();
            jsonObject[attributes.value("language_code")] = attributes.value("value").toString();
            m_elementStack.last()->set_names(jsonObject);
        }
    }
    else if (qName == "attributeConfig")
    {
        qFatalIf(!matchParent("ConfigurableModel"), "attributeConfig in wrong place");
        auto element = new Element(nullptr);
        elementFromXml(element, attributes);
        m_schema->rootElement()->appendElement(element);
        m_elementStack.append(element);
    }
    else if (qName == "listItem")
    {
        qFatalIf(!matchParent("attributeConfig") && !matchParent("treeNode") && !matchParent("children"), "attributeConfig child has wrong parent");
        auto element = new Element(nullptr);
        elementFromXml(element, attributes, "l:");
        m_elementStack.last()->appendElement(element);
        m_elementStack.append(element);
    }
    else if ((qName == "treeNode") || (qName == "children"))
    {
        qFatalIf(!matchParent("attributeConfig") && !matchParent("treeNode") && !matchParent("children"), "attributeConfig child has wrong parent");
        auto element = new Element(nullptr);
        elementFromXml(element, attributes, "t:");
        m_elementStack.last()->appendElement(element);
        m_elementStack.append(element);
    }
    else if (qName == "attribute")
    {
        // Create name Element.
        qFatalIf(!matchParent("node"), "attribute has wrong parent");
        auto element = new Element(nullptr);
        auto elementUid = attributes.value("id").toString();
        element->set_uid(elementUid);
        element->set_icon(attributes.value("imageFile").toString());
        m_schema->rootElement()->appendElement(element);
        m_elementStack.append(element);

        // Build attribute metadata so we can turn it into a field later.
        qFatalIf(!matchParent("node"), "attribute has bad parent");
        qFatalIf(m_attribute.count(), "Attribute map should be clear");

        auto dataType = attributes.value("type").toString();
        m_attribute["type"] = dataType;
        m_attribute["uid"] = attributes.value("id").toString();
        m_attribute["exportUid"] = "a:0:" + attributes.value("dmUuid").toString();
        m_attribute["nameElementUid"] = attributes.value("id").toString();
        m_attribute["tag"] = QVariantMap();
        m_attribute["minValue"] = attributes.value("minValue").toString();
        m_attribute["maxValue"] = attributes.value("maxValue").toString();
        m_attribute["key"] = attributes.value("attributeKey").toString();

        auto listElementUid = attributes.value("configId").toString();
        if (!listElementUid.isEmpty() && (dataType == "LIST" || dataType == "TREE" || dataType == "MLIST"))
        {
            m_attribute["listElementUid"] = listElementUid;
        }
        m_attribute["required"] = attributes.value("required").toString() != "false";

        m_attribute["regex"] = attributes.value("regex").toString();

        // Add this field to the current uid list.
        m_nodeStack.last()->appendFieldUid(m_attribute["uid"].toString());
    }
    else if (qName == "option")
    {
        qFatalIf(!matchParent("attribute"), "option has wrong parent");
        qFatalIf(m_attribute.count() == 0, "No last field");

        auto tag = m_attribute["tag"].toMap();
        auto doubleValue = attributes.value("doubleValue").toString();
        auto stringValue = attributes.value("stringValue").toString();
        auto uuidValue = attributes.value("dmUuid").toString();

        auto optionId = attributes.value("id").toString();
        if (optionId == "IS_VISIBLE")
        {
            m_attribute["hidden"] = doubleValue == "0.0";
            m_attribute["relevant"] = doubleValue == "2.0" ? stringValue.trimmed() : "";

            if (!m_attribute["relevant"].toString().isEmpty())
            {
                m_schema->setSetting("hasConditionalFields", true);
            }
        }
        else if (optionId == "MULTISELECT")
        {
            if (doubleValue == "1.0")
            {
                m_attribute["type"] = "BOOLEAN";
            }
        }
        else if (optionId == "DEFAULT_VALUE")
        {
            QVariant defaultValue;
            auto attributeType = m_attribute["type"].toString();
            if (attributeType == "NUMERIC")
            {
                defaultValue = doubleValue.toDouble();
            }
            else if (attributeType == "BOOLEAN")
            {
                if (doubleValue == "1.0")
                {
                    defaultValue = m_attribute["required"].isValid();
                }
            }
            else if (!uuidValue.isEmpty())
            {
                defaultValue = uuidValue;
            }
            else
            {
                defaultValue = stringValue;
            }

            m_attribute["default_value"] = defaultValue;
        }
        else if (optionId == "FLATTEN_TREE")
        {
            if (doubleValue == "1.0")
            {
                m_attribute["listFlatten"] = true;
                m_attribute["listSorted"] = true;
                m_schema->setSetting("hasFlattenedTree", true);
            }
        }
        else if (optionId == "NUMERIC")
        {
            if (doubleValue == "1.0")
            {
                m_attribute["numeric"] = true;
            }
        }
        else if (optionId == "ENTER_ONCE")
        {
            if (stringValue != "NONE")
            {
                tag["ENTER_ONCE"] = stringValue;
            }
        }
        else if (optionId == "INPUT_GROUP")
        {
            if (doubleValue == "1.0")
            {
                tag.remove("ENTER_ONCE"); // Suppress enter_once mechanism - should be done by desktop.
                tag["INPUT_GROUP"] = true;
                m_schema->setSetting("hasMatrixQuestions", true);
            }
        }
        else if (optionId.startsWith("HELP_"))
        {
            auto hint = m_attribute["hint"].toMap();
            hint.insert(optionId, stringValue);
            m_attribute["hint"] = hint;
            m_schema->setSetting("hasFieldHelp", true);
        }
        else if (optionId == "QR_CODE")
        {
            m_attribute["barcode"] = doubleValue == "1.0";
            if (doubleValue == "1.0")
            {
                m_schema->setSetting("hasBarcodeFields", true);
            }
        }
        else if (optionId == "USE_NUMPAD")
        {
            m_attribute["numbers"] = doubleValue == "1.0";
            if (doubleValue == "1.0")
            {
                m_schema->setSetting("hasNumpadFields", true);
            }
        }
        else if (optionId == "GEOM_AUTO_GPS_SEC")
        {
            m_attribute["autoGPSTimeInSeconds"] = static_cast<int>(doubleValue.toDouble());
        }
        else if (optionId == "MANUAL_DRAW")
        {
            m_attribute["allowManual"] = true;
        }
        else if (optionId == "GPS_AUTO")
        {
            m_attribute["allowAutoGPS"] = true;
        }
        else if (optionId == "GPS_MANUAL")
        {
            m_attribute["allowManualGPS"] = true;
        }

        m_attribute["tag"] = tag;
    }
    else if (qName == "extraData")
    {
        auto map = QVariantMap();
        map.insert("id", attributes.value("type").toString());

        auto extraData = m_schema->getSetting("extraData").toList();
        extraData.append(map);
        m_schema->setSetting("extraData", extraData);
    }
    else if ((qName == "stringKey") || (qName == "integerKey"))
    {
        qFatalIf(!matchParent("extraData"), "data has wrong parent");
        auto extraData = m_schema->getSetting("extraData").toList();
        auto map = extraData.constLast().toMap();
        auto value = QVariant(attributes.value("value").toString());
        if (qName == "integerKey")
        {
            value = value.toInt();
        }
        map.insert(attributes.value("key").toString(), value);
        extraData.removeLast();
        extraData.append(map);
        m_schema->setSetting("extraData", extraData);
    }

    m_stack.append(qName);

    return true;
}

bool SMARTModelHandler::endElement(const QString& qName)
{
    if (qName == "ConfigurableModel")
    {
        // Clear any trees+lists that have a default that is not a valid Element uid.
        // This happens if the data model specifies a node rather than a leaf.
        for (auto field: m_remapDefaultValue.values())
        {
            qDebug() << "Error: field has invalid default value: " << field->uid();
            field->set_defaultValue(QVariant());
        }
        m_remapDefaultValue.clear();
    }
    else if (qName == "languages")
    {
        auto languageList = QVariantList();

        for (auto languageIt = m_languages.constBegin(); languageIt != m_languages.constEnd(); languageIt++)
        {
            auto code = *languageIt;
            auto locale = QLocale(code);

            languageList.append(QVariantMap
            {
                { "code", code },
                { "name", locale.languageToString(locale.language()) + " (" + locale.territoryToString(locale.territory()) + ")" }
            });
        }

        std::sort(languageList.begin(), languageList.end(), [](const QVariant& r1, const QVariant& r2) -> bool
        {
            auto n1 = r1.toMap()["name"].toString();
            auto n2 = r2.toMap()["name"].toString();
            return n1.compare(n2) < 0;
        });

        for (auto i = 0; i < languageList.count(); i++)
        {
            if (languageList[i].toMap()["code"].toString() == m_defaultLanguage)
            {
                m_schema->set_languageIndex(i);
                break;
            }
        }

        m_schema->set_languages(languageList);
    }
    else if (qName == "signatureType")
    {
        m_elementStack.removeLast();
    }
    else if (qName == "signature")
    {
        m_attribute.clear();
    }
    else if (qName == "AttachmentTagType" || qName == "attachmentTagType")
    {
        m_elementStack.removeLast();
    }
    else if (qName == "attachmentTags")
    {
        if (m_elementStack.last()->uid() == SMARTSchema::ATTACHMENT_TAGS)
        {
            m_elementStack.removeLast();
        }
    }
    else if (qName == "nodes")
    {
        m_elementStack.removeLast();
    }
    else if (qName == "node")
    {
        m_elementStack.removeLast();
        m_nodeStack.removeLast();
        m_lastAttribute.clear();
    }
    else if (qName == "attributeConfig")
    {
        m_elementStack.removeLast();
    }
    else if ((qName == "listItem") || (qName == "treeNode") || (qName == "children"))
    {
        m_elementStack.removeLast();
    }
    else if (qName == "attribute")
    {
        m_elementStack.removeLast();
        qFatalIf(m_attribute.count() == 0, "Last field should be valid");
        BaseField *field = nullptr;
        auto fieldType = m_attribute.value("type");

        if (fieldType == "TEXT")
        {
            auto stringField = new StringField();
            stringField->set_multiLine(!m_attribute["numbers"].toBool());
            stringField->set_numbers(m_attribute["numbers"].toBool());
            stringField->set_barcode(m_attribute["barcode"].toBool());
            if (!m_attribute.value("regex").toString().isEmpty())
            {
                stringField->set_constraint("regex(., '" + m_attribute["regex"].toString() + "')");
            }
            field = stringField;

            // Special handling for device fields.
            auto attributeKey = m_attribute["key"].toString();
            if (QStringList { "device_id", "device_username" }.contains(attributeKey))
            {
                m_attribute["hidden"] = true;
                m_attribute["default_value"] = "__replace__";

                auto tag = m_attribute["tag"].toMap();
                tag["attributeKey"] = attributeKey;
                m_attribute["tag"] = tag;
                stringField->set_multiLine(false);
            }
        }
        else if (fieldType == "LIST")
        {
            field = new StringField();
        }
        else if (fieldType == "TREE")
        {
            field = new StringField();
        }
        else if (fieldType == "MLIST")
        {
            field = new CheckField();

            // Tag this as an MLIST so our post-processor does not turn subsequent fields into a record.
            auto tag = m_attribute["tag"].toMap();
            tag["MLIST"] = true;
            m_attribute["tag"] = tag;

            // Add the default items to the remap list.
            auto defaultValue = m_attribute.value("default_value").toString();
            if (!defaultValue.isEmpty())
            {
                auto defaultValues = defaultValue.split(',');
                for (auto it = defaultValues.constBegin(); it != defaultValues.constEnd(); it++)
                {
                    m_remapDefaultValue.insert(*it, field);
                }
            }
        }
        else if (fieldType == "BOOLEAN")
        {
            field = new CheckField();
        }
        else if (fieldType == "NUMERIC")
        {
            auto numberField = new NumberField();
            field = numberField;

            auto valueText = m_attribute.value("minValue").toString();
            bool ok = false;
            auto value = valueText.toDouble(&ok);
            if (ok)
            {
                numberField->set_minValue(value);
            }

            valueText = m_attribute.value("maxValue").toString();
            value = valueText.toDouble(&ok);
            if (ok)
            {
                numberField->set_maxValue(value);
            }

            // SMART did not want to create a new field, so we are using regex for the number of decimals.
            auto regex = m_attribute.value("regex").toString();
            numberField->set_decimals(regex.isEmpty() ? 3 : regex.toInt());

            // Special handling for GPS fields.
            auto attributeKey = m_attribute["key"].toString();
            if (attributeKey.startsWith("gps_"))
            {
                m_attribute["hidden"] = true;
                m_attribute["default_value"] = 0;

                auto tag = m_attribute["tag"].toMap();
                tag["attributeKey"] = attributeKey;
                m_attribute["tag"] = tag;
            }

            //
            // SMART Desktop does not have support for number lists directly. Instead, it requires
            // two attributes: a check list followed by a number field. The number field must be
            // tagged with the "numeric" option.
            //
            // Since this is discovered only when processing the number field, go back to the prior
            // attribute, pick up the relevant attributes (like name, list, etc) and then remove the
            // prior attribute from the model.
            //
            if (m_lastAttribute["type"] == "BOOLEAN" && !m_lastAttribute["listElementUid"].toString().isEmpty() &&
                m_attribute.value("numeric").toBool())
            {
                auto lastField = m_modelField->field(m_modelField->fieldCount() - 1);
                m_remapDefaultValue.remove(m_lastAttribute["default_value"].toString(), lastField);

                m_modelField->removeLastField();
                m_nodeStack.last()->removeFieldUid(m_lastAttribute["uid"].toString());

                m_attribute["nameElementUid"] = m_lastAttribute["nameElementUid"];
                m_attribute["listElementUid"] = m_lastAttribute["listElementUid"];
                m_attribute["required"] = m_lastAttribute["required"];
                m_attribute["hidden"] = m_lastAttribute["hidden"];
                m_attribute["listFlatten"] = m_lastAttribute["listFlatten"];
                m_attribute["listSorted"] = m_lastAttribute["listSorted"];
                m_attribute["hint"] = m_lastAttribute["hint"];
            }
        }
        else if (fieldType == "DATE")
        {
            field = new DateField();
        }
        else if (fieldType == "DATE_AND_TIME")
        {
            field = new DateTimeField();
        }
        else if (fieldType == "TIME")
        {
            field = new TimeField();
        }
        else if (fieldType == "UUID")
        {
            field = new StringField();
        }
        else if (fieldType == "IMAGE")
        {
            field = new StringField();
        }
        else if (fieldType == "POLYGON")
        {
            auto areaField = new AreaField();
            field = areaField;

            areaField->set_allowManual(m_attribute.value("allowManual", false).toBool());
            areaField->set_allowAutoGPS(m_attribute.value("allowAutoGPS", false).toBool());
            areaField->set_allowManualGPS(m_attribute.value("allowManualGPS", false).toBool());
            areaField->set_autoGPSTimeInSeconds(m_attribute.value("autoGPSTimeInSeconds", 5).toInt());

            m_schema->setSetting("hasAreaField", true);
            m_schema->setSetting("areaAllowManual", areaField->allowManual());
            m_schema->setSetting("areaAllowAutoGPS", areaField->allowAutoGPS());
            m_schema->setSetting("areaAllowManualGPS", areaField->allowManualGPS());
            m_schema->setSetting("areaAutoGPSTimeInSeconds", areaField->autoGPSTimeInSeconds());
        }
        else if (fieldType == "LINE")
        {
            auto lineField = new LineField();
            field = lineField;

            lineField->set_allowManual(m_attribute.value("allowManual", false).toBool());
            lineField->set_allowAutoGPS(m_attribute.value("allowAutoGPS", false).toBool());
            lineField->set_allowManualGPS(m_attribute.value("allowManualGPS", false).toBool());
            lineField->set_autoGPSTimeInSeconds(m_attribute.value("autoGPSTimeInSeconds", 5).toInt());

            m_schema->setSetting("hasLineField", true);
            m_schema->setSetting("lineAllowManual", lineField->allowManual());
            m_schema->setSetting("lineAllowAutoGPS", lineField->allowAutoGPS());
            m_schema->setSetting("lineAllowManualGPS", lineField->allowManualGPS());
            m_schema->setSetting("lineAutoGPSTimeInSeconds", lineField->autoGPSTimeInSeconds());
        }
        else
        {
            qFatalIf(true, "Unknown field type");
        }

        // For trees and single select lists, remap the default value.
        if (m_attribute["default_value"].isValid() && m_attribute.contains("listElementUid") && fieldType != "MLIST")
        {
            m_remapDefaultValue.insert(m_attribute["default_value"].toString(), field);
        }

        field->set_uid(m_attribute["uid"].toString());
        field->set_exportUid(m_attribute["exportUid"].toString());
        field->set_nameElementUid(m_attribute["nameElementUid"].toString());
        field->set_listElementUid(m_attribute["listElementUid"].toString());
        field->set_required(m_attribute["required"]);
        field->set_hidden(m_attribute["hidden"].toBool());
        field->set_listFlatten(m_attribute["listFlatten"].toBool());
        field->set_listSorted(m_attribute["listSorted"].toBool());
        field->set_defaultValue(m_attribute["default_value"]);

        auto hintMap = m_attribute["hint"].toMap();
        if (!hintMap.isEmpty())
        {
            // No longer using web view for help content.
            //field->set_hintLink(hintMap["HELP_HTML_FILE"].toString());

            auto hintElementUid = field->uid() + "/hint";
            auto hintElement = new Element(nullptr);
            hintElement->set_uid(hintElementUid);
            hintElement->set_name(hintMap["HELP_TEXT"].toString());
            hintElement->set_icon(hintMap["HELP_IMAGE_FILE"].toString());
            hintElement->addTag("iconLocation", hintMap["HELP_IMAGE_LOCATION"].toString());
            m_schema->rootElement()->appendElement(hintElement);

            field->set_hintElementUid(hintElementUid);
        }

        field->set_relevant(m_attribute["relevant"].toString());
        field->set_tag(m_attribute["tag"].toMap());
        m_modelField->appendField(field);

        m_lastAttribute = m_attribute;
        m_attribute.clear();
    }

    qFatalIf(m_stack.last() != qName, "Invalid XML");
    m_stack.removeLast();

    return true;
}

// bool SMARTModelHandler::characters(const QString& str)
// {
//     Q_UNUSED(str)
//     return true;
// }

// bool SMARTModelHandler::fatalError(const QXmlParseException& exception)
// {
//     qFatalIf(true, "Parse error at line reading xml: " + exception.message());
//     return false;
// }

// QString SMARTModelHandler::errorString() const
// {
//     return "";
// }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SMARTSchema

SMARTSchema::SMARTSchema(QObject *parent): QObject(parent)
{
}

bool SMARTSchema::parse(Project* project, const QString& path, QVariantList* languagesOut, int* languageIndexOut, QVariantMap* samplingGeoJsonOut)
{
    m_profile = Utils::variantMapFromJsonFile(path + "/ct_profile.json");
    m_project = Utils::variantMapFromJsonFile(path + "/project.json");
    m_smartVersion = qFloor(m_project.value("smart_version", 6.00).toDouble() * 100);

    auto setSettings = [&](Schema* schema)
    {
        schema->setSetting("surveyMode", m_project.value("metadata").toString().contains("survey", Qt::CaseInsensitive));
        schema->setSetting("caUuid", m_project.value("cauuid").toString());
        schema->setSetting("useDistanceAndBearing", m_profile.value("RECORD_DISTANCE_BEARING", false) == true);
        schema->setSetting("useGroupUI", m_profile.value("INCIDENT_GROUP_UI", false) == true);
        schema->setSetting("useObserver", m_profile.value("RECORD_OBSERVER", false) == true);
        schema->setSetting("manualUpload", m_profile.value("DATA_SERVER").toMap().value("FREQUENCY_MIN", -1).toInt() == -1);
        schema->setSetting("disableEditing", m_profile.value("DISABLE_EDITING", false) == true);
        schema->setSetting("allowHistory", m_profile.value("HISTORY_MODE", true) == true);
        schema->setSetting("allowArchive", m_profile.value("ARCHIVE_MODE", true) == true);
    };

    auto buildSchema = [&](const QString& metadataFileName, const QString& modelFileName, const QString& typeName)
    {
        auto schema = Schema();
        m_schema = &schema;
        parseMetadata(path + "/" + metadataFileName, samplingGeoJsonOut);
        parseModel(path + "/" + modelFileName);
        setSettings(m_schema);

        // Data model statistics for telemetry.
        auto leafElements = m_schema->getLeafElements(FIELD_MODEL_TREE);
        auto incidentTypeCount = 0;
        for (auto* element : leafElements)
        {
            if (!element->fieldUids().isEmpty()) incidentTypeCount++;
        }
        auto modelRecord = qobject_cast<RecordField*>(m_schema->getField(FIELD_MODEL));
        m_schema->setSetting("fieldCount", modelRecord ? modelRecord->fieldCount() : 0);
        m_schema->setSetting("incidentTypeCount", incidentTypeCount);

        schema.set_rendererName(SMARTRenderer::RENDERER_NAME);
        schema.setProjectInfo(project->title(), project->subtitle(), project->icon());
        schema.saveToQmlFile(path + "/Form" + typeName + ".qml");
        *languagesOut = schema.languages();
        *languageIndexOut = schema.languageIndex();
    };

    auto metadataFileName = m_project.value("metadata").toString();
    auto definitionFileName = m_project.value("definition").toString();

    // Root.
    {
        auto schema = Schema();
        setSettings(&schema);
        schema.setProjectInfo(project->title(), project->subtitle(), project->icon());
        schema.saveToQmlFile(path + "/Form.qml");
    }

    // Collect.
    if (metadataFileName == "smartcollect_metadata.json")
    {
        buildSchema(metadataFileName, definitionFileName, TYPE_COLLECT);
    }

    // Patrol and Survey.
    if (metadataFileName == "patrol_metadata.json" || metadataFileName == "survey_metadata.json")
    {
        buildSchema(metadataFileName, definitionFileName, TYPE_PATROL);
    }

    // Independent Incident.
    if (metadataFileName == "incident_metadata.json")
    {
        buildSchema(metadataFileName, definitionFileName, TYPE_INCIDENT);
    }

    // Independent Incident as part of Patrol or Survey.
    if (m_project.contains("incident"))
    {
        auto incident = m_project["incident"].toMap();
        metadataFileName = incident["metadata"].toString();
        definitionFileName = incident["definition"].toString();
        buildSchema(metadataFileName, definitionFileName, TYPE_INCIDENT);
    }

    return true;
}

void SMARTSchema::parseOptions(const QString& optionPrefix, const QVariantList& options, Element* baseElement, QStringList* optionUidsOut) const
{
    if (options.isEmpty())
    {
        return;
    }

    for (auto optionIt = options.constBegin(); optionIt != options.constEnd(); optionIt++)
    {
        auto optionMap = optionIt->toMap();
        auto optionUid = optionMap["uuid"].toString();
        optionUidsOut->append(optionUid);

        auto optionElement = new Element(nullptr);
        optionElement->set_uid(optionUid);
        optionElement->set_exportUid(optionPrefix + optionUid);

        if (!optionMap["label"].isNull())
        {
            optionElement->set_name(optionMap["label"].toString());
        }
        else
        {
            for (auto labelIt = optionMap.constKeyValueBegin(); labelIt != optionMap.constKeyValueEnd(); labelIt++)
            {
                if (labelIt->first.startsWith("label_"))
                {
                    auto names = optionElement->names();
                    auto keyToAdd = labelIt->first.split("_").last();
                    names.insert(keyToAdd, labelIt->second.toString());
                    optionElement->set_names(names);
                }
            }
        }

        if (optionMap.contains("ct_settings"))
        {
            optionElement->addTag("ct_settings", optionMap["ct_settings"]);
        }

        if (optionMap.contains("max_speed"))
        {
            optionElement->addTag("max_speed", optionMap["max_speed"]);
        }

        qFatalIf(optionElement->name().isEmpty() && optionElement->names().isEmpty(), "No name for list option for field");

        baseElement->appendElement(optionElement);

        parseOptions(optionPrefix, optionMap.value("options").toList(), optionElement, optionUidsOut);
    }
}

void SMARTSchema::addMetadataItem(const QString& fieldName, const QVariantMap& fieldValues) const
{
    // Extract labels from an object - anything that starts with "label_".
    auto findLabels = [&](const QVariantMap& values) -> QJsonObject
    {
        auto result = QJsonObject();
        for (auto it = values.constKeyValueBegin(); it != values.constKeyValueEnd(); it++)
        {
            if (it->first.startsWith("label_"))
            {
                auto languageCode = QString(it->first).remove(0, 6);
                result.insert(languageCode, it->second.toString());
            }
            else if (it->first == "label")
            {
                result.insert("default", it->second.toString());
            }
        }

        return result;
    };

    auto baseElement = new Element(nullptr);
    baseElement->set_uid(fieldName);

    static const auto translateLabels = QMap<QString, QString>
    {
        { "Patrol Transport Type", Schema::TR_TRANSPORT_TYPE },
        { "Is Armed", Schema::TR_IS_ARMED },
        { "Team", Schema::TR_TEAM },
        { "Station", Schema::TR_STATION },
        { "Mandate", Schema::TR_MANDATE },
        { "Objective", Schema::TR_OBJECTIVE },
        { "Comment", Schema::TR_COMMENT },
        { "Employees", Schema::TR_EMPLOYEES },
        { "Leader", Schema::TR_LEADER },
        { "Pilot", Schema::TR_PILOT },
        { "Incident Type", Schema::TR_INCIDENT_TYPE }
    };

    auto labels = findLabels(fieldValues);
    auto defaultLabel = labels.value("default").toString();
    if (translateLabels.contains(defaultLabel))
    {
        labels["default"] = translateLabels[defaultLabel];
    }

    if (labels.count() == 1)
    {
        baseElement->set_name(labels.value(labels.toVariantMap().firstKey()).toString());
    }
    else
    {
        baseElement->set_names(labels);
    }

    m_schema->rootElement()->appendElement(baseElement);

    BaseField* field = nullptr;
    auto defaultValue = fieldValues["default"];
    auto required = fieldValues["isRequired"].toBool();

    auto fieldType = fieldValues["type"].toString();
    auto isFixed = fieldValues["isFixed"].toBool();
    auto requiredBy = fieldValues.value("required_by", QVariantMap()).toMap();

    if (fieldType == "TEXT")
    {
        auto stringField = new StringField();
        stringField->set_multiLine(true);
        field = stringField;
    }
    else if (fieldType == "SINGLE_CHOICE")
    {
        field = new StringField();

        // Sort employee and station lists.
        field->set_listSorted(fieldName == "SMART_Leader" || fieldName == "SMART_Pilot" || fieldName == "SMART_Station");
    }
    else if (fieldType == "MULTI_CHOICE")
    {
        field = new CheckField();

        // Sort employee lists.
        field->set_listSorted(fieldName == FIELD_EMPLOYEES);

        if (fieldValues.contains("defaults"))
        {
            auto valueMap = QVariantMap();
            auto defaults = fieldValues["defaults"].toList();
            for (auto it = defaults.constBegin(); it != defaults.constEnd(); it++)
            {
                valueMap.insert(it->toString(), true);
            }
            defaultValue = valueMap;
        }
    }
    else if (fieldType == "BOOLEAN")
    {
        field = new CheckField();
        if (required)
        {
            defaultValue = defaultValue.toBool();
        }
    }
    else if (fieldType == "NUMERIC")
    {
        field = new NumberField();
    }
    else if (fieldType == "DATE")
    {
        if (!defaultValue.toString().isEmpty())
        {
            defaultValue = "'" + defaultValue.toString() + "'";
        }

        if (fieldName == FIELD_PATROL_START_DATE)
        {
            field = new StringField();
        }
        else
        {
            field = new DateField();
        }
    }
    else if ((fieldType == "DATE_AND_TIME") || (fieldType == "TIME"))
    {
        if (!defaultValue.toString().isEmpty())
        {
            defaultValue = "'" + defaultValue.toString() + "'";
        }

        if (fieldName == FIELD_PATROL_START_TIME)
        {
            field = new StringField();
        }
        else
        {
            field = new TimeField();
        }
    }
    else if (fieldType == "UUID")
    {
        field = new StringField();
    }
    else if (fieldType == "MULTI_SELECT_LIST")
    {
        field = new CheckField(); // Haven't seen this yet.
    }
    else if (fieldType == "IMAGE")
    {
        field = new StringField();
    }
    else
    {
        qFatalIf(true, "Unknown field type: " + fieldType);
    }

    field->set_uid(fieldName);
    field->set_nameElementUid(fieldName);

    if (!fieldValues["options"].isNull())
    {
        static const std::pair<QString, QString> optionPrefixes[] = {
            { "SMART_Employees",       "e:"  },
            { "SMART_Mandate",         "pm:" },
            { "SMART_PatrolTransport", "tt:" },
            { "SMART_Station",         "ps:" },
            { "SMART_Team",            "pt:" },
            { "SMART_MissionAttList",  "ml:" },
            { "SMART_MissionAtt",      "ma:" },
            { "SMART_SamplingUnit",    "su:" },
        };

        QString optionPrefix;
        for (auto item: optionPrefixes)
        {
            if (fieldName == item.first)
            {
                optionPrefix = item.second;
                break;
            }
        }

        auto optionUids = QStringList();
        parseOptions(optionPrefix, fieldValues["options"].toList(), baseElement, &optionUids);

        field->set_listElementUid(fieldName);

        // Groups: SMART 7.5+.
        if (!fieldValues["groups"].toJsonArray().isEmpty())
        {
            m_schema->setSetting("hasOptionGroups", true);

            auto groupsElement = new Element(nullptr);
            groupsElement->set_uid(fieldName + "/groups");
            m_schema->appendElement(groupsElement);

            auto groups = fieldValues["groups"].toJsonArray();

            auto index = 1;
            for (auto groupIt = groups.constBegin(); groupIt != groups.constEnd(); groupIt++)
            {
                auto groupMap = groupIt->toObject().toVariantMap();
                auto groupOptionsUids = groupMap.value("options").toStringList();

                // Remove options which are in this group.
                for (auto optionIt = groupOptionsUids.constBegin(); optionIt != groupOptionsUids.constEnd(); optionIt++)
                {
                    optionUids.removeOne(*optionIt);
                }

                // Create a new group.
                auto groupElement = new Element(nullptr);
                groupElement->set_uid(fieldName + "/group/" + QString::number(index++));
                groupElement->set_names(findLabels(groupMap));
                groupElement->set_elementUids(groupOptionsUids);
                groupsElement->appendElement(groupElement);
            }

            groupsElement->set_elementUids(optionUids);

            field->set_listElementUid(groupsElement->uid());
        }
    }
    else if (!fieldValues["parent"].isNull())
    {
        auto parentValue = fieldValues["parent"].toString();
        field->set_listElementUid(parentValue);
        field->set_listFilterByField(parentValue);

        m_schema->setSetting("hasCascadingLists", true);
    }

    field->addTag("metadata", true);

    if (isFixed)
    {
        field->addTag("fixed", true);
    }

    auto requiredIf = QString();
    if (!requiredBy.isEmpty())
    {
        for (auto requiredByIt = requiredBy.constKeyValueBegin(); requiredByIt != requiredBy.constKeyValueEnd(); requiredByIt++)
        {
            auto requiredValues = requiredByIt->second.toStringList();
            if (requiredValues.isEmpty())
            {
                continue;
            }

            if (!requiredIf.isEmpty())
            {
                requiredIf += " or ";
            }

            requiredIf += '(';

            for (auto i = 0; i < requiredValues.count(); i++)
            {
                if (i > 0)
                {
                    requiredIf += " or ";
                }

                requiredIf += "${" + requiredByIt->first + "} = '" + requiredValues[i] + "'";
            }

            requiredIf += ')';
        }

        field->set_relevant(requiredIf.isEmpty() ? "false()" : requiredIf);

        if (!requiredIf.isEmpty())
        {
            m_schema->setSetting("hasConditionalFields", true);
        }
    }

    field->set_hidden(!fieldValues["isVisible"].toBool());
    field->set_required(requiredIf.isEmpty() ? QVariant(required) : requiredIf);
    field->set_defaultValue(defaultValue);

    m_schema->appendField(field);
}

void SMARTSchema::parseMetadata(const QString& metadataFilePath, QVariantMap* samplingGeoJsonOut) const
{
    auto groupListField = new ListField();
    groupListField->set_uid(FIELD_GROUPS);
    groupListField->set_required(true);
    groupListField->set_hidden(true);
    m_schema->appendField(groupListField);

    auto locationField = new LocationField();
    locationField->set_uid(FIELD_LOCATION);
    locationField->set_name(Schema::TR_LOCATION);
    locationField->set_required(true);
    locationField->set_allowManual(m_profile.value("MANUAL_GPS", false).toBool() || m_profile.value("USE_MAP_ON_SKIP", false).toBool());
    locationField->set_fixCount(m_profile.value("SIGHTING_FIX_COUNT", 0).toInt());
    m_schema->appendField(locationField);

    auto distanceField = new NumberField();
    distanceField->set_uid(FIELD_DISTANCE);
    distanceField->set_name(Schema::TR_DISTANCE);
    distanceField->set_minValue(0.0);
    distanceField->set_maxValue(10000.0);
    distanceField->set_decimals(2);
    distanceField->set_hidden(true);
    m_schema->appendField(distanceField);

    auto bearingField = new NumberField();
    bearingField->set_uid(FIELD_BEARING);
    bearingField->set_name(Schema::TR_BEARING);
    bearingField->set_minValue(0.0);
    bearingField->set_maxValue(359.0);
    bearingField->set_decimals(2);
    bearingField->set_hidden(true);
    m_schema->appendField(bearingField);

    auto observerField = new StringField();
    observerField->set_uid(FIELD_OBSERVER);
    observerField->set_name(Schema::TR_OBSERVER);
    observerField->set_hidden(true);
    observerField->set_required(true);
    observerField->set_listSorted(true);
    m_schema->appendField(observerField);

    auto sightingTypeField = new StringField();
    sightingTypeField->set_uid(FIELD_OBSERVATION_TYPE);
    sightingTypeField->set_required(true);
    sightingTypeField->set_hidden(true);
    sightingTypeField->addTag("metadata", true);
    m_schema->appendField(sightingTypeField);

    auto patrolLegCounterField = new NumberField();
    patrolLegCounterField->set_uid(FIELD_PATROL_LEG_COUNTER);
    patrolLegCounterField->set_hidden(true);
    patrolLegCounterField->set_defaultValue(1);
    patrolLegCounterField->set_decimals(0);
    m_schema->appendField(patrolLegCounterField);

    auto patrolLegDistanceField = new NumberField();
    patrolLegDistanceField->set_uid(FIELD_PATROL_LEG_DISTANCE);
    patrolLegDistanceField->set_hidden(true);
    patrolLegDistanceField->set_defaultValue(0);
    patrolLegDistanceField->set_decimals(4);
    m_schema->appendField(patrolLegDistanceField);

    auto hasIncidentType = false;

    auto jsonArray = Utils::variantListFromJsonFile(metadataFilePath);

    for (auto itemIt = jsonArray.constBegin(); itemIt != jsonArray.constEnd(); itemIt++)
    {
        auto itemObject = itemIt->toMap();
        qFatalIf(itemObject.count() != 1, "Expects each object to have one value");

        auto fieldName = itemObject.firstKey();
        auto fieldValues = itemObject[fieldName].toMap();

        // Found employees, so connect to the observer field.
        // Collect instances do not have employee lists, but keep the observer field around
        // so that there is less special casing between flavors.
        if (fieldName == FIELD_EMPLOYEES)
        {
            observerField->set_listElementUid(FIELD_EMPLOYEES);

            // Filter employees for patrols and surveys based on those selected.
            if (!QFileInfo(metadataFilePath).fileName().startsWith("incident"))
            {
                observerField->set_listFilterByField("SMART_Employees");
            }
        }

        if (fieldName == FIELD_INCIDENT_TYPE)
        {
            m_schema->setSetting("useIncidentType", fieldValues.value("isVisible", true).toBool());
            hasIncidentType = true;
        }

        // Make the sampling units look like other attributes - also correct for a typo coming from SMART ("SMART_SampingUnit").
        // Create a goto manager layer from the sampling unit data.
        if (fieldName == "SMART_SampingUnit" || fieldName == FIELD_SAMPLING_UNIT)
        {
            auto options = QVariantList();
            auto optionNull = QVariantMap();
            optionNull["label"] = Schema::TR_NONE;
            optionNull["id"] = "__sampling_unit_uid__";
            optionNull["uuid"] = "null";
            options.append(optionNull);

            auto samplingUnits = itemObject[fieldName].toList();
            auto gotoItems = QVariantList();

            for (auto samplingUnitIt = samplingUnits.constBegin(); samplingUnitIt != samplingUnits.constEnd(); samplingUnitIt++)
            {
                auto samplingUnitMap = samplingUnitIt->toMap();
                auto id = samplingUnitMap["id"];
                samplingUnitMap["label"] = id;
                options.append(samplingUnitMap);

                auto gotoItem = QVariantMap();
                auto gotoItemProps = QVariantMap();
                gotoItemProps["id"] = id;
                gotoItem["geometry"] = samplingUnitMap["geometry"];
                gotoItem["type"] = "Feature";
                gotoItem["properties"] = gotoItemProps;
                gotoItems.append(gotoItem);
            }

            samplingGeoJsonOut->insert("decoder", "sourceparser_smartsurveylayer");
            samplingGeoJsonOut->insert("features", gotoItems);
            samplingGeoJsonOut->insert("type", "FeatureCollection");

            fieldValues["default"] = optionNull["uuid"];
            fieldValues["isFixed"] = false;
            fieldValues["type"] = "SINGLE_CHOICE";
            fieldValues["isRequired"] = true;
            fieldValues["isVisible"] = true;
            fieldValues["options"] = options;
            fieldValues["label"] = Schema::TR_SAMPLING_UNIT;

            fieldName = FIELD_SAMPLING_UNIT;
            m_schema->setSetting("samplingUnits", true);
            m_schema->setSetting("samplingUnitCount", samplingUnits.count());
        }

        addMetadataItem(fieldName, fieldValues);
    }

    // Create a dummy incidentType field.
    if (!hasIncidentType)
    {
        auto incidentTypeField = new StringField();
        incidentTypeField->set_uid(FIELD_INCIDENT_TYPE);
        incidentTypeField->set_hidden(true);
        incidentTypeField->set_required(false);
        m_schema->appendField(incidentTypeField);
    }
}

PhotoField* SMARTSchema::createPhotoField(const QString& fieldUidSuffix, bool required) const
{
    auto photoField = new PhotoField();
    photoField->set_uid(QString(required ? FIELD_PHOTOS_REQUIRED : FIELD_PHOTOS) + fieldUidSuffix);
    photoField->set_name(Schema::TR_PHOTOS);
    photoField->set_exportUid(FIELD_PHOTO_PREFIX);
    photoField->set_required(required);
    photoField->set_minCount(0);
    photoField->set_maxCount(m_profile.value("MAX_PHOTO_COUNT", 3).toInt());
    photoField->set_showName(false);

    auto resizeImage = m_profile.value("RESIZE_IMAGE", false).toBool();
    photoField->set_resolutionX(resizeImage ? m_profile.value("IMAGE_WIDTH").toInt() : DEFAULT_PHOTO_RESOLUTION_X);
    photoField->set_resolutionY(resizeImage ? m_profile.value("IMAGE_HEIGHT").toInt() : DEFAULT_PHOTO_RESOLUTION_Y);

    return photoField;
}

AudioField* SMARTSchema::createAudioField(const QString& fieldUidSuffix) const
{
    auto audioField = new AudioField();
    audioField->set_uid(QString(FIELD_AUDIO) + fieldUidSuffix);
    audioField->set_name(Schema::TR_AUDIO_NOTE);
    audioField->set_exportUid(FIELD_AUDIO_PREFIX);

    return audioField;
}

void SMARTSchema::parseModel(const QString& modelFilePath) const
{
    auto modelField = new RecordField();
    modelField->set_uid(FIELD_MODEL);
    m_schema->appendField(modelField);

    auto groupField = new StringField();
    groupField->set_uid(FIELD_SIGHTING_GROUP_ID);
    modelField->appendField(groupField);

    auto treeField = new StringField();
    treeField->set_uid(FIELD_MODEL_TREE);
    treeField->set_listElementUid(FIELD_MODEL_TREE);
    modelField->appendField(treeField);

    modelField->appendField(createPhotoField("", false));
    modelField->appendField(createPhotoField("", true));

    // Process the model.
    QFile modelFile(modelFilePath);
    qFatalIf(!modelFile.open(QFile::ReadOnly | QFile::Text), "Failed to read " + modelFilePath);

    QFile xmlInputSource(modelFilePath);
    xmlInputSource.open(QIODevice::ReadOnly);
    SMARTModelHandler modelHandler(m_schema, modelField);

    QXmlStreamReader xmlReader(xmlInputSource.readAll());

    while (!xmlReader.atEnd())
    {
        xmlReader.readNext();

        if (xmlReader.isStartElement())
        {
            modelHandler.startElement(xmlReader.name().toString(), xmlReader.attributes());
        }
        else if (xmlReader.isEndElement())
        {
            modelHandler.endElement(xmlReader.name().toString());

        }
        else if (xmlReader.isCharacters() && !xmlReader.isWhitespace())
        {
            //modelHandler.characters(xmlReader.text().toString());
        }
    }

    // QXmlSimpleReader xmlReader;
    // xmlReader.setContentHandler(&modelHandler);
    // xmlReader.setErrorHandler(&modelHandler);

    // QXmlInputSource xmlInputSource(&modelFile);
    // qFatalIf(!xmlReader.parse(xmlInputSource), "Failed to parse " + modelFilePath);

    modelField->appendField(createAudioField(""));

    // Complete model processing.
    postProcessModel(modelField);
}

void SMARTSchema::postProcessModel(RecordField* modelField) const
{
    // Creator for audio and photo fields.
    auto createAttachmentFields = [&](RecordField* recordField, const QString& suffix, bool allowed, bool required)
    {
        if (allowed)
        {
            recordField->insertField(0, createPhotoField(suffix, required));
        }

        if (allowed  && m_smartVersion >= 700)
        {
            recordField->appendField(createAudioField(suffix));
        }
    };

    // Handle complex nodes.
    auto leafElements = m_schema->getLeafElements(m_schema->rootElement()->uid());
    for (auto leafElementIt = leafElements.constBegin(); leafElementIt != leafElements.constEnd(); leafElementIt++)
    {
        auto leafElement = *leafElementIt;
        auto fieldUids = leafElement->fieldUids();
        if (fieldUids.isEmpty())
        {
            continue;
        }

        auto photoFirst = m_schema->getSetting("photoFirst").toBool();
        auto photoAllowed = photoFirst || leafElement->getTagValue("photoAllowed") == "true";
        auto photoRequired = leafElement->getTagValue("photoRequired") == "true" && !photoFirst;
        auto audioAllowed = photoAllowed && m_smartVersion >= 700;
        auto handled = false;

        // Dynamic observations.
        if (leafElement->getTagValue("collectMultipleObs") == "true")
        {
            m_schema->setSetting("hasCollectMultipleObs", true);

            auto recordFieldUid = leafElement->uid() + ".dynamic";
            auto recordField = new RecordField();
            recordField->set_uid(recordFieldUid);
            modelField->appendField(recordField);

            auto newFieldUids = QStringList();

            for (auto i = 0; i < fieldUids.count(); i++)
            {
                auto field = m_schema->getField(fieldUids[i]);
                auto enterOnce = field->getTagValue("ENTER_ONCE").toString();

                // If ENTER_ONCE is empty, then this field is part of the dynamic set.
                if (enterOnce.isEmpty())
                {
                    if (!newFieldUids.contains(recordFieldUid))
                    {
                        newFieldUids.append(recordFieldUid);
                    }

                    modelField->detachField(field);
                    recordField->appendField(field);
                }
                else
                {
                    newFieldUids.append(fieldUids[i]);

                    if (enterOnce == "END")
                    {
                        modelField->detachField(field);
                        modelField->appendField(field);
                    }
                }
            }

            if (m_smartVersion >= 700)
            {
                createAttachmentFields(recordField, "." + recordFieldUid, photoAllowed, photoRequired);
            }

            fieldUids = newFieldUids;

            // Cleanup sub-record if it is empty.
            // This is a GIGO case, but nothing prevents it and this feature is not easy to use.
            if (recordField->fieldCount() == 0)
            {
                modelField->detachField(recordField);
                delete recordField;
            }

            handled = true;
        }

        // Multi-select: does not co-exist with dynamic observations (above).
        if (!handled)
        {
            auto newFieldUids = QStringList();

            for (auto i = 0; i < fieldUids.count(); i++)
            {
                auto fieldUid = fieldUids[i];
                auto field = m_schema->getField(fieldUid);
                newFieldUids.append(fieldUid);

                // Skip this for MLISTs, which are just simple check lists.
                if (field->getTagValue("MLIST").toBool())
                {
                    field->removeTag("MLIST");
                    continue;
                }

                // Look for check-list and number-list fields.
                if (field->listElementUid().isEmpty() || (!qobject_cast<CheckField*>(field) && !qobject_cast<NumberField*>(field)))
                {
                    continue;
                }

                auto recordFieldUid = leafElement->uid() + ".multiselect";
                newFieldUids.append(recordFieldUid);

                auto recordField = new RecordField();
                recordField->set_uid(recordFieldUid);
                recordField->set_nameElementUid(field->nameElementUid());
                recordField->set_masterFieldUid(fieldUid);

                // Move the fields into the record.
                for (auto j = i + 1; j < fieldUids.count(); j++)
                {
                    auto field = m_schema->getField(fieldUids[j]);
                    modelField->detachField(field);
                    recordField->appendField(field);
                }

                if (m_smartVersion >= 700)
                {
                    createAttachmentFields(recordField, "." + recordFieldUid, photoAllowed, photoRequired);
                }

                fieldUids = newFieldUids;

                // Tag this field so that export can cheaply know to give it special treatment.
                field->addTag("master", true);

                // Hide the record if there are no fields.
                recordField->set_hidden(recordField->fieldCount() == 0);
                modelField->appendField(recordField);

                if (recordField->fieldCount() > 0)
                {
                    m_schema->setSetting("hasMultiSelectFollowups", true);
                }

                handled = true;
                break;
            }
        }

        // Matrix questions.
        if (!handled)
        {
            auto newFieldUids = QStringList();
            auto recordField = (RecordField*)nullptr;

            auto i = 0;
            while (i < fieldUids.count())
            {
                auto fieldUid = fieldUids[i];
                auto field = m_schema->getField(fieldUid);

                // Skip normal fields.
                if (!field->getTagValue("INPUT_GROUP").toBool())
                {
                    newFieldUids.append(fieldUid);
                    i++;
                    continue;
                }

                // Matrix field detected: move into a separate record field.
                if (recordField == nullptr)
                {
                    auto recordFieldUid = leafElement->uid() + ".matrix";
                    recordField = new RecordField();
                    recordField->set_uid(recordFieldUid);
                    newFieldUids.append(recordFieldUid);
                    modelField->appendField(recordField);
                }

                modelField->detachField(field);
                recordField->appendField(field);

                i++;
            }

            // Compute the number of matrix dimensions for the record.
            if (recordField)
            {
                fieldUids = newFieldUids;

                auto matrixCount = 0;
                for (matrixCount = 0; matrixCount < recordField->fieldCount(); matrixCount++)
                {
                    if (recordField->field(matrixCount)->listElementUid().isEmpty())
                    {
                        break;
                    }

                    recordField->field(matrixCount)->set_hidden(true);
                }

                recordField->set_matrixCount(matrixCount);

                m_schema->setSetting("hasMatrixQuestions", true);

                //handled = true;
            }
        }

        // Prepend a photo field as needed.
        if (photoAllowed)
        {
            fieldUids.insert(0, photoRequired ? FIELD_PHOTOS_REQUIRED : FIELD_PHOTOS);
        }

        // Append an audio field as needed.
        if (audioAllowed)
        {
            fieldUids.append(FIELD_AUDIO);
        }

        leafElement->set_fieldUids(fieldUids);
    }
}

} // namespace CyberTracker
