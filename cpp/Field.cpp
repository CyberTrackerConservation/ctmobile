#include "pch.h"
#include "Field.h"
#include "App.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BaseField

BaseField::BaseField(QObject* parent): QObject(parent)
{
    m_readonly = false;
    m_hidden = false;
    m_listFlatten = false;
    m_listSorted = false;
    m_listOther = false;
}

BaseField::~BaseField()
{
}

BaseField* BaseField::parentField() const
{
    return qobject_cast<BaseField*>(parent());
}

bool BaseField::isRecordField() const
{
    return false;
}

void BaseField::appendAttachments(const QVariant& /*value*/, QStringList* /*attachmentsOut*/) const
{
}

QString BaseField::toXml(const QVariant& value) const
{
    return value.toString();
}

QVariant BaseField::fromXml(const QVariant& value) const
{
    return value;
}

QVariant BaseField::save(const QVariant& value) const
{
    return value;
}

QVariant BaseField::load(const QVariant& value) const
{
    return value;
}

void BaseField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "uid", m_uid);
    Utils::writeQml(stream, depth, "exportUid", m_exportUid);
    Utils::writeQml(stream, depth, "nameElementUid", m_nameElementUid);
    Utils::writeQml(stream, depth, "hintElementUid", m_hintElementUid);
    Utils::writeQml(stream, depth, "hintLink", m_hintLink);
    Utils::writeQml(stream, depth, "sectionElementUid", m_sectionElementUid);
    Utils::writeQml(stream, depth, "listElementUid", m_listElementUid);
    Utils::writeQml(stream, depth, "listFilterByField", m_listFilterByField);
    Utils::writeQml(stream, depth, "listFilterByTag", m_listFilterByTag);
    Utils::writeQml(stream, depth, "listFlatten", m_listFlatten, false);
    Utils::writeQml(stream, depth, "listSorted", m_listSorted, false);
    Utils::writeQml(stream, depth, "listOther", m_listOther, false);
    Utils::writeQml(stream, depth, "tag", m_tag);
    Utils::writeQml(stream, depth, "required", m_required);
    Utils::writeQml(stream, depth, "hidden", m_hidden, false);
    Utils::writeQml(stream, depth, "readonly", m_readonly, false);
    Utils::writeQml(stream, depth, "relevant", m_relevant);
    Utils::writeQml(stream, depth, "constraint", m_constraint);
    Utils::writeQml(stream, depth, "constraintElementUid", m_constraintElementUid);
    Utils::writeQml(stream, depth, "defaultValue", m_defaultValue);
    Utils::writeQml(stream, depth, "formula", m_formula);
    Utils::writeQml(stream, depth, "parameters", m_parameters);
}

QString BaseField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    return value.toString();
}

bool BaseField::isValid(const QVariant& value, bool required) const
{
    // Null value that is not required gets a pass.
    if (!value.isValid() && !required)
    {
        return true;
    }

    // Required values must be valid.
    if (required)
    {
        if (value.toString().isEmpty() && value.toMap().empty() && value.toList().empty())
        {
            return false;
        }
    }

    // Value is valid.
    return true;
}

void BaseField::addTag(const QString& key, const QVariant& value)
{
    m_tag.insert(key, value);
}

void BaseField::removeTag(const QString& key)
{
    m_tag.remove(key);
}

QVariant BaseField::getTagValue(const QString& key, const QVariant& defaultValue) const
{
    return m_tag.value(key, defaultValue);
}

QVariant BaseField::getParameter(const QString& key, const QVariant& defaultValue) const
{
    return m_parameters.value(key, defaultValue);
}

void BaseField::addParameter(const QString& key, const QVariant& value)
{
    m_parameters[key] = value;
}

