#include "GoogleFormParser.h"
#include "App.h"

GoogleFormParser::GoogleFormParser(QObject* parent): QObject(parent)
{
}

bool GoogleFormParser::parse(const QString& formFilePath, const QString& targetFolder, QString* parserErrorOut)
{
    GoogleFormParser parser;
    return parser.execute(formFilePath, targetFolder, parserErrorOut);
}

bool GoogleFormParser::execute(const QString& formFilePath, const QString& targetFolder, QString* parserErrorOut)
{
    // Load form.
    auto formMap = Utils::variantMapFromJsonFile(formFilePath);
    if (formMap.isEmpty())
    {
        qDebug() << "Form is empty";
        return false;
    }

    // Process the form.
    m_targetFolder = Utils::removeTrailingSlash(targetFolder);
    processForm(formMap);

    // Post process.
    postProcess(m_fieldManager.rootField());

    // title.
    auto title = formMap["title"].toString();
    m_settings["title"] = title.isEmpty() ? tr("Untitled form") : title;

    // Email.
    auto email = formMap["email"].toString();
    m_settings["email"] = email.isEmpty() ? "unknown@unknown.com" : email;

    // subtitle.
    auto formParams = QVariantMap();
    auto subtitle = parseParams(&formParams, formMap["description"].toString());
    subtitle.replace('\n', ' ');
    m_settings["subtitle"] = subtitle;

    // Miscellaneous settings.
    auto parseStringSetting = [&](const QString& name, const QString& defaultValue = "", bool downloadHttps = false)
    {
        auto value = formParams.value(name, defaultValue).toString().trimmed();
        if (downloadHttps && value.startsWith("https:"))
        {
            value = downloadFile(value, name);
        }

        if (!value.isEmpty())
        {
            m_settings[name] = value;
        }

        formParams.remove(name);
    };

    auto parseBoolSetting = [&](const QString& name, bool defaultValue)
    {
        m_settings[name] = formParams.value(name, defaultValue).toBool();
        formParams.remove(name);
    };

    auto parseIntSetting = [&](const QString& name, int defaultValue)
    {
        m_settings[name] = formParams.value(name, defaultValue).toInt();
        formParams.remove(name);
    };

    parseStringSetting("icon", "qrc:/Google/logo.svg", true);
    parseStringSetting("iconDark", QString(), true);
    parseStringSetting("offlineMapUrl");
    parseBoolSetting("immersive", false);
    parseBoolSetting("wizardMode", true);
    parseIntSetting("submitInterval", 0);

    // colors.
    auto colors = QVariantMap
    {
        { "primary", "#414452" },
        { "accent", "#2696EF" },
    };

    auto colorsOverride = formParams.value("colors").toMap();
    if (!colorsOverride.isEmpty())
    {
        colors.insert(colorsOverride);
        formParams.remove("colors");
    }
    m_settings["colors"] = colors;

    // File upload folder.
    auto driveFolderId = formParams.value("fileFolder").toString().trimmed();
    if (!driveFolderId.isEmpty())
    {
        if (driveFolderId.startsWith("https:"))
        {
            if (driveFolderId.endsWith('/'))
            {
                driveFolderId.remove(driveFolderId.length() - 1, 1);
            }

            auto lastSlashIndex = driveFolderId.lastIndexOf("/");
            if (lastSlashIndex != -1)
            {
                driveFolderId = driveFolderId.right(driveFolderId.length() - lastSlashIndex - 1);
            }
        }

        m_settings["driveFolderId"] = driveFolderId.split('?').first();
    }
    formParams.remove("fileFolder");

    // Add summary text and icon.
    if (!m_summaryTexts.isEmpty())
    {
        formParams["summaryText"] = m_summaryTexts.join(" ");
    }

    if (!m_summaryIcons.isEmpty())
    {
        formParams["summaryIcon"] = m_summaryIcons.join(" ");
    }

    // Save targets.
    if (!m_saveTargets.isEmpty())
    {
        auto save = formParams["save"].toMap();
        save["targets"] = m_saveTargets;
        formParams["save"] = save;
    }

    // Snap location.
    if (!m_snapLocation.isEmpty())
    {
        auto save = formParams["save"].toMap();
        save["snapLocation"] = m_snapLocation;
        formParams["save"] = save;
    }

    // Complete.
    auto rootField = m_fieldManager.rootField();
    rootField->set_version(formMap["version"].toString());
    rootField->set_parameters(formParams);

    m_fieldManager.saveToQmlFile(m_targetFolder + "/Fields.qml");
    m_elementManager.saveToQmlFile(m_targetFolder + "/Elements.qml");
    Utils::writeJsonToFile(m_targetFolder + "/Settings.json", Utils::variantMapToJson(m_settings));

    if (m_fieldManager.rootField()->fieldCount() == 0)
    {
        *parserErrorOut = tr("No questions");
        return false;
    }

    return true;
}

