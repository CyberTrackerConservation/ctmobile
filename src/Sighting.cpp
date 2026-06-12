#include "pch.h"
#include "Sighting.h"
#include "TrackFile.h"
#include "App.h"

namespace CyberTracker {

Sighting::Sighting(QObject* parent): QObject(parent)
{
    qFatalIf(true, "Must be created by Form");
}

Sighting::Sighting(const QVariantMap& data, const QString& stateSpace, Schema* schema, const QString& languageCode, QObject* parent): QObject(parent)
{
    m_stateSpace = stateSpace;

    m_recordManager = new RecordManager(schema, languageCode, this);

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

void Sighting::setNote(const QString& projectUid, const QString& sightingUid, const QString& note)
{
    auto database = App::instance()->database();
    auto dbSighting = Database::DbData();
    database->loadData(projectUid, sightingUid, &dbSighting);
    dbSighting.data["note"] = note;
    database->saveData(projectUid, sightingUid, dbSighting);
}

void Sighting::reset()
{
    m_recordManager->reset();

    m_variables.clear();
    m_controlStates.clear();
    m_pageStack.clear();
    m_wizardRecordUid.clear();
    m_wizardRules.clear();
    m_wizardPageStack.clear();

    m_deviceId = m_sessionId = m_username = "";
    m_createTime = m_updateTime = "";
    m_trackFile.clear();
    m_classicData.clear();
    m_classicBlob.clear();
}

QString Sighting::stateSpace() const
{
    return m_stateSpace;
}

QString Sighting::schemaHash() const
{
    return m_schemaHash;
}

void Sighting::set_schemaHash(const QString& value)
{
    m_schemaHash = value;
}

Schema* Sighting::schema() const
{
    return m_recordManager->schema();
}

void Sighting::set_schema(Schema* value)
{
    m_recordManager->set_schema(value);
}

QString Sighting::languageCode() const
{
    return m_recordManager->languageCode();
}

void Sighting::set_languageCode(const QString& value)
{
    m_recordManager->set_languageCode(value);
}

QVariantMap Sighting::iconHashes() const
{
    return m_iconHashes;
}

void Sighting::set_iconHashes(const QVariantMap& value)
{
    m_iconHashes = value;
}

QString Sighting::getIconHash(const QString& icon) const
{
    if (!icon.isEmpty() && m_iconHashes.contains(icon))
    {
        return m_iconHashes.value(icon).toString();
    }

    return icon;
}

QString Sighting::getFieldIconHash(const QString& fieldUid) const
{
    return getIconHash(schema()->getFieldIcon(fieldUid));
}

QString Sighting::getElementIconHash(const QString& elementUid) const
{
    return getIconHash(schema()->getElementIcon(elementUid));
}

bool Sighting::trackOnly() const
{
    return m_trackOnly;
}

void Sighting::set_trackOnly(bool value)
{
    m_trackOnly = value;
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
    return m_deviceId;
}

void Sighting::set_deviceId(const QString& deviceId)
{
    m_deviceId = deviceId;
}

QString Sighting::username() const
{
    return m_username;
}

void Sighting::set_username(const QString& username)
{
    m_username = username;
}

QString Sighting::sessionId() const
{
    return m_sessionId;
}

void Sighting::set_sessionId(const QString& sessionId)
{
    m_sessionId = sessionId;
}

QString Sighting::createTime() const
{
    return m_createTime;
}

void Sighting::set_createTime(const QString& createTime)
{
    m_createTime = createTime;
}

QString Sighting::updateTime() const
{
    return m_updateTime;
}

void Sighting::set_updateTime(const QString& updateTime)
{
    m_updateTime = updateTime;
}

QVariantList Sighting::pageStack() const
{
    return m_pageStack;
}

void Sighting::set_pageStack(const QVariantList& value)
{
    m_pageStack = value;
}

QString Sighting::lastPageUrl() const
{
    return m_pageStack.isEmpty() ? "" : m_pageStack.constLast().toMap().value("pageUrl").toString();
}

bool Sighting::immersive() const
{
    return m_immersive;
}

void Sighting::set_immersive(bool value)
{
    m_immersive = value;
}

QString Sighting::wizardRecordUid() const
{
    return m_wizardRecordUid;
}

void Sighting::set_wizardRecordUid(const QString& value)
{
    m_wizardRecordUid = value;
}

QVariantMap Sighting::wizardRules() const
{
    return m_wizardRules;
}

void Sighting::set_wizardRules(const QVariantMap& value)
{
    m_wizardRules = value;
}

QVariantList Sighting::wizardPageStack() const
{
    return m_wizardPageStack;
}

void Sighting::set_wizardPageStack(const QVariantList& value)
{
    m_wizardPageStack = value;
}

QString Sighting::note() const
{
    return m_note;
}

void Sighting::set_note(const QString& value)
{
    m_note = value;
}

QVariantList Sighting::classicData() const
{
    return m_classicData;
}

void Sighting::set_classicData(const QVariantList& value)
{
    m_classicData = value;
}

QByteArray Sighting::classicBlob() const
{
    return m_classicBlob;
}

void Sighting::set_classicBlob(const QByteArray& value)
{
    m_classicBlob = value;
}

QVariantMap Sighting::save(QStringList* attachmentsOut) const
{
    if (attachmentsOut)
    {
        // Append track file.
        if (!m_trackFile.isEmpty())
        {
            attachmentsOut->append(m_trackFile);
        }

        // Append classic media.
        for (auto attributeIt = m_classicData.constBegin(); attributeIt != m_classicData.constEnd(); attributeIt++)
        {
            auto attachment = attributeIt->toMap().value("attachment").toString();
            if (!attachment.isEmpty())
            {
                attachmentsOut->append(attachment);
            }
        }
    }

    return QVariantMap
    {
        { "version", 1 },
        { "schemaHash", m_schemaHash },
        { "iconHashes", m_iconHashes },
        { "deviceId", m_deviceId },
        { "username", m_username },
        { "sessionId", m_sessionId },
        { "trackOnly", m_trackOnly },
        { "createTime", m_createTime },
        { "updateTime", m_updateTime },
        { "globals", m_globals },
        { "variables", m_variables },
        { "trackFile", m_trackFile },
        { "records", m_recordManager->save(false, attachmentsOut) },
        { "controlStates", m_controlStates },
        { "pages", m_pageStack },
        { "immersive", m_immersive },
        { "wizardRecordUid", m_wizardRecordUid },
        { "wizardRules", m_wizardRules },
        { "wizardPages", m_wizardPageStack },
        { "note", m_note },
        { "classicData", m_classicData },
        { "classicBlob", m_classicBlob }
    };
}

QVariantMap Sighting::saveForExport(QStringList* attachmentsOut) const
{
    attachmentsOut && !m_trackFile.isEmpty() ? attachmentsOut->append(m_trackFile) : void();

    return QVariantMap
    {
        { "version", 1 },
        { "schemaHash", m_schemaHash },
        { "iconHashes", m_iconHashes },
        { "deviceId", m_deviceId },
        { "username", m_username },
        { "sessionId", m_sessionId },
        { "trackOnly", m_trackOnly },
        { "createTime", m_createTime },
        { "updateTime", m_updateTime },
        { "globals", m_globals },
        { "variables", m_variables },
        { "trackFile", m_trackFile },
        { "records", m_recordManager->save(true, attachmentsOut) },
        { "note", m_note },
        { "classicData", m_classicData },
        { "classicBlob", m_classicBlob }
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

    m_schemaHash = data.value("schemaHash").toString();
    m_iconHashes = data.value("iconHashes").toMap();
    m_deviceId = data.value("deviceId").toString();
    m_username = data.value("username").toString();
    m_sessionId = data.value("sessionId").toString();
    m_trackOnly = data.value("trackOnly").toBool();

    // Handle legacy time format.
    auto legacyTime = [&](const QVariant& t) -> QString
    {
        if (t.userType() == QMetaType::QString)
        {
            return t.toString();
        }

        auto l = t.toLongLong();
        return (l == 0 || l == -1) ? "" : App::instance()->timeManager()->formatDateTime(l);
    };

    m_createTime = legacyTime(data["createTime"]);
    m_updateTime = legacyTime(data["updateTime"]);

    // Load everything else.
    m_controlStates = data["controlStates"].toMap();
    m_globals = data["globals"].toMap();
    m_variables = data["variables"].toMap();
    m_trackFile = data["trackFile"].toString();

    m_pageStack = data["pages"].toList();
    m_immersive = data["immersive"].toBool();
    m_wizardRecordUid = data["wizardRecordUid"].toString();
    m_wizardRules = data["wizardRules"].toMap();
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

    m_note = data["note"].toString();
    m_classicData = data["classicData"].toList();
    m_classicBlob = data["classicBlob"].toByteArray();
}

QString Sighting::toXml(const QString& formId, const QString& formVersion, const QString& formhubUuid, QStringList* attachmentsOut) const
{
    auto xmlStream = QByteArray();
    QXmlStreamWriter xmlWriter(&xmlStream);
    xmlWriter.setAutoFormatting(true);

    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("Data");
    xmlWriter.writeNamespace("http://openrosa.org/javarosa", "jr");
    xmlWriter.writeNamespace("http://www.opendatakit.org/xforms", "odk");
    xmlWriter.writeNamespace("http://openrosa.org/xforms", "orx");
    xmlWriter.writeAttribute("id", formId);

    if (!formVersion.isEmpty())
    {
        xmlWriter.writeAttribute("version", formVersion);
    }

    if (!formhubUuid.isEmpty())
    {
        xmlWriter.writeStartElement("formhub");
        xmlWriter.writeTextElement("uuid", formhubUuid);
        xmlWriter.writeEndElement();
    }

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

    if (!formVersion.isEmpty())
    {
        xmlWriter.writeTextElement("__version__", formVersion);
    }

    xmlWriter.writeStartElement("meta");
    xmlWriter.writeTextElement("deviceID", "uuid:" + App::instance()->settings()->deviceId());
    xmlWriter.writeTextElement("instanceID", "uuid:" + rootRecordUid());
    xmlWriter.writeEndElement();

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

QVariantMap Sighting::globals() const
{
    return m_globals;
}

void Sighting::set_globals(const QVariantMap& value)
{
    m_globals = value;
}

QVariantMap Sighting::locationMap() const
{
    // Prefer the snapLocationField.
    auto fieldUid = getSnapLocationFieldUid();

    // If not found, search for the first location field off the root.
    if (fieldUid.isEmpty())
    {
        m_recordManager->rootRecord()->enumFieldValues([&](const FieldValue& fieldValue, bool* stopOut)
        {
            if (qobject_cast<LocationField*>(fieldValue.field()) == nullptr)
            {
                return;
            }

            fieldUid = fieldValue.fieldUid();
            *stopOut = true;
        });
    }

    return !fieldUid.isEmpty() ? m_recordManager->rootRecord()->getFieldValue(fieldUid).toMap() : QVariantMap();
}

std::unique_ptr<Location> Sighting::locationPtr() const
{
    return std::unique_ptr<Location>(new Location(locationMap(), (QObject*)this));
}

bool Sighting::findAudioField(const QString& filename, QString* recordUidOut, QString* fieldUidOut)
{
    auto result = false;

    m_recordManager->enumFieldValues(rootRecordUid(), true,
        nullptr,
        [&](const FieldValue& fieldValue)
        {
            if (result)
            {
                return;
            }

            auto audioField = qobject_cast<AudioField*>(fieldValue.field());
            if (!audioField)
            {
                return;
            }

            result = AudioField::getFilename(fieldValue.value()) == filename;
            if (result)
            {
                *recordUidOut = fieldValue.recordUid();
                *fieldUidOut = fieldValue.fieldUid();
            }
        },
        nullptr);

    return result;
}

QVariantList Sighting::geoshapes() const
{
    auto result = QVariantList();

    if (!m_trackFile.isEmpty())
    {
        auto points = TrackFile(App::instance()->getMediaFilePath(m_trackFile)).getPoints();
        result.append(QVariantMap {{ "type", "track" }, { "value", points }});
    }

    m_recordManager->enumFieldValues(rootRecordUid(), true,
        nullptr,
        [&](const FieldValue& fieldValue)
        {
            auto field = fieldValue.field();

            if (qobject_cast<LocationField*>(field))
            {
                auto location = fieldValue.value().toMap();
                if (!location.isEmpty())
                {
                    result.append(QVariantMap {{ "type", "point" }, { "value", location }});
                }

                return;
            }

            if (qobject_cast<LineField*>(field))
            {
                auto points = fieldValue.value().toList();
                if (!points.isEmpty())
                {
                    result.append(QVariantMap {{ "type", "line" }, { "value", points }});
                }
                return;
            }

            if (qobject_cast<AreaField*>(field))
            {
                auto points = fieldValue.value().toList();
                if (!points.isEmpty())
                {
                    result.append(QVariantMap {{ "type", "area" }, { "value", points }});
                }

                return;
            }
        },
        nullptr);

    return result;
}

QString Sighting::summaryText(bool fieldNames) const
{
    auto result = QStringList();

    auto summaryValue = getFieldParameter(rootRecordUid(), "", "summaryText").toString().trimmed();
    auto summaryFields = QStringList();
    if (!summaryValue.isEmpty())
    {
        summaryFields = summaryValue.split(' ');
        summaryFields.removeAll(" ");
        for (auto i = 0; i < summaryFields.count(); i++)
        {
            result.append("");
        }
    }

    m_recordManager->enumFieldValues(
        rootRecordUid(), true,
        nullptr,
        [&](const FieldValue& fieldValue)
        {
            // Skip invisible and unnamed fields.
            if (!fieldValue.isVisible() || !fieldValue.isNamed())
            {
                return;
            }

            // Skip location field, since it does not make a good summary field.
            if (qobject_cast<LocationField*>(fieldValue.field()))
            {
                return;
            }

            // Classic should display "true" and null strings as their field name.
            auto stringField = qobject_cast<StringField*>(fieldValue.field());
            auto value = fieldValue.value();
            if (stringField && stringField->decodeValue() && value.isValid() && (value == "true" || value == "1" || value.toString().trimmed().isEmpty()))
            {
                result.append(fieldValue.name());
                return;
            }

            auto displayValue = fieldValue.displayValue();
            if (displayValue.isEmpty())
            {
                return;
            }

            // If no summary fields are specified, just append the value.
            auto finalValue = fieldNames ? QString("%1 (%2)").arg(fieldValue.name(), displayValue) : displayValue;
            if (summaryFields.isEmpty())
            {
                result.append(finalValue);
                return;
            }

            // Place the summary in the specified index.
            auto index = summaryFields.indexOf(fieldValue.fieldUid());
            if (index != -1)
            {
                result[index] = finalValue;
                return;
            }
        },
        nullptr
    );

    result.removeAll(" ");
    result.removeAll("");

    return result.join(", ");
}

QString Sighting::summaryIcon() const
{
    auto elementUid = QString();

    auto summaryValue = getFieldParameter(rootRecordUid(), "", "summaryIcon").toString().trimmed();
    auto summaryFields = QStringList();
    if (!summaryValue.isEmpty())
    {
        summaryFields = summaryValue.split(' ');
        summaryFields.removeAll(" ");
    }

    m_recordManager->enumFieldValues(
        rootRecordUid(), true,
        nullptr,
        [&](const FieldValue& fieldValue)
        {
            // Already found it.
            if (!elementUid.isEmpty())
            {
                return;
            }

            // Skip invisible and unnamed fields.
            if (!fieldValue.isVisible() || !fieldValue.isNamed())
            {
                return;
            }

            auto index = summaryFields.indexOf(fieldValue.fieldUid());
            if (index == -1)
            {
                return;
            }

            // Pick up the result of single select fields.
            auto field = fieldValue.field();
            if (qobject_cast<StringField*>(field) && !field->listElementUid().isEmpty() && !fieldValue.value().toString().isEmpty())
            {
                auto element = schema()->getElement(fieldValue.value().toString());
                if (element != nullptr && !element->icon().isEmpty())
                {
                    elementUid = element->uid();
                    return;
                }
            }

            // Pick up the fields icon itself.
            auto nameElementUid = field->nameElementUid();
            if (!nameElementUid.isEmpty())
            {
                auto element = schema()->getElement(nameElementUid);
                if (element != nullptr && !element->icon().isEmpty())
                {
                    elementUid = element->uid();
                    return;
                }
            }
        },
        nullptr
    );

    return elementUid.isEmpty() ? QString() : schema()->getElementIcon(elementUid);
}

QString Sighting::statusIcon(Database::DataFlags flags) const
{
    if (flags.testFlag(Database::DataFlag::DB_TRANSCRIBING_FLAG))
    {
        return "qrc:/icons/transcribe.svg";
    }
    else if (flags.testFlag(Database::DataFlag::DB_SUBMITTED_FLAG))
    {
        return "qrc:/icons/cloud_upload_outline.svg";
    }
    else if (flags.testFlag(Database::DataFlag::DB_COMPLETED_FLAG))
    {
        return "qrc:/icons/check_bold.svg";
    }

    return "qrc:/icons/dots_horizontal.svg";
}

QStringList Sighting::referencedIcons() const
{
    auto result = QStringList();

    m_recordManager->enumFieldValues(
        rootRecordUid(), true,
        nullptr,
        [&](const FieldValue& fieldValue)
        {
            auto state = fieldValue.state();
            if (state != FieldState::UserSet && state != FieldState::Constant)
            {
                return;
            }

            // Append field icon.
            auto fieldIcon = schema()->getFieldIcon(fieldValue.fieldUid());
            if (!fieldIcon.isEmpty())
            {
                result.append(fieldIcon);
            }

            // Append value icons.
            auto valueElementUids = QStringList();
            fieldValue.field()->appendElementUids(fieldValue.value(), &valueElementUids);
            for (auto elementUidIt = valueElementUids.constBegin(); elementUidIt != valueElementUids.constEnd(); elementUidIt++)
            {
                auto valueIcon = schema()->getElementIcon(*elementUidIt);
                if (!valueIcon.isEmpty())
                {
                    result.append(valueIcon);
                }
            }
        },
        nullptr
        );

    return result;
}

QStringList Sighting::archiveFiles() const
{
    auto result = QStringList { m_schemaHash };
    for (auto iconIt = m_iconHashes.constKeyValueBegin(); iconIt != m_iconHashes.constKeyValueEnd(); iconIt++)
    {
        result.append(iconIt->second.toString());
    }

    return result;
}

QString Sighting::searchText() const
{
    auto schema = m_recordManager->schema();
    auto strings = QStringList { schema->title(), schema->subtitle(), m_username, m_note };

    m_recordManager->enumFieldValues(
        rootRecordUid(), true,
        nullptr,
        [&](const FieldValue& fieldValue) // fieldValueCallback.
        {
            strings.append(fieldValue.name());
            strings.append(fieldValue.displayValue());
        },
        nullptr
    );

    return strings.join(" ");
}

QVariantMap Sighting::symbols() const
{
    return QVariantMap
    {
        { "version", schema()->version() },
        { "deviceId", m_deviceId },
        { "sessionId", m_sessionId },
        { "username", m_username },
        { "createTime", m_createTime },
        { "updateTime", m_updateTime },
        { "wizardMode", !m_wizardRecordUid.isEmpty() }
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

bool Sighting::snapTrack(TrackFile* trackFile, const TrackFileMetadata& metadata, const QString& trackFieldUid, const QString& trackFileFormat)
{
    // Rebuild the info block now that all the data is there.
    auto trackFileInfo = trackFile->rebuildInfo(metadata);
    if (trackFileInfo.count < 2)
    {
        qDebug() << "Ignore track with too few points: " << trackFileInfo.count;
        trackFile->reset();
        return false;
    }

    // trackFieldUid was specified, so export to the requested format and fill in the field.
    if (!trackFieldUid.isEmpty())
    {
        auto exportedFilePath = trackFile->exportFile(trackFileFormat, true);
        if (!exportedFilePath.isEmpty())
        {
            auto mediaFilename = App::instance()->moveToMedia(exportedFilePath);
            setRootFieldValue(trackFieldUid, mediaFilename);
        }
        else
        {
            qDebug() << "snapTrack: failed to snap track to format: " << trackFileFormat;
        }
    }

    // Move the file to the media path and add it to the sighting.
    set_trackFile(trackFile->moveToMedia());

    return true;
}

QString Sighting::trackFile() const
{
    return m_trackFile;
}

void Sighting::set_trackFile(const QString& value)
{
    m_trackFile = value;
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
    m_deviceId = m_sessionId = m_username = "";
    m_createTime = m_updateTime = "";
    m_trackFile.clear();
}

void Sighting::recalculate(FieldValueChange* fieldValueChange, bool emitChanges)
{
    // Snap current system state.
    if (m_deviceId.isEmpty())
    {
        m_deviceId = App::instance()->settings()->deviceId();
    }

    if (m_username.isEmpty())
    {
        m_username = App::instance()->settings()->username();
    }

    // Calculate.
    auto trackChanges = emitChanges && (this->receivers(SIGNAL(recalculated(const FieldValueChanges&, const FieldValueChanges&))) > 0);
    auto fieldValueChanges = FieldValueChanges();
    if (fieldValueChange)
    {
        fieldValueChanges.append(*fieldValueChange);
    }

    auto symbols = this->symbols();
    auto finalVars = m_globals;
    finalVars.insert(m_variables);

    m_recordManager->recalculate(&fieldValueChanges, &symbols, &finalVars, trackChanges);

    if (trackChanges)
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

    auto finalVars = m_globals;
    finalVars.insert(m_variables);
    if (variables)
    {
        finalVars.insert(*variables);
    }

    return m_recordManager->evaluate(expression, contextRecordUid, contextFieldUid, &symbols, &finalVars);
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
    auto field = schema()->getField(fieldUid);
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

void Sighting::setFieldValues(const QString& recordUid, const QVariantMap& values)
{
    auto record = m_recordManager->getRecord(recordUid);
    for (auto it = values.constKeyValueBegin(); it != values.constKeyValueEnd(); it++)
    {
        if (!it->second.isValid())
        {
            record->resetFieldValue(it->first);
        }
        else
        {
            record->setFieldValue(it->first, it->second);
        }
    }

    recalculate();
}

void Sighting::resetFieldValue(const QString& recordUid, const QString& fieldUid)
{
    auto record = getRecord(recordUid);
    auto oldValue = record->getFieldValue(fieldUid);
    record->resetFieldValue(fieldUid);

    // Cleanup orphaned child records if the field being reset is RecordField.
    if (schema()->getField(fieldUid)->isRecordField())
    {
        m_recordManager->garbageCollect();
    }

    auto fieldValueChange = FieldValueChange { recordUid, fieldUid, oldValue };
    recalculate(&fieldValueChange);
}

void Sighting::setFieldState(const QString& recordUid, const QString& fieldUid, FieldState state)
{
    getRecord(recordUid)->setFieldState(fieldUid, state);
}

void Sighting::resetRecord(const QString& recordUid)
{
    // Reset in order so that side effects are predictable.
    getRecord(recordUid)->recordField()->enumFields([&](BaseField* field)
    {
        resetFieldValue(recordUid, field->uid());
    });
}

QVariant Sighting::getFieldParameter(const QString& recordUid, const QString& fieldUid, const QString& key, const QVariant& defaultValue) const
{
    // Get root field parameters.
    auto globalParams = schema()->rootField()->parameters();

    // Get field parameters.
    auto fieldParams = QVariantMap();
    if (fieldUid.isEmpty() && !recordUid.isEmpty())
    {
        // This is a group, so pick up the record field uid.
        auto record = getRecord(recordUid);
        if (record)
        {
            fieldParams = record->recordField()->parameters();
        }
    }
    else
    {
        fieldParams = schema()->getField(fieldUid)->parameters();
    }

    // Get result.
    auto fieldResult = Utils::getParam(fieldParams, key);
    auto fieldResultMap = fieldResult.toMap();
    auto globalResult = Utils::getParam(globalParams, key);
    auto globalResultMap = globalResult.toMap();

    // If not a map, process simply.
    if (fieldResultMap.isEmpty() && globalResultMap.isEmpty())
    {
        if (fieldResult.isValid())
        {
            return fieldResult;
        }

        if (globalResult.isValid())
        {
            return globalResult;
        }

        return defaultValue;
    }

    // Merge maps.
    auto resultMap = QVariantMap();
    resultMap.insert(globalResultMap);
    resultMap.insert(fieldResultMap);

    return QVariant(resultMap);
}

QString Sighting::getSnapLocationFieldUid() const
{
    auto fieldUid = getFieldParameter(rootRecordUid(), "", "save.snapLocation").toString();

    if (fieldUid.isEmpty())
    {
        return QString();
    }

    auto field = qobject_cast<LocationField*>(schema()->getField(fieldUid));
    if (field == nullptr)
    {
        qDebug() << "snapLocation is not a location field: " << fieldUid;
        return QString();
    }

    return fieldUid;
}

QVariantMap Sighting::findSnapLocation(const QString& /*recordUid*/, const QString& /*fieldUid*/) const
{
    auto locationFieldUid = getSnapLocationFieldUid();
    if (locationFieldUid.isEmpty())
    {
        return QVariantMap();
    }

    return QVariantMap
    {
        { "recordUid", rootRecordUid() },
        { "fieldUid", locationFieldUid },
        { "valid", Utils::locationValid(locationMap()) }
    };
}

QVariantList Sighting::findSaveTargets(const QString& recordUid, const QString& fieldUid) const
{
    auto result = QVariantList();

    // Check if the field has a "save.targets" parameter.
    auto targets = getFieldParameter(rootRecordUid(), "", "save.targets").toList();
    if (targets.isEmpty())
    {
        return result;
    }

    // Find the output field: can be empty.
    auto outputFieldUid = QString();
    m_recordManager->rootRecord()->recordField()->enumFields([&](BaseField* field)
    {
        // Already found.
        if (!outputFieldUid.isEmpty())
        {
            return;
        }

        // Must be 'select_one saveTargets'.
        if (field->listElementUid() != "saveTarget")
        {
            return;
        }

        if (qobject_cast<StringField*>(field) == nullptr)
        {
            qDebug() << "saveTarget output field is not a select_one field: " << field->uid();
            return;
        }

        // Must be visible.
        if (!field->hidden())
        {
            qDebug() << "saveTarget output field must be hidden: " << field->uid();
            return;
        }

        // Found.
        outputFieldUid = field->uid();
        qDebug() << "Found saveTarget output field: " << outputFieldUid;
    });

    // Build a list of possible targets and drill into groups.
    auto possibleTargets = QStringList();

    // Stop field is the current page, if empty it means a group.
    auto stopFieldUid = fieldUid;
    if (stopFieldUid.isEmpty())
    {
        stopFieldUid = getRecord(recordUid)->recordFieldUid();
    }

    std::function<void(Record* record)> appendPossibleTargets = [&](Record* record)
    {
        record->enumFieldValues([&](const FieldValue& fieldValue, bool* stopOut)
        {
            if (fieldValue.isVisible())
            {
                possibleTargets.append(Utils::lastPathComponent(fieldValue.fieldUid()));

                auto recordField = RecordField::fromField(fieldValue.field());
                if (recordField && recordField->group())
                {
                    auto groupRecord = m_recordManager->getRecord(fieldValue.value().toStringList().constFirst());
                    appendPossibleTargets(groupRecord);
                }
            }

            *stopOut = Utils::compareLastPathComponents(stopFieldUid, fieldValue.fieldUid());
        });
    };

    appendPossibleTargets(m_recordManager->rootRecord());

    // Build a model of viable targets.
    for (auto targetIt = targets.constBegin(); targetIt != targets.constEnd(); targetIt++)
    {
        auto target = targetIt->toMap();
        auto targetFieldUid = target.value("question").toString();
        if (!possibleTargets.contains(targetFieldUid))
        {
            qDebug() << "Save target skipped because not reachable: " << target;
            continue;
        }

        auto targetElementUid = "saveTarget/" + target.value("choice").toString();
        if (schema()->getElement(targetElementUid) == nullptr)
        {
            qDebug() << "Save target skipped because choice not found: " << targetElementUid;
            continue;
        }

        result.append(QVariantMap {{ "elementUid", targetElementUid }, { "fieldUid", targetFieldUid }, { "delay", target.value("delay", false).toBool() }, { "outputFieldUid", outputFieldUid }});
    }

    // In the non-immersive case, add an option to go home.
    // WizardPage.qml understands an empty field and element as 'Home'.
    if (!m_immersive && !result.isEmpty())
    {
        result.append(QVariantMap {{ "elementUid", "" }, { "fieldUid", "" }, { "delay", false }, { "outputFieldUid", outputFieldUid }});
    }

    return result;
}

QVariantMap Sighting::findTrackCommand() const
{
    auto trackCommands = getFieldParameter(rootRecordUid(), "", "save.track").toList();

    auto vars = variables();

    // Find the first matching track condition.
    for (auto trackCommandIt = trackCommands.constBegin(); trackCommandIt != trackCommands.constEnd(); trackCommandIt++)
    {
        auto trackCommand = trackCommandIt->toMap();
        auto condition = trackCommand.value("condition").toString();

        if (!condition.isEmpty() && !evaluate(condition, rootRecordUid(), "", &vars).toBool())
        {
            continue;
        }

        qDebug() << "save.track match: " << trackCommand;
        return trackCommand;
    }

    return QVariantMap();
}

} // namespace CyberTracker
