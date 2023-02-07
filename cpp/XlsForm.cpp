#include "XlsForm.h"
#include "App.h"

constexpr char CT_PREFIX[] = "bind::ct:";
constexpr int CT_PREFIX_LENGTH = 9;

//=================================================================================================
namespace
{

class ParserException : public std::runtime_error
{
public:
    ParserException(const char *msg) : runtime_error(msg) {}
    ~ParserException() throw() {}
};

}

//=================================================================================================
// XlsFormParser.

XlsFormParser::XlsFormParser()
{
}

void XlsFormParser::configureProject(Project* project, const QVariantMap& settings)
{
    project->set_languages(settings.value("languages").toList());

    auto params = settings.value("parameters").toMap();

    if (params.contains("colors"))
    {
        project->set_colors(params.value("colors").toMap());
    }

    project->set_defaultImmersive(params.value("immersive", project->defaultImmersive()).toBool());
    project->set_defaultWizardMode(params.value("wizardMode", project->defaultWizardMode()).toBool());
    project->set_defaultSendLocationInterval(params.value("sendLocationInterval").toInt());
    project->set_esriLocationServiceUrl(params.value("esriLocationServiceUrl").toString());
    project->set_defaultSubmitInterval(params.value("submitInterval").toInt());
    project->set_offlineMapUrl(params.value("offlineMapUrl").toString());

    if (params.contains("subtitle"))
    {
        project->set_subtitle(params.value("subtitle").toString());
    }

    if (params.contains("icon"))
    {
        project->set_icon(params.value("icon").toString());
    }

    if (params.contains("iconDark"))
    {
        project->set_iconDark(params.value("iconDark").toString());
    }
}

void XlsFormParser::configureProject(const QString& projectUid, const QVariantMap& settings)
{
    App::instance()->projectManager()->modify(projectUid, [&](Project* project)
    {
        configureProject(project, settings);
    });
}

bool XlsFormParser::parseSettings(const QString& xlsxFilePath, QVariantMap* settingsOut)
{
    settingsOut->clear();

    xlnt::workbook wb;

    try
    {
        wb.load(xlsxFilePath.toStdString());
        *settingsOut = settingsRowFromSheet(wb.sheet_by_title("settings"));
    }
    catch (xlnt::exception e)
    {
        qDebug() << "Exception getting version: " << e.what();
    }

    return !settingsOut->isEmpty();
}

bool XlsFormParser::getFormVersion(const QString& xlsxFilePath, QString* versionOut)
{
    auto settings = QVariantMap();
    if (!parseSettings(xlsxFilePath, &settings))
    {
        return false;
    }

    *versionOut = settings.value("version").toString();

    return !versionOut->isEmpty();
}

bool XlsFormParser::parse(const QString& xlsxFilePath, const QString& targetFolder, const QString& projectUid, QString* errorStringOut)
{
    auto result = false;

    // Hack to detect missing xlsx for projects before xlsx support.
    if (!QFile::exists(xlsxFilePath))
    {
        qDebug() << "Xlsx file does not exist";
        return false;
    }

    XlsFormParser parser;

    try
    {
        result = parser.execute(xlsxFilePath, targetFolder, projectUid);
    }
    catch (const ParserException& e)
    {
        result = false;
        *errorStringOut = e.what();
        qDebug() << "XLSForm parse error: " << errorStringOut;
        QFile::remove(targetFolder + "/Elements.qml.old");
        QFile::rename(targetFolder + "/Elements.qml", targetFolder + "/Elements.qml.old");
        QFile::remove(targetFolder + "/Fields.qml");
        QFile::rename(targetFolder + "/Fields.qml", targetFolder + "/Fields.qml.old");
    }

    return result;
}

