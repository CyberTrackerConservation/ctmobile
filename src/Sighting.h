#pragma once
#include "pch.h"
#include "Record.h"
#include "Location.h"
#include "Database.h"
#include "Evaluator.h"
#include "TrackFile.h"

namespace CyberTracker {

class Sighting: public QObject
{
    Q_OBJECT

    Q_PROPERTY (QString stateSpace READ stateSpace CONSTANT)
    Q_PROPERTY (QString schemaHash READ schemaHash CONSTANT)
    Q_PROPERTY (Schema* schema READ schema CONSTANT)
    Q_PROPERTY (QString languageCode READ languageCode CONSTANT)
    Q_PROPERTY (QVariantMap iconHashes READ iconHashes CONSTANT)

    Q_PROPERTY (RecordManager* recordManager READ recordManager CONSTANT)
    Q_PROPERTY (QString rootRecordUid READ rootRecordUid CONSTANT)
    Q_PROPERTY (bool isValid READ isValid CONSTANT)

    Q_PROPERTY (QString deviceId READ deviceId CONSTANT)
    Q_PROPERTY (QString username READ username CONSTANT)
    Q_PROPERTY (QString sessionId READ sessionId CONSTANT)
    Q_PROPERTY (bool trackOnly READ trackOnly CONSTANT)
    Q_PROPERTY (QString createTime READ createTime CONSTANT)
    Q_PROPERTY (QString updateTime READ updateTime CONSTANT)
    Q_PROPERTY (QVariantMap location READ locationMap CONSTANT)
    Q_PROPERTY (QVariantList geoshapes READ geoshapes CONSTANT)
    Q_PROPERTY (QString summaryText READ summaryText CONSTANT)
    Q_PROPERTY (QString summaryIcon READ summaryIcon CONSTANT)
    Q_PROPERTY (QStringList referencedIcons READ referencedIcons CONSTANT)
    Q_PROPERTY (QStringList archiveFiles READ archiveFiles CONSTANT)
    Q_PROPERTY (QString searchText READ searchText CONSTANT)

    Q_PROPERTY (QVariantMap symbols READ symbols CONSTANT)
    Q_PROPERTY (QVariantMap controlStates READ controlStates CONSTANT)
    Q_PROPERTY (QVariantMap variables READ variables CONSTANT)
    Q_PROPERTY (QString trackFile READ trackFile CONSTANT)

    Q_PROPERTY (QVariantList pageStack READ pageStack CONSTANT)
    Q_PROPERTY (QString wizardRecordUid READ wizardRecordUid CONSTANT)
    Q_PROPERTY (QVariantMap wizardRules READ wizardRules CONSTANT)
    Q_PROPERTY (QVariantList wizardPageStack READ wizardPageStack CONSTANT)

    Q_PROPERTY (QString note READ note CONSTANT)
    Q_PROPERTY (QVariantList classicData READ classicData CONSTANT)
    Q_PROPERTY (QByteArray classicBlob READ classicBlob CONSTANT)

public:
    explicit Sighting(QObject* parent);
    explicit Sighting(const QVariantMap& data, const QString& stateSpace = QString(), Schema* schema = nullptr, const QString& languageCode = "system", QObject* parent = nullptr);
    virtual ~Sighting();

    static void setNote(const QString& projectUid, const QString& sightingUid, const QString& note);

    void reset();

    QString stateSpace() const;
    QString schemaHash() const;
    void set_schemaHash(const QString& value);
    Schema* schema() const;
    void set_schema(Schema* value);
    QString languageCode() const;
    void set_languageCode(const QString& value);
    QVariantMap iconHashes() const;
    void set_iconHashes(const QVariantMap& value);

    QString getIconHash(const QString& icon) const;
    QString getFieldIconHash(const QString& fieldUid) const;
    QString getElementIconHash(const QString& elementUid) const;

    RecordManager* recordManager();
    QString rootRecordUid() const;
    Record* getRecord(const QString& recordUid) const;

    bool isValid();

    void regenerateUids();

    QString deviceId() const;
    void set_deviceId(const QString& deviceId);
    QString username() const;
    void set_username(const QString& username);
    QString sessionId() const;
    void set_sessionId(const QString& sessionId);
    bool trackOnly() const;
    void set_trackOnly(bool value);

    QString createTime() const;
    void set_createTime(const QString& createTime);
    QString updateTime() const;
    void set_updateTime(const QString& updateTime);

