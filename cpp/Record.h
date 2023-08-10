#pragma once
#include "pch.h"
#include "Field.h"
#include "Evaluator.h"

enum class FieldState
{
    None = 0,
    Default = 1,
    UserSet = 2,
    Calculated = 3,
    Constant = 4
};

enum class FieldFlag
{
    None = 0x00,
    Hidden = 0x01,
    Record = 0x02,
    Required = 0x04,
    Relevant = 0x08,
    Constraint = 0x10,
    Valid = 0x20,
    Skip = 0x40
};

typedef QFlags<FieldFlag> FieldFlags;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FieldValue

class Record;
class RecordManager;

class FieldValue: public QObject
{
    Q_OBJECT

    Q_PROPERTY (QString name READ name CONSTANT)
    Q_PROPERTY (QVariant value READ value CONSTANT)
    Q_PROPERTY (QString xmlValue READ xmlValue CONSTANT)
    Q_PROPERTY (QStringList attachments READ attachments CONSTANT)

    Q_PROPERTY (QString exportUid READ exportUid CONSTANT)
    Q_PROPERTY (QString hint READ hint CONSTANT)
    Q_PROPERTY (QString displayValue READ displayValue CONSTANT)
    Q_PROPERTY (QString constraintMessage READ constraintMessage CONSTANT)
    Q_PROPERTY (QString requiredMessage READ requiredMessage CONSTANT)

    Q_PROPERTY (bool isRelevant READ isRelevant CONSTANT)
    Q_PROPERTY (bool isVisible READ isVisible CONSTANT)
    Q_PROPERTY (bool isValid READ isValid CONSTANT)
    Q_PROPERTY (bool isRecordField READ isRecordField CONSTANT)

public:
    explicit FieldValue(QObject* parent = nullptr);
    explicit FieldValue(const QString& fieldUid, QObject* parent);

    void init(const QString& fieldUid);

    Record* record() const;
    QString recordUid() const;

    BaseField* field() const;
    QString fieldUid() const;
    QString parentFieldUid() const;

    QString name() const;
    QVariant value() const;
    FieldState state() const;
    FieldFlags flags() const;

    QString xmlValue() const;
    QStringList attachments() const;

    QString exportUid() const;
    QString hint() const;
    QString displayValue() const;
    QString constraintMessage() const;
    QString requiredMessage() const;

    bool isRelevant() const;
    bool isVisible() const;
    bool isValid() const;
    bool isRecordField() const;

private:
    QString m_fieldUid;
    mutable FieldManager* m_fieldManager = nullptr;
};

struct FieldValueChange
{
    QString recordUid;
    QString fieldUid;
    QVariant oldValue = QVariant();
};

typedef QList<FieldValueChange> FieldValueChanges;

struct FieldValueChangesHelper
{
public:
    void init(FieldValueChanges* fieldValueChanges);
    bool contains(const FieldValueChange* fieldValueChange);
    bool containsField(const QString& fieldUid);
    bool append(const FieldValueChange& fieldValueChange);
    bool containsFieldReference(const QString& expression);

private:
    FieldValueChanges* m_fieldValueChanges = nullptr;
    QHash<QString, bool> m_containHash;
    QHash<QString, bool> m_fieldTailHash;
    QHash<QString, bool> m_fieldFullHash;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Record

class Record: public QObject
{
    Q_OBJECT

    Q_PROPERTY (QString recordUid READ recordUid CONSTANT)
    Q_PROPERTY (QString recordFieldUid READ recordFieldUid CONSTANT)
    Q_PROPERTY (QString parentRecordUid READ parentRecordUid CONSTANT)
    Q_PROPERTY (QString nameElementUid READ nameElementUid CONSTANT)
    Q_PROPERTY (const RecordField* recordField READ recordField CONSTANT)
    Q_PROPERTY (QVariantMap data READ data CONSTANT)
    Q_PROPERTY (QStringList attachments READ attachments CONSTANT)
    Q_PROPERTY (QString summary READ summary CONSTANT)
    Q_PROPERTY (QString name READ name CONSTANT)
    Q_PROPERTY (QString icon READ icon CONSTANT)
    Q_PROPERTY (int index READ index CONSTANT)
    Q_PROPERTY (int isEmpty READ isEmpty CONSTANT)
    Q_PROPERTY (int isValid READ isValid CONSTANT)

public:
    explicit Record(QObject* parent = nullptr);
    explicit Record(const QVariantMap& data, QObject* parent);
    explicit Record(const QString& recordUid, const QString& parentRecordUid, const QString& recordFieldUid, QObject* parent);

    void reset();

    QString recordUid() const;
    QString recordFieldUid() const;
    QString parentRecordUid() const;
    QString nameElementUid() const;
    void set_nameElementUid(const QString& value);
    const RecordField* recordField() const;
    QVariantMap data() const;
    QVariantMap save(bool onlyRelevant = false) const;
    void load(const QVariantMap& data);
    QStringList attachments() const;
    QString summary() const;
    QString name() const;
    QString icon() const;
    int index() const;