bool XlsFormParser::execute(const QString& xlsxFilePath, const QString& targetFolder, const QString& updateProjectUid)
{
    m_folderPath = QFileInfo(xlsxFilePath).path();

    xlnt::workbook wb;

    // Load the file.
    try
    {
        wb.load(xlsxFilePath.toStdString());
    }
    catch (xlnt::exception e)
    {
        qDebug() << "Exception loading workbook: " << e.what();
        return false;
    }

    // Process "survey" table.
    auto startRowIndex = 1;
    try
    {
        processSurvey(wb.sheet_by_title("survey"), &startRowIndex, m_fieldManager.rootField(), m_elementManager.rootElement());
    }
    catch (xlnt::exception e)
    {
        qDebug() << "Exception parsing survey: " << e.what();
        return false;
    }

    // Process "choices" table.
    try
    {
        processChoices(wb.sheet_by_title("choices"), 1, m_elementManager.rootElement());
    }
    catch (xlnt::key_not_found)
    {
        // Fall through, no choices sheet.
    }
    catch (xlnt::exception e)
    {
        qDebug() << "Exception parsing choices: " << e.what();
        return false;
    }

    // Process "settings" table.
    auto settingsMap = QVariantMap();
    try
    {
        settingsMap = settingsRowFromSheet(wb.sheet_by_title("settings"));
    }
    catch (xlnt::key_not_found)
    {
        // Fall through, no settings sheet.
    }
    catch (xlnt::exception e)
    {
        qDebug() << "Exception parsing settings: " << e.what();
        return false;
    }

    // Post process.
    postProcess(m_fieldManager.rootField());

    // Complete.
    settingsMap["languages"] = m_languages;
    settingsMap["requireUsername"] = m_requireUsername;

    auto rootField = m_fieldManager.rootField();
    auto params = rootField->parameters();
    params.insert(settingsMap.value("parameters").toMap());
    rootField->set_parameters(params);
    rootField->set_version(settingsMap.value("version").toString());

    auto targetFolderFinal = Utils::removeTrailingSlash(targetFolder);
    m_fieldManager.saveToQmlFile(targetFolderFinal + "/Fields.qml");
    m_elementManager.saveToQmlFile(targetFolderFinal + "/Elements.qml");
    Utils::writeJsonToFile(targetFolderFinal + "/Settings.json", Utils::variantMapToJson(settingsMap));

    if (!updateProjectUid.isEmpty())
    {
        XlsFormParser::configureProject(updateProjectUid, settingsMap);
    }

    return m_elementManager.rootElement()->elementCount() != 0 &&
           m_fieldManager.rootField()->fieldCount() != 0;
}

void XlsFormParser::parseStringToMap(const QString& header, const QString& value, QVariantMap* outLanguages, bool removeMarkdown)
{
    auto code = QString();
    auto name = QString();

    if (header.contains("::"))
    {
        name = code = header.right(header.length() - header.lastIndexOf(':') - 1);

        auto index1 = name.lastIndexOf('(');
        auto index2 = name.lastIndexOf(')');

        if (index1 != -1 && index1 < index2)
        {
            code = name.mid(index1 + 1, index2 - index1 - 1);
            name = name.left(index1).trimmed();
        }
    }
    else
    {
        code = name = QString("Default");
    }

    code = code.toLower();

    auto item = QVariantMap {{ "code", code }, { "name", name }};
    if (!m_languages.contains(item))
    {
        m_languages.append(item);
    }

    if (removeMarkdown && !value.isEmpty())
    {
        m_textDocument.setMarkdown(value, QTextDocument::MarkdownDialectCommonMark);
        auto cleanValue = m_textDocument.toPlainText();
        if (cleanValue.isEmpty())
        {
            qDebug() << "Bad formatting: " << value;
            cleanValue = value;
        }
        outLanguages->insert(code, cleanValue);
    }
    else
    {
        outLanguages->insert(code, value);
    }
}

QString XlsFormParser::findAsset(const QString& name) const
{
    if (!name.isEmpty())
    {
        auto searchPaths = QStringList { "", "assets/", "esriinfo/media/" };

        for (auto it = searchPaths.constBegin(); it != searchPaths.constEnd(); it++)
        {
            auto filePath = m_folderPath + "/" + *it + name;
            if (QFile::exists(filePath))
            {
                return *it + name;
            }
        }
    }

    return "";
}