QString GoogleFormParser::parseParams(QVariantMap* params, const QString& description)
{
    auto desc = description.trimmed();
    if (desc.isEmpty())
    {
        return "";
    }

    auto result = QStringList();
    auto paramLine = QStringList();

    auto lines = desc.split("\n");
    for (auto lineIt = lines.constBegin(); lineIt != lines.constEnd(); lineIt++)
    {
        auto line = lineIt->trimmed();
        if (line.startsWith("#"))
        {
            line.remove(0, 1);
            paramLine.append(line);
        }
        else
        {
            result.append(*lineIt);
        }
    }

    for (auto lineIt = paramLine.constBegin(); lineIt != paramLine.constEnd(); lineIt++)
    {
        auto index = lineIt->indexOf('=');
        auto attribute = lineIt->trimmed();
        auto valueText = QString("true");

        if (index != -1)
        {
            attribute = lineIt->left(index).trimmed();
            valueText = lineIt->right(lineIt->length() - index - 1).trimmed();
        }

        Utils::insertParameter(params, attribute, valueText);
    }

    return result.join('\n');
}

void GoogleFormParser::processForm(const QVariantMap& formMap)
{
    auto sectionField = m_fieldManager.rootField();

    m_settings["hasFileUploadItem"] = formMap.value("hasFileUploadItem", false);

    auto items = formMap["items"].toList();

    for (auto itemIt = items.constBegin(); itemIt != items.constEnd(); itemIt++)
    {
        auto item = itemIt->toMap();
        auto itemId = item["itemId"].toString();
        auto itemType = item["type"].toString();

        auto name = item["title"].toString().trimmed();
        auto icon = QString();

        auto params = QVariantMap();

        auto hintName = parseParams(&params, item["helpText"].toString());
        auto hintIcon = QString();
        auto hintLink = QString();

        auto field = (BaseField*)nullptr;

        if (params.value("saveTarget").toBool())
        {
            m_saveTargets.append(QVariantMap {{ "choice", itemId }, { "question", itemId }});
            auto element = new Element();
            element->set_uid("saveTarget/" + itemId);
            element->set_name(name.isEmpty() ? "??" : name);
            m_elementManager.rootElement()->appendElement(element);
        }

        // GRID.
        if (item.contains("questionIds"))
        {
            field = parseQuestionGrid(items, itemId, item, params);
        }
        // questionItem.
        else if (item.contains("questionId"))
        {
            field = parseQuestion(items, itemId, item, params);
        }
        // textItem.
        else if (itemType == "SECTION_HEADER")
        {
            field = new StaticField();
        }
        // imageItem.
        else if (itemType == "IMAGE")
        {
            hintIcon = "qrc:/icons/image_outline.svg";
            hintName = parseParams(&params, item["helpText"].toString());

            field = new StaticField();
        }
        // pageBreakItem.
        else if (itemType == "PAGE_BREAK")
        {
            sectionField = m_fieldManager.rootField();
            auto groupField = new RecordField();
            groupField->set_minCount(1);
            groupField->set_maxCount(1);
            groupField->addTag("section", true);
            field = groupField;

            // Add the navigation parameter.
            auto navType = item["navType"].toString();

            auto gotoSectionId = QString();
            if (navType == "GO_TO_PAGE")
            {
                gotoSectionId = item["gotoPage"].toString();
            }
            else if (navType == "CONTINUE")
            {
                gotoSectionId = itemId;
            }
            else if (navType == "SUBMIT")
            {
                gotoSectionId = "SUBMIT";
            }
            else if (navType == "RESTART")
            {
                gotoSectionId = items.constFirst().toMap().value("itemId").toString();
            }

            if (!gotoSectionId.isEmpty() && isForwardLink(items, itemId, gotoSectionId))
            {
                field->addTag("skipTo", gotoSectionId);
            }
        }

        // Skipping field.
        if (!field)
        {
            continue;
        }

        // Name Element.
        auto nameElement = new Element();
        nameElement->set_uid(itemId);
        nameElement->set_name(name.isEmpty() ? "??" : name);
        nameElement->set_icon(downloadFile(icon, itemId));
        m_elementManager.rootElement()->appendElement(nameElement);

        // Add the hint if specified.
        if (!hintName.isEmpty() || !hintIcon.isEmpty())
        {
            auto hintElement = new Element();
            hintElement->set_uid(itemId + "/hint");
            hintElement->set_name(hintName);
            hintElement->set_icon(downloadFile(hintIcon, itemId + "_hint"));
            nameElement->appendElement(hintElement);

            field->set_hintElementUid(hintElement->uid());
        }

        // Add the field.
        field->set_uid(itemId);
        field->set_nameElementUid(nameElement->uid());
        field->set_hintLink(hintLink);
        field->set_parameters(params);
        sectionField->appendField(field);

        if (field->listOther())
        {
            auto otherField = new StringField();
            otherField->set_uid(field->uid() + "_other");
            otherField->set_exportUid(field->exportUid() + ".other_option_response"); // GoogleForms well-known-text.
            otherField->set_nameElementUid(field->nameElementUid());
            otherField->set_relevant("selected(${" + field->uid() + "}, '__other__')");
            otherField->set_required(field->required());
            otherField->set_parameters(params);
            sectionField->appendField(otherField);
        }

        if (itemType == "PAGE_BREAK")
        {
            sectionField = (RecordField*)RecordField::fromField(field);
        }
    }
}