QString BaseField::safeExportUid() const
{
    return !m_exportUid.isEmpty() ? m_exportUid : Utils::lastPathComponent(m_uid);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RecordField

RecordField::RecordField(QObject* parent): BaseField(parent)
{
    m_matrixCount = m_minCount = m_maxCount = 0;
}

RecordField::~RecordField()
{
    qDeleteAll(m_fields);
}

bool RecordField::isRecordField() const
{
    return true;
}

const RecordField* RecordField::fromField(const BaseField* field)
{
    return qobject_cast<const RecordField*>(field);
}

QQmlListProperty<BaseField> RecordField::fields()
{
    return QQmlListProperty<BaseField>(this, &m_fields);
}

int RecordField::fieldCount() const
{
    return m_fields.count();
}

BaseField* RecordField::field(int index) const
{
    return m_fields.at(index);
}

void RecordField::enumFields(const std::function<void(BaseField* field)>& callback) const
{
    for (auto it = m_fields.constBegin(); it != m_fields.constEnd(); it++)
    {
        callback(*it);
    }
}

QVariant RecordField::save(const QVariant& value) const
{
    return value.toList();
}

QVariant RecordField::load(const QVariant& value) const
{
    return value.toList();
}

void RecordField::appendField(BaseField* field)
{
    m_fields.append(field);
}

void RecordField::insertField(int index, BaseField* field)
{
    m_fields.insert(index, field);
}

void RecordField::detachField(BaseField* field)
{
    m_fields.removeOne(field);
}

void RecordField::removeFirstField()
{
    delete m_fields.first();
    m_fields.removeFirst();
}

void RecordField::removeLastField()
{
    delete m_fields.last();
    m_fields.removeLast();
}

void RecordField::clearFields()
{
    qDeleteAll(m_fields);
    m_fields.clear();
}

bool RecordField::group() const
{
    return m_minCount == 1 && m_maxCount == 1;
}

bool RecordField::dynamic() const
{
    if (m_minCount != m_maxCount && m_maxCount > 0)
    {
        return true;
    }

    return m_masterFieldUid.isEmpty() && m_matrixCount == 0 && m_repeatCount.isEmpty();
}

bool RecordField::wizardFieldList() const
{
    return m_parameters.contains("field-list") || (m_parameters.value("appearance") == "field-list");
}

int RecordField::fieldIndex(BaseField* field) const
{
    return m_fields.indexOf(field);
}

void RecordField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "RecordField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth + 1, "masterFieldUid", m_masterFieldUid);
    Utils::writeQml(stream, depth + 1, "matrixCount", m_matrixCount, 0);
    Utils::writeQml(stream, depth + 1, "minCount", m_minCount, 0);
    Utils::writeQml(stream, depth + 1, "maxCount", m_maxCount, 0);
    Utils::writeQml(stream, depth + 1, "version", m_version);
    Utils::writeQml(stream, depth + 1, "repeatCount", m_repeatCount);

    for (auto field: m_fields)
    {
        field->toQml(stream, depth + 1);
    }

    Utils::writeQml(stream, depth, "}");
}

QString RecordField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    if (group()) // Nothing for groups.
    {
        return QString();
    }

    auto recordCount = value.toList().count();
    if (recordCount == 0)
    {
        return QString();
    }

    return QString("%1 %2").arg(QString::number(recordCount), recordCount == 1 ? tr("record") : tr("records"));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StaticField

StaticField::StaticField(QObject* parent): BaseField(parent)
{
}

void StaticField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "StaticField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth, "}");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StringField

StringField::StringField(QObject* parent): BaseField(parent)
{
    m_maxLength = 0;
    m_multiLine = false;
    m_barcode = false;
    m_numbers = false;
}

QVariant StringField::save(const QVariant& value) const
{
    return value.toString();
}

QVariant StringField::load(const QVariant& value) const
{
    return value.toString();
}

void StringField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "StringField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth + 1, "inputMask", m_inputMask);
    Utils::writeQml(stream, depth + 1, "multiLine", m_multiLine, false);
    if (maxLength() != 0)
    {
        Utils::writeQml(stream, depth + 1, "maxLength", m_maxLength);
    }
    Utils::writeQml(stream, depth + 1, "barcode", m_barcode, false);
    Utils::writeQml(stream, depth + 1, "numbers", m_numbers, false);
    Utils::writeQml(stream, depth, "}");
}

QString StringField::toDisplayValue(ElementManager* elementManager, const QVariant &value) const
{
    auto valueString = value.toString();

    if (m_listElementUid.isEmpty() || valueString.isEmpty())
    {
        return valueString;
    }

    return elementManager->getElementName(value.toString());
}

