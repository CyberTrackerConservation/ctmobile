#pragma once
#include "pch.h"
#include "Record.h"
#include "Location.h"
#include "Evaluator.h"

class Sighting: public QObject
{
    Q_OBJECT

    Q_PROPERTY (RecordManager* recordManager READ recordManager CONSTANT)
    Q_PROPERTY (QString rootRecordUid READ rootRecordUid CONSTANT)
    Q_PROPERTY (bool isValid READ isValid CONSTANT)

    Q_PROPERTY (QString deviceId READ deviceId CONSTANT)
    Q_PROPERTY (QString username READ username CONSTANT)
    Q_PROPERTY (QString createTime READ createTime CONSTANT)
    Q_PROPERTY (QString updateTime READ updateTime CONSTANT)
    Q_PROPERTY (QVariantMap location READ locationMap CONSTANT)
    Q_PROPERTY (QString summaryText READ summaryText CONSTANT)
    Q_PROPERTY (QUrl summaryIcon READ summaryIcon CONSTANT)

    Q_PROPERTY (QVariantMap symbols READ symbols CONSTANT)
    Q_PROPERTY (QVariantMap controlStates READ controlStates CONSTANT)
    Q_PROPERTY (QVariantMap variables READ variables CONSTANT)
    Q_PROPERTY (QString trackFile READ trackFile CONSTANT)

    Q_PROPERTY (QVariantList pageStack READ pageStack CONSTANT)
    Q_PROPERTY (QString wizardRecordUid READ wizardRecordUid CONSTANT)
    Q_PROPERTY (QVariantMap wizardRules READ wizardRules CONSTANT)
    Q_PROPERTY (QVariantList wizardPageStack READ wizardPageStack CONSTANT)

public:
    static constexpr uint32_t DB_NO_FLAG        = 0x00000000;
    static constexpr uint32_t DB_SIGHTING_FLAG  = 0x00000001;
    static constexpr uint32_t DB_LOCATION_FLAG  = 0x00000002;
    static constexpr uint32_t DB_EXPORTED_FLAG  = 0x00000004;
    static constexpr uint32_t DB_UPLOADED_FLAG  = 0x00000008;
    static constexpr uint32_t DB_PENDING_FLAG   = 0x00000010;
    static constexpr uint32_t DB_SUBMITTED_FLAG = 0x00000020;
    static constexpr uint32_t DB_READONLY_FLAG  = 0x00000040;
    static constexpr uint32_t DB_COMPLETED_FLAG = 0x00000080;
    static constexpr uint32_t DB_SNAPPED_FLAG   = 0x00000100;

    explicit Sighting(QObject* parent = nullptr);
    explicit Sighting(const QVariantMap& data, QObject* parent);
    virtual ~Sighting();

    void reset();

    RecordManager* recordManager();
    QString rootRecordUid() const;
    Record* getRecord(const QString& recordUid) const;

    bool isValid();

    void regenerateUids();

    QString deviceId() const;
    QString username() const;
    QString createTime() const;
    QString updateTime() const;
    QVariantMap locationMap() const;
    std::unique_ptr<Location> locationPtr() const;
    QString summaryText(bool fieldNames = false) const;
    QUrl summaryIcon() const;
    QUrl statusIcon(int flags) const;

    QVariantMap symbols() const;
    QVariantMap controlStates() const;
    QVariantMap variables() const;

    QString trackFile() const;
    void set_trackFile(const QString& value);

    QVariantList pageStack() const;
    void set_pageStack(const QVariantList& value);
    QString lastPageUrl() const;

    QString wizardRecordUid() const;
    void set_wizardRecordUid(const QString& value);
    QVariantMap wizardRules() const;
    void set_wizardRules(const QVariantMap& value);
    QVariantList wizardPageStack() const;
    void set_wizardPageStack(const QVariantList& value);

    void recalculate(FieldValueChange* fieldValueChange = nullptr);
    QVariant evaluate(const QString& expression, const QString& contextRecordUid, const QString& contextFieldUid, const QVariantMap* variables) const;

    void snapCreateTime(const QString& now = QString());
    void snapUpdateTime(const QString& now = QString());

    QVariantMap save(QStringList* attachmentsOut = nullptr) const;
    QVariantMap saveForExport(QStringList* attachmentsOut) const;
    void load(const QVariantMap& data);

    QString toXml(const QString& formId, const QString& formVersion, QStringList* attachmentsOut) const;

    QVariant getRootFieldValue(const QString& fieldUid, const QVariant& defaultValue = QVariant()) const;
    void setRootFieldValue(const QString& fieldUid, const QVariant& value);
    void resetRootFieldValue(const QString& fieldUid);

    QVariant getControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& defaultValue = QVariant()) const;
    void setControlState(const QString& recordUid, const QString& fieldUid, const QString& name, const QVariant& value);

    QVariant getVariable(const QString& name, const QVariant& defaultValue = QVariant()) const;
    void setVariable(const QString& name, const QVariant& value = QVariant());

    void buildRecordModel(QVariantList* recordModel, const std::function<bool(const FieldValue& fieldValue, int depth)>& filter) const;

    QString newRecord(const QString& parentRecordUid, const QString& recordFieldUid);
    void deleteRecord(const QString& recordUid);

    QVariant getFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& defaultValue) const;
    void setFieldValue(const QString& recordUid, const QString& fieldUid, const QVariant& value);
    void resetFieldValue(const QString& recordUid, const QString& fieldUid);
    void setFieldState(const QString& recordUid, const QString& fieldUid, FieldState state);
    void resetRecord(const QString& recordUid);

    QVariant getFieldParameter(const QString& recordUid, const QString& fieldUid, const QString& key, const QVariant& defaultValue = QVariant()) const;
    QString getSnapLocationFieldUid() const;
    QString getTrackFileFieldUid(QString* formatOut = nullptr) const;
    QVariantList findSaveTargets(const QString& recordUid, const QString& fieldUid) const;
    QVariantMap findSnapLocation(const QString& recordUid, const QString& fieldUid) const;
    QVariantMap findTrackSetting() const;

private:
    RecordManager* m_recordManager = nullptr;
    QString m_deviceId;
    QString m_username;
    QString m_createTime;
    QString m_updateTime;
    QVariantList m_pageStack;
    QString m_wizardRecordUid;
    QVariantMap m_wizardRules;
    QVariantList m_wizardPageStack;
    QVariantMap m_controlStates;
    QVariantMap m_variables;
    QString m_trackFile;

signals:
    void recalculated(const FieldValueChanges& fieldValueChanges, const FieldValueChanges& recordChildValueChanges);
};