QString GoogleFormParser::downloadFile(const QString& url, const QString& targetName)
{
    if (url.isEmpty())
    {
        return "";
    }

    if (url.startsWith("qrc:/"))
    {
        return url;
    }

    auto filePath = App::instance()->downloadFile(url);
    if (filePath.isEmpty())
    {
        qDebug() << "Failed to download content for item: " << targetName;
        return "";
    }

    auto targetFilename = QString("%1.%2").arg(targetName, QFileInfo(filePath).suffix());
    auto targetFilePath = m_targetFolder + "/" + targetFilename;
    QFile::remove(targetFilePath);
    QFile::rename(filePath, targetFilePath);

    return targetFilename;
}

QString GoogleFormParser::findNextPageBreakItemId(const QVariantList& items, const QString& itemId) const
{
    auto found = false;
    for (auto itemIt = items.constBegin(); itemIt != items.constEnd(); itemIt++)
    {
        auto item = itemIt->toMap();
        auto currItemId = item["itemId"].toString();
        if (currItemId == itemId)
        {
            found = true;
            continue;
        }

        if (found && item.value("type").toString() == "PAGE_BREAK")
        {
            return currItemId;
        }
    }

    return "";
}

bool GoogleFormParser::isForwardLink(const QVariantList& items, const QString& currItemId, const QString& gotoItemId) const
{
    if (gotoItemId == "SUBMIT")
    {
        return true;
    }

    auto currIndex = -1;
    auto gotoIndex = -1;

    for (auto i = 0; i < items.length(); i++)
    {
        auto itemId = items.at(i).toMap()["itemId"].toString();
        if (currItemId == itemId)
        {
            currIndex = i;
        }

        if (gotoItemId == itemId)
        {
            gotoIndex = i;
        }
    }

    auto result = gotoIndex >= currIndex;

    if (!result)
    {
        qDebug() << "Skipping backward link from " << currItemId << " to " << gotoItemId;
    }

    return result;
}