QString StringField::toXml(const QVariant& value) const
{
    if (listElementUid().isEmpty())
    {
        return value.toString();
    }
    else
    {
        // BUGBUG: should we use exportUid here?

        // Elements need to have unique ids, but ODK list items may duplicate.
        // We get around this by uniquifying the Element uid, but this means that
        // it cannot be used in XPath expressions. This returns just the part of the
        // uid that is needed for matching.
        return value.toString().split('/').last();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NumberField

NumberField::NumberField(QObject* parent): BaseField(parent)
{
    m_decimals = -1;
    m_minValue = m_maxValue = m_step = std::numeric_limits<double>::quiet_NaN();
}

QVariant NumberField::save(const QVariant& value) const
{
    // Number.
    if (listElementUid().isEmpty())
    {
        return value.toDouble();
    }

    // Map of numbers.
    return value.toMap();
}

QVariant NumberField::load(const QVariant& value) const
{
    // Number.
    if (listElementUid().isEmpty())
    {
        return value.toDouble();
    }

    // List of numbers.
    return value.toMap();
}

void NumberField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "NumberField {");
    BaseField::toQml(stream, depth + 1);

    // If the number of decimals was not set, then pick it up from the default value.
    auto decimals = m_decimals;
    if (decimals == -1)
    {
        auto splitList = defaultValue().toString().split(".");
        decimals = splitList.count() == 2 ? splitList[1].length() : -1;
    }

    if (decimals != -1)
    {
        Utils::writeQml(stream, depth + 1, "decimals", decimals);
    }

    if (!std::isnan(m_minValue))
    {
        Utils::writeQml(stream, depth + 1, "minValue", m_minValue);
    }

    if (!std::isnan(m_maxValue))
    {
        Utils::writeQml(stream, depth + 1, "maxValue", m_maxValue);
    }

    if (!std::isnan(m_step))
    {
        Utils::writeQml(stream, depth + 1, "step", m_step);
    }

    Utils::writeQml(stream, depth + 1, "inputMask", m_inputMask);
    Utils::writeQml(stream, depth, "}");
}

QString NumberField::toDisplayValue(ElementManager* elementManager, const QVariant& value) const
{
    if (!m_listElementUid.isEmpty())
    {
        auto valueMap = value.toMap();
        if (valueMap.isEmpty())
        {
            return QString();
        }

        auto listItems = QStringList();

        elementManager->enumChildren(m_listElementUid, true, [&](Element* element)
        {
            auto elementUid = element->uid();
            if (valueMap.contains(elementUid))
            {
                auto n = elementManager->getElementName(elementUid);
                auto v = valueMap.value(elementUid).toString();
                listItems.append(n + " (" + v + ")");
            }
        });

        return listItems.join(", ");
    }

    return value.toString();
}

bool NumberField::isValid(const QVariant& value, bool required) const
{
    // Number.
    if (listElementUid().isEmpty())
    {
        if (!required && !value.isValid())
        {
            return BaseField::isValid(value, required);
        }

        return checkSingleValue(value) && BaseField::isValid(value, required);
    }

    // List of numbers.
    auto valueMap = value.toMap();
    for (auto it = valueMap.constKeyValueBegin(); it != valueMap.constKeyValueEnd(); it++)
    {
        if (!checkSingleValue(it->second))
        {
            return false;
        }
    }

    return true;
}

