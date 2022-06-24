#include "pch.h"
#include "Sighting.h"
#include "Form.h"
#include "App.h"

namespace
{
    // Global dependencies.
    TimeManager* timeManager()
    {
        return App::instance()->timeManager();
    }

    // Form dependencies.
    FieldManager* fieldManager(const Sighting* s)
    {
        return qobject_cast<Form*>(s->parent())->fieldManager();
    }

    ElementManager* elementManager(const Sighting* s)
    {
        return qobject_cast<Form*>(s->parent())->elementManager();
    }

    const QVariantMap* formGlobals(const Sighting* s)
    {
        return qobject_cast<Form*>(s->parent())->globals();
    }

    QString formUsername(const Sighting* s)
    {
        auto result = App::instance()->settings()->username();
        if (result.isEmpty())
        {
            result = qobject_cast<Form*>(s->parent())->project()->username();
        }

        return result;
    }
}

Sighting::Sighting(QObject* parent): QObject(parent)
{
    qFatalIf(true, "Must be created by Form");
}

Sighting::Sighting(const QVariantMap& data, QObject* parent): QObject(parent)
{
    m_recordManager = new RecordManager(this);

    load(data);

    // Remove references to this record in the control states.
    connect(m_recordManager, &RecordManager::recordDeleted, this, [&](const QString& recordUid)
    {
        auto controlStates = m_controlStates;
        for (auto it = controlStates.keyBegin(); it != controlStates.keyEnd(); it++)
        {
            if (it->startsWith(recordUid))
            {
                m_controlStates.remove(*it);
            }
        }
    });
}

Sighting::~Sighting()
{
}

void Sighting::reset()
{
    m_recordManager->reset();

    m_variables.clear();
    m_controlStates.clear();
    m_pageStack.clear();
    m_wizardRecordUid.clear();
    m_wizardFieldUids.clear();
    m_wizardPageStack.clear();

    m_deviceId = m_username = "";
    m_createTime = m_updateTime = "";
}

RecordManager* Sighting::recordManager()
{
    return m_recordManager;
}

QString Sighting::rootRecordUid() const
{
    return m_recordManager->rootRecordUid();
}

Record* Sighting::getRecord(const QString& recordUid) const
{
    return m_recordManager->getRecord(recordUid);
}

bool Sighting::isValid()
{
    return m_recordManager->rootRecord()->isValid();
}

QString Sighting::deviceId() const
{
    return !m_deviceId.isEmpty() ? m_deviceId : App::instance()->settings()->deviceId();
}

QString Sighting::username() const
{
    return !m_username.isEmpty() ? m_username : formUsername(this);
}

QString Sighting::createTime() const
{
    return m_createTime;
}

QString Sighting::updateTime() const
{
    return m_updateTime;
}

QVariantList Sighting::pageStack() const
{
    return m_pageStack;
}

void Sighting::set_pageStack(const QVariantList& value)
{
    m_pageStack = value;
}

QString Sighting::wizardRecordUid() const
{
    return m_wizardRecordUid;
}

void Sighting::set_wizardRecordUid(const QString& value)
{
    m_wizardRecordUid = value;
}

QStringList Sighting::wizardFieldUids() const
{
    return m_wizardFieldUids;
}

void Sighting::set_wizardFieldUids(const QStringList& value)
{
    m_wizardFieldUids = value;
}

QVariantList Sighting::wizardPageStack() const
{
    return m_wizardPageStack;
}

void Sighting::set_wizardPageStack(const QVariantList& value)
{
    m_wizardPageStack = value;
}

void Sighting::snapCreateTime(const QString& now)
{
    m_createTime = now.isEmpty() ? timeManager()->currentDateTimeISO() : now;
}

void Sighting::snapUpdateTime(const QString& now)
{
    m_updateTime = now.isEmpty() ? timeManager()->currentDateTimeISO() : now;
}