Element* GoogleFormParser::parseChoices(const QVariantList& items, const QString& itemId, const QVariantList& choices, bool addOther, bool* hasNavOut)
{
    auto hasNav = false;
    auto choicesElement = new Element();
    choicesElement->set_uid(itemId + "/options");
    m_elementManager.rootElement()->appendElement(choicesElement);

    auto choiceIndex = 0;
    for (auto choiceIt = choices.constBegin(); choiceIt != choices.constEnd(); choiceIt++, choiceIndex++)
    {
        auto choice = choiceIt->toMap();
        auto choiceUid = QString("%1_%2").arg(itemId, QString::number(choiceIndex));
        auto choiceName = choice.value("name").toString();
        auto exportUid = choice.value("exportUid", choiceName).toString();

        auto navType = choice.value("navType").toString();
        hasNav |= !navType.isEmpty();
        auto gotoSectionId = choice.value("gotoPage").toString();
        if (navType == "NEXT_SECTION")
        {
            auto nextSectionId = findNextPageBreakItemId(items, itemId);
            if (!nextSectionId.isEmpty())
            {
                gotoSectionId = nextSectionId;
            }
        }
        else if (navType == "RESTART_FORM")
        {
            gotoSectionId = items.constFirst().toMap()["itemId"].toString();
        }
        else if (navType == "SUBMIT")
        {
            gotoSectionId = "SUBMIT";
        }

        auto element = new Element();
        if (!gotoSectionId.isEmpty() && isForwardLink(items, itemId, gotoSectionId))
        {
            element->addTag("skipTo", gotoSectionId);
        }

        auto nameParts = exportUid.trimmed().split('|');
        if (nameParts.length() > 1)
        {
            choiceName = nameParts.constFirst();
            auto targetFilename = downloadFile(nameParts.constLast(), choiceUid);
            element->set_icon(targetFilename);
        }

        element->set_uid(choiceUid);
        element->set_name(choiceName);
        element->set_exportUid(exportUid);
        choicesElement->appendElement(element);
    }

    // Other: disallowed for navigation.
    if (addOther && !hasNav)
    {
        auto element = new Element();
        element->set_uid(QString("%1/__other__").arg(itemId));
        element->set_exportUid("__other_option__"); // GoogleForms well-known-text.
        element->set_name("tr:Other");
        element->set_other(true);
        choicesElement->appendElement(element);
    }

    if (hasNavOut)
    {
        *hasNavOut = hasNav;
    }

    return choicesElement;
}