// For number lists so the UI can validate an individual number.
bool NumberField::checkSingleValue(const QVariant& value) const
{
    auto minValue = std::isnan(m_minValue) ? -DBL_MAX : m_minValue;
    auto maxValue = std::isnan(m_maxValue) ? DBL_MAX : m_maxValue;

    auto d = value.toDouble();
    return d >= minValue && d <= maxValue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CheckField

CheckField::CheckField(QObject* parent): BaseField(parent)
{
}

QString CheckField::toXml(const QVariant& value) const
{
    if (listElementUid().isEmpty())
    {
        return value.toBool() ? "true()" : "false()";
    }
    else
    {
        auto array = QStringList();
        auto valueMap = value.toMap();
        for (auto it = valueMap.constKeyValueBegin(); it != valueMap.constKeyValueEnd(); it++)
        {
            if (it->second.toBool())
            {
                // Elements need to have unique ids, but ODK list items may duplicate.
                // We get around this by uniquifying the Element uid, but this means that
                // it cannot be used in XPath expressions. This returns just the part of the
                // uid that is needed for matching.
                array.append(it->first.split('/').last());
            }
        }

        return array.join(" ");
    }
}

QVariant CheckField::save(const QVariant& value) const
{
    // Boolean.
    if (listElementUid().isEmpty())
    {
        return value.toBool();
    }

    // List of booleans.
    auto result = QStringList();
    auto valueMap = value.toMap();
    for (auto it = valueMap.constKeyValueBegin(); it != valueMap.constKeyValueEnd(); it++)
    {
        if (it->second.toBool())
        {
            result.append(it->first);
        }
    }

    return result;
}

QVariant CheckField::load(const QVariant& value) const
{
    // Boolean.
    if (listElementUid().isEmpty())
    {
        return value.toBool();
    }

    // List of booleans.
    auto array = value.toStringList();
    auto result = QVariantMap();
    for (auto it = array.constBegin(); it != array.constEnd(); it++)
    {
        result.insert(*it, true);
    }

    return result;
}

void CheckField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "CheckField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth, "}");
}

QString CheckField::toDisplayValue(ElementManager* elementManager, const QVariant& value) const
{
    if (!m_listElementUid.isEmpty())
    {
        auto valueMap = value.toMap();
        if (valueMap.isEmpty())
        {
            return QString();
        }

        auto listItems = QStringList();

        elementManager->enumChildren(m_listElementUid, true, [&](Element* element)
        {
            auto elementUid = element->uid();
            if (valueMap.contains(elementUid))
            {
                listItems.append(elementManager->getElementName(elementUid));
            }
        });

        if (m_listSorted)
        {
            listItems.sort(Qt::CaseInsensitive);
        }

        return listItems.join(", ");
    }

    return value.toBool() ? tr("Yes") : tr("No");
}

bool CheckField::isValid(const QVariant& value, bool required) const
{
    if (listElementUid().isEmpty())
    {
        return true;
    }

    return !required || value.toMap().count() > 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AcknowledgeField

AcknowledgeField::AcknowledgeField(QObject* parent): BaseField(parent)
{
}

QString AcknowledgeField::toXml(const QVariant& value) const
{
    return value.toBool() ? "OK" : "";
}

QVariant AcknowledgeField::fromXml(const QVariant& value) const
{
    return value.toString() == "OK";
}

QVariant AcknowledgeField::save(const QVariant& value) const
{
    return value.toBool();
}

QVariant AcknowledgeField::load(const QVariant& value) const
{
    return value.toBool();
}

void AcknowledgeField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "AcknowledgeField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth, "}");
}

QString AcknowledgeField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    return value.toBool() ? tr("OK") : QString();
}

bool AcknowledgeField::isValid(const QVariant& value, bool required) const
{
    return !required || value.toBool();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LocationField

LocationField::LocationField(QObject* parent): BaseField(parent)
{
    m_fixCount = 0;
    m_allowManual = true;
}

QString LocationField::toXml(const QVariant& value) const
{
    auto valueMap = value.toMap();

    auto y = valueMap.value("y", 0).toDouble();
    auto x = valueMap.value("x", 0).toDouble();
    auto z = valueMap.value("z", 0).toDouble();
    auto a = valueMap.value("a", 0).toDouble();

    return QString("%1 %2 %3 %4").arg(QString::number(y), QString::number(x), QString::number(z), QString::number(std::isnan(a) ? 0 : a));
}

QVariant LocationField::fromXml(const QVariant& value) const
{
    auto values = value.toString().split(' ');
    qFatalIf(values.count() != 4, "Bad GPS string");
    return QVariantMap {
        { "y", values[0] }, { "x", values[1] }, { "z", values[2] }, { "a", values[3] },
        { "spatialReference", QVariantMap {{ "wkid", 4326 }} }
    };
}

QVariant LocationField::save(const QVariant& value) const
{
    return value.toMap();
}

QVariant LocationField::load(const QVariant& value) const
{
    return value.toMap();
}

void LocationField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "LocationField {");
    BaseField::toQml(stream, depth + 1);
    if (m_fixCount > 0)
    {
        Utils::writeQml(stream, depth + 1, "fixCount", m_fixCount);
        Utils::writeQml(stream, depth + 1, "allowManual", m_allowManual, true);
    }
    Utils::writeQml(stream, depth, "}");
}