QVariantMap XlsFormParser::parseCT(const QVariantMap& parameters, const QString& header, const QString& value)
{
    auto valueText = value.trimmed();
    if (valueText.isEmpty())
    {
        return QVariantMap();
    }

    // Convert "TRUE", "FALSE", "YES", "NO" to lowercase boolean values.
    auto fixups = QMap<QString, QString> { { "TRUE", "true" }, { "FALSE", "false" }, { "YES", "true" }, { "NO", "false" } };
    valueText = fixups.value(valueText.toUpper(), valueText);

    // Try a JSON type.
    auto valueJson = "{ \"v\": " + valueText + " }";
    auto v = QJsonDocument::fromJson(valueJson.toLatin1()).object().toVariantMap().value("v");
    if (!v.isValid() || v.isNull())
    {
        // object, array, bool, integer and double failed -> must be text.
        v = valueText;
    }

    auto attribute = header.right(header.length() - CT_PREFIX_LENGTH);

    // 'parameters' must be an object.
    if (attribute == "parameters")
    {
        return v.toMap();
    }

    // Check for full attribute value.
    auto parts = attribute.split('.');

    // attribute is full name.
    if (parts.length() == 1)
    {
        return QVariantMap { { attribute, v } };
    }

    // attribute is a.b format, e.g. content.style.
    if (parts.length() == 2)
    {
        auto mapName = parts.constFirst();
        auto mapValue = parts.constLast();

        auto result = parameters.value(mapName).toMap();
        result[mapValue] = v;
        return QVariantMap { { mapName, result } };
    }

    // Format not supported.
    qDebug() << "Invalid attribute name: " << attribute;
    return QVariantMap();
}


QVariantMap XlsFormParser::parseParameters(const QString& params) const
{
    auto result = QVariantMap();
    
    auto strings = params.split(' ');
    for (auto it = strings.constBegin(); it != strings.constEnd(); it++)
    {
        auto nv = (*it).split('=');
        auto name = nv.first();

        if (nv.count() == 1)
        {
            result.insert(name, true);
            continue;
        }

        result.insert(name, nv.last());
    }

    return result;
}

QVariantMap XlsFormParser::settingsRowFromSheet(const xlnt::worksheet& sheet)
{
    auto result = QVariantMap();
    auto rows = sheet.rows();

    if (rows.length() >= 2)
    {
        auto columnHeaders = sheet.rows().front();

        for (auto cell: rows[1])
        {
            auto columnIndex = cell.column_index() - 1;
            auto header = QString(columnHeaders[columnIndex].to_string().c_str()).trimmed();
            auto value = QString(cell.to_string().c_str()).trimmed();

            if (header.startsWith(CT_PREFIX))
            {
                auto parameters = result.value("parameters").toMap();
                parameters.insert(parseCT(parameters, header, value));
                result["parameters"] = parameters;
            }
            else
            {
                result[header] = value;
            }
        }
    }

    return result;
}

