#include "Record.h"
#include "Form.h"

namespace
{
    FieldManager* fieldManager(const QObject* obj)
    {
        return Form::parentForm(obj)->fieldManager();
    }

    ElementManager* elementManager(const QObject* obj)
    {
        return Form::parentForm(obj)->elementManager();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FieldValue

FieldValue::FieldValue(QObject* parent): QObject(parent)
{
    qFatalIf(true, "Must be created by Record");
}

FieldValue::FieldValue(const QString& fieldUid, QObject* parent): QObject(parent), m_fieldUid(fieldUid)
{
}

void FieldValue::init(const QString& fieldUid)
{
    m_fieldUid = fieldUid;
}

Record* FieldValue::record() const
{
    return qobject_cast<Record*>(parent());
}

QString FieldValue::recordUid() const
{
    return record()->recordUid();
}

BaseField* FieldValue::field() const
{
    if (m_fieldManager == nullptr)
    {
        m_fieldManager = ::fieldManager(this);
    }

    return m_fieldManager->getField(m_fieldUid);
}

QString FieldValue::fieldUid() const
{
    return m_fieldUid;
}

QString FieldValue::name() const
{
    return record()->getFieldName(m_fieldUid);
}

QVariant FieldValue::value() const
{
    return record()->getFieldValue(m_fieldUid, QVariant());
}

FieldState FieldValue::state() const
{
    return record()->getFieldState(m_fieldUid);
}

FieldFlags FieldValue::flags() const
{
    return record()->getFieldFlags(m_fieldUid);
}

QString FieldValue::xmlValue() const
{
    return field()->toXml(value());
}

QStringList FieldValue::attachments() const
{
    auto result = QStringList();
    field()->appendAttachments(value(), &result);

    return result;
}

QString FieldValue::exportUid() const
{
    return field()->safeExportUid();
}

QString FieldValue::hint() const
{
    return record()->getFieldHint(m_fieldUid);
}

QString FieldValue::displayValue() const
{
    return record()->getFieldDisplayValue(m_fieldUid);
}

QString FieldValue::constraintMessage() const
{
    return record()->getFieldConstraintMessage(m_fieldUid);
}

bool FieldValue::isRelevant() const
{
    return record()->testFieldFlag(m_fieldUid, FieldFlag::Relevant);
}

bool FieldValue::isVisible() const
{
    return record()->getFieldVisible(m_fieldUid);
}

bool FieldValue::isValid() const
{
    return record()->getFieldValid(m_fieldUid);
}

bool FieldValue::isRecordField() const
{
    return record()->testFieldFlag(m_fieldUid, FieldFlag::Record);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FieldValueChangesHelper

void FieldValueChangesHelper::init(FieldValueChanges* fieldValueChanges)
{
    m_fieldValueChanges = fieldValueChanges;

    for (auto it = m_fieldValueChanges->constBegin(); it != m_fieldValueChanges->constEnd(); it++)
    {
        m_containHash.insert(it->recordUid + it->fieldUid, true);
        m_fieldTailHash.insert(Utils::lastPathComponent(it->fieldUid), true);
        m_fieldFullHash.insert(it->fieldUid, true);
    }
}

bool FieldValueChangesHelper::contains(const FieldValueChange* fieldValueChange)
{
    return m_containHash.contains(fieldValueChange->recordUid + fieldValueChange->fieldUid);
}

bool FieldValueChangesHelper::containsField(const QString& fieldUid)
{
    return m_fieldFullHash.contains(fieldUid);
}

bool FieldValueChangesHelper::append(const FieldValueChange& fieldValueChange)
{
    if (!contains(&fieldValueChange))
    {
        m_fieldValueChanges->append(fieldValueChange);
        m_containHash.insert(fieldValueChange.recordUid + fieldValueChange.fieldUid, true);
        m_fieldTailHash.insert(Utils::lastPathComponent(fieldValueChange.fieldUid), true);
        m_fieldFullHash.insert(fieldValueChange.fieldUid, true);

        return true;
    }

    return false;
}

bool FieldValueChangesHelper::containsFieldReference(const QString& expression)
{
    for (auto it = m_fieldTailHash.constBegin(); it != m_fieldTailHash.constEnd(); it++)
    {
        if (expression.contains("${" + it.key() + '}'))
        {
            return true;
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Record

Record::Record(QObject* parent): QObject(parent)
{
    qFatalIf(true, "Must be created by Sighting");
}

Record::Record(const QVariantMap& data, QObject* parent): QObject(parent)
{
    load(data);
}

Record::Record(const QString& recordUid, const QString& parentRecordUid, const QString& recordFieldUid, QObject* parent):
    QObject(parent), m_recordUid(recordUid), m_parentRecordUid(parentRecordUid), m_recordFieldUid(recordFieldUid)
{
}

void Record::reset()
{
    auto fieldValues = m_fieldValues;

    for (auto it = fieldValues.constBegin(); it != fieldValues.constEnd(); it++)
    {
        if (it.value().state != FieldState::Constant)
        {
            m_fieldValues.remove(it.key());
        }
    }
}

RecordManager* Record::recordManager() const
{
    return qobject_cast<RecordManager*>(parent());
}

QString Record::recordUid() const
{
    return m_recordUid;
}

QString Record::recordFieldUid() const
{
    return m_recordFieldUid;
}

QString Record::parentRecordUid() const
{
    return m_parentRecordUid;
}

QString Record::nameElementUid() const
{
    return m_nameElementUid;
}

void Record::set_nameElementUid(const QString& value)
{
    m_nameElementUid = value;
}

const RecordField* Record::recordField() const
{
    return RecordField::fromField(fieldManager(this)->getField(m_recordFieldUid));
}

QVariantMap Record::data() const
{
    return save(false);
}

QVariantMap Record::save(bool onlyRelevant) const
{
    auto values = QVariantMap();
    auto states = QVariantMap();

    enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
    {
        if (onlyRelevant && !fieldValue.isRelevant())
        {
            return;
        }

        auto value = fieldValue.value();
        auto state = fieldValue.state();
        auto flags = fieldValue.flags();

        // Only keep if non-default or it might be computed.
        // TODO(justin): is there a better way to know if we can recompute it?
        if (state == FieldState::Default && !value.isValid())
        {
            return;
        }

        // Add value and state.
        if (value.isValid())
        {
            values[fieldValue.fieldUid()] = fieldValue.field()->save(value);
        }

        states[fieldValue.fieldUid()] = (int)state | (flags << 16);
    });

    return QVariantMap
    {
        { "uid", m_recordUid },
        { "parentUid", m_parentRecordUid },
        { "fieldUid", m_recordFieldUid },
        { "nameElementUid", m_nameElementUid },
        { "fieldStates", states },
        { "fieldValues", values }
    };
}

void Record::load(const QVariantMap& data)
{
    m_recordUid = data["uid"].toString();
    m_parentRecordUid = data["parentUid"].toString();
    m_recordFieldUid = data["fieldUid"].toString();
    m_nameElementUid = data["nameElementUid"].toString();

    auto states = data["fieldStates"].toMap();
    auto values = data["fieldValues"].toMap();

    m_fieldValues.clear();

    enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
    {
        auto fieldUid = fieldValue.fieldUid();

        auto valueData = ValueData();
        if (states.contains(fieldUid))
        {
            auto jsonState = states.value(fieldUid, 0).toUInt();
            valueData.state = FieldState(jsonState & 0xFFFF);
            valueData.flags = FieldFlags(jsonState >> 16);
        }

        if (values.contains(fieldUid))
        {
            valueData.value = fieldValue.field()->load(values.value(fieldUid));
        }

        m_fieldValues[fieldUid] = valueData;
    });
}

QStringList Record::attachments() const
{
    auto result = QStringList();

    enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
    {
        fieldValue.field()->appendAttachments(fieldValue.value(), &result);
    });

    return result;
}

QString Record::summary() const
{
    auto result = QStringList();

    enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
    {
        if (!fieldValue.isVisible())
        {
            return;
        }

        auto displayValue = fieldValue.displayValue();
        if (!displayValue.isEmpty())
        {
            result.append(displayValue);
        }
    });

    return result.join(", ");
}

QString Record::name() const
{
    auto result = QString();
    auto elementManager = ::elementManager(this);

    auto recordField = this->recordField();
    if (recordField->matrixCount() > 0)
    {
        for (auto i = 0; i < recordField->matrixCount(); i++)
        {
            auto fieldUid = recordField->field(i)->uid();
            auto value = elementManager->getElementName(getFieldValue(fieldUid).toString());
            result += i == 0 ? value : " + " + value;
        }
    }
    else if (recordField->group())
    {
        FieldValue fieldValue(m_recordFieldUid, (QObject*)this);
        result = fieldValue.name();
    }
    else if (this != recordManager()->rootRecord())
    {
        result = !m_nameElementUid.isEmpty() ? elementManager->getElementName(m_nameElementUid) : tr("Record") + " " + QString::number(index() + 1);
    }

    return result;
}

QString Record::icon() const
{
    if (recordField()->matrixCount() == 0)
    {
        return !m_nameElementUid.isEmpty() ? elementManager(this)->getElementIcon(m_nameElementUid).toString() : "";
    }

    return QString();
}

int Record::index() const
{
    return qobject_cast<RecordManager*>(parent())->getRecordIndex(m_recordUid);
}

void Record::enumFieldValues(const std::function<void(const FieldValue& fieldValue, bool* stopOut)>& fieldValueCallback) const
{
    auto recordField = this->recordField();
    auto stop = false;
    FieldValue fieldValue("", (QObject*)this);

    for (auto i = 0; i < recordField->fieldCount() && !stop; i++)
    {
        auto field = recordField->field(i);
        auto fieldUid = field->uid();

        fieldValue.init(fieldUid);
        fieldValueCallback(fieldValue, &stop);
    }
}

void Record::remapRecordUids(const std::function<QString(const QString& recordUid)>& lookup)
{
    m_recordUid = lookup(m_recordUid);

    if (!m_parentRecordUid.isEmpty())
    {
        m_parentRecordUid = lookup(m_parentRecordUid);
    }

    enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
    {
        if (!fieldValue.field()->isRecordField())
        {
            return;
        }

        auto recordUids = fieldValue.value().toStringList();
        for (auto i = 0; i < recordUids.count(); i++)
        {
            recordUids.replace(i, lookup(recordUids[i]));
        }

        m_fieldValues[fieldValue.fieldUid()].value = recordUids;
    });
}

bool Record::hasFieldValue(const QString& fieldUid) const
{
    return m_fieldValues.contains(fieldUid) && m_fieldValues.value(fieldUid).value.isValid();
}

QVariant Record::getFieldValue(const QString& fieldUid, const QVariant& defaultValue) const
{
    auto result = m_fieldValues.value(fieldUid).value;
    if (!result.isValid())
    {
        result = defaultValue;
    }

    return result;
}

void Record::setFieldValue(const QString& fieldUid, const QVariant& value)
{
    if (getFieldState(fieldUid) == FieldState::Constant)
    {
        qDebug() << "Cannot setFieldValue on a constant field: " << fieldUid;
        return;
    }

    if (!value.isValid())
    {
        resetFieldValue(fieldUid);
        return;
    }

    m_fieldValues[fieldUid].value = QVariant(value);
}

void Record::resetFieldValue(const QString& fieldUid)
{
    if (getFieldState(fieldUid) == FieldState::Constant)
    {
        return;
    }

    m_fieldValues.remove(fieldUid);
    m_fieldValues.insert(fieldUid, ValueData { QVariant(), FieldState::None, FieldFlags() });
}

bool Record::isEmpty() const
{
    auto result = true;

    enumFieldValues([&](const FieldValue& fieldValue, bool* stopOut)
    {
        auto state = fieldValue.state();

        if (state == FieldState::Constant)
        {
            return;
        }

        if (state == FieldState::UserSet || fieldValue.value().isValid())
        {
            result = false;
            *stopOut = true;
            return;
        }
    });

    return result;
}

bool Record::isValid(const QStringList& fieldUids) const
{
    auto result = true;

    enumFieldValues([&](const FieldValue& fieldValue, bool* stopOut)
    {
        if (!fieldUids.isEmpty() && fieldUids.indexOf(fieldValue.fieldUid()) == -1)
        {
            return;
        }

        if (!fieldValue.isVisible())
        {
            return;
        }

        if (!fieldValue.isValid())
        {
            *stopOut = true;
            result = false;
            return;
        }
    });

    return result;
}

FieldState Record::getFieldState(const QString& fieldUid) const
{
    return m_fieldValues.value(fieldUid).state;
}

void Record::setFieldState(const QString& fieldUid, FieldState state)
{
    m_fieldValues[fieldUid].state = state;
}

FieldFlags Record::getFieldFlags(const QString& fieldUid) const
{
    return m_fieldValues[fieldUid].flags;
}

bool Record::testFieldFlag(const QString& fieldUid, FieldFlag flag) const
{
    return m_fieldValues[fieldUid].flags.testFlag(flag);
}

void Record::setFieldFlag(const QString& fieldUid, FieldFlag flag, bool on)
{
    m_fieldValues[fieldUid].flags.setFlag(flag, on);
}

void Record::setFieldFlags(const QString& fieldUid, FieldFlags flags)
{
    m_fieldValues[fieldUid].flags = flags;
}

bool Record::getFieldVisible(const QString& fieldUid) const
{
    auto flags = getFieldFlags(fieldUid);
    auto visible = !flags.testFlag(FieldFlag::Hidden) && !flags.testFlag(FieldFlag::Skip) && flags.testFlag(FieldFlag::Relevant);

    // Already computed as hidden.
    if (!visible)
    {
        return false;
    }

    // Ignore non-groups.
    auto recordField = RecordField::fromField(fieldManager(this)->getField(fieldUid));
    if (!recordField || !recordField->group())
    {
        return true;
    }

    // Ensure all group members are visible.
    auto groupRecordUid = getFieldValue(fieldUid).toStringList().constFirst();
    auto groupRecord = recordManager()->getRecord(groupRecordUid);
    for (auto i = 0; i < recordField->fieldCount(); i++)
    {
        if (groupRecord->getFieldVisible(recordField->field(i)->uid()))
        {
            return true;
        }
    }

    // No group members are visible.
    return false;
}

bool Record::getFieldValid(const QString& fieldUid) const
{
    auto flags = getFieldFlags(fieldUid);
    auto result = flags.testFlag(FieldFlag::Valid);

    // Check the children of record fields.
    if (result && flags.testFlag(FieldFlag::Record))
    {
        auto recordUids = getFieldValue(fieldUid).toStringList();
        for (auto it = recordUids.constBegin(); it != recordUids.constEnd(); it++)
        {
            if (!recordManager()->getRecord(*it)->isValid())
            {
                result = false;
                break;
            }
        }
    }

    return result;
}

QString Record::getFieldName(const QString& fieldUid) const
{
    auto fieldManager = ::fieldManager(this);
    auto elementManager = ::elementManager(this);

    // Just return the record name if we have no field.
    if (fieldUid.isEmpty())
    {
        return name();
    }

    // Use the field name if specified.
    auto field = fieldManager->getField(fieldUid);
    auto result = recordManager()->expandFieldText(m_recordUid, elementManager->getElementName(field->nameElementUid()));
    if (!result.isEmpty())
    {
        return result;
    }

    // Skip if not a record field.
    if (!field->isRecordField())
    {
        if (field->nameElementUid().isEmpty())
        {
            return field->uid();
        }

        return QString();
    }

    // Build a matrix name.
    auto recordField = RecordField::fromField(field);
    if (recordField->matrixCount() > 0)
    {
        auto result = QString();
        for (auto i = 0; i < recordField->matrixCount(); i++)
        {
            auto nameUid = recordField->field(i)->nameElementUid();
            auto name = recordManager()->expandFieldText(m_recordUid, elementManager->getElementName(nameUid));
            result += i == 0 ? name : " x " + name;
        }

        return result;
    }

    return QString(tr("Records"));
}

QString Record::getFieldHint(const QString& fieldUid) const
{
    auto fieldManager = ::fieldManager(this);
    auto elementManager = ::elementManager(this);

    auto elementUid = fieldManager->getField(fieldUid)->hintElementUid();
    if (elementUid.isEmpty())
    {
        return QString();
    }

    return recordManager()->expandFieldText(m_recordUid, elementManager->getElementName(elementUid));
}

QString Record::getFieldConstraintMessage(const QString& fieldUid) const
{
    auto fieldManager = ::fieldManager(this);
    auto elementManager = ::elementManager(this);

    auto field = fieldManager->getField(fieldUid);
    if (!field->constraint().isEmpty() && !field->constraintElementUid().isEmpty())
    {
        return recordManager()->expandFieldText(m_recordUid, elementManager->getElementName(field->constraintElementUid()));
    }

    return tr("Value required");
}

QString Record::getFieldDisplayValue(const QString& fieldUid, const QString& defaultValue) const
{
    auto field = ::fieldManager(this)->getField(fieldUid);
    auto value = getFieldValue(fieldUid);

    // For matrices, filter to non-empty records.
    auto recordField = RecordField::fromField(field);
    if (recordField && recordField->matrixCount() > 0)
    {
        auto recordUids = value.toStringList();
        auto activeRecordUids = QStringList();
        for (auto it = recordUids.constBegin(); it != recordUids.constEnd(); it++)
        {
            if (!recordManager()->getRecord(*it)->isEmpty())
            {
                activeRecordUids.append(*it);
            }
        }

        value = activeRecordUids;
    }

    auto result = field->toDisplayValue(elementManager(this), value);
    if (result.isEmpty())
    {
        result = defaultValue;
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RecordManager

RecordManager::RecordManager(QObject* parent): QObject(parent)
{
}

RecordManager::~RecordManager()
{
    reset();
}

void RecordManager::reset()
{
    qDeleteAll(m_records);
    m_records.clear();
}

QList<Record*> RecordManager::records()
{
    return m_records;
}

void RecordManager::remapRecordUids(const std::function<QString(const QString& recordUid)>& lookup)
{
    for (auto it = m_records.begin(); it != m_records.end(); it++)
    {
        (*it)->remapRecordUids(lookup);
    }
}

QVariantList RecordManager::save(bool onlyRelevant, QStringList* attachmentsOut) const
{
    auto result = QVariantList();

    enumFieldValues(rootRecordUid(), onlyRelevant,
        [&](const Record& record, bool* /*stopOut*/)
        {
            result.append(record.save(onlyRelevant));
        },
        [&](const FieldValue& fieldValue)
        {
            if (attachmentsOut)
            {
                attachmentsOut->append(fieldValue.attachments());
            }
        },
        nullptr);

    return result;
}

void RecordManager::load(const QVariantList& recordsArray)
{
    reset();

    auto fieldManager = ::fieldManager(this);

    for (auto it = recordsArray.constBegin(); it != recordsArray.constEnd(); it++)
    {
        auto recordData = it->toMap();
        auto recordFieldUid = recordData["fieldUid"].toString();

        if (!fieldManager->getField(recordFieldUid))
        {
            qDebug() << "Ignoring record because field missing: " << recordFieldUid;
            continue;
        }

        m_records.append(new Record(recordData, this));
    }
}

Record* RecordManager::rootRecord() const
{
    return m_records[0];
}

QString RecordManager::rootRecordUid() const
{
    return m_records[0]->recordUid();
}

Record* RecordManager::getRecord(const QString& recordUid) const
{
    for (auto it = m_records.constBegin(); it != m_records.constEnd(); it++)
    {
        if ((*it)->recordUid() == recordUid)
        {
            return *it;
        }
    }

    return nullptr;
}

int RecordManager::getRecordIndex(const QString& recordUid) const
{
    auto record = getRecord(recordUid);
    auto parentRecordUid = record->parentRecordUid();

    if (parentRecordUid.isEmpty())
    {
        return 0;
    }

    return getRecord(parentRecordUid)->getFieldValue(record->recordFieldUid()).toStringList().indexOf(recordUid);
}

QStringList RecordManager::attachments() const
{
    auto result = QStringList();
    for (auto it = m_records.constBegin(); it != m_records.constEnd(); it++)
    {
        result.append((*it)->attachments());
    }

    return result;
}

bool RecordManager::hasRecords() const
{
    return !m_records.isEmpty();
}

bool RecordManager::hasRecord(const QString& recordUid) const
{
    for (auto it = m_records.constBegin(); it != m_records.constEnd(); it++)
    {
        if ((*it)->recordUid() == recordUid)
        {
            return true;
        }
    }

    return false;
}

QStringList RecordManager::recordUids() const
{
    auto result = QStringList();
    for (auto it = m_records.constBegin(); it != m_records.constEnd(); it++)
    {
        result.append((*it)->recordUid());
    }

    return result;
}

void RecordManager::enumFieldValues(
    const QString& recordUid, bool onlyRelevant,
    const std::function<void(const Record& record, bool* skipRecordOut)>& recordBeginCallback,
    const std::function<void(const FieldValue& fieldValue)>& fieldValueCallback,
    const std::function<void(const Record& record)>& recordEndCallback) const
{
    auto record = getRecord(recordUid);

    if (recordBeginCallback)
    {
        auto skipRecord = false;
        recordBeginCallback(*record, &skipRecord);
        if (skipRecord)
        {
            return;
        }
    }

    record->enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
    {
        // Optionally hide irrelevant fields.
        if (onlyRelevant && !fieldValue.isRelevant())
        {
            return;
        }

        // Run the callback.
        if (fieldValueCallback)
        {
            fieldValueCallback(fieldValue);
        }

        // Enum child records.
        if (fieldValue.field()->isRecordField())
        {
            auto childRecordUids = fieldValue.value().toStringList();
            for (auto it = childRecordUids.constBegin(); it != childRecordUids.constEnd(); it++)
            {
                enumFieldValues(*it, onlyRelevant, recordBeginCallback, fieldValueCallback, recordEndCallback);
            }
        }
    });

    if (recordEndCallback)
    {
        recordEndCallback(*record);
    }
}

QString RecordManager::newRecord(const QString& parentRecordUid, const QString& recordFieldUid)
{
    auto recordUid = QUuid::createUuid().toString(QUuid::Id128);

    auto record = new Record(recordUid, parentRecordUid, recordFieldUid, this);
    m_records.append(record);

    // Add a link to this record to the parent record.
    auto parentRecord = getRecord(parentRecordUid);
    if (parentRecord)
    {
        auto recordList = parentRecord->getFieldValue(recordFieldUid).toStringList();
        recordList.append(recordUid);
        parentRecord->setFieldValue(recordFieldUid, recordList);
        parentRecord->setFieldState(recordFieldUid, FieldState::Default);
    }

    return recordUid;
}

bool RecordManager::detachRecord(Record* record)
{
    auto result = true;

    // Remove the link to this record from the parent record (if it exists).
    auto parentRecordUid = record->parentRecordUid();
    if (!parentRecordUid.isEmpty())
    {
        auto parentRecord = getRecord(parentRecordUid);
        if (parentRecord)
        {
            auto recordFieldUid = record->recordFieldUid();
            auto recordList = parentRecord->getFieldValue(recordFieldUid).toStringList();
            result = recordList.removeOne(record->recordUid());
            parentRecord->setFieldValue(recordFieldUid, recordList.isEmpty() ? QVariant() : recordList);
        }
    }

    return result;
}

void RecordManager::deleteRecord(const QString& recordUid)
{
    auto record = getRecord(recordUid);
    auto success = detachRecord(record);
    qFatalIf(!success, "Broken parent -> child link during deleteRecord()");

    // Remove the record.
    m_records.removeOne(record);
    delete record;

    // Let others know in case of cleanup.
    emit recordDeleted(recordUid);

    // Cleanup orphaned children.
    garbageCollect();
}

void RecordManager::garbageCollect()
{
    auto goodRecordUids = QStringList { rootRecordUid() };

    // Traverse the tree and make sure that the record links are valid.
    // If the child record has been removed or has the wrong parent, then it will be
    // considered an orphan and cleaned up.
    std::function<void(Record*)> traverse = [&](Record* record)
    {
        record->enumFieldValues([&](const FieldValue& fieldValue, bool* /*stopOut*/)
        {
            if (!fieldValue.field()->isRecordField())
            {
                return;
            }

            auto childRecordUids = fieldValue.value().toStringList();
            auto fixedChildRecordUids = QStringList();
            auto changed = false;
            for (auto it = childRecordUids.constBegin(); it != childRecordUids.constEnd(); it++)
            {
                auto childRecordUid = *it;
                auto childRecord = getRecord(childRecordUid);

                // Child record must exist.
                if (!childRecord)
                {
                    changed = true;
                    continue;
                }

                // Child record must have the parentRecordUid be set to this one.
                if (childRecord->parentRecordUid() != record->recordUid())
                {
                    changed = true;
                    continue;
                }

                // Record is good.
                fixedChildRecordUids.append(childRecordUid);
                traverse(childRecord);
            }

            // Something changed, so fix the list.
            if (changed)
            {
                record->setFieldValue(fieldValue.fieldUid(), fixedChildRecordUids);
            }

            // Track all good records.
            goodRecordUids.append(fixedChildRecordUids);
        });
    };

    traverse(rootRecord());

    // Get orphaned records.
    auto orphanRecordUids = QStringList();
    for (auto it = m_records.constBegin(); it != m_records.constEnd(); it++)
    {
        auto recordUid = (*it)->recordUid();
        if (!goodRecordUids.contains(recordUid))
        {
            orphanRecordUids.append(recordUid);
        }
    }

    // Delete orphans.
    for (auto it = orphanRecordUids.constBegin(); it != orphanRecordUids.constEnd(); it++)
    {
        auto recordUid = *it;
        qDebug() << "Deleting orphan record: " << recordUid;

        auto record = getRecord(recordUid);
        m_records.removeOne(record);
        delete record;

        emit recordDeleted(recordUid);
    }
}

void RecordManager::recalculate(FieldValueChanges* fieldValueChanges, const QVariantMap* symbols, const QVariantMap* variables, bool trackChanges)
{
    // Initialize the evaluator.
    Evaluator evaluator(symbols, variables, [&](const QString& contextRecordUid, const QString& findFieldUid)
    {
        return queryFieldValue(contextRecordUid, findFieldUid);
    });

    // Helper to track the change list.
    FieldValueChangesHelper fieldValueChangesHelper;
    fieldValueChangesHelper.init(fieldValueChanges);

    // Calculate.
    auto recompute = false;
    auto recomputeCount = 0;
    auto requestGarbageCollect = false;

    for (auto it = m_records.begin(); it != m_records.end(); )
    {
        // Fail safe.
        if (recomputeCount > 10)
        {
            qDebug() << "Recompute count is large: " << recomputeCount;
            break;
        }

        auto record = *it++;
        auto recordField = RecordField::fromField(fieldManager(this)->getField(record->recordFieldUid()));

        auto recordsChanged = false;

        // Enumerate the fields in this record.
        recordField->enumFields([&](const BaseField* field)
        {
            auto fieldUid = field->uid();
            auto value = record->getFieldValue(fieldUid);
            auto state = record->getFieldState(fieldUid);
            auto flags = record->getFieldFlags(fieldUid);

            // Compute the new value and state.
            auto newValue = QVariant();
            auto newState = FieldState::None;
            if (computeFieldValue(record, field, value, state, evaluator, &newValue, &newState))
            {
                recompute = true;
                recordsChanged = recordsChanged || field->isRecordField();

                record->setFieldValue(fieldUid, newValue);
                record->setFieldState(fieldUid, newState);

                // Add the change to the tracking list.
                if (trackChanges)
                {
                    fieldValueChangesHelper.append(FieldValueChange { record->recordUid(), fieldUid, value });
                }
            }

            // Compute the new flags.
            auto newFlags = FieldFlags();
            if (computeFieldValueFlags(record, field, value, flags, evaluator, &newFlags))
            {
                // Recompute only if the valid or relevant flag changed.
                auto intersect = flags ^ newFlags;
                recompute = recompute || (intersect & FieldFlag::Valid) || (intersect & FieldFlag::Relevant);

                record->setFieldFlags(fieldUid, newFlags);
            }

            // Compute side effects.
            auto sideEffects = !field->isRecordField() ?
                computeSideEffects(fieldValueChangesHelper, record, field, value, newValue) :
                computeSideEffectsRecord(fieldValueChangesHelper, record, RecordField::fromField(field), value, newValue, &requestGarbageCollect);

            if (sideEffects)
            {
                recompute = recordsChanged = true;

                if (trackChanges)
                {
                    fieldValueChangesHelper.append(FieldValueChange { record->recordUid(), fieldUid, value });
                }
            }
        });

        // Restart if records have changed.
        if (recordsChanged)
        {
            it = m_records.begin();
            recomputeCount++;
            recordsChanged = recompute = false;
            continue;
        }

        // Restart if at the end and something changed.
        if (it == m_records.end() && recompute)
        {
            it = m_records.begin();
            recomputeCount++;
            recordsChanged = recompute = false;
            continue;
        }
    }

    if (requestGarbageCollect)
    {
        garbageCollect();
    }
}

QVariant RecordManager::evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid, const QVariantMap* symbols, const QVariantMap* variables) const
{
    Evaluator evaluator(symbols, variables, [&](const QString& contextRecordUid, const QString& findFieldUid)
    {
        return queryFieldValue(contextRecordUid, findFieldUid);
    });

    return evaluator.evaluate(expression, contextRecordUid, contextFieldUid);
}

QVariant RecordManager::queryFieldValue(const QString& recordUid, const QString& findFieldUid) const
{
    auto result = QVariant();
    if (queryFieldValuesParents(recordUid, findFieldUid, &result))
    {
        return result;
    }

    auto results = QVariantList();
    if (queryFieldValuesChildren(recordUid, findFieldUid, &results))
    {
        return results.count() == 1 ? results.first() : results;
    }

    // Not found: return an empty variant.
    return QVariant();
}

QString RecordManager::expandFieldText(const QString& recordUid, const QString& fieldText) const
{
    if (!fieldText.contains("${"))
    {
        return fieldText;
    }

    auto result = fieldText;

    auto fieldUids = fieldText.split('$');
    for (auto it = fieldUids.begin(); it != fieldUids.end(); it++)
    {
        if (it->startsWith("{"))
        {
            it->remove(0, 1);
            it->truncate(it->indexOf('}'));

            auto foundValue = queryFieldValue(recordUid, *it);
            if (foundValue.isValid())
            {
                if (!foundValue.toList().isEmpty())
                {
                    foundValue = foundValue.toList().constFirst();
                }

                result.replace("${" + *it + "}", foundValue.toString());
            }
        }
    }

    return result;
}

bool RecordManager::queryFieldValuesParents(const QString& recordUid, const QString& findFieldUid, QVariant* foundFieldValueOut) const
{
    auto record = getRecord(recordUid);

    record->enumFieldValues([&](const FieldValue& fieldValue, bool* stopOut)
    {
        if (!Utils::compareLastPathComponents(fieldValue.fieldUid(), findFieldUid))
        {
            return;
        }

        *stopOut = true;

        if (fieldValue.isRelevant() && fieldValue.value().isValid())
        {
            foundFieldValueOut->setValue(fieldValue.xmlValue());
        }
    });

    if (foundFieldValueOut->isValid())
    {
        return true;
    }

    if (!record->parentRecordUid().isEmpty())
    {
        return queryFieldValuesParents(record->parentRecordUid(), findFieldUid, foundFieldValueOut);
    }

    return false;
}

bool RecordManager::queryFieldValuesChildren(const QString& recordUid, const QString& findFieldUid, QVariantList* fieldValuesOut) const
{
    auto record = getRecord(recordUid);

    record->enumFieldValues([&](const FieldValue& fieldValue, bool* stopOut)
    {
        auto matchFieldUid = Utils::compareLastPathComponents(fieldValue.fieldUid(), findFieldUid);
        *stopOut = matchFieldUid;

        if (!fieldValue.isRelevant() || !fieldValue.value().isValid())
        {
            return;
        }

        if (fieldValue.isRecordField())
        {
            auto recordUids = fieldValue.value().toStringList();
            for (auto it = recordUids.constBegin(); it != recordUids.constEnd(); it++)
            {
                queryFieldValuesChildren(*it, findFieldUid, fieldValuesOut);
            }
        }

        if (matchFieldUid)
        {
            fieldValuesOut->append(fieldValue.xmlValue());
        }
    });

    return !fieldValuesOut->isEmpty();
}

bool RecordManager::computeFieldValue(Record* record, const BaseField* field, const QVariant& value, FieldState state, Evaluator& evaluator, QVariant* valueOut, FieldState* stateOut)
{
    *valueOut = value;
    *stateOut = state;

    // User set state always wins.
    if (state == FieldState::UserSet)
    {
        return false;
    }

    auto recordUid = record->recordUid();
    auto fieldUid = field->uid();
    auto changed = false;

    // Default value.
    if (state == FieldState::None)
    {
        // Evaluate defaultValue as a formula if it is a string.
        auto defaultValue = field->defaultValue();
        if (defaultValue.isValid() && defaultValue.userType() == QMetaType::QString)
        {
            auto newValueXml = evaluator.evaluate(defaultValue.toString(), recordUid, fieldUid);
            if (newValueXml.isValid())
            {
                defaultValue = field->fromXml(newValueXml);
            }
        }

        *stateOut = FieldState::Default;
        valueOut->setValue(defaultValue);
        changed = true;
    }

    // Formula.
    auto formula = field->formula();
    if (!formula.isEmpty() && !(formula.startsWith("once(") && value.isValid()))
    {
        auto newValueXml = evaluator.evaluate(formula, recordUid, fieldUid);
        if (newValueXml.isValid())
        {
            auto newValue = field->fromXml(newValueXml);

            changed = changed || (state != FieldState::Calculated) || (value != newValue);
            *stateOut = FieldState::Calculated;
            valueOut->setValue(newValue);
        }
    }

    // Evaluate repeatCount.
    if (field->isRecordField() && !RecordField::fromField(field)->repeatCount().isEmpty())
    {
        auto repeatCountXml = evaluator.evaluate(RecordField::fromField(field)->repeatCount(), recordUid, fieldUid);
        auto repeatCountIsInt = false;
        auto repeatCountAsInt = qBound(0, repeatCountXml.toInt(&repeatCountIsInt), 100);

        if (repeatCountIsInt && repeatCountAsInt != value.toStringList().count())
        {
            auto recordList = value.toStringList();
            auto recordDelta = repeatCountAsInt - recordList.count();

            if (recordDelta > 0)
            {
                for (auto i = 0; i < recordDelta; i++)
                {
                    newRecord(recordUid, fieldUid);
                }
            }
            else if (recordDelta < 0)
            {
                for (auto i = 0; i < -recordDelta; i++)
                {
                    deleteRecord(recordList[recordList.count() - i - 1]);
                }
            }

            if (recordDelta != 0)
            {
                valueOut->setValue(record->getFieldValue(fieldUid));
                changed = true;
            }
        }
    }

    return changed;
}

bool RecordManager::computeFieldValueFlags(Record* record, const BaseField* field, const QVariant& value, FieldFlags flags, Evaluator& evaluator, FieldFlags* flagsOut) const
{
    auto recordUid = record->recordUid();
    auto fieldUid = field->uid();

    auto newFlags = FieldFlags();
    auto required = false;
    auto relevant = false;
    auto constraint = false;
    auto valid = false;
    auto skip = false;
    auto isRecordField = field->isRecordField();

    // FieldFlag::FieldHidden.
    if (field->hidden())
    {
        newFlags.setFlag(FieldFlag::Hidden);
    }

    // FieldFlag::Record.
    if (isRecordField)
    {
        newFlags.setFlag(FieldFlag::Record);
    }

    // FieldFlag::Required.
    {
        auto requiredVariant = field->required();

        if (requiredVariant.type() == QVariant::Type::Bool)
        {
            required = requiredVariant.toBool();
        }
        else if (requiredVariant.type() == QVariant::Type::String)
        {
            required = evaluator.evaluate(requiredVariant.toString(), recordUid, fieldUid).toBool();
        }

        if (required)
        {
            newFlags.setFlag(FieldFlag::Required);
        }
    }

    // FieldFlag::Relevant.
    {
        relevant = true;
        if (!field->relevant().isEmpty())
        {
            relevant = evaluator.evaluate(field->relevant(), recordUid, fieldUid).toBool();
        }

        if (relevant)
        {
            newFlags.setFlag(FieldFlag::Relevant);
        }
    }

    // FieldFlag::Constraint.
    {
        constraint = true;
        if (!field->constraint().isEmpty())
        {
            constraint = evaluator.evaluate(field->constraint(), recordUid, fieldUid).toBool();
        }

        if (constraint)
        {
            newFlags.setFlag(FieldFlag::Constraint);
        }
    }

    // FieldFlag::Valid.
    {
        // Filter a list according to another list, e.g. Employees and Leader.
        if (value.isValid() && !field->listElementUid().isEmpty() && !field->listFilterByField().isEmpty())
        {
            auto filterValue = record->getFieldValue(field->listFilterByField()).toMap();
            if (!filterValue.value(value.toString()).toBool())
            {
                constraint = false;
            }
        }

        valid = constraint && field->isValid(value, required);

        if (valid)
        {
            newFlags.setFlag(FieldFlag::Valid);
        }
    }

    // FieldFlag::Skip.
    {
        if (field->isRecordField() && value.toStringList().isEmpty() && !RecordField::fromField(field)->dynamic())
        {
            skip = true;
        }

        if (skip)
        {
            newFlags.setFlag(FieldFlag::Skip);
        }
    }

    // Return the new flags and whether they changed.
    *flagsOut = newFlags;
    return flags != newFlags;
}

bool RecordManager::computeSideEffects(FieldValueChangesHelper& fieldValueChangesHelper, Record* record, const BaseField* field, const QVariant& oldValue, const QVariant& /*newValue*/)
{
    auto fieldUid = field->uid();

    // Cascading selects: reset if parent is changed.
    if (!field->listFilterByTag().isEmpty() &&
        record->getFieldState(fieldUid) == FieldState::UserSet &&
        fieldValueChangesHelper.containsFieldReference(field->listFilterByTag()))
    {
        record->resetFieldValue(fieldUid);
        return true;
    }

    // Filter a list according to another list, e.g. Employees and Leader.
    if (oldValue.isValid() && !field->listElementUid().isEmpty() && !field->listFilterByField().isEmpty())
    {
        auto filterValue = record->getFieldValue(field->listFilterByField()).toMap();
        if (!filterValue.value(oldValue.toString()).toBool())
        {
            record->resetFieldValue(fieldUid);
            return true;
        }
    }

    return false;
}

bool RecordManager::computeSideEffectsRecord(FieldValueChangesHelper& fieldValueChangesHelper, Record* record, const RecordField* recordField, const QVariant& oldValue, const QVariant& newValue, bool* garbageCollectOut)
{
    auto recordUid = record->recordUid();
    auto fieldUid = recordField->uid();

    // Auto-create min count records.
    if (newValue.toStringList().isEmpty() && recordField->minCount() > 0)
    {
        for (auto i = 0; i < recordField->minCount(); i++)
        {
            newRecord(recordUid, fieldUid);
        }

        return true;
    }

    // Auto-create matrix records.
    if (newValue.toStringList().isEmpty() && recordField->matrixCount() > 0)
    {
        auto lists = QList<QStringList>();
        for (auto i = 0; i < recordField->matrixCount(); i++)
        {
            auto listElementUid = recordField->field(i)->listElementUid();
            auto leafElementUids = elementManager(this)->getLeafElementUids(listElementUid);
            lists.append(leafElementUids);
        }

        auto combinations = Utils::buildListCombinations(lists);
        for (auto it = combinations.constBegin(); it != combinations.constEnd(); it++)
        {
            auto subRecordUid = newRecord(recordUid, recordField->uid());
            auto subRecord = getRecord(subRecordUid);

            for (auto i = 0; i < it->count(); i++)
            {
                auto fieldUid = recordField->field(i)->uid();
                subRecord->setFieldValue(fieldUid, it->at(i));
                subRecord->setFieldState(fieldUid, FieldState::Constant);
            }
        }

        return true;
    }

    // Create records based on masterFieldUid.
    auto masterFieldUid = recordField->masterFieldUid();
    if (!masterFieldUid.isEmpty())
    {
        // Check if the master field changed or there are no records.
        // No records happens either initially or if the record field was reset.
        if (!fieldValueChangesHelper.containsField(masterFieldUid) && newValue.toStringList().count() != 0)
        {
            return false;
        }

        // If the dependent RecordField is reset, then propagate the change to the parent. This will recreate all the child records - see below.
        auto masterField = fieldManager(this)->getField(masterFieldUid);
        auto masterFieldElementUids = record->getFieldValue(masterFieldUid).toMap().keys();

        // Cleanup old records.
        auto oldRecordUids = oldValue.toStringList();
        for (auto it = oldRecordUids.constBegin(); it != oldRecordUids.constEnd(); it++)
        {
            auto oldRecordUid = *it;
            auto oldRecord = getRecord(oldRecordUid);
            auto oldRecordElementUid = oldRecord->nameElementUid();
            if (masterFieldElementUids.contains(oldRecordElementUid))
            {
                // We already have a record for this, so skip it.
                masterFieldElementUids.removeOne(oldRecord->nameElementUid());
            }
            else
            {
                // Detach the record and request the garbage collector clean up later.
                detachRecord(oldRecord);
                *garbageCollectOut = true;
            }
        }

        // Create new records.
        for (auto it = masterFieldElementUids.constBegin(); it != masterFieldElementUids.constEnd(); it++)
        {
            auto newRecordUid = newRecord(recordUid, fieldUid);
            auto newRecord = getRecord(newRecordUid);
            newRecord->set_nameElementUid(*it);
        }

        // Order the records according to the element list order.
        auto elementUids = elementManager(this)->getLeafElementUids(masterField->listElementUid());
        auto newRecordUids = record->getFieldValue(fieldUid).toStringList();
        std::sort(newRecordUids.begin(), newRecordUids.end(), [&](const QString& recordUid1, const QString& recordUid2) -> bool
        {
            auto record1 = getRecord(recordUid1);
            auto record2 = getRecord(recordUid2);
            auto elementUid1 = record1->nameElementUid();
            auto elementUid2 = record2->nameElementUid();

            return elementUids.indexOf(elementUid1) < elementUids.indexOf(elementUid2);
        });

        // Update if changed.
        if (oldRecordUids != newRecordUids)
        {
            record->setFieldValue(fieldUid, newRecordUids);
            return true;
        }
    }

    // No change.
    return false;
}