QString LocationField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    auto pointMap = value.toMap();
    return pointMap.isEmpty() ? "" : App::instance()->getLocationText(pointMap["x"].toDouble(), pointMap["y"].toDouble());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LineField

LineField::LineField(QObject* parent): BaseField(parent)
{
    m_allowManual = true;
}

QString LineField::toXml(const QVariant& value) const
{
    return Utils::pointsToXml(value.toList());
}

QVariant LineField::fromXml(const QVariant& value) const
{
    return Utils::pointsFromXml(value.toString());
}

QVariant LineField::save(const QVariant& value) const
{
    return value.toList();
}

QVariant LineField::load(const QVariant& value) const
{
    return value.toList();
}

void LineField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "LineField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth + 1, "allowManual", m_allowManual, true);
    Utils::writeQml(stream, depth, "}");
}

QString LineField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    auto points = value.toList();
    return points.isEmpty() ? "" : QString("%1 %2, %3").arg(QString::number(points.count()), tr("points"), App::instance()->getDistanceText(Utils::distanceInMeters(points)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AreaField

AreaField::AreaField(QObject* parent): BaseField(parent)
{
    m_allowManual = true;
}

QString AreaField::toXml(const QVariant& value) const
{
    // Close the polygon if the first and last points are not equal.
    auto points = value.toList();
    if (points.count() > 1)
    {
        auto p1 = points.first().toList();
        auto p2 = points.last().toList();
        if (!qFuzzyCompare(p1[0].toFloat(), p2[0].toFloat()) || !qFuzzyCompare(p1[1].toFloat(), p2[1].toFloat()))
        {
            points.append(QVariant(p1));
        }
    }

    return Utils::pointsToXml(points);
}

QVariant AreaField::fromXml(const QVariant& value) const
{
    // Remove the last point if it is the same as the first point.
    auto points = Utils::pointsFromXml(value.toString());
    if (points.count() > 1)
    {
        auto p1 = points.first().toList();
        auto p2 = points.last().toList();
        if (qFuzzyCompare(p1[0].toFloat(), p2[0].toFloat()) && qFuzzyCompare(p1[1].toFloat(), p2[1].toFloat()))
        {
            points.removeLast();
        }
    }

    return points;
}

QVariant AreaField::save(const QVariant& value) const
{
    return value.toList();
}

QVariant AreaField::load(const QVariant& value) const
{
    return value.toList();
}

void AreaField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "AreaField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth + 1, "allowManual", m_allowManual, true);
    Utils::writeQml(stream, depth, "}");
}

QString AreaField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    auto points = value.toList();
    return points.isEmpty() ? "" : QString("%1 %2, %3").arg(QString::number(points.count()), tr("points"), App::instance()->getAreaText(Utils::areaInSquareMeters(points)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DateField

DateField::DateField(QObject* parent): BaseField(parent)
{
}

QString DateField::toXml(const QVariant& value) const
{
    return Utils::decodeTimestamp(value.toString()).date().toString(Qt::DateFormat::ISODate);
}

QVariant DateField::fromXml(const QVariant& value) const
{
    auto dateTime = QDateTime();
    dateTime.setDate(QDate::fromString(value.toString(), Qt::DateFormat::ISODate));
    return Utils::encodeTimestamp(dateTime);
}

QVariant DateField::save(const QVariant& value) const
{
    return toXml(value);
}

QVariant DateField::load(const QVariant& value) const
{
    return fromXml(value.toString());
}

void DateField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "DateField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth + 1, "minDate", m_minDate);
    Utils::writeQml(stream, depth + 1, "maxDate", m_maxDate);
    Utils::writeQml(stream, depth, "}");
}

bool DateField::isValid(const QVariant& value, bool required) const
{
    if (!BaseField::isValid(value, required))
    {
        return false;
    }

    if (value.toString().isEmpty())
    {
        return required ? false : true;
    }

    auto date = QDate::fromString(value.toString(), Qt::DateFormat::ISODate);

    if (!m_minDate.isEmpty())
    {
        auto minDate = QDate::fromString(m_minDate, Qt::DateFormat::ISODate);
        if (date < minDate)
        {
            return false;
        }
    }

    if (!m_maxDate.isEmpty())
    {
        auto maxDate = QDate::fromString(m_maxDate, Qt::DateFormat::ISODate);
        if (date > maxDate)
        {
            return false;
        }
    }

    return date.isValid();
}

QString DateField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    return App::instance()->formatDate(QDateTime::fromString(value.toString(), Qt::ISODate).date());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DateTimeField

DateTimeField::DateTimeField(QObject* parent): BaseField(parent)
{
}

QVariant DateTimeField::save(const QVariant& value) const
{
    return value.toString();
}

QVariant DateTimeField::load(const QVariant& value) const
{
    return value.toString();
}

void DateTimeField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "DateTimeField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth + 1, "minDate", m_minDate);
    Utils::writeQml(stream, depth + 1, "maxDate", m_maxDate);
    Utils::writeQml(stream, depth, "}");
}

QString DateTimeField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    return App::instance()->formatDateTime(Utils::decodeTimestamp(value.toString()));
}

