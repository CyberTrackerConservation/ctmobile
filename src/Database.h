#pragma once
#include "pch.h"

namespace CyberTracker {

class Database: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, filePath)
    QML_READONLY_AUTO_PROPERTY (QString, connectionName)

public:
    explicit Database(QObject* parent = nullptr);
    explicit Database(const QString& filePath, QObject* parent = nullptr);
    virtual ~Database();

    void compact();
    
    void transaction(std::function<bool()> callback);

    void addProject(const QString& projectUid);
    void getProjects(QStringList* projectUidsOut) const;
    void saveProject(const QString& projectUid, const QVariantMap& data);
    bool testProject(const QString& projectUid) const;
    void loadProject(const QString& projectUid, QVariantMap* dataOut) const;
    void removeProject(const QString& projectUid);
    void clearProjects();

    enum class DataFlag
    {
        DB_NO_FLAG           = 0x00000000,
        DB_SIGHTING_FLAG     = 0x00000001,
        //DB_LOCATION_FLAG   = 0x00000002,
        DB_EXPORTED_FLAG     = 0x00000004,
        DB_UPLOADED_FLAG     = 0x00000008,
        DB_TRANSCRIBING_FLAG = 0x00000010,
        DB_SUBMITTED_FLAG    = 0x00000020,
        DB_READONLY_FLAG     = 0x00000040,
        DB_COMPLETED_FLAG    = 0x00000080,
    };
    Q_DECLARE_FLAGS(DataFlags, DataFlag)

    struct DbData
    {
        qint64 rowId = -1;
        QString stateSpace = "";
        QString schemaHash;
        QString timestamp;
        double x = std::numeric_limits<double>::quiet_NaN();
        double y = std::numeric_limits<double>::quiet_NaN();
        DataFlags flags;
        QVariantMap data;
        QStringList attachments;
    };

    void enumLegacyData(std::function<void(const QString& projectUid, const QString& stateSpace, const QString& uid, DataFlags flags, const QVariantMap& data, const QStringList& attachments)> callback) const;
    void dropLegacyData();

    bool hasData(const QString& projectUid, const QString& stateSpace, DataFlags flags) const;
    bool hasData(const QString& projectUid, const QString& stateSpace, DataFlags flagsOn, DataFlags flagsOff) const;
    void saveData(const QString& projectUid, const QString& uid, const DbData& dbData);
    void saveDataBatch(std::function<void()> callback);
    void loadData(const QString& projectUid, const QString& uid, DbData* dbDataOut) const;
    bool testData(const QString& projectUid, const QString& uid) const;
    void deleteData(const QString& projectUid);
    void deleteData(const QString& projectUid, const QString& uid);
    void deleteData(const QString& projectUid, const QString& stateSpace, DataFlags flags);
    void archiveData(const QString& newProjectUid, const QString& projectUid, const QString& uid);
    void archiveData(const QString& newProjectUid, const QString& projectUid, const QString& stateSpace, DataFlags flags);

    typedef std::function<void(const QString& projectUid, const QString& uid, const DbData& dbData)> EnumDataCallback;
    void enumData(const QString& whereSql, const QVariantMap& whereVars, EnumDataCallback callback) const;
    void enumData(const QString& projectUid, const QString& stateSpace, DataFlags flagsAny, EnumDataCallback callback) const;
    void enumData(const QString& projectUid, const QString& stateSpace, DataFlags flagsOn, DataFlags flagsOff, EnumDataCallback callback) const;
    void getData(const QString& projectUid, const QString& stateSpace, DataFlags flagsAny, QStringList* uidsOut) const;
    void getData(const QString& projectUid, const QString& stateSpace, DataFlags flagsOn, DataFlags flagsOff, QStringList* uidsOut) const;
    DataFlags getDataFlags(const QString& projectUid, const QString& uid) const;
    void setDataFlags(const QString& projectUid, const QString& uid, DataFlags flags, bool on = true);
    void setDataFlags(const QString& projectUid, const QString& stateSpace, DataFlags matchFlagsOn, DataFlags matchFlagsOff, DataFlags flags);
    void setDataFlagsAll(const QString& projectUid, DataFlags flags, bool on = true);
    void getDataQueueState(const QString& projectUid, const QString& uid, QVariantMap* stateOut) const;
    void setDataQueueState(const QString& projectUid, const QString& uid, const QVariantMap& state);
    QString getDataStateSpace(const QString& projectUid, const QString& uid) const;

    void saveFormState(const QString& projectUid, const QString& stateSpace, const QVariantMap& data, const QStringList& attachments, const QStringList& streamers);
    void loadFormState(const QString& projectUid, const QString& stateSpace, QVariantMap* dataOut, QStringList* attachmentsOut = nullptr, QStringList* streamersOut = nullptr) const;
    void deleteFormState(const QString& projectUid, const QString& stateSpace);

    void addExport(const QString& exportUid, const QVariantMap& data);
    void getExports(QStringList* exportUidsOut) const;
    void loadExport(const QString& exportUid, QVariantMap* dataOut) const;
    void saveExport(const QString& exportUid, const QVariantMap& data);
    void removeExport(const QString& exportUid);

    struct DbAuditItem
    {
        QString timestamp;
        QString projectTitle;
        QString description;
        bool internal;
    };

    void addAudit(const DbAuditItem& auditItem);
    void enumAudit(std::function<void(const DbAuditItem& auditItem)> callback) const;
    void clearAudit();

    struct DbBackupFile
    {
        QString timestamp;
        QString projectTitle;
        QString filename;
    };

    void addBackupFile(const DbBackupFile& backupFile);
    void enumBackupFiles(std::function<void(const DbBackupFile& backupFile)> callback) const;
    void deleteBackupFile(const QString& filename);
    void clearBackupFiles();

    enum class QueueFlag
    {
        DB_NO_FLAG            = 0,
        DB_AUTO_DELETE        = 1,
        DB_COMPLETE_FLAG      = 0x80,
    };
    Q_DECLARE_FLAGS(QueueFlags, QueueFlag)

    struct DbQueueItem
    {
        QString uid;
        QString projectUid;
        QString dataUid;
        QString summary;
        QString submitTime;
        QString uploadTime;
        QVariantMap state;
        QueueFlags flags;
        QString type;
        QString url;
        QByteArray payload;
        QString payloadError;
        QByteArray webhook;
        QString webhookError;
        QStringList attachments;
    };

    void appendQueueItem(const DbQueueItem& queueItem);
    void loadQueueItem(const QString& uid, DbQueueItem* queueItemOut);
    void enumQueueItems(std::function<void(const DbQueueItem& queueItem)> callback);
    void setQueueItemState(const QString& uid, const QVariantMap& state);
    void setQueueItemFlags(const QString& uid, QueueFlags flags, bool on = true);
    void setQueueItemPayloadError(const QString& uid, const QString& error, const QString& timestamp);
    void setQueueItemWebhookError(const QString& uid, const QString& error);
    void deleteQueueItems(QueueFlags flags);
    void deleteQueueAttachments(QueueFlags flags);
    void resetQueueItems();

    struct DbTranscribeItem
    {
        QString filename;
        QString projectUid;
        QString stateSpace;
        QString dataUid;
        QString prompt;
        QString schema;
        QVariantMap result;
        QString error;
        QString fillRecordUid;
        QStringList fillFieldUids;
    };

    bool testTranscribeItem(const QString& projectUid, const QString& dataUid) const;
    bool hasPendingTranscribeItems(const QString& projectUid, const QString& dataUid) const;
    void appendTranscribeItem(const DbTranscribeItem& transcribeItem);
    void updateTranscribeItem(const DbTranscribeItem& transcribeItem);
    void deleteTranscribeItem(const QString& filename);
    void enumTranscribeItems(std::function<void(const DbTranscribeItem& transcribeItem)> callback) const;

    QStringList getStreamers();
    bool hasStreamers(const QString& projectUid);

    QStringList getAttachments();

    struct DbMapPackage
    {
        QString id;
        QString folder;
        QString projectUid;
        QString name;
        QString version;
        QString updateUrl;
        QString eTag;
        QString lastUpdate;
    };

    struct DbMapLayer
    {
        QString id;
        QString packageId;
        QString filename;
        QString name;
        QString category;
        bool active = true;
        double opacity = 1.0;
        QVariantMap symbol;
        bool lookup = false;
        int sequence = 0;
    };

    void saveMapPackage(const DbMapPackage& package);
    bool loadMapPackage(const QString& id, DbMapPackage* packageOut) const;
    void enumMapPackages(std::function<void(const DbMapPackage& package)> callback) const;
    void enumMapPackagesByProject(const QString& projectUid, std::function<void(const DbMapPackage& package)> callback) const;
    void deleteMapPackage(const QString& id);

    void saveMapLayer(const DbMapLayer& layer);
    bool loadMapLayer(const QString& id, DbMapLayer* layerOut) const;
    void enumMapLayers(std::function<void(const DbMapLayer& layer)> callback) const;
    void enumMapLayersByPackage(const QString& packageId, std::function<void(const DbMapLayer& layer)> callback) const;
    void deleteMapLayer(const QString& id);
    void deleteMapLayersByPackage(const QString& packageId);
    int getMaxMapLayerSequence() const;

private:
    quint64 m_threadId;
    QSqlQuery* m_batchQuery = nullptr;
    void verifyThread() const;
    void faultIf(bool faultIfTrue, const QSqlQuery& query, const QString& message) const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Database::DataFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(Database::QueueFlags)

} // namespace CyberTracker