QVariantMap Sighting::save(QStringList* attachmentsOut) const
{
    return QVariantMap
    {
        { "version", 1 },
        { "deviceId", deviceId() },
        { "username", username() },
        { "createTime", m_createTime },
        { "updateTime", m_updateTime },
        { "variables", m_variables },
        { "records", m_recordManager->save(false, attachmentsOut) },
        { "controlStates", m_controlStates },
        { "pages", m_pageStack },
        { "wizardRecordUid", m_wizardRecordUid },
        { "wizardFieldUids", m_wizardFieldUids },
        { "wizardPages", m_wizardPageStack },
    };
}

QVariantMap Sighting::saveForExport(QStringList* attachmentsOut) const
{
    return QVariantMap
    {
        { "version", 1 },
        { "deviceId", deviceId() },
        { "username", username() },
        { "createTime", m_createTime },
        { "updateTime", m_updateTime },
        { "variables", m_variables },
        { "records", m_recordManager->save(true, attachmentsOut) }
    };
}

void Sighting::load(const QVariantMap& data)
{
    if (data.isEmpty())
    {
        reset();
        return;
    }

    int version = data.value("version", 1).toInt();
    Q_UNUSED(version)

    m_deviceId = data.value("deviceId").toString();
    m_username = data.value("username").toString();

    // Handle legacy time format.
    auto legacyTime = [&](const QVariant& t) -> QString
    {
        if (t.userType() == QMetaType::QString)
        {
            return t.toString();
        }

        auto l = t.toLongLong();
        return (l == 0 || l == -1) ? "" : timeManager()->formatDateTime(l);
    };

    m_createTime = legacyTime(data["createTime"]);
    m_updateTime = legacyTime(data["updateTime"]);

    // Load everything else.
    m_controlStates = data["controlStates"].toMap();
    m_variables = data["variables"].toMap();

    m_pageStack = data["pages"].toList();
    m_wizardRecordUid = data["wizardRecordUid"].toString();
    m_wizardFieldUids = data["wizardFieldUids"].toStringList();
    m_wizardPageStack = data["wizardPages"].toList();

    // Load records: legacy format is serialized JSON.
    if (data["records"].userType() == QMetaType::QByteArray)
    {
        auto jsonDocument = QJsonDocument::fromJson(data["records"].toByteArray());
        qFatalIf(!jsonDocument.isArray(), "Records should be an array");
        m_recordManager->load(jsonDocument.array().toVariantList());
    }
    else
    {
        m_recordManager->load(data["records"].toList());
    }
}

QString Sighting::toXml(const QString& formId, const QString& formVersion, QStringList* attachmentsOut) const
{
    auto xmlStream = QByteArray();
    QXmlStreamWriter xmlWriter(&xmlStream);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.setCodec("UTF-8");

    xmlWriter.writeStartElement(formId);
    xmlWriter.writeNamespace("http://openrosa.org/javarosa", "jr");
    xmlWriter.writeNamespace("http://www.opendatakit.org/xforms", "odk");
    xmlWriter.writeNamespace("http://openrosa.org/xforms", "orx");
    xmlWriter.writeAttribute("id", formId);

    if (!formVersion.isEmpty())
    {
        xmlWriter.writeAttribute("version", formVersion);
    }

    xmlWriter.writeStartElement("meta");
    xmlWriter.writeTextElement("instanceID", "uuid:" + rootRecordUid());
    xmlWriter.writeEndElement();

    auto flattenRecord = [&](const Record& record) -> bool
    {
        return record.recordUid() == rootRecordUid() || record.recordField()->getTagValue("flattenXml").toBool();
    };

    m_recordManager->enumFieldValues(
        rootRecordUid(), true,
        [&](const Record& record, bool* /*skipRecordOut*/) // recordBeginCallback.
        {
            if (!flattenRecord(record))
            {
                xmlWriter.writeStartElement(record.recordField()->safeExportUid());
            }
        },
        [&](const FieldValue& fieldValue) // fieldValueCallback.
        {
            if (!fieldValue.isRecordField())
            {
                attachmentsOut->append(fieldValue.attachments());
                xmlWriter.writeTextElement(fieldValue.exportUid(), fieldValue.xmlValue());
            }
        },
        [&](const Record& record) // recordEndCallback.
        {
            if (!flattenRecord(record))
            {
                xmlWriter.writeEndElement();
            }
        }
    );

    xmlWriter.writeEndElement();

    return QString(xmlStream);
}