bool DateTimeField::isValid(const QVariant& value, bool required) const
{
    if (!BaseField::isValid(value, required))
    {
        return false;
    }

    if (value.toString().isEmpty())
    {
        return required ? false : true;
    }

    auto date = Utils::decodeTimestamp(value.toString()).date();

    if (!m_minDate.isEmpty())
    {
        auto minDate = Utils::decodeTimestamp(m_minDate).date();
        if (date < minDate)
        {
            return false;
        }
    }

    if (!m_maxDate.isEmpty())
    {
        auto maxDate = Utils::decodeTimestamp(m_maxDate).date();
        if (date > maxDate)
        {
            return false;
        }
    }

    return date.isValid();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TimeField

TimeField::TimeField(QObject* parent): BaseField(parent)
{
}

QString TimeField::toXml(const QVariant& value) const
{
    return Utils::decodeTimestamp(value.toString()).time().toString(Qt::DateFormat::ISODate);
}

QVariant TimeField::fromXml(const QVariant& value) const
{
    auto dateTime = QDateTime();
    dateTime.setMSecsSinceEpoch(0);
    dateTime.setTime(QTime::fromString(value.toString(), Qt::DateFormat::ISODate));
    return Utils::encodeTimestamp(dateTime);
}

QVariant TimeField::save(const QVariant& value) const
{
    return toXml(value);
}

QVariant TimeField::load(const QVariant& value) const
{
    return fromXml(value.toString());
}

void TimeField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "TimeField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth, "}");
}

QString TimeField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    return App::instance()->formatTime(Utils::decodeTimestamp(value.toString()).time());
}

bool TimeField::isValid(const QVariant& value, bool required) const
{
    if (!BaseField::isValid(value, required))
    {
        return false;
    }

    if (value.toString().isEmpty())
    {
        return required ? false : true;
    }

    auto time = Utils::decodeTimestamp(value.toString()).time();

    return time.isValid();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ListField

ListField::ListField(QObject* parent): BaseField(parent)
{
}

QVariant ListField::save(const QVariant& value) const
{
    return value.toList();
}

QVariant ListField::load(const QVariant& value) const
{
    return value.toList();
}

void ListField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "ListField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth, "}");
}

QString ListField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    return value.toString();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PhotoField

PhotoField::PhotoField(QObject* parent): BaseField(parent)
{
    m_minCount = 0;
    m_maxCount = 1;
    m_resolutionX = -1;
    m_resolutionY = -1;
}

void PhotoField::appendAttachments(const QVariant& value, QStringList* attachmentsOut) const
{
    attachmentsOut->append(value.toStringList());
}

QVariant PhotoField::save(const QVariant& value) const
{
    return value.toStringList();
}