RecordField* GoogleFormParser::parseQuestionGrid(const QVariantList& items, const QString& itemId, const QVariantMap& item, const QVariantMap& params)
{
    auto autoNext = params.value("autoNext").toBool();

    auto choicesElement = new Element();
    choicesElement->set_uid(itemId + "/options");
    m_elementManager.rootElement()->appendElement(choicesElement);

    auto columns = item["columns"].toStringList();
    auto choiceIndex = 0;
    for (auto choiceIt = columns.constBegin(); choiceIt != columns.constEnd(); choiceIt++, choiceIndex++)
    {
        auto element = new Element();
        auto choiceUid = QString("%1_%2").arg(itemId, QString::number(choiceIndex));
        auto choiceName = *choiceIt;
        auto exportUid = choiceName;

        auto nameParts = exportUid.trimmed().split('|');
        if (nameParts.length() > 1)
        {
            choiceName = nameParts.constFirst();
            auto targetFilename = downloadFile(nameParts.constLast(), choiceUid);
            element->set_icon(targetFilename);
        }

        element->set_uid(choiceUid);
        element->set_name(choiceName);
        element->set_exportUid(exportUid);
        choicesElement->appendElement(element);
    }


    // Questions.
    auto groupField = new RecordField();
    groupField->addParameter("field-list", true);
    groupField->set_minCount(1);
    groupField->set_maxCount(1);

    auto rows = item["rows"].toStringList();
    auto questionIds = item["questionIds"].toStringList();

    auto rowIndex = 0;
    for (auto questionIt = questionIds.constBegin(); questionIt != questionIds.constEnd(); questionIt++, rowIndex++)
    {
        auto questionId = *questionIt;
        auto title = rows[rowIndex];

        auto field = (BaseField*)nullptr;
        auto itemType = item.value("type").toString();
        if (itemType == "GRID")
        {
            field = new StringField();
            field->addParameter("autoNext", autoNext);
        }
        else if (itemType == "CHECKBOX_GRID")
        {
            field = new CheckField();
        }
        else
        {
            qDebug() << "Unknown field type";
            continue;
        }

        auto uid = itemId + "_" + questionId;

        auto nameElement = new Element();
        nameElement->set_uid(uid);
        nameElement->set_name(title);
        m_elementManager.rootElement()->appendElement(nameElement);

        auto exportUid = QString("entry.%1").arg(questionId);

        field->set_uid(uid);
        field->set_exportUid(exportUid);
        field->set_nameElementUid(uid);
        field->set_listElementUid(choicesElement->uid());
        field->set_required(item.value("required", false).toBool());
        groupField->appendField(field);
    }

    return groupField;
}