XlsFormParser::SurveyRow XlsFormParser::surveyRowFromSheet(const xlnt::worksheet& sheet, const xlnt::cell_vector& row)
{
    auto result = SurveyRow();
    auto columnHeaders = sheet.rows().front();

    for (auto cell: row)
    {
        auto header = QString(columnHeaders[cell.column_index() - 1].to_string().c_str()).trimmed();
        auto value = QString(cell.to_string().c_str()).trimmed();

        if (header == "type")
        {
            if (value.startsWith("select one from ", Qt::CaseInsensitive))
            {
                value.remove(0, QString("select one from ").length());
                value.insert(0, "select_one ");
            }
            else if (value.startsWith("select multiple from ", Qt::CaseInsensitive))
            {
                value.remove(0, QString("select multiple from ").length());
                value.insert(0, "select_multiple ");
            }
            else if (value == "begin repeat")
            {
                value = "begin_repeat";
            }
            else if (value == "end repeat")
            {
                value = "end_repeat";
            }
            else if (value == "begin group")
            {
                value = "begin_group";
            }
            else if (value == "end group")
            {
                value = "end_group";
            }

            result.type = value;
        }
        else if (header == "name")
        {
            result.name = value;
        }
        else if (header.startsWith("label"))
        {
            parseStringToMap(header, value, &result.label, true);
        }
        else if (header.startsWith("hint"))
        {
            parseStringToMap(header, value, &result.hint, false);
        }
        else if (header == "appearance")
        {
            result.appearance = value;
        }
        else if (header == "required")
        {
            result.required = value.toLower();
        }
        else if (header.startsWith("required_message"))
        {
            parseStringToMap(header, value, &result.requiredMessage, true);
        }
        else if (header == "read_only" || header == "readonly")
        {
            result.readonly = value;
        }
        else if (header == "default")
        {
            result.defaultValue = value == "null" ? QVariant() : value;
        }
        else if (header == "calculation")
        {
            result.calculation = value;
        }
        else if (header == "constraint")
        {
            result.constraint = value;
        }
        else if (header.startsWith("constraint_message"))
        {
            parseStringToMap(header, value, &result.constraintMessage, true);
        }
        else if (header == "relevant")
        {
            result.relevant = value;
        }
        else if (header == "choice_filter")
        {
            result.choiceFilter = value;
        }
        else if (header == "repeat_count")
        {
            result.repeatCount = value;
        }
        else if (header == "media::audio" || header == "audio")
        {
            result.mediaAudio = findAsset(value);
        }
        else if (header == "media::image" || header == "image")
        {
            result.mediaImage = findAsset(value);
        }
        else if (header == "bind::type")
        {
            result.bindType = value;
        }
        else if (header == "bind::esri:fieldType")
        {
            result.bindEsriFieldType = value;
        }
        else if (header == "bind::esri:fieldLength")
        {
            result.bindEsriFieldLength = value;
        }
        else if (header == "bind::esri:fieldAlias")
        {
            result.bindEsriFieldAlias = value;
        }
        else if (header == "bind::esri:parameters")
        {
            result.bindEsriParameters = value;
        }
        else if (header == "body::esri:style")
        {
            result.bodyEsriStyle = value;
        }
        else if (header == "body::accept")
        {
            result.bodyAccept = value;
        }
        else if (header == "parameters" || header == "ct_parameters")
        {
            auto values = parseParameters(value);
            if (!values.isEmpty())
            {
                result.parameters.insert(values);
            }
        }
        else if (header.startsWith(CT_PREFIX))
        {
            result.parameters.insert(parseCT(result.parameters, header, value));
        }
    }

    return result;
}

XlsFormParser::ChoiceRow XlsFormParser::choiceRowFromSheet(const xlnt::worksheet& sheet, const xlnt::cell_vector& row)
{
    auto result = ChoiceRow();
    auto columnHeaders = sheet.rows().front();

    for (auto cell: row)
    {
        auto header = QString(columnHeaders[cell.column_index() - 1].to_string().c_str()).trimmed();
        auto value = QString(cell.to_string().c_str()).trimmed();

        if (header == "list_name" || header == "list name")
        {
            result.listName = value;
        }
        else if (header == "name")
        {
            result.name = value;
        }
        else if (header.startsWith("label"))
        {
            parseStringToMap(header, value, &result.label, true);
        }
        else if (header == "media::image" || header == "image")
        {
            result.image = findAsset(value);
        }
        else
        {
            result.tags[header] = value;
        }
    }

    return result;
}