QVariant Sighting::getRootFieldValue(const QString& fieldUid, const QVariant& defaultValue) const
{
    return m_recordManager->rootRecord()->getFieldValue(fieldUid, defaultValue);
}

void Sighting::setRootFieldValue(const QString& fieldUid, const QVariant& value)
{
    m_recordManager->rootRecord()->setFieldValue(fieldUid, value);
}

void Sighting::resetRootFieldValue(const QString& fieldUid)
{
    m_recordManager->rootRecord()->resetFieldValue(fieldUid);
}

QVariant Sighting::getControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& defaultValue) const
{
    return m_controlStates.value(recordUid + '.' + fieldUid + '.' + name, defaultValue);
}

void Sighting::setControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& value)
{
    auto finalName = recordUid + '.' + fieldUid + '.' + name;

    if (value.isValid())
    {
        m_controlStates[finalName] = value;
    }
    else
    {
        m_controlStates.remove(finalName);
    }
}

QVariant Sighting::getVariable(const QString& name, const QVariant& defaultValue) const
{
    return m_variables.value(name, defaultValue);
}

void Sighting::setVariable(const QString& name, const QVariant& value)
{
    if (value.isValid())
    {
        m_variables[name] = value;
    }
    else
    {
        m_variables.remove(name);
    }
}

QVariantMap Sighting::location() const
{
    auto result = QVariantMap();

    m_recordManager->rootRecord()->enumFieldValues([&](const FieldValue& fieldValue, bool* stopOut)
    {
        if (!fieldValue.isRelevant() || !fieldValue.isValid() || fieldValue.isRecordField())
        {
            return;
        }

        if (qobject_cast<LocationField*>(fieldValue.field()))
        {
            result = fieldValue.value().toMap();
            *stopOut = true;
        }
    });

    return result;
}

QString Sighting::summary() const
{
    auto result = QStringList();
    auto titleMode = false;

    m_recordManager->enumFieldValues(
        rootRecordUid(), true,
        nullptr,
        [&](const FieldValue& fieldValue)
        {
            if (!fieldValue.isVisible())
            {
                return;
            }

            auto title = fieldValue.field()->parameters().contains("title");
            if (!titleMode && title)
            {
                result.clear();
                titleMode = true;
            }

            if (titleMode && !title)
            {
                return;
            }

            auto displayValue = fieldValue.displayValue();
            if (displayValue.isEmpty())
            {
                return;
            }

            result.append(displayValue);
        },
        nullptr
    );

    return result.join(", ");
}

QVariantMap Sighting::symbols() const
{
    return QVariantMap
    {
        { "version", fieldManager(this)->rootField()->version() },
        { "createTime", m_createTime },
        { "updateTime", m_updateTime },
        { "deviceId", deviceId() },
        { "username", username() },
    };
}

QVariantMap Sighting::controlStates() const
{
    return m_controlStates;
}

QVariantMap Sighting::variables() const
{
    return m_variables;
}