BaseField* GoogleFormParser::parseQuestion(const QVariantList& items, const QString& itemId, const QVariantMap& item, const QVariantMap& params)
{
    auto autoNext = params.value("autoNext").toBool();
    auto type = item["type"].toString();
    auto exportUid = QString("entry.%1").arg(item.value("questionId").toString());
    auto required = item.value("required", false).toBool();

    if (params.value("summaryText").toBool())
    {
        m_summaryTexts.append(itemId);
    }

    if (params.value("summaryIcon").toBool())
    {
        m_summaryIcons.append(itemId);
    }

    auto createField = [&]() -> BaseField*
    {
        if (type == "TEXT" || type == "PARAGRAPH_TEXT")
        {
            // Constraint message.
            auto constraintMessage = params.value("constraintMessage").toString();
            auto constraintElementUid = QString();
            if (!constraintMessage.isEmpty())
            {
                auto element = new Element();
                element->set_uid(itemId + "/constraint");
                element->set_name(constraintMessage);
                m_elementManager.rootElement()->appendElement(element);
                constraintElementUid = element->uid();
            }

            // Field types.
            if (params.value("start").toBool())
            {
                auto field = new CalculateField();
                field->set_formula("createTime");
                field->set_hidden(true);
                return field;
            }

            if (params.value("end").toBool())
            {
                auto field = new CalculateField();
                field->set_formula("updateTime");
                field->set_hidden(true);
                return field;
            }

            if (params.value("sessionId").toBool())
            {
                auto field = new CalculateField();
                field->set_formula("sessionId");
                field->set_hidden(true);
                return field;
            }

            if (params.value("username").toBool())
            {
                auto field = new CalculateField();
                field->set_formula("username");
                field->set_hidden(true);
                m_settings["requireUsername"] = true;
                return field;
            }

            if (params.value("geopoint").toBool())
            {
                if (params.value("snapOnSave").toBool())
                {
                    m_snapLocation = itemId;
                }

                auto field = new LocationField();
                field->set_fixCount(params.value("fixCount", 3).toInt());
                field->set_allowManual(params.value("allowManual", true).toBool());
                return field;
            }

            if (params.value("geotrace").toBool())
            {
                auto field = new LineField();
                return field;
            }

            if (params.value("geoshape").toBool())
            {
                auto field = new AreaField();
                return field;
            }

            if (params.value("number").toBool())
            {
                auto field = new NumberField();
                field->set_minValue(params.value("minValue", field->minValue()).toDouble());
                field->set_maxValue(params.value("maxValue", field->maxValue()).toDouble());
                field->set_decimals(params.value("decimals", field->decimals()).toInt());
                field->set_step(params.value("step", field->step()).toDouble());
                if (params.contains("defaultValue"))
                {
                    field->set_defaultValue(params.value("defaultValue").toDouble());
                }
                field->set_constraintElementUid(constraintElementUid);

                return field;
            }

            if (params.value("photo").toBool())
            {
                m_settings["needsFileFolder"] = true;
                auto field = new PhotoField();
                auto maxPixels = params.value("maxPixels", 2048).toInt();
                field->set_resolutionX(maxPixels);
                field->set_resolutionY(maxPixels);
                field->set_minCount(params.value("minCount", field->minCount()).toInt());
                field->set_maxCount(params.value("maxCount", field->maxCount()).toInt());
                return field;
            }

            if (params.value("sketch").toBool())
            {
                m_settings["needsFileFolder"] = true;
                auto field = new SketchField();
                return field;
            }

            if (params.value("audio").toBool())
            {
                m_settings["needsFileFolder"] = true;
                auto field = new AudioField();
                field->set_maxSeconds(params.value("maxSeconds", field->maxSeconds()).toInt());
                field->set_sampleRate(params.value("sampleRate", field->sampleRate()).toInt());
                return field;
            }

            if (params.value("file").toBool())
            {
                m_settings["needsFileFolder"] = true;
                auto field = new FileField();
                return field;
            }

            if (params.value("barcode").toBool())
            {
                auto field = new StringField();
                field->set_barcode(true);
                return field;
            }

            auto field = new StringField();

            auto constraintRegex = params.value("constraintRegex").toString();
            if (!constraintRegex.isEmpty())
            {
                field->set_constraint("regex(., '" + constraintRegex + "')");
            }

            field->set_multiLine(type == "PARAGRAPH_TEXT");
            field->set_inputMask(params.value("inputMask").toString());
            field->set_defaultValue(params.value("defaultValue").toString());
            field->set_numbers(params.value("numberInput").toBool());
            field->set_constraintElementUid(constraintElementUid);

            return field;
        }

        if (type == "DATE")
        {
            auto field = new DateField();
            field->set_hideYear(!item.value("includeYear").toBool());
            return field;
        }

        if (type == "DATETIME")
        {
            auto field = new DateTimeField();
            field->set_hideYear(!item.value("includeYear").toBool());
            return field;
        }

        if (type == "TIME")
        {
            auto field = new TimeField();
            return field;
        }

        if (type == "DURATION")
        {
            auto field = new StringField();
            field->set_inputMask("00:00:00");
            field->set_constraint("regex(., '^([0-1]?[0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]$')");
            field->set_numbers(true);
            field->addTag("duration", true);

            return field;
        }

        if (type == "SCALE")
        {
            auto highValue = item.value("upperBound").toInt();
            auto highLabel = item.value("leftLabel").toString();
            auto lowValue = item.value("lowerBound").toInt();
            auto lowLabel = item.value("rightLabel").toString();

            auto choices = QVariantList();
            for (auto value = lowValue; value <= highValue; value++)
            {
                auto name = QString::number(value);
                if (value == lowValue && !lowLabel.isEmpty())
                {
                    name += " - " + lowLabel;
                }
                else if (value == highValue && !highLabel.isEmpty())
                {
                    name += " - " + highLabel;
                }

                choices.append(QVariantMap {{ "value", name }, { "exportUid", QString::number(value) }});
            }

            auto field = new StringField();
            auto optionsElement = parseChoices(items, itemId, choices, false);
            field->set_listElementUid(optionsElement->uid());
            field->addParameter("autoNext", autoNext);

            return field;
        }

        if (type == "MULTIPLE_CHOICE" || type == "LIST")
        {
            auto field = new StringField();
            field->addParameter("autoNext", autoNext);
            auto hasOther = item["hasOther"].toBool();
            auto hasNav = false;
            auto choicesElement = parseChoices(items, itemId, item["choices"].toList(), hasOther, &hasNav);

            required = required || hasNav; // Nav choices are required and other is not allowed.
            field->set_listOther(hasOther && !hasNav);
            field->set_listElementUid(choicesElement->uid());

            return field;
        }

        if (type == "CHECKBOX")
        {
            auto field = new CheckField();
            auto hasOther = item["hasOther"].toBool();
            field->set_listOther(hasOther);
            auto choicesElement = parseChoices(items, itemId, item["choices"].toList(), hasOther);
            field->set_listElementUid(choicesElement->uid());

            return field;
        }

        return nullptr;
    };

    auto field = createField();
    if (field)
    {
        field->set_exportUid(exportUid);
        field->set_required(required);
    }

    return field;
}