QVariant PhotoField::load(const QVariant& value) const
{
    auto result = QStringList();

    // Strip the file path for legacy import.
    auto source = value.toStringList();
    for (auto it = source.constBegin(); it != source.constEnd(); it++)
    {
        auto filename = *it;
        if (filename.contains("/"))
        {
            filename = QFileInfo(filename).fileName();
        }

        result.append(filename);
    }

    return result;
}

QString PhotoField::toXml(const QVariant& value) const
{
    auto attachments = QStringList();
    appendAttachments(value, &attachments);
    return attachments.join(";");
}

void PhotoField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "PhotoField {");
    BaseField::toQml(stream, depth + 1);

    Utils::writeQml(stream, depth + 1, "minCount", m_minCount, 0);
    Utils::writeQml(stream, depth + 1, "maxCount", m_maxCount, 1);
    Utils::writeQml(stream, depth + 1, "resolutionX", m_resolutionX, -1);
    Utils::writeQml(stream, depth + 1, "resolutionY", m_resolutionY, -1);

    Utils::writeQml(stream, depth, "}");
}

QString PhotoField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    auto photoCount = value.toStringList().count();
    return photoCount > 0 ? tr("Photos") + " (" + QString::number(photoCount) + ")" : "";
}

bool PhotoField::isValid(const QVariant& value, bool required) const
{
    if (!BaseField::isValid(value, required))
    {
        return false;
    }

    if (required)
    {
        auto filenames = value.toStringList();
        for (auto it = filenames.constBegin(); it != filenames.constEnd(); it++)
        {
            if (!it->isEmpty() && QFile::exists(App::instance()->getMediaFilePath(*it)))
            {
                return true;
            }
        }

        return false;
    }
    else
    {
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AudioField

AudioField::AudioField(QObject* parent): BaseField(parent)
{
    m_maxSeconds = 60;
    m_sampleRate = 16000;
}

void AudioField::appendAttachments(const QVariant& value, QStringList* attachmentsOut) const
{
    auto filename = value.toMap().value("filename").toString();
    if (!filename.isEmpty())
    {
        attachmentsOut->append(filename);
    }
}

QVariant AudioField::save(const QVariant& value) const
{
    return value.toMap();
}

QVariant AudioField::load(const QVariant& value) const
{
    return value.toMap();
}

QString AudioField::toXml(const QVariant& value) const
{
    auto attachments = QStringList();
    appendAttachments(value, &attachments);

    return attachments.join(';');
}

void AudioField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "AudioField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth + 1, "maxSeconds", m_maxSeconds);
    Utils::writeQml(stream, depth + 1, "sampleRate", m_sampleRate, 16000);
    Utils::writeQml(stream, depth, "}");
}

QString AudioField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    auto duration = value.toMap().value("duration", 0).toInt();
    return duration != 0 ? QString::number(duration / 1000) + " " + tr("seconds") : "";
}

bool AudioField::isValid(const QVariant& value, bool required) const
{
    if (!BaseField::isValid(value, required))
    {
        return false;
    }

    if (required)
    {
        auto filename = value.toMap().value("filename").toString();
        if (!filename.isEmpty() && QFile::exists(App::instance()->getMediaFilePath(filename)))
        {
            return true;
        }

        return false;
    }
    else
    {
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SketchField

SketchField::SketchField(QObject* parent): BaseField(parent)
{
}

QVariant SketchField::save(const QVariant& value) const
{
    return value.toMap();
}

QVariant SketchField::load(const QVariant& value) const
{
    return value.toMap();
}

void SketchField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "SketchField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth, "}");
}

QString SketchField::toXml(const QVariant& value) const
{
    return value.toMap().value("filename").toString();
}

QString SketchField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    return !value.toMap().isEmpty() ? tr("Yes") : tr("No");
}