void Sighting::regenerateUids()
{
    QMap<QString, QString> uidMap;

    // Create a mapping from old uid to new uid.
    auto recordUids = m_recordManager->recordUids();
    for (auto it = recordUids.constBegin(); it != recordUids.constEnd(); it++)
    {
        uidMap[*it] = QUuid::createUuid().toString(QUuid::Id128);
    }

    auto lookup = [&](const QString& recordUid) -> QString
    {
        qFatalIf(!uidMap.contains(recordUid), "Bad record id");
        return uidMap[recordUid];
    };

    // Adjust all records.
    m_recordManager->remapRecordUids(lookup);

    // Adjust the wizard.
    if (!m_wizardRecordUid.isEmpty())
    {
        m_wizardRecordUid = lookup(m_wizardRecordUid);
    }

    for (auto i = 0; i < m_wizardPageStack.count(); i++)
    {
        auto pageParams = m_wizardPageStack[i].toMap();
        pageParams["recordUid"] = lookup(pageParams["recordUid"].toString());
        m_wizardPageStack.replace(i, pageParams);
    }

    // Adjust control states.
    auto controlStates = m_controlStates;
    for (auto it = controlStates.constKeyValueBegin(); it != controlStates.constKeyValueEnd(); it++)
    {
        auto keyStrings = it->first.split(".");
        if (keyStrings.isEmpty())
        {
            continue;
        }

        if (uidMap.contains(keyStrings.constFirst()))
        {
            keyStrings.replace(0, lookup(keyStrings.constFirst()));
            m_controlStates.remove(it->first);
            m_controlStates.insert(keyStrings.join('.'), it->second);
        }
    }

    // Reset the timestamps.
    m_createTime = m_updateTime = "";
}

void Sighting::recalculate(FieldValueChange* fieldValueChange)
{
    auto emitChanges = this->receivers(SIGNAL(recalculated(const FieldValueChanges&, const FieldValueChanges&))) > 0;

    auto fieldValueChanges = FieldValueChanges();
    if (fieldValueChange)
    {
        fieldValueChanges.append(*fieldValueChange);
    }

    auto symbols = this->symbols();
    auto finalVars = *::formGlobals(this);
    finalVars.insert(m_variables);

    m_recordManager->recalculate(&fieldValueChanges, &symbols, &finalVars, emitChanges);

    if (emitChanges)
    {
        // Compute records changed.
        auto recordChildValueChanges = FieldValueChanges();
        auto sentRecordHash = QHash<QString, bool>();
        for (auto it = fieldValueChanges.constBegin(); it != fieldValueChanges.constEnd(); it++)
        {
            for (auto currRecordUid = it->recordUid; !currRecordUid.isEmpty(); )
            {
                auto record = m_recordManager->getRecord(currRecordUid);
                auto changedRecordUid = record->parentRecordUid();
                auto changedFieldUid = record->recordFieldUid();

                auto hashId = changedRecordUid + changedFieldUid;

                if (!sentRecordHash.contains(hashId))
                {
                    sentRecordHash.insert(hashId, true);
                    recordChildValueChanges.append(FieldValueChange { changedRecordUid, changedFieldUid });
                }

                currRecordUid = changedRecordUid;
            }
        }

        // Emit the event with all the changes.
        emit recalculated(fieldValueChanges, recordChildValueChanges);
    }
}

QVariant Sighting::evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid, const QVariantMap* variables) const
{
    auto symbols = this->symbols();

    auto finalVars = *::formGlobals(this);
    finalVars.insert(m_variables);
    if (variables)
    {
        finalVars.insert(*variables);
    }

    return m_recordManager->evaluate(expression, contextRecordUid, contextFieldUid, &symbols, &finalVars);
}