void XlsFormParser::processSurvey(const xlnt::worksheet& sheet, int* startRowIndex, RecordField* recordField, Element* rootElement)
{
    auto rows = sheet.rows();
    m_requireUsername = false;

    while (*startRowIndex < (int)rows.length())
    {
        SurveyRow surveyRow = surveyRowFromSheet(sheet, rows[*startRowIndex]);

        *startRowIndex = *startRowIndex + 1;

        if (surveyRow.type == "end_repeat" || surveyRow.type == "end_group")
        {
            return;
        }

        if (surveyRow.name.isEmpty())
        {
            continue;
        }

        // Survey123 generates fields like this for their UI.
        if (surveyRow.name.startsWith("generated_"))
        {
            continue;
        }

        auto fieldUid = surveyRow.name;

        BaseField* field = nullptr;
        auto fieldType = surveyRow.type.trimmed().toLower();

        if (fieldType == "begin_repeat")
        {
            auto f = new RecordField();
            f->set_uid(fieldUid);

            auto repeatCount = surveyRow.repeatCount;
            if (!repeatCount.isEmpty())
            {
                auto repeatCountIsInt = false;
                auto repeatCountAsInt = repeatCount.toInt(&repeatCountIsInt);

                if (repeatCountIsInt && repeatCountAsInt > 0)
                {
                    f->set_minCount(repeatCountAsInt);
                    f->set_maxCount(repeatCountAsInt);
                }
                else
                {
                    f->set_repeatCount(repeatCount);
                }
            }

            processSurvey(sheet, startRowIndex, f, rootElement);
            field = f;
        }
        else if (fieldType == "begin_group")
        {
            auto f = new RecordField();
            f->set_uid(fieldUid);
            f->set_minCount(1);
            f->set_maxCount(1);
            processSurvey(sheet, startRowIndex, f, rootElement);
            field = f;
        }
        else if (fieldType == "integer")
        {
            auto f = new NumberField();
            f->set_decimals(0);
            field = f;
        }
        else if (fieldType == "decimal")
        {
            auto f = new NumberField();
            f->set_decimals(4);
            field = f;
        }
        else if (fieldType == "range")
        {
            auto f = new NumberField();

            auto start = surveyRow.parameters.value("start", 0);
            surveyRow.parameters.remove("start");
            auto end = surveyRow.parameters.value("end", 0);
            surveyRow.parameters.remove("end");
            auto step = surveyRow.parameters.value("step", 1);
            surveyRow.parameters.remove("step");

            if (start.toString().contains(".") || end.toString().contains(".") || step.toString().contains("."))
            {
                f->set_decimals(4);
            }
            else
            {
                f->set_decimals(0);
                surveyRow.parameters["autoNext"] = surveyRow.appearance.contains("quick");
                surveyRow.appearance = "range";
            }

            f->set_minValue(start.toInt());
            f->set_maxValue(end.toInt());
            f->set_step(step.toInt());
            field = f;
        }
        else if (fieldType == "text")
        {
            auto f = new StringField();
            f->set_multiLine(surveyRow.appearance == "multiline");
            f->set_numbers(surveyRow.appearance == "numbers");
            field = f;
        }
        else if (fieldType.startsWith("select_one"))
        {
            auto f = new StringField();
            auto listSpec = surveyRow.type.split(' ');
            listSpec.removeAll("");
            auto listName = listSpec.count() >= 2 ? listSpec[1] : "";

            f->set_listElementUid(listName);
            f->set_listFilterByTag(surveyRow.choiceFilter);
            f->set_listOther(listSpec.count() == 3 && listSpec.last() == "or_other");

            if (!surveyRow.defaultValue.toString().isEmpty())
            {
                surveyRow.defaultValue = listName + '/' + surveyRow.defaultValue.toString();
            }

            surveyRow.parameters["autoNext"] = surveyRow.appearance.contains("quick");

            field = f;
        }
        else if (fieldType.startsWith("select_multiple"))
        {
            auto f = new CheckField();
            auto listSpec = surveyRow.type.split(' ');
            listSpec.removeAll("");
            auto listName = listSpec.count() >= 2 ? listSpec[1] : "";

            f->set_listElementUid(listName);
            f->set_listFilterByTag(surveyRow.choiceFilter);
            f->set_listOther(listSpec.count() == 3 && listSpec.last() == "or_other");

            // Remap the default value.
            if (!surveyRow.defaultValue.toString().isEmpty())
            {
                auto defaultValues = surveyRow.defaultValue.toString().split(' ');
                if (!defaultValues.isEmpty())
                {
                    auto defaultValueMap = QVariantMap();
                    for (auto it = defaultValues.constBegin(); it != defaultValues.constEnd(); it++)
                    {
                        defaultValueMap[*it] = true;
                    }
                    surveyRow.defaultValue = defaultValueMap;
                }
            }

            field = f;
        }
        else if (fieldType == "note")
        {
            auto f = new StaticField();
            field = f;
        }
        else if (fieldType == "geopoint")
        {
            auto f = new LocationField();
            f->set_allowManual(true);
            f->set_fixCount(surveyRow.parameters.value("fixCount", 4).toInt());
            field = f;
        }
        else if (fieldType == "geotrace")
        {
            auto f = new LineField();
            field = f;
        }
        else if (fieldType == "geoshape")
        {
            auto f = new AreaField();
            field = f;
        }
        else if (fieldType == "date")
        {
            auto f = new DateField();
            field = f;
        }
        else if (fieldType == "time")
        {
            auto f = new TimeField();
            field = f;
        }
        else if (fieldType == "datetime")
        {
            auto f = new DateTimeField();
            field = f;
        }
        else if (fieldType == "photo" && surveyRow.appearance == "signature")
        {
            auto f = new SketchField();
            field = f;
        }
        else if (fieldType == "image")
        {
            if (surveyRow.appearance.contains("signature"))
            {
                field = new SketchField();
            }
            else
            {
                auto f = new PhotoField();
                f->set_maxCount(1);

                if (surveyRow.parameters.contains("max-pixels"))
                {
                    auto maxPixels = surveyRow.parameters["max-pixels"].toInt();
                    f->set_resolutionX(maxPixels);
                    f->set_resolutionY(maxPixels);
                }
                field = f;
            }
        }
        else if (fieldType == "audio")
        {
            auto f = new AudioField();
            field = f;
        }
        else if (fieldType == "file")
        {
            auto f = new FileField();

            auto accept = surveyRow.bodyAccept.trimmed().toLower().remove(' ');
            auto filters = accept.split(',');
            f->set_accept(filters.join(' '));
            f->set_format(surveyRow.parameters["format"].toString().trimmed().toLower());
            field = f;
        }
        else if (fieldType == "barcode")
        {
            auto f = new StringField();
            f->set_barcode(true);
            field = f;
        }
        else if (fieldType == "calculate")
        {
            auto f = new CalculateField();
            f->set_hidden(true);
            surveyRow.readonly = surveyRow.label.isEmpty() && surveyRow.label.isEmpty() ? "true" : "false";

            field = f;
        }
        else if (fieldType == "acknowledge")
        {
            auto f = new AcknowledgeField();
            field = f;
        }
        else if (fieldType == "hidden")
        {
            field = new StringField();
            field->set_hidden(true);
        }
        else if (fieldType == "today")
        {
            auto f = new CalculateField();
            surveyRow.calculation = "today()";
            field = f;
        }
        else if (fieldType == "deviceid")
        {
            auto f = new StringField();
            surveyRow.defaultValue = "concat('ct:', deviceId)";
            field = f;
        }
        else if (fieldType == "phonenumber")
        {
            auto f = new StringField();
            surveyRow.defaultValue = "unknown";
            field = f;
        }
        else if (fieldType == "username" || fieldType == "email")
        {
            auto f = new CalculateField();
            surveyRow.calculation = "username";
            field = f;
            m_requireUsername = true;
        }
        else if (fieldType == "subscriberid")
        {
            auto f = new StringField();
            surveyRow.defaultValue = "unknown";
            field = f;
        }
        else if (fieldType == "imei")
        {
            auto f = new StringField();
            auto deviceImei = App::instance()->deviceImei();
            surveyRow.defaultValue = deviceImei.isEmpty() ? App::instance()->settings()->deviceId() : deviceImei;
            field = f;
        }
        else if (fieldType == "simserial")
        {
            auto f = new StringField();
            surveyRow.defaultValue = "unknown";
            field = f;
        }
        else if (fieldType == "start")
        {
            field =  new CalculateField();
            surveyRow.calculation = "createTime";
        }
        else if (fieldType == "end")
        {
            field = new CalculateField();
            surveyRow.calculation = "updateTime";
        }
        else
        {
            //recordField->addTag(type, uid);
            continue;
        }

        // Uniquify the fieldUid.
        auto uniqueFieldUid = fieldUid;
        if (!field->isRecordField() && !recordField->uid().isEmpty())
        {
            uniqueFieldUid = recordField->uid() + '/' + fieldUid;
        }

        // Create Elements.
        Element* nameElement = nullptr;
        if (!surveyRow.label.isEmpty())
        {
            nameElement = new Element();
            nameElement->set_uid(uniqueFieldUid);
            nameElement->set_names(QJsonObject::fromVariantMap(surveyRow.label));
            nameElement->set_icon(surveyRow.mediaImage);
            nameElement->set_audio(surveyRow.mediaAudio);
            rootElement->appendElement(nameElement);
            m_fieldElements.insert(nameElement->uid(), nameElement);
        }

        Element* hintElement = nullptr;
        if (!surveyRow.hint.isEmpty())
        {
            hintElement = new Element();
            hintElement->set_uid(uniqueFieldUid + "/hint");
            hintElement->set_names(QJsonObject::fromVariantMap(surveyRow.hint));
            rootElement->appendElement(hintElement);
            m_fieldElements.insert(hintElement->uid(), hintElement);
        }

        Element* constraintElement = nullptr;
        if (!surveyRow.constraint.isEmpty() && !surveyRow.constraintMessage.isEmpty())
        {
            constraintElement = new Element();
            constraintElement->set_uid(uniqueFieldUid + "/constraint");
            constraintElement->set_names(QJsonObject::fromVariantMap(surveyRow.constraintMessage));
            nameElement->appendElement(constraintElement);
            m_fieldElements.insert(constraintElement->uid(), constraintElement);
        }

        field->set_uid(uniqueFieldUid);
        field->set_nameElementUid(nameElement ? nameElement->uid() : "");
        field->set_exportUid(fieldUid != surveyRow.name ? surveyRow.name : "");
        field->set_defaultValue(surveyRow.defaultValue);
        field->set_relevant(surveyRow.relevant);
        field->set_readonly(surveyRow.readonly == "true" || surveyRow.readonly == "yes");
        field->set_required(surveyRow.required == "true" || surveyRow.required == "yes");
        field->set_constraint(surveyRow.constraint);
        field->set_constraintElementUid(constraintElement ? constraintElement->uid() : "");
        field->set_hintElementUid(hintElement ? hintElement->uid() : "");
        field->set_formula(surveyRow.calculation);
        field->set_hidden(field->hidden() || field->nameElementUid().isEmpty() || surveyRow.parameters.value("hidden").toBool() || surveyRow.appearance.contains("hidden"));
        field->set_parameters(surveyRow.parameters);

        if (!surveyRow.appearance.isEmpty())
        {
            field->addParameter("appearance", surveyRow.appearance);
        }

        recordField->appendField(field);

        if (field->listOther())
        {
            auto otherField = new StringField();
            otherField->set_uid(field->uid() + "_other");
            otherField->set_nameElementUid(field->nameElementUid());
            otherField->set_relevant("selected(${" + surveyRow.name + "}, '__other__')");
            otherField->set_required(true);
            recordField->appendField(otherField);
        }
    }
}