    QVariantMap locationMap() const;
    std::unique_ptr<Location> locationPtr() const;
    QVariantList geoshapes() const;
    QString summaryText(bool fieldNames = false) const;
    QString summaryIcon() const;
    QString statusIcon(Database::DataFlags flags) const;
    QStringList referencedIcons() const;
    QStringList archiveFiles() const;
    QString searchText() const;

    QVariantMap symbols() const;
    QVariantMap controlStates() const;
    QVariantMap variables() const;

    bool snapTrack(TrackFile* trackFile, const TrackFileMetadata& metadata, const QString& trackFieldUid, const QString& trackFileFormat);

    QString trackFile() const;
    void set_trackFile(const QString& value);

    QVariantList pageStack() const;
    void set_pageStack(const QVariantList& value);
    QString lastPageUrl() const;

    bool immersive() const;
    void set_immersive(bool value);
    QString wizardRecordUid() const;
    void set_wizardRecordUid(const QString& value);
    QVariantMap wizardRules() const;
    void set_wizardRules(const QVariantMap& value);
    QVariantList wizardPageStack() const;
    void set_wizardPageStack(const QVariantList& value);

    QString note() const;
    void set_note(const QString& value);

    QVariantList classicData() const;
    void set_classicData(const QVariantList& value);

    QByteArray classicBlob() const;
    void set_classicBlob(const QByteArray& value);

    void recalculate(FieldValueChange* fieldValueChange = nullptr, bool emitChanges = true);
    QVariant evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid, const QVariantMap* variables) const;

    QVariantMap save(QStringList* attachmentsOut = nullptr) const;
    QVariantMap saveForExport(QStringList* attachmentsOut) const;
    void load(const QVariantMap& data);

    QString toXml(const QString& formId, const QString& formVersion, const QString& formhubUuid, QStringList* attachmentsOut) const;

    QVariant getRootFieldValue(const QString& fieldUid, const QVariant& defaultValue = QVariant()) const;
    void setRootFieldValue(const QString& fieldUid, const QVariant& value);
    void resetRootFieldValue(const QString& fieldUid);

    QVariant getControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& defaultValue = QVariant()) const;
    void setControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& value);

    QVariant getVariable(const QString& name, const QVariant& defaultValue = QVariant()) const;
    void setVariable(const QString& name, const QVariant& value = QVariant());

    QVariantMap globals() const;
    void set_globals(const QVariantMap& value);

    QString newRecord(const QString& parentRecordUid, const QString& recordFieldUid);
    void deleteRecord(const QString& recordUid);

    QVariant getFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& defaultValue = QVariant()) const;
    void setFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& value);
    void setFieldValues(const QString& recordUid, const QVariantMap& values);
    void resetFieldValue(const QString& recordUid, const QString& fieldUid);
    void setFieldState(const QString& recordUid, const QString& fieldUid, FieldState state);
    void resetRecord(const QString& recordUid);

    QVariant getFieldParameter(const QString& recordUid, const QString& fieldUid, const QString& key, const QVariant& defaultValue = QVariant()) const;
    QString getSnapLocationFieldUid() const;
    QVariantList findSaveTargets(const QString& recordUid, const QString& fieldUid) const;
    QVariantMap findSnapLocation(const QString& recordUid, const QString& fieldUid) const;
    QVariantMap findTrackCommand() const;

    bool findAudioField(const QString& filename, QString* recordUidOut, QString* fieldUidOut);

private:
    RecordManager* m_recordManager = nullptr;
    QString m_schemaHash;
    QVariantMap m_iconHashes;
    QString m_stateSpace;
    QString m_deviceId;
    QString m_sessionId;
    bool m_trackOnly = false;
    QString m_username;
    QString m_createTime;
    QString m_updateTime;
    QVariantList m_pageStack;
    bool m_immersive = false;
    QString m_wizardRecordUid;
    QVariantMap m_wizardRules;
    QVariantList m_wizardPageStack;
    QVariantMap m_controlStates;
    QVariantMap m_globals;
    QVariantMap m_variables;
    QString m_trackFile;
    QString m_note;
    QVariantList m_classicData;
    QByteArray m_classicBlob;

signals:
    void recalculated(const FieldValueChanges& fieldValueChanges, const FieldValueChanges& recordChildValueChanges);
};

} // namespace CyberTracker