class PostProcessor
{
public:
    PostProcessor(FieldManager* fieldManager, ElementManager* elementManager)
    {
        m_fieldManager = fieldManager;
        m_elementManager = elementManager;
    }

    void execute()
    {
        // Build a list of all the sections.
        auto sections = buildSections();

        // Build all paths through the form.
        auto paths = QList<Path>();
        traverse(&sections, 0, [&](Path* path)
        {
            paths.append(*path);
        });

        // Set the relevant property for each sections if it can be reached.
        for (auto sectionIt = sections.begin() + 1; sectionIt != sections.end(); sectionIt++)
        {
            auto sectionField = sectionIt->recordField;
            auto relevant = QStringList{};
            auto found = false;

            // Look for paths containing the section.
            for (auto pathIt = paths.constBegin(); pathIt != paths.constEnd(); pathIt++)
            {
                if (!pathIt->sectionUids.contains(sectionField->uid()))
                {
                    continue;
                }

                found = true;
                if (!pathIt->expression.isEmpty())
                {
                    relevant.append("(" + pathIt->expression.join(" and ") + ")");
                }
            }

            // If no path to the section is found, it must never be relevant.
            if (!found)
            {
                relevant = QStringList {{ "false()" }};
            }

            // Create an expression containing all the ways to reach the section.
            sectionField->set_relevant(relevant.join(" or "));
        }
    }

private:
    struct Section
    {
        QString sectionUid;
        QString nextSectionUid;
        RecordField* recordField = nullptr;
    };

    typedef QList<Section> SectionList;

    struct Path
    {
        QStringList expression;
        QStringList sectionUids;
    };

    // Create a list of all the record fields marked with the section parameter.
    SectionList buildSections()
    {
        auto sections = SectionList { Section { m_fieldManager->rootField()->uid(), "", m_fieldManager->rootField()} };
        m_fieldManager->rootField()->enumFields([&](BaseField* field)
        {
            if (!field->getTagValue("section", false).toBool())
            {
                return;
            }

            auto skipTo = field->getTagValue("skipTo").toString();
            sections.last().nextSectionUid = skipTo;
            sections.append(Section { field->uid(), "", qobject_cast<RecordField*>(field) });
        });

        return sections;
    }

    // Find the index of a section in the list.
    int getSectionIndex(SectionList* sections, const QString sectionUid)
    {
        auto i = 0;
        for (auto it = sections->constBegin(); it != sections->constEnd(); it++, i++)
        {
            if (it->sectionUid == sectionUid)
            {
                return i;
            }
        }

        return -1;
    }