void XlsFormParser::processChoices(const xlnt::worksheet& sheet, int startRowIndex, Element* rootElement)
{
    auto rows = sheet.rows();

    m_choices.clear();

    QString lastListName;
    Element* lastElement = nullptr;

    auto addOther = [&]()
    {
        if (lastElement)
        {
            auto element = new Element();
            element->set_uid(lastListName + "/__other__");
            element->set_exportUid("other");
            element->set_name("tr:Other");
            element->set_other(true);
            lastElement->appendElement(element);
        }
    };

    auto choiceRows = QList<ChoiceRow>();
    auto choiceOrder = QStringList();
    for (auto i = startRowIndex; i < (int)(sheet.rows().length()); i++)
    {
        auto choiceRow = choiceRowFromSheet(sheet, rows[i]);
        if (!choiceRow.listName.isEmpty() && !choiceRow.name.isEmpty())
        {
            choiceRows.append(choiceRow);
            choiceOrder.append(choiceRow.name);
        }
    }

    std::sort(choiceRows.begin(), choiceRows.end(), [&](const ChoiceRow& r1, const ChoiceRow& r2) -> bool
    {
        if (r1.listName != r2.listName)
        {
            return r1.listName < r2.listName;
        }
        else
        {
            return choiceOrder.indexOf(r1.name) < choiceOrder.indexOf(r2.name);
        }
    });

    for (auto choiceRowIt = choiceRows.constBegin(); choiceRowIt != choiceRows.constEnd(); choiceRowIt++)
    {
        auto choiceRow = *choiceRowIt;
        if (choiceRow.listName != lastListName || !lastElement)
        {
            addOther();

            if (m_fieldElements.contains(choiceRow.listName))
            {
                lastElement = m_fieldElements[choiceRow.listName];
            }
            else
            {
                lastElement = new Element();
                lastElement->set_uid(choiceRow.listName);
                rootElement->appendElement(lastElement);
            }

            lastListName = choiceRow.listName;
        }

        auto elementUid = lastListName + '/' + choiceRow.name;
        if (!m_choices.contains(elementUid))
        {
            m_choices.insert(elementUid, true);

            auto element = new Element();
            element->set_uid(lastListName + '/' + choiceRow.name);
            element->set_exportUid(element->uid() != choiceRow.name ? choiceRow.name : "");
            element->set_names(QJsonObject::fromVariantMap(choiceRow.label));
            element->set_icon(choiceRow.image);
            element->set_tag(choiceRow.tags);
            lastElement->appendElement(element);
        }
    }

    addOther();
}