void SketchField::appendAttachments(const QVariant& value, QStringList* attachmentsOut) const
{
    auto filename = value.toMap().value("filename").toString();
    if (!filename.isEmpty())
    {
        attachmentsOut->append(filename);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateField

CalculateField::CalculateField(QObject* parent): BaseField(parent)
{
    m_number = false;
}

QVariant CalculateField::save(const QVariant& value) const
{
    return m_number ? QVariant(value.toDouble()) : value.toString();
}

QVariant CalculateField::load(const QVariant& value) const
{
    return m_number ? QVariant(value.toDouble()) : value.toString();
}

void CalculateField::toQml(QTextStream& stream, int depth) const
{
    Utils::writeQml(stream, depth, "CalculateField {");
    BaseField::toQml(stream, depth + 1);
    Utils::writeQml(stream, depth + 1, "number", m_number, false);
    Utils::writeQml(stream, depth, "}");
}

QString CalculateField::toDisplayValue(ElementManager* /*elementManager*/, const QVariant& value) const
{
    return value.toString();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FieldManager

FieldManager::FieldManager(QObject* parent): QObject(parent)
{
    m_rootField = new RecordField();
}

FieldManager::~FieldManager()
{
    if (!m_cacheKey.isEmpty())
    {
        App::instance()->releaseQml(m_cacheKey);
    }
    else
    {
        delete m_rootField;
    }
}

RecordField* FieldManager::rootField() const
{
    return m_rootField;
}

BaseField* FieldManager::getField(const QString& fieldUid) const
{
    return fieldUid.isEmpty() ? m_rootField : m_fieldMap.value(fieldUid, nullptr);
}

QString FieldManager::getFieldType(const QString& fieldUid) const
{
    return getField(fieldUid)->metaObject()->className();
}

void FieldManager::populateMap(BaseField* field)
{
    // Insert this field.
    m_fieldMap.insert(field->uid(), field);

    // Insert the children.
    if (!field->isRecordField())
    {
        return;
    }

    auto recordField = RecordField::fromField(field);
    for (int i = 0; i < recordField->fieldCount(); i++)
    {
        auto f = recordField->field(i);
        qFatalIf(m_fieldMap.contains(f->uid()), "Duplicate field: " + f->uid());
        m_fieldMap.insert(f->uid(), f);
        populateMap(f);
    }
}

void FieldManager::loadFromQmlFile(const QString& filePath)
{
    m_fieldMap.clear();

    if (!m_cacheKey.isEmpty())
    {
        App::instance()->releaseQml(m_cacheKey);
        m_cacheKey.clear();
    }
    else
    {
        delete m_rootField;
    }

    m_rootField = nullptr;

    if (filePath.isEmpty())
    {
        return;
    }

    if (!QFile::exists(filePath))
    {
        qDebug() << "QML file not found: " << filePath;
        return;
    }

    m_rootField = qobject_cast<RecordField*>(App::instance()->acquireQml(filePath, &m_cacheKey));
    qFatalIf(!m_rootField, "Failed to load Fields QML: " + filePath);
    populateMap(m_rootField);
}

void FieldManager::saveToQmlFile(const QString& filePath)
{
    QFile file(filePath);
    file.remove();
    qFatalIf(!file.open(QIODevice::WriteOnly | QIODevice::Text), "Failed to create file");

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream.setGenerateByteOrderMark(true);
    stream << "import CyberTracker 1.0\n\n";
    m_rootField->toQml(stream, 0);
}

void FieldManager::appendField(RecordField* parent, BaseField* field)
{
    if (parent == nullptr)
    {
        parent = rootField();
    }

    parent->appendField(field);
    populateMap(field);
}

QStringList FieldManager::buildFieldList(const QString& recordFieldUid, const QString& filterTagName, const QVariant& filterTagValue) const
{
    auto fieldList = QStringList();
    auto recordField = RecordField::fromField(getField(recordFieldUid));
    qFatalIf(!recordField, "Field must be a record field");

    for (int i = 0; i < recordField->fieldCount(); i++)
    {
        auto field = recordField->field(i);
        if (field->hidden())
        {
            continue;
        }

        if (!filterTagName.isEmpty())
        {
            auto fieldTag = field->tag();
            if (fieldTag.isEmpty())
            {
                continue;
            }

            if (!fieldTag.contains(filterTagName))
            {
                continue;
            }

            if (fieldTag.value(filterTagName) != filterTagValue)
            {
                continue;
            }
        }

        fieldList.append(field->uid());
    }

    return fieldList;
}

void FieldManager::completeUpdate()
{
    m_fieldMap.clear();
    populateMap(m_rootField);
}