    void enumFieldValues(const std::function<void(const FieldValue& fieldValue, bool* stopOut)>& fieldValueCallback) const;

    void setRecordUid(const QString& value);
    void remapRecordUids(const std::function<QString(const QString& recordUid)>& lookup);

    bool hasFieldValue(const QString& fieldUid) const;
    QVariant getFieldValue(const QString& fieldUid, const QVariant& defaultValue = QVariant()) const;
    void setFieldValue(const QString& fieldUid, const QVariant& value);
    void resetFieldValue(const QString& fieldUid);

    bool isEmpty() const;
    bool isValid(const QStringList& fieldUids = QStringList()) const;

    FieldState getFieldState(const QString& fieldUid) const;
    void setFieldState(const QString& fieldUid, FieldState fieldState);
    FieldFlags getFieldFlags(const QString& fieldUid) const;
    bool testFieldFlag(const QString& fieldUid, FieldFlag flag) const;
    void setFieldFlag(const QString& fieldUid, FieldFlag flag, bool on = true);
    void setFieldFlags(const QString& fieldUid, FieldFlags flags);

    bool getFieldVisible(const QString& fieldUid) const;
    bool getFieldValid(const QString& fieldUid) const;

    QString getFieldName(const QString& fieldUid) const;
    QString getFieldHint(const QString& fieldUid) const;
    QString getFieldConstraintMessage(const QString& fieldUid) const;
    QString getFieldRequiredMessage(const QString& fieldUid) const;
    QString getFieldDisplayValue(const QString& fieldUid, const QString& defaultValue = QString()) const;

private:
    QString m_recordUid;
    QString m_parentRecordUid;
    QString m_recordFieldUid;
    QString m_nameElementUid;

    struct ValueData
    {
        QVariant value;
        FieldState state = FieldState::None;
        FieldFlags flags = FieldFlags();
    };

    QMap<QString, ValueData> m_fieldValues;
    RecordManager* recordManager() const;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RecordManager

class RecordManager: public QObject
{
    Q_OBJECT
    Q_PROPERTY (Record* rootRecord READ rootRecord CONSTANT)
    Q_PROPERTY (QList<Record*> records READ records CONSTANT)
    Q_PROPERTY (QString rootRecordUid READ rootRecordUid CONSTANT)
    Q_PROPERTY (QStringList attachments READ attachments CONSTANT)

public:
    RecordManager(QObject* parent = nullptr);
    virtual ~RecordManager();

    void reset();

    void remapRecordUids(const std::function<QString(const QString& recordUid)>& lookup);

    QVariantList save(bool onlyRelevant, QStringList* attachmentsOut) const;
    void load(const QVariantList& records);

    Record* rootRecord() const;
    QString rootRecordUid() const;
    Record* getRecord(const QString& recordUid) const;
    int getRecordIndex(const QString& recordUid) const;

    QStringList attachments() const;

    bool hasRecords() const;
    bool hasRecord(const QString& recordUid) const;
    bool hasRecordChanged(const QString& recordUid) const;
    QStringList recordUids() const;

    void enumFieldValues(
        const QString& recordUid, bool onlyRelevant,
        const std::function<void(const Record& record, bool* skipRecordOut)>& recordBeginCallback,
        const std::function<void(const FieldValue& fieldValue)>& fieldValueCallback,
        const std::function<void(const Record& record)>& recordEndCallback) const;

    QString newRecord(const QString& parentRecordUid, const QString& recordFieldUid);
    void deleteRecord(const QString& recordUid);

    void garbageCollect();

    QVariant queryFieldValue(const QString& recordUid, const QString& findFieldUid) const;
    QString expandFieldText(const QString& recordUid, const QString& fieldText) const;

    void recalculate(FieldValueChanges* fieldValueChanges, const QVariantMap* symbols, const QVariantMap* variables, bool trackChanges);
    QVariant evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid, const QVariantMap* symbols, const QVariantMap* variables) const;

signals:
    void recordDeleted(const QString& recordUid);

private:
    QList<Record*> m_records;
    QList<Record*> records();

    bool detachRecord(Record* record);
    bool queryFieldValuesParents(const QString& recordUid, const QString& findFieldUid, QVariant* foundFieldValueOut) const;
    bool queryFieldValuesChildren(const QString& recordUid, const QString& findFieldUid, QVariantList* fieldValuesOut) const;
    bool computeFieldValue(Record* record, const BaseField* field, const QVariant& value, FieldState state, Evaluator& evaluator, QVariant* valueOut, FieldState* stateOut);
    bool computeFieldValueFlags(Record* record, const BaseField* field, const QVariant& value, FieldFlags flags, Evaluator& evaluator, FieldFlags* flagsOut) const;
    bool computeSideEffects(FieldValueChangesHelper& fieldValueChangesHelper, Record* record, const BaseField* field, const QVariant& oldValue, const QVariant& newValue);
    bool computeSideEffectsRecord(FieldValueChangesHelper& fieldValueChangesHelper, Record* record, const RecordField* recordField, const QVariant& oldValue, const QVariant& newValue, bool* pendGarbageCollectOut);
};