void XlsFormParser::postProcess(RecordField* recordField)
{
    auto i = 0;
    while (i < recordField->fieldCount())
    {
        auto field = recordField->field(i);
        auto fieldUid = field->uid();

        // Ensure default value is valid.
        if (qobject_cast<StringField*>(field) && !field->listElementUid().isEmpty() && !field->defaultValue().toString().isEmpty())
        {
            auto defaultValue = field->defaultValue().toString();
            if (m_elementManager.getElement(defaultValue) == nullptr)
            {
                field->set_defaultValue(QVariant());
            }
        }

        // Matrix.
        auto currRecordField = qobject_cast<RecordField*>(field);
        if (currRecordField && currRecordField->group() && currRecordField->fieldCount() > 1 &&
            fieldUid.endsWith("_header") && field->parameters().value("appearance").toString().startsWith("w"))
        {
            // Set the matrix name to the first column.
            currRecordField->set_nameElementUid(currRecordField->field(0)->nameElementUid());

            // Unhide, because now it has a name.
            currRecordField->set_hidden(false);

            // Get the column names.
            auto columnNameElementUids = QStringList();
            for (auto j = 0; j < currRecordField->fieldCount(); j++)
            {
                auto columnField = qobject_cast<StaticField*>(currRecordField->field(j));
                qFatalIf(!columnField, "Bad column field type, expected StaticField");
                columnNameElementUids.append(columnField->nameElementUid());
            }

            // Remove all fields in the matrix record, because these are just the column headers.
            currRecordField->clearFields();

            // Find matrix rows (which are themselves RecordFields) and add them to the matrix.
            auto uidStart = fieldUid.left(fieldUid.indexOf("_header"));
            auto rowRecordFields = QList<RecordField*>();
            for (auto j = i + 1; i < recordField->fieldCount(); j++)
            {
                auto nextRecordField = qobject_cast<RecordField*>(recordField->field(j));
                if (!nextRecordField || !nextRecordField->group() || !nextRecordField->uid().startsWith(uidStart) ||
                    nextRecordField->fieldCount() != columnNameElementUids.count())
                {
                    // Not part of this matrix.
                    break;
                }

                rowRecordFields.append(nextRecordField);
            }

            for (auto it = rowRecordFields.constBegin(); it != rowRecordFields.constEnd(); it++)
            {
                auto rowRecordField = *it;

                // Use the name from the first field.
                rowRecordField->set_nameElementUid(rowRecordField->field(0)->nameElementUid());

                // Unhide, because now it has a name.
                rowRecordField->set_hidden(false);

                // Update the names.
                for (auto j = 0; j < rowRecordField->fieldCount(); j++)
                {
                    rowRecordField->field(j)->set_nameElementUid(columnNameElementUids[j]);
                }

                // Remove the first field, which is the header.
                rowRecordField->removeFirstField();

                // Move under the matrix RecordField.
                recordField->detachField(rowRecordField);
                currRecordField->appendField(rowRecordField);
            }

            // Since we artifically moved all the records under this record, we need to flatten it during export.
            currRecordField->addTag("flattenXml", true);
        }
        else if (currRecordField && currRecordField->group() && currRecordField->wizardFieldList() &&
            currRecordField->fieldCount() > 1 &&
            !currRecordField->field(0)->listElementUid().isEmpty() &&
            currRecordField->field(0)->parameters().value("appearance").toString() == "label")
        {
            // The first row is just a label row for a simple matrix.
            currRecordField->removeFirstField();
        }
        else if (currRecordField) // Just a regular record field: recurse into it.
        {
            postProcess(currRecordField);
        }

        i++;
    }
}