    // Build paths through the form.
    void traverse(SectionList* sections, int startIndex, std::function<void(Path* path)> pathFound, Path* path = nullptr)
    {
        Path stackPath;
        if (path == nullptr)
        {
            path = &stackPath;
        }

        auto section = sections->at(startIndex);
        path->sectionUids.append(section.sectionUid);

        auto deadEnd = false;

        section.recordField->enumFields([&](BaseField* field)
        {
            // Skip if path is completed.
            if (deadEnd)
            {
                return;
            }

            // Skip non-radio lists.
            auto listField = qobject_cast<StringField*>(field);
            if (!listField || listField->listElementUid().isEmpty())
            {
                return;
            }

            // allHandled will be true if all items have skipTo set.
            auto allHandled = true;

            // Enum radio list elements.
            m_elementManager->enumChildren(listField->listElementUid(), true, [&](Element* element)
            {
                auto elementUid = element->uid();
                auto expression = "selected(${" + field->uid() + "}, '" + elementUid + "')";

                // Submit means no further sections.
                auto skipTo = element->getTagValue("skipTo").toString();
                if (skipTo == "SUBMIT")
                {
                    auto forkPath = Path();
                    forkPath = *path;
                    forkPath.expression.append(expression);
                    pathFound(&forkPath);
                    return;
                }

                // Skip to section.
                auto skipToSectionIndex = -1;
                if (!skipTo.isEmpty())
                {
                    skipToSectionIndex = getSectionIndex(sections, skipTo);
                    if (skipToSectionIndex <= startIndex)
                    {
                        qDebug() << "Cannot skip back from Element: " << elementUid;
                        skipToSectionIndex = -1;
                        skipTo.clear();
                    }
                }

                if (!skipTo.isEmpty())
                {
                    auto forkPath = *path;
                    forkPath.expression.append(expression);
                    traverse(sections, skipToSectionIndex, pathFound, &forkPath);
                    return;
                }

                // At least one item falls through.
                allHandled = false;
            });

            // If the field is required and all options are handled, then the current path is dead.
            if (allHandled && listField->required().toBool())
            {
                deadEnd = true;
                return;
            }
        });

        // All branches are already created - this path is done.
        if (deadEnd)
        {
            return;
        }

        // Path is complete: no next section or submit.
        if (section.nextSectionUid.isEmpty() || section.nextSectionUid == "SUBMIT")
        {
            pathFound(path);
            return;
        }

        // Get the next section index.
        auto nextSectionIndex = getSectionIndex(sections, section.nextSectionUid);
        if (nextSectionIndex <= startIndex)
        {
            qDebug() << "Cannot fall through back from Section: " << section.sectionUid;
            return;
        }

        // Continue to the next section.
        traverse(sections, nextSectionIndex, pathFound, path);
    }

private:
    FieldManager* m_fieldManager = nullptr;
    ElementManager* m_elementManager = nullptr;
};

void GoogleFormParser::postProcess(RecordField* recordField)
{
    recordField->enumFields([&](BaseField* field)
    {
        if (!field->getTagValue("section", false).toBool())
        {
            return;
        }

        auto section = qobject_cast<RecordField*>(field);
        if (section->fieldCount() == 0)
        {
            auto emptyUid = section->uid() + "_empty";

            auto element = new Element();
            element->set_uid(emptyUid);
            element->set_name("tr:Empty");
            m_elementManager.appendElement(nullptr, element);

            auto staticField = new StaticField();
            staticField->set_uid(emptyUid);
            staticField->set_nameElementUid(emptyUid);
            m_fieldManager.appendField(section, staticField);
        }
    });

    m_fieldManager.completeUpdate();
    m_elementManager.completeUpdate();
    PostProcessor pp(&m_fieldManager, &m_elementManager);
    pp.execute();
}