void Sighting::buildRecordModel(QVariantList* recordModel, const std::function<bool(const FieldValue& fieldValue, int depth)>& filter) const
{
    auto depth = -1;
    auto elementManager = ::elementManager(this);

    m_recordManager->enumFieldValues(rootRecordUid(), true,
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
                        { "fieldName", fieldValue.name() },
                        { "fieldValue", fieldValue.displayValue() },
                        { "recordUid", fieldValue.recordUid() },
                        { "depth", depth },
                        { "group", RecordField::fromField(field)->group() }
                    };

                    if (!field->sectionElementUid().isEmpty())
                    {
                        map["sectionName"] = elementManager->getElementName(field->sectionElementUid());
                    }

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

            // Handle LocationField.
            auto locationField = qobject_cast<LocationField*>(field);
            if (locationField)
            {
                recordModel->append(QVariantMap {
                    { "delegate", "location" },
                    { "fieldName", fieldValue.name() },
                    { "fieldValue", fieldValue.displayValue() },
                    { "value", fieldValue.value() },
                    { "recordUid", fieldValue.recordUid() },
                    { "depth", depth } });

                return;
            }

            // Handle PhotoField.
            auto photoField = qobject_cast<PhotoField*>(field);
            if (photoField)
            {
                auto photoCount = fieldValue.value().toList().count();
                if (photoCount > 0)
                {
                    recordModel->append(QVariantMap {
                        { "delegate", "photos" },
                        { "fieldName", fieldValue.name() },
                        { "fieldValue", QString::number(photoCount) +  " " + (photoCount == 1 ? tr("photo") : tr("photos")) },
                        { "filenames", fieldValue.value() },
                        { "recordUid", fieldValue.recordUid() },
                        { "depth", depth } });
                }

                return;
            }

            // Handle all other fields.
            auto displayValue = fieldValue.displayValue();
            if (!displayValue.isEmpty())
            {
                auto map = QVariantMap {
                    { "delegate", "field" },
                    { "fieldName", fieldValue.name() },
                    { "fieldValue", displayValue },
                    { "recordUid", fieldValue.recordUid() },
                    { "depth", depth } };

                if (!field->sectionElementUid().isEmpty())
                {
                    map["sectionName"] = elementManager->getElementName(field->sectionElementUid());
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

QString Sighting::newRecord(const QString& parentRecordUid, const QString& recordFieldUid)
{
    auto oldValue = m_recordManager->getRecord(parentRecordUid)->getFieldValue(recordFieldUid);
    auto recordUid = m_recordManager->newRecord(parentRecordUid, recordFieldUid);

    auto fieldValueChange = FieldValueChange { parentRecordUid, recordFieldUid, oldValue };
    recalculate(&fieldValueChange);

    return recordUid;
}

void Sighting::deleteRecord(const QString& recordUid)
{
    auto record = getRecord(recordUid);
    auto parentRecordUid = record->parentRecordUid();
    auto recordFieldUid = record->recordFieldUid();

    auto oldValue = m_recordManager->getRecord(parentRecordUid)->getFieldValue(recordFieldUid);

    m_recordManager->deleteRecord(recordUid);

    auto fieldValueChange = FieldValueChange { parentRecordUid, recordFieldUid, oldValue };
    recalculate(&fieldValueChange);
}

QVariant Sighting::getFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& defaultValue) const
{
    return getRecord(recordUid)->getFieldValue(fieldUid, defaultValue);
}

void Sighting::setFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& value)
{
    auto field = fieldManager(this)->getField(fieldUid);
    qFatalIf(!field, "Field not found: " + fieldUid);
    qFatalIf(field->isRecordField(), "RecordFields are system managed");

    auto oldValue = m_recordManager->getRecord(recordUid)->getFieldValue(fieldUid);

    if (oldValue != value)
    {
        auto record = getRecord(recordUid);
        record->setFieldState(fieldUid, FieldState::UserSet);
        record->setFieldValue(fieldUid, value);

        auto fieldValueChange = FieldValueChange { recordUid, fieldUid, oldValue };
        recalculate(&fieldValueChange);
    }
}

void Sighting::resetFieldValue(const QString& recordUid, const QString& fieldUid)
{
    auto record = getRecord(recordUid);
    auto oldValue = record->getFieldValue(fieldUid);
    record->resetFieldValue(fieldUid);

    // Cleanup orphaned child records if the field being reset is RecordField.
    if (fieldManager(this)->getField(fieldUid)->isRecordField())
    {
        m_recordManager->garbageCollect();
    }

    auto fieldValueChange = FieldValueChange { recordUid, fieldUid, oldValue };
    recalculate(&fieldValueChange);
}

void Sighting::resetRecord(const QString& recordUid)
{
    // Reset in order so that side effects are predictable.
    getRecord(recordUid)->recordField()->enumFields([&](BaseField* field)
    {
        resetFieldValue(recordUid, field->uid());
    });
}
