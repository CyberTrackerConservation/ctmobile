#include "pch.h"
#include "Database.h"

namespace CyberTracker {

namespace  {
bool s_preparedOnce = false;

QRecursiveMutex s_mutex;

class Mutex
{
public:
    Mutex()
    {
        if (s_mutex.tryLock())
        {
            return;
        }

        //qDebug() << "Lock contention";
        s_mutex.lock();
    }

    ~Mutex()
    {
        s_mutex.unlock();
    }
};

}

Database::Database(QObject* parent): QObject(parent)
{
}

Database::Database(const QString& filePath, QObject* parent): QObject(parent)
{
    Mutex lock;

    m_filePath = filePath;
    m_connectionName = "ct_db_" + QUuid::createUuid().toString(QUuid::Id128);
    m_threadId = (quint64)QThread::currentThread();

    auto db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(filePath);
    qFatalIf(!db.open(), "Cannot open database: " + filePath);

    if (!s_preparedOnce)
    {
        s_preparedOnce = true;
        QSqlQuery query = QSqlQuery(db);
        auto success = query.prepare("CREATE TABLE IF NOT EXISTS projects (projectUid TEXT, data BLOB)");
        faultIf(!success, query, "Failed to create projects table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create projects table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS data (projectUid TEXT, stateSpace TEXT, uid TEXT, schemaHash TEXT, timestamp TEXT, x REAL, y REAL, flags INTEGER, data BLOB, queueState TEXT, attachments TEXT)");
        faultIf(!success, query, "Failed to create data table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create data table 2");
        success = query.prepare("CREATE UNIQUE INDEX IF NOT EXISTS dataIndex ON data (uid)");
        faultIf(!success, query, "Failed to create data table 3");
        success = query.exec();
        faultIf(!success, query, "Failed to create datatable 4");

        success = query.prepare("CREATE TABLE IF NOT EXISTS formState (projectUid TEXT, stateSpace TEXT, data BLOB, attachments TEXT, streamers TEXT)");
        faultIf(!success, query, "Failed to create formState table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create formState table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS exports (exportUid TEXT, data BLOB)");
        faultIf(!success, query, "Failed to create exports table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create exports table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS backups (timestamp TEXT, projectTitle TEXT, filename TEXT)");
        faultIf(!success, query, "Failed to create backups table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create backups table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS queue (uid TEXT, projectUid TEXT, dataUid TEXT, summary TEXT, submitTime TEXT, uploadTime TEXT, state TEXT, flags INTEGER, type TEXT, url TEXT, payload TEXT, payloadError TEXT, webhook TEXT, webhookError TEXT, attachments TEXT)");
        faultIf(!success, query, "Failed to create queue table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create queue table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS transcribe (filename TEXT, projectUid TEXT, stateSpace TEXT, dataUid TEXT, prompt TEXT, schema TEXT, result TEXT, error TEXT, fillRecordUid TEXT, fillFieldUids TEXT)");
        faultIf(!success, query, "Failed to create transcribe table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create transcribe table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS audit (timestamp TEXT, projectTitle TEXT, description TEXT)");
        faultIf(!success, query, "Failed to create audit table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create audit table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS mapPackages (id TEXT PRIMARY KEY, folder TEXT, projectUid TEXT, name TEXT, version TEXT, updateUrl TEXT, eTag TEXT, lastUpdate TEXT)");
        faultIf(!success, query, "Failed to create mapPackages table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create mapPackages table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS mapLayers (id TEXT PRIMARY KEY, packageId TEXT, filename TEXT, name TEXT, category TEXT, active INTEGER, opacity REAL, symbol TEXT, lookup INTEGER, sequence INTEGER)");
        faultIf(!success, query, "Failed to create mapLayers table 1");
        success = query.exec();
        faultIf(!success, query, "Failed to create mapLayers table 2");

        // Add attachments to formState table.
        success = query.prepare("ALTER TABLE formState ADD attachments TEXT");
        if (success)
        {
            query.exec();
        }

        // Add streamers to formState table.
        success = query.prepare("ALTER TABLE formState ADD streamers TEXT");
        if (success)
        {
            query.exec();
        }

        // Add queueState to data table.
        success = query.prepare("ALTER TABLE data ADD queueState TEXT");
        if (success)
        {
            query.exec();
        }
    }
}

Database::~Database()
{
    Mutex lock;
    verifyThread();

    QSqlDatabase::database(m_connectionName).close();
    QSqlDatabase::removeDatabase(m_connectionName);
}

void Database::verifyThread() const
{
    qFatalIf((quint64)QThread::currentThread() != m_threadId, "Wrong thread");
}

void Database::faultIf(bool faultIfTrue, const QSqlQuery& query, const QString& message) const
{
    qFatalIf(faultIfTrue, message + ": " + query.lastError().text());
}

void Database::compact()
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("VACUUM");
    faultIf(!success, query, "Failed compact 1");
    success = query.exec();
    faultIf(!success, query, "Failed compact 2");
}

void Database::transaction(std::function<bool()> callback)
{
    Mutex lock;
    verifyThread();

    auto db = QSqlDatabase::database(m_connectionName);
    db.transaction();
    auto result = false;

    try
    {
        result = callback();
    }
    catch (...)
    {
        result = false;
    }

    if (result)
    {
        db.commit();
    }
    else
    {
        db.rollback();
    }
}

void Database::addProject(const QString& projectUid)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT EXISTS(SELECT 1 FROM projects WHERE (projectUid = (:projectUid)))");
    faultIf(!success, query, "Failed to find project 1");
    query.bindValue(":projectUid", projectUid);
    success = query.exec() && query.next();
    faultIf(!success, query, "Failed to find project 2");
    auto exists = query.value(0).toInt() == 1;

    if (!exists)
    {
        success = query.prepare("INSERT INTO projects (projectUid) VALUES (:projectUid)");
        faultIf(!success, query, "Failed to add project 1");
        query.bindValue(":projectUid", projectUid);
        success = query.exec();
        faultIf(!success, query, "Failed to add project 2");
    }
}

void Database::getProjects(QStringList* projectUidsOut) const
{
    projectUidsOut->clear();

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT projectUid FROM projects");
    faultIf(!success, query, "Failed to get projects 1");
    success = query.exec();
    faultIf(!success, query, "Failed to get projects 2");

    while (query.next())
    {
        projectUidsOut->append(query.value(0).toString());
    }
}

void Database::removeProject(const QString& projectUid)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM projects WHERE (projectUid = (:projectUid))");
    faultIf(!success, query, "Failed to remove project 1");
    query.bindValue(":projectUid", projectUid);
    query.exec();

    success = query.prepare("DELETE FROM formState WHERE (projectUid = (:projectUid))");
    faultIf(!success, query, "Failed to remove project 2");
    query.bindValue(":projectUid", projectUid);
    query.exec();

    success = query.prepare("DELETE FROM data WHERE (projectUid = (:projectUid))");
    faultIf(!success, query, "Failed to remove project 3");
    query.bindValue(":projectUid", projectUid);
    query.exec();

    success = query.prepare("DELETE FROM queue WHERE (projectUid = (:projectUid))");
    faultIf(!success, query, "Failed to remove project 5");
    query.bindValue(":projectUid", projectUid);
    query.exec();

    success = query.prepare("DELETE FROM transcribe WHERE (projectUid = (:projectUid))");
    faultIf(!success, query, "Failed to remove project 6");
    query.bindValue(":projectUid", projectUid);
    query.exec();
}

void Database::clearProjects()
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM projects");
    faultIf(!success, query, "Failed to delete projects");
    query.exec();
}

void Database::saveProject(const QString& projectUid, const QVariantMap& data)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("UPDATE projects SET data = :data WHERE (projectUid = (:projectUid))");
    faultIf(!success, query, "Failed to save project 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":data", Utils::variantMapToBlob(data));
    success = query.exec();
    faultIf(!success, query, "Failed to save project 2");
}

bool Database::testProject(const QString& projectUid) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT EXISTS(SELECT 1 FROM projects WHERE (projectUid = (:projectUid)))");
    faultIf(!success, query, "Failed to test project 1: " + query.lastError().text());
    query.bindValue(":projectUid", projectUid);
    success = query.exec();

    if (success && query.next())
    {
        return query.value(0).toBool();
    }

    return false;
}

void Database::loadProject(const QString& projectUid, QVariantMap* dataOut) const
{
    dataOut->clear();

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT data FROM projects WHERE (projectUid = (:projectUid))");
    faultIf(!success, query, "Failed to load project 1: " + query.lastError().text());
    query.bindValue(":projectUid", projectUid);
    success = query.exec();
    faultIf(!success, query, "Failed to load project 2: " + query.lastError().text());
    success = query.next();
    faultIf(!success, query, "Failed to load project 3: " + query.lastError().text());

    auto dataBlob = query.value(0).toByteArray();
    if (!dataBlob.isEmpty())
    {
        *dataOut = Utils::variantMapFromBlob(dataBlob);
    }
}

void Database::enumLegacyData(std::function<void(const QString& projectUid, const QString& stateSpace, const QString& uid, DataFlags flags, const QVariantMap& data, const QStringList& attachments)> callback) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT projectUid, uid, stateSpace, flags, data, attachments FROM sightings ORDER BY rowid");
    if (!success)
    {
        // Legacy data already migrated.
        return;
    }

    success = query.exec();
    faultIf(!success, query, "Failed to enumLegacyData 1");

    while (query.next())
    {
        auto projectUid = query.value(0).toString();
        auto uid = query.value(1).toString();
        auto stateSpace = query.value(2).toString();
        auto flags = DataFlags(query.value(3).toUInt());
        auto data = Utils::variantMapFromBlob(query.value(4).toByteArray());
        auto attachments = Utils::stringToList(query.value(5).toString());

        callback(projectUid, stateSpace, uid, flags, data, attachments);
    }
}

void Database::dropLegacyData()
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    // Legacy.
    auto success = query.prepare("DROP TABLE sightings");
    if (!success)
    {
        // Legacy data already migrated.
        return;
    }

    success = query.exec();
    faultIf(!success, query, "Failed to dropLegacyData");
}

bool Database::hasData(const QString& projectUid, const QString& stateSpace, DataFlags flags) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = false;

    if (stateSpace == "*")
    {
        success = query.prepare("SELECT EXISTS(SELECT 1 FROM data WHERE (projectUid = (:projectUid)) AND (flags & (:flags) <> 0))");
    }
    else
    {
        success = query.prepare("SELECT EXISTS(SELECT 1 FROM data WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace)) AND (flags & (:flags) <> 0))");
        query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    }

    faultIf(!success, query, "Failed to hasData 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":flags", static_cast<uint>(flags));
    success = query.exec() && query.next();
    faultIf(!success, query, "Failed to hasData 2");

    return query.value(0).toInt() == 1;
}

bool Database::hasData(const QString& projectUid, const QString& stateSpace, DataFlags flagsOn, DataFlags flagsOff) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto whereSql = QStringList { "(projectUid = (:projectUid))" };
    if (stateSpace != "*")
    {
        whereSql.append("(stateSpace = (:stateSpace))");
    }
    whereSql.append("(flags & (:flagsOn) = (:flagsOn))");
    whereSql.append("(flags & (:flagsOff) = 0)");

    auto success = query.prepare("SELECT EXISTS(SELECT 1 FROM data WHERE " + whereSql.join(" AND ") + ")");
    faultIf(!success, query, "Failed to hasData 1");

    query.bindValue(":projectUid", projectUid);
    if (stateSpace != "*")
    {
        query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    }
    query.bindValue(":flagsOn", static_cast<uint>(flagsOn));
    query.bindValue(":flagsOff", static_cast<uint>(flagsOff));

    success = query.exec() && query.next();
    faultIf(!success, query, "Failed to hasData 2");

    return query.value(0).toInt() == 1;
}

void Database::saveData(const QString& projectUid, const QString& uid, const DbData& dbData)
{
    if (m_batchQuery)
    {
        m_batchQuery->bindValue(":projectUid", projectUid);
        m_batchQuery->bindValue(":stateSpace", dbData.stateSpace);
        m_batchQuery->bindValue(":uid", uid);
        m_batchQuery->bindValue(":schemaHash", dbData.schemaHash);
        m_batchQuery->bindValue(":timestamp", dbData.timestamp);
        m_batchQuery->bindValue(":x", dbData.x);
        m_batchQuery->bindValue(":y", dbData.y);
        m_batchQuery->bindValue(":flags", static_cast<uint>(dbData.flags));
        m_batchQuery->bindValue(":data", Utils::variantMapToBlob(dbData.data));
        m_batchQuery->bindValue(":attachments", Utils::listToString(dbData.attachments));
        if (!m_batchQuery->exec())
        {
            //qDebug() << "saveDataBatch failed: " << m_batchQuery->lastError().text();
        }

        return;
    }

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT EXISTS(SELECT 1 FROM data WHERE (projectUid = (:projectUid)) AND (uid = (:uid)))");
    faultIf(!success, query, "Failed to save data 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec() && query.next();
    faultIf(!success, query, "Failed to save data 2");
    auto exists = query.value(0).toInt() == 1;

    if (exists)
    {
        success = query.prepare("UPDATE data SET schemaHash = :schemaHash, timestamp = :timestamp, x = :x, y = :y, data = :data, attachments = :attachments WHERE (uid = (:uid)) AND (projectUid = (:projectUid))");
    }
    else
    {
        success = query.prepare("INSERT INTO data (projectUid, stateSpace, uid, schemaHash, timestamp, x, y, flags, data, attachments) VALUES (:projectUid, :stateSpace, :uid, :schemaHash, :timestamp, :x, :y, :flags, :data, :attachments)");
        query.bindValue(":stateSpace", dbData.stateSpace);
        query.bindValue(":flags", static_cast<uint>(dbData.flags));
    }

    faultIf(!success, query, "Failed to save data 3");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    query.bindValue(":schemaHash", dbData.schemaHash);
    query.bindValue(":timestamp", dbData.timestamp);
    query.bindValue(":x", dbData.x);
    query.bindValue(":y", dbData.y);
    query.bindValue(":data", Utils::variantMapToBlob(dbData.data));
    query.bindValue(":attachments", Utils::listToString(dbData.attachments));

    success = query.exec();
    faultIf(!success, query, "Failed to save data 4");
}

void Database::saveDataBatch(std::function<void()> callback)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);

    auto query = QSqlQuery(db);
    auto success = query.prepare("INSERT INTO data (projectUid, stateSpace, uid, schemaHash, timestamp, x, y, flags, data, attachments) VALUES (:projectUid, :stateSpace, :uid, :schemaHash, :timestamp, :x, :y, :flags, :data, :attachments)");
    faultIf(!success, query, "Failed to prepare batch query");

    m_batchQuery = &query;
    callback();
    m_batchQuery = nullptr;
}

void Database::loadData(const QString& /*projectUid*/, const QString& uid, DbData* dbDataOut) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT stateSpace, schemaHash, timestamp, x, y, flags, data, attachments FROM data WHERE (uid = (:uid))");
    faultIf(!success, query, "Failed to load data 1");
    query.bindValue(":uid", uid);
    success = query.exec();
    faultIf(!success, query, "Failed to load data 2");
    success = query.next();
    faultIf(!success, query, "Failed to load data 3");

    dbDataOut->stateSpace = query.value(0).toString();
    dbDataOut->schemaHash = query.value(1).toString();
    dbDataOut->timestamp = query.value(2).toString();
    dbDataOut->x = query.value(3).toDouble();
    dbDataOut->y = query.value(4).toDouble();
    dbDataOut->flags = DataFlags(query.value(5).toUInt());
    dbDataOut->data = Utils::variantMapFromBlob(query.value(6).toByteArray());
    dbDataOut->attachments = Utils::stringToList(query.value(7).toString());
}

bool Database::testData(const QString& /*projectUid*/, const QString& uid) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT uid FROM data WHERE (uid = (:uid))");
    faultIf(!success, query, "Failed to testData 1");
    query.bindValue(":uid", uid);
    success = query.exec();
    faultIf(!success, query, "Failed to testData 2");
    success = query.next();

    return success;
}

void Database::deleteData(const QString& projectUid)
{
    qDebug() << "Delete all data: " << projectUid;

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM data WHERE (projectUid = (:projectUid))");
    faultIf(!success, query, "Failed to deleteData 1");
    query.bindValue(":projectUid", projectUid);
    success = query.exec();
    faultIf(!success, query, "Failed to deleteData 2");
}

void Database::deleteData(const QString& projectUid, const QString& uid)
{
    qDebug() << "Delete data: " << projectUid << uid;

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    // Delete existing data if it exists.
    auto success = query.prepare("DELETE FROM data WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    faultIf(!success, query, "Failed to deleteData 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec();
    faultIf(!success, query, "Failed to deleteData 2");
}

void Database::deleteData(const QString& projectUid, const QString& stateSpace, DataFlags flags)
{
    qDebug() << "Delete data: " << projectUid;

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM data WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace)) AND (flags & (:flags) = (:flags))");
    faultIf(!success, query, "Failed to deleteData 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    query.bindValue(":flags", static_cast<uint>(flags));
    success = query.exec();
    faultIf(!success, query, "Failed to deleteData 2");
}

void Database::archiveData(const QString& archiveProjectUid, const QString& projectUid, const QString& uid)
{
    qDebug() << "Archive data: " << projectUid << uid;

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    // Archive existing data if it exists.
    auto success = query.prepare("UPDATE data SET projectUid = (:archiveProjectUid) WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    faultIf(!success, query, "Failed to archiveData 1");
    query.bindValue(":archiveProjectUid", archiveProjectUid);
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec();
    faultIf(!success, query, "Failed to archiveData 2");
}

void Database::archiveData(const QString& archiveProjectUid, const QString& projectUid, const QString& stateSpace, DataFlags flags)
{
    qDebug() << "Archive data: " << projectUid;

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("UPDATE data SET projectUid = (:archiveProjectUid) WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace)) AND (flags & (:flags) = (:flags))");
    faultIf(!success, query, "Failed to archiveData 1");
    query.bindValue(":archiveProjectUid", archiveProjectUid);
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    query.bindValue(":flags", static_cast<uint>(flags));
    success = query.exec();
    faultIf(!success, query, "Failed to archiveData 2");
}

void Database::enumData(const QString& whereSql, const QVariantMap& whereVars, EnumDataCallback callback) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto whereClause = whereSql.isEmpty() ? "" : " WHERE " + whereSql;

    auto success = query.prepare("SELECT rowid, projectUid, uid, stateSpace, schemaHash, timestamp, x, y, flags, data, queueState, attachments FROM data " + whereClause + " ORDER BY rowid");
    faultIf(!success, query, "Failed to enumData 1");

    for (auto whereVarIt = whereVars.constKeyValueBegin(); whereVarIt != whereVars.constKeyValueEnd(); whereVarIt++)
    {
        query.bindValue(whereVarIt->first, whereVarIt->second);
    }

    success = query.exec();
    faultIf(!success, query, "Failed to enumData 2");
    auto dbData = DbData();

    while (query.next())
    {
        dbData.rowId = query.value(0).toLongLong();
        auto projectUid = query.value(1).toString();
        auto uid = query.value(2).toString();

        dbData.stateSpace = query.value(3).toString();
        dbData.schemaHash = query.value(4).toString();
        dbData.timestamp = query.value(5).toString();
        dbData.x = query.value(6).toDouble();
        dbData.y = query.value(7).toDouble();
        dbData.flags = DataFlags(query.value(8).toUInt());
        dbData.data = Utils::variantMapFromBlob(query.value(9).toByteArray());
        dbData.attachments = Utils::stringToList(query.value(10).toString());

        callback(projectUid, uid, dbData);
    }
}

void Database::enumData(const QString& projectUid, const QString& stateSpace, DataFlags flagsAny, EnumDataCallback callback) const
{
    Mutex lock;
    verifyThread();

    auto whereSql = QStringList();
    auto whereVars = QVariantMap();
    if (projectUid != "*")
    {
        whereSql.append("(projectUid = (:projectUid))");
        whereVars.insert(":projectUid", projectUid);
    }
    if (stateSpace != "*")
    {
        whereSql.append("(stateSpace = (:stateSpace))");
        whereVars.insert(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    }
    whereSql.append("(flags & (:flagsAny) <> 0)");
    whereVars.insert(":flagsAny", static_cast<uint>(flagsAny));

    return enumData(whereSql.join(" AND "), whereVars, callback);
}

void Database::enumData(const QString& projectUid, const QString& stateSpace, DataFlags flagsOn, DataFlags flagsOff, EnumDataCallback callback) const
{
    Mutex lock;
    verifyThread();

    auto whereSql = QStringList();
    auto whereVars = QVariantMap();
    if (projectUid != "*")
    {
        whereSql.append("(projectUid = (:projectUid))");
        whereVars.insert(":projectUid", projectUid);
    }
    if (stateSpace != "*")
    {
        whereSql.append("(stateSpace = (:stateSpace))");
        whereVars.insert(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    }
    whereSql.append("(flags & (:flagsOn) = (:flagsOn)) AND (flags & (:flagsOff) = 0)");
    whereVars.insert(":flagsOn", static_cast<uint>(flagsOn));
    whereVars.insert(":flagsOff", static_cast<uint>(flagsOff));

    return enumData(whereSql.join(" AND "), whereVars, callback);
}

void Database::getData(const QString& projectUid, const QString& stateSpace, DataFlags flagsAny, QStringList* uidsOut) const
{
    uidsOut->clear();

    Mutex lock;

    enumData(projectUid, stateSpace, flagsAny, [&](const QString& /*projectUid*/, const QString& uid, const DbData& /*dbData*/)
    {
        uidsOut->append(uid);
    });
}

void Database::getData(const QString& projectUid, const QString& stateSpace, DataFlags flagsOn, DataFlags flagsOff, QStringList* uidsOut) const
{
    uidsOut->clear();

    Mutex lock;

    enumData(projectUid, stateSpace, flagsOn, flagsOff, [&](const QString& /*projectUid*/, const QString& uid, const DbData& /*dbData*/)
    {
        uidsOut->append(uid);
    });
}

Database::DataFlags Database::getDataFlags(const QString& projectUid, const QString& uid) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT flags FROM data WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    faultIf(!success, query, "Failed to getDataFlags 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec();
    faultIf(!success, query, "Failed to getDataFlags 2");
    success = query.next();
    faultIf(!success, query, "Failed to getDataFlags 3");

    return DataFlags(success ? query.value(0).toUInt() : 0);
}

void Database::setDataFlags(const QString& projectUid, const QString& uid, DataFlags flags, bool on)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = false;

    if (on)
    {
        success = query.prepare("UPDATE data SET flags = flags | :flags WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
        faultIf(!success, query, "Failed to setDataFlags 1");
    }
    else
    {
        flags = ~flags;
        success = query.prepare("UPDATE data SET flags = flags & :flags WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
        faultIf(!success, query, "Failed to setDataFlags 2");
    }

    query.bindValue(":flags", static_cast<uint>(flags));
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);

    success = query.exec();
    faultIf(!success, query, "Failed to setDataFlags 3");
}

void Database::setDataFlags(const QString& projectUid, const QString& stateSpace, DataFlags matchFlagsOn, DataFlags matchFlagsOff, DataFlags flags)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto flagsSql = QString("(flags & (:flagsOn) = (:flagsOn)) AND (flags & (:flagsOff) = 0)");

    auto success = query.prepare("UPDATE data SET flags = flags | :flags WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace)) AND " + flagsSql);
    faultIf(!success, query, "Failed to setDataFlags 1");

    query.bindValue(":projectUid", projectUid);
    query.bindValue(":stateSpace", stateSpace);
    query.bindValue(":flagsOn", static_cast<uint>(matchFlagsOn));
    query.bindValue(":flagsOff", static_cast<uint>(matchFlagsOff));
    query.bindValue(":flags", static_cast<uint>(flags));

    success = query.exec();
    faultIf(!success, query, "Failed to setDataFlags 2");
}

void Database::setDataFlagsAll(const QString& projectUid, DataFlags flags, bool on)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = false;

    if (on)
    {
        success = query.prepare("UPDATE data SET flags = flags | :flags WHERE (projectUid = (:projectUid))");
        faultIf(!success, query, "Failed to setDataFlagsAll 1");
    }
    else
    {
        flags = ~flags;
        success = query.prepare("UPDATE data SET flags = flags & :flags WHERE (projectUid = (:projectUid))");
        faultIf(!success, query, "Failed to setDataFlagsAll 2");
    }

    query.bindValue(":flags", static_cast<uint>(flags));
    query.bindValue(":projectUid", projectUid);

    success = query.exec();
    faultIf(!success, query, "Failed to setDataFlagsAll 3");
}

void Database::getDataQueueState(const QString& projectUid, const QString& uid, QVariantMap* stateOut) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT queueState FROM data WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    faultIf(!success, query, "Failed to getDataQueueState 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec();
    faultIf(!success, query, "Failed to getDataQueueState 2");
    success = query.next();

    // Allow failure if the data has been deleted.
    stateOut->clear();
    if (success)
    {
        *stateOut = Utils::variantMapFromJson(query.value(0).toByteArray());
    }
}

void Database::setDataQueueState(const QString& projectUid, const QString& uid, const QVariantMap& state)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("UPDATE data SET queueState = :state WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    faultIf(!success, query, "Failed to setDataQueueState 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    query.bindValue(":state", Utils::variantMapToJson(state, QJsonDocument::Compact));
    success = query.exec();

    // Allow failure if the data has been deleted.
    if (!success)
    {
        qDebug() << "Failed to update state for " << projectUid << uid;
    }
}

QString Database::getDataStateSpace(const QString& projectUid, const QString& uid) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT stateSpace FROM data WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    faultIf(!success, query, "Failed to getDataStateSpace 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec();
    faultIf(!success, query, "Failed to getDataStateSpace 2");
    success = query.next();
    faultIf(!success, query, "Failed to getDataStateSpace 3");

    return success ? query.value(0).toString() : QString();
}

void Database::saveFormState(const QString& projectUid, const QString& stateSpace, const QVariantMap& data, const QStringList& attachments, const QStringList& streamers)
{
    deleteFormState(projectUid, stateSpace);

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("INSERT INTO formState (projectUid, stateSpace, data, attachments, streamers) VALUES (:projectUid, :stateSpace, :data, :attachments, :streamers)");
    faultIf(!success, query, "Failed to saveFormState 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    query.bindValue(":data", Utils::variantMapToBlob(data));
    query.bindValue(":attachments", attachments.isEmpty() ? QVariant() : Utils::listToString(attachments));
    query.bindValue(":streamers", streamers.isEmpty() ? QVariant() : Utils::listToString(streamers));
    success = query.exec();
    faultIf(!success, query, "Failed to saveFormState 2");
}

void Database::loadFormState(const QString& projectUid, const QString& stateSpace, QVariantMap* dataOut, QStringList* attachmentsOut, QStringList* streamersOut) const
{
    if (dataOut)
    {
        dataOut->clear();
    }

    if (attachmentsOut)
    {
        attachmentsOut->clear();
    }

    if (streamersOut)
    {
        streamersOut->clear();
    }

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT data, attachments, streamers FROM formState WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace))");
    faultIf(!success, query, "Failed to load formState 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    success = query.exec() && query.next();

    if (success)
    {
        if (dataOut)
        {
            auto dataBlob = query.value(0).toByteArray();
            qFatalIf(dataBlob.isEmpty(), "Failed to load formState 3");
            *dataOut = Utils::variantMapFromBlob(dataBlob);
        }

        if (attachmentsOut)
        {
            *attachmentsOut = Utils::stringToList(query.value(1).toString());
        }

        if (streamersOut)
        {
            *streamersOut = Utils::stringToList(query.value(2).toString());
        }
    }
}

void Database::deleteFormState(const QString& projectUid, const QString& stateSpace)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = false;

    if (stateSpace == "*")
    {
        success = query.prepare("DELETE FROM formState WHERE (projectUid = (:projectUid))");
    }
    else
    {
        success = query.prepare("DELETE FROM formState WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace))");
        query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    }

    faultIf(!success, query, "Failed to delete formState 1");
    query.bindValue(":projectUid", projectUid);
    query.exec();   // Will fail if the formState is not present.
}

void Database::addExport(const QString& exportUid, const QVariantMap& data)
{
    removeExport(exportUid);

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("INSERT INTO exports (exportUid, data) VALUES (:exportUid, :data)");
    faultIf(!success, query, "Failed to add export 1");
    query.bindValue(":exportUid", exportUid);
    query.bindValue(":data", Utils::variantMapToBlob(data));
    success = query.exec();
    faultIf(!success, query, "Failed to add export 2");
}

void Database::getExports(QStringList* exportUidsOut) const
{
    exportUidsOut->clear();

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT exportUid FROM exports");
    faultIf(!success, query, "Failed to get exports 1");
    success = query.exec();
    faultIf(!success, query, "Failed to get exports 2");

    while (query.next())
    {
        exportUidsOut->append(query.value(0).toString());
    }
}

void Database::loadExport(const QString& exportUid, QVariantMap* dataOut) const
{
    dataOut->clear();

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT data FROM exports WHERE (exportUid = (:exportUid))");
    faultIf(!success, query, "Failed to load export 1");
    query.bindValue(":exportUid", exportUid);
    success = query.exec();
    faultIf(!success, query, "Failed to load export 2");
    success = query.next();
    faultIf(!success, query, "Failed to load export 3");

    auto dataBlob = query.value(0).toByteArray();
    if (!dataBlob.isEmpty())
    {
        *dataOut = Utils::variantMapFromBlob(dataBlob);
    }
}

void Database::saveExport(const QString& exportUid, const QVariantMap& data)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("UPDATE exports SET data = :data WHERE (exportUid = (:exportUid))");
    faultIf(!success, query, "Failed to save export 1");
    query.bindValue(":exportUid", exportUid);
    query.bindValue(":data", Utils::variantMapToBlob(data));
    success = query.exec();
    faultIf(!success, query, "Failed to save export 2");
}

void Database::removeExport(const QString& exportUid)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM exports WHERE (exportUid = (:exportUid))");
    faultIf(!success, query, "Failed to remove export 1");
    query.bindValue(":exportUid", exportUid);
    query.exec();
}

void Database::addBackupFile(const DbBackupFile& backupFile)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("INSERT INTO backups (timestamp, projectTitle, filename) VALUES (:timestamp, :projectTitle, :filename)");
    faultIf(!success, query, "Failed to add backup file 1");
    query.bindValue(":timestamp", backupFile.timestamp);
    query.bindValue(":projectTitle", backupFile.projectTitle);
    query.bindValue(":filename", backupFile.filename);
    success = query.exec();
    faultIf(!success, query, "Failed to add backup file 2");
}

void Database::enumBackupFiles(std::function<void(const DbBackupFile& backupFile)> callback) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT timestamp, projectTitle, filename FROM backups ORDER BY rowid");
    faultIf(!success, query, "Failed to enum backup files 1");
    success = query.exec();
    faultIf(!success, query, "Failed to enum backup files 2");

    while (query.next())
    {
        auto backupFile = DbBackupFile();
        backupFile.timestamp = query.value(0).toString();
        backupFile.projectTitle = query.value(1).toString();
        backupFile.filename = query.value(2).toString();
        callback(backupFile);
    }
}

void Database::deleteBackupFile(const QString& filename)
{
    qDebug() << "Delete backupFile: " << filename;

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    // Delete existing data if it exists.
    auto success = query.prepare("DELETE FROM backups WHERE (filename = (:filename))");
    faultIf(!success, query, "Failed to deleteBackupFile 1");
    query.bindValue(":filename", filename);
    success = query.exec();
    faultIf(!success, query, "Failed to deleteBackupFile 2");
}

void Database::clearBackupFiles()
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM backups");
    faultIf(!success, query, "Failed to clear backup files 1");
    success = query.exec();
    faultIf(!success, query, "Failed to clear backup files 2");
}

// Queue.
void Database::appendQueueItem(const DbQueueItem& queueItem)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("INSERT INTO queue (uid, projectUid, dataUid, summary, submitTime, uploadTime, state, flags, type, url, payload, payloadError, webhook, webhookError, attachments) VALUES (:uid, :projectUid, :dataUid, :summary, :submitTime, :uploadTime, :state, :flags, :type, :url, :payload, :payloadError, :webhook, :webhookError, :attachments)");
    faultIf(!success, query, "Failed to appendQueueItem 1");

    query.bindValue(":uid", queueItem.uid);
    query.bindValue(":projectUid", queueItem.projectUid);
    query.bindValue(":dataUid", queueItem.dataUid);
    query.bindValue(":summary", queueItem.summary);
    query.bindValue(":submitTime", queueItem.submitTime);
    query.bindValue(":uploadTime", queueItem.uploadTime);
    query.bindValue(":state", Utils::variantMapToJson(queueItem.state, QJsonDocument::Compact));
    query.bindValue(":flags", queueItem.flags.toInt());
    query.bindValue(":type", queueItem.type);
    query.bindValue(":url", queueItem.url);
    query.bindValue(":payload", queueItem.payload);
    query.bindValue(":payloadError", queueItem.payloadError);
    query.bindValue(":webhook", queueItem.webhook);
    query.bindValue(":webhookHrror", queueItem.webhookError);
    query.bindValue(":attachments", Utils::listToString(queueItem.attachments));

    success = query.exec();
    faultIf(!success, query, "Failed to appendQueueItem 2");
}

void Database::loadQueueItem(const QString& uid, DbQueueItem* queueItemOut)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT uid, projectUid, dataUid, summary, submitTime, uploadTime, state, flags, type, url, payload, payloadError, webhook, webhookError, attachments FROM queue WHERE (uid = (:uid)) ORDER BY rowid");
    faultIf(!success, query, "Failed to loadQueueItem 1");

    query.bindValue(":uid", uid);
    success = query.exec();
    faultIf(!success, query, "Failed to loadQueueItem 2");

    success = query.next();
    faultIf(!success, query, "Failed to loadQueueItem 3");

    queueItemOut->uid = query.value(0).toString();
    queueItemOut->projectUid = query.value(1).toString();
    queueItemOut->dataUid = query.value(2).toString();
    queueItemOut->summary = query.value(3).toString();
    queueItemOut->submitTime = query.value(4).toString();
    queueItemOut->uploadTime = query.value(5).toString();
    queueItemOut->state = Utils::variantMapFromJson(query.value(6).toByteArray());
    queueItemOut->flags = QueueFlags::fromInt(query.value(7).toInt());
    queueItemOut->type = query.value(8).toString();
    queueItemOut->url = query.value(9).toString();
    queueItemOut->payload = query.value(10).toByteArray();
    queueItemOut->payloadError = query.value(11).toString();
    queueItemOut->webhook = query.value(12).toByteArray();
    queueItemOut->webhookError = query.value(13).toString();
    queueItemOut->attachments = Utils::stringToList(query.value(14).toString());
}

void Database::enumQueueItems(std::function<void(const DbQueueItem& queueItem)> callback)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT uid, projectUid, dataUid, summary, submitTime, uploadTime, state, flags, type, url, payload, payloadError, webhook, webhookError, attachments FROM queue ORDER BY rowid");
    faultIf(!success, query, "Failed to enumQueueItems 1");

    success = query.exec();
    faultIf(!success, query, "Failed to enumQueueItems 2");
    auto queueItem = DbQueueItem();

    while (query.next())
    {
        queueItem.uid = query.value(0).toString();
        queueItem.projectUid = query.value(1).toString();
        queueItem.dataUid = query.value(2).toString();
        queueItem.summary = query.value(3).toString();
        queueItem.submitTime = query.value(4).toString();
        queueItem.uploadTime = query.value(5).toString();
        queueItem.state = Utils::variantMapFromJson(query.value(6).toByteArray());
        queueItem.flags = QueueFlags::fromInt(query.value(7).toInt());
        queueItem.type = query.value(8).toString();
        queueItem.url = query.value(9).toString();
        queueItem.payload = query.value(10).toByteArray();
        queueItem.payloadError = query.value(11).toString();
        queueItem.webhook = query.value(12).toByteArray();
        queueItem.webhookError = query.value(13).toString();
        queueItem.attachments = Utils::stringToList(query.value(14).toString());

        callback(queueItem);
    }
}

void Database::setQueueItemState(const QString& uid, const QVariantMap& state)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("UPDATE queue SET state = :state WHERE (uid = (:uid))");
    faultIf(!success, query, "Failed to setQueueItemState 1");
    query.bindValue(":uid", uid);
    query.bindValue(":state", Utils::variantMapToJson(state, QJsonDocument::Compact));
    success = query.exec();
    faultIf(!success, query, "Failed to setQueueItemState 3");
}

void Database::setQueueItemFlags(const QString& uid, QueueFlags flags, bool on)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = false;
    if (on)
    {
        success = query.prepare("UPDATE queue SET flags = flags | :flags WHERE (uid = (:uid))");
        faultIf(!success, query, "Failed to setQueueItemFlags 1");
    }
    else
    {
        flags = ~flags;
        success = query.prepare("UPDATE queue SET flags = flags & :flags WHERE (uid = (:uid))");
        faultIf(!success, query, "Failed to setQueueItemFlags 2");
    }

    query.bindValue(":uid", uid);
    query.bindValue(":flags", flags.toInt());
    success = query.exec();
    faultIf(!success, query, "Failed to setQueueItemFlags 3");
}

void Database::setQueueItemPayloadError(const QString& uid, const QString& error, const QString& timestamp)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("UPDATE queue SET payloadError = :payloadError, uploadTime = :uploadTime WHERE (uid = (:uid))");
    faultIf(!success, query, "Failed to setQueueItemPayloadError 1");
    query.bindValue(":uid", uid);
    query.bindValue(":payloadError", error);
    query.bindValue(":uploadTime", timestamp);
    success = query.exec();
    faultIf(!success, query, "Failed to setQueueItemPayloadError 2");
}

void Database::setQueueItemWebhookError(const QString& uid, const QString& error)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("UPDATE queue SET webhookError = :error WHERE (uid = (:uid))");
    faultIf(!success, query, "Failed to setQueueItemWebhookError 1");
    query.bindValue(":uid", uid);
    query.bindValue(":error", error);
    success = query.exec();
    faultIf(!success, query, "Failed to setQueueItemWebhookError 2");
}

void Database::deleteQueueItems(QueueFlags flags)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM queue WHERE (flags & (:flags) = (:flags))");
    faultIf(!success, query, "Failed to deleteQueueItems 1");
    query.bindValue(":flags", flags.toInt());
    success = query.exec();
    faultIf(!success, query, "Failed to deleteQueueItems 2");
}

void Database::deleteQueueAttachments(QueueFlags flags)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("UPDATE queue SET attachments = '' WHERE (flags & (:flags) = (:flags))");
    faultIf(!success, query, "Failed to deleteQueueItems 1");
    query.bindValue(":flags", flags.toInt());
    success = query.exec();
    faultIf(!success, query, "Failed to deleteQueueItems 2");
}

void Database::resetQueueItems()
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM queue");
    faultIf(!success, query, "Failed to resetQueueItems 1");
    query.exec();
    faultIf(!success, query, "Failed to resetQueueItems 2");
}

// Transcribe.
bool Database::testTranscribeItem(const QString& projectUid, const QString& dataUid) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT EXISTS(SELECT 1 FROM transcribe WHERE (projectUid = (:projectUid)) AND (dataUid = (:dataUid)))");
    faultIf(!success, query, "Failed to testTranscribeItem 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":dataUid", dataUid);
    success = query.exec() && query.next();
    faultIf(!success, query, "Failed to testTranscribeItem 2");
    return query.value(0).toInt() == 1;
}

bool Database::hasPendingTranscribeItems(const QString& projectUid, const QString& dataUid) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT EXISTS(SELECT 1 FROM transcribe WHERE (projectUid = (:projectUid)) AND (dataUid = (:dataUid)) AND (error IS NULL OR error = '' OR error != 'Success'))");
    faultIf(!success, query, "Failed to hasPendingTranscribeItems 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":dataUid", dataUid);
    success = query.exec() && query.next();
    faultIf(!success, query, "Failed to hasPendingTranscribeItems 2");
    return query.value(0).toInt() == 1;
}

void Database::appendTranscribeItem(const DbTranscribeItem& transcribeItem)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("INSERT INTO transcribe (filename, projectUid, stateSpace, dataUid, prompt, schema, fillRecordUid, fillFieldUids) VALUES (:filename, :projectUid, :stateSpace, :dataUid, :prompt, :schema, :fillRecordUid, :fillFieldUids)");
    query.bindValue(":filename", transcribeItem.filename);
    query.bindValue(":projectUid", transcribeItem.projectUid);
    query.bindValue(":stateSpace", transcribeItem.stateSpace);
    query.bindValue(":dataUid", transcribeItem.dataUid);
    query.bindValue(":prompt", transcribeItem.prompt);
    query.bindValue(":schema", transcribeItem.schema);
    query.bindValue(":fillRecordUid", transcribeItem.fillRecordUid);
    query.bindValue(":fillFieldUids", Utils::listToString(transcribeItem.fillFieldUids));
    faultIf(!success, query, "Failed to appendTranscribeItem 1");
    success = query.exec();
    faultIf(!success, query, "Failed to appendTranscribeItem 2");
}

void Database::updateTranscribeItem(const DbTranscribeItem& transcribeItem)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("UPDATE transcribe SET result = :result, error = :error WHERE (filename = (:filename))");
    faultIf(!success, query, "Failed to updateTranscribeItem 1");
    query.bindValue(":filename", transcribeItem.filename);
    query.bindValue(":result", Utils::variantMapToJson(transcribeItem.result));
    query.bindValue(":error", transcribeItem.error);
    success = query.exec();
    faultIf(!success, query, "Failed to updateTranscribeItem 2");
}

void Database::deleteTranscribeItem(const QString& filename)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    // Delete existing data if it exists.
    auto success = query.prepare("DELETE FROM transcribe WHERE (filename = (:filename))");
    faultIf(!success, query, "Failed to deleteTranscribeItem 1");
    query.bindValue(":filename", filename);
    /*success = */query.exec();

    // Failure is okay, it just means that it was consumed already.
    //faultIf(!success, query, "Failed to deleteTranscribeItem 2");
}

void Database::enumTranscribeItems(std::function<void(const DbTranscribeItem& transcribeItem)> callback) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT filename, projectUid, stateSpace, dataUid, prompt, schema, result, error, fillRecordUid, fillFieldUids FROM transcribe ORDER BY rowid");
    faultIf(!success, query, "Failed to enumTranscribeItems 1");
    success = query.exec();
    faultIf(!success, query, "Failed to enumTranscribeItems 2");

    while (query.next())
    {
        auto transcribeItem = DbTranscribeItem();
        transcribeItem.filename = query.value(0).toString();
        transcribeItem.projectUid = query.value(1).toString();
        transcribeItem.stateSpace = query.value(2).toString();
        transcribeItem.dataUid = query.value(3).toString();
        transcribeItem.prompt = query.value(4).toString();
        transcribeItem.schema = query.value(5).toString();
        transcribeItem.result = Utils::variantMapFromJson(query.value(6).toByteArray());
        transcribeItem.error = query.value(7).toString();
        transcribeItem.fillRecordUid = query.value(8).toString();
        transcribeItem.fillFieldUids = Utils::stringToList(query.value(9).toString());

        callback(transcribeItem);
    }
}

// Audit.
void Database::addAudit(const DbAuditItem& auditItem)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("INSERT INTO audit (timestamp, projectTitle, description) VALUES (:timestamp, :projectTitle, :description)");
    faultIf(!success, query, "Failed to add audit 1");
    query.bindValue(":timestamp", auditItem.timestamp);
    query.bindValue(":projectTitle", auditItem.projectTitle);
    query.bindValue(":description", auditItem.description);
    success = query.exec();
    faultIf(!success, query, "Failed to add audit 2");
}

void Database::enumAudit(std::function<void(const DbAuditItem& auditItem)> callback) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT timestamp, projectTitle, description FROM audit ORDER BY rowid");
    faultIf(!success, query, "Failed to enumAudit 1");
    success = query.exec();
    faultIf(!success, query, "Failed to enumAudit 2");

    while (query.next())
    {
        auto entry = DbAuditItem();
        entry.timestamp = query.value(0).toString();
        entry.projectTitle = query.value(1).toString();
        entry.description = query.value(2).toString();
        callback(entry);
    }
}

void Database::clearAudit()
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM audit");
    faultIf(!success, query, "Failed to clearAudit 1");
    success = query.exec();
    faultIf(!success, query, "Failed to clearAudit 2");
}

QStringList Database::getStreamers()
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto streamers = QStringList();

    auto success = query.prepare("SELECT streamers FROM formState WHERE streamers is not null");
    faultIf(!success, query, "Failed to getStreamers 1");
    success = query.exec();
    faultIf(!success, query, "Failed to getStreamers 2");
    while (query.next())
    {
        streamers.append(Utils::stringToList(query.value(0).toString()));
    }

    return streamers;
}

bool Database::hasStreamers(const QString& projectUid)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT streamers FROM formState WHERE (projectUid = (:projectUid)) AND (streamers is not null)");
    query.bindValue(":projectUid", projectUid);
    faultIf(!success, query, "Failed to hasStreamers 1");
    success = query.exec();
    faultIf(!success, query, "Failed to hasStreamers 2");

    return query.next();
}

QStringList Database::getAttachments()
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto attachments = QStringList();

    auto success = query.prepare("SELECT attachments FROM data WHERE attachments is not null");
    faultIf(!success, query, "Failed to getAttachments 1");
    success = query.exec();
    faultIf(!success, query, "Failed to getAttachments 2");
    while (query.next())
    {
        attachments.append(Utils::stringToList(query.value(0).toString()));
    }

    success = query.prepare("SELECT attachments FROM formState WHERE attachments is not null");
    faultIf(!success, query, "Failed to getAttachments 3");
    success = query.exec();
    faultIf(!success, query, "Failed to getAttachments 4");
    while (query.next())
    {
        attachments.append(Utils::stringToList(query.value(0).toString()));
    }

    success = query.prepare("SELECT attachments FROM queue WHERE attachments is not null");
    faultIf(!success, query, "Failed to getAttachments 5");
    success = query.exec();
    faultIf(!success, query, "Failed to getAttachments 6");
    while (query.next())
    {
        attachments.append(Utils::stringToList(query.value(0).toString()));
    }

    success = query.prepare("SELECT filename FROM transcribe WHERE filename is not null");
    faultIf(!success, query, "Failed to getAttachments 7");
    success = query.exec();
    faultIf(!success, query, "Failed to getAttachments 8");
    while (query.next())
    {
        auto filename = query.value(0).toString();
        if (!attachments.contains(filename))
        {
            attachments.append(filename);
        }
    }

    return attachments;
}

void Database::saveMapPackage(const DbMapPackage& package)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("INSERT OR REPLACE INTO mapPackages (id, folder, projectUid, name, version, updateUrl, eTag, lastUpdate) VALUES (:id, :folder, :projectUid, :name, :version, :updateUrl, :eTag, :lastUpdate)");
    faultIf(!success, query, "Failed to save map package 1");
    query.bindValue(":id", package.id);
    query.bindValue(":folder", package.folder);
    query.bindValue(":projectUid", package.projectUid);
    query.bindValue(":name", package.name);
    query.bindValue(":version", package.version);
    query.bindValue(":updateUrl", package.updateUrl);
    query.bindValue(":eTag", package.eTag);
    query.bindValue(":lastUpdate", package.lastUpdate);
    success = query.exec();
    faultIf(!success, query, "Failed to save map package 2");
}

bool Database::loadMapPackage(const QString& id, DbMapPackage* packageOut) const
{
    *packageOut = DbMapPackage();

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT id, folder, projectUid, name, version, updateUrl, eTag, lastUpdate FROM mapPackages WHERE (id = (:id))");
    faultIf(!success, query, "Failed to load map package 1");
    query.bindValue(":id", id);
    success = query.exec();
    faultIf(!success, query, "Failed to load map package 2");

    if (!query.next())
    {
        return false;
    }

    packageOut->id = query.value(0).toString();
    packageOut->folder = query.value(1).toString();
    packageOut->projectUid = query.value(2).toString();
    packageOut->name = query.value(3).toString();
    packageOut->version = query.value(4).toString();
    packageOut->updateUrl = query.value(5).toString();
    packageOut->eTag = query.value(6).toString();
    packageOut->lastUpdate = query.value(7).toString();
    return true;
}

void Database::enumMapPackages(std::function<void(const DbMapPackage& package)> callback) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT id, folder, projectUid, name, version, updateUrl, eTag, lastUpdate FROM mapPackages");
    faultIf(!success, query, "Failed to enum map packages 1");
    success = query.exec();
    faultIf(!success, query, "Failed to enum map packages 2");

    while (query.next())
    {
        auto package = DbMapPackage();
        package.id = query.value(0).toString();
        package.folder = query.value(1).toString();
        package.projectUid = query.value(2).toString();
        package.name = query.value(3).toString();
        package.version = query.value(4).toString();
        package.updateUrl = query.value(5).toString();
        package.eTag = query.value(6).toString();
        package.lastUpdate = query.value(7).toString();
        callback(package);
    }
}

void Database::enumMapPackagesByProject(const QString& projectUid, std::function<void(const DbMapPackage& package)> callback) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT id, folder, projectUid, name, version, updateUrl, eTag, lastUpdate FROM mapPackages WHERE (projectUid = (:projectUid))");
    faultIf(!success, query, "Failed to enum map packages by project 1");
    query.bindValue(":projectUid", projectUid);
    success = query.exec();
    faultIf(!success, query, "Failed to enum map packages by project 2");

    while (query.next())
    {
        auto package = DbMapPackage();
        package.id = query.value(0).toString();
        package.folder = query.value(1).toString();
        package.projectUid = query.value(2).toString();
        package.name = query.value(3).toString();
        package.version = query.value(4).toString();
        package.updateUrl = query.value(5).toString();
        package.eTag = query.value(6).toString();
        package.lastUpdate = query.value(7).toString();
        callback(package);
    }
}

void Database::deleteMapPackage(const QString& id)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM mapPackages WHERE (id = (:id))");
    faultIf(!success, query, "Failed to delete map package 1");
    query.bindValue(":id", id);
    query.exec();
}

void Database::saveMapLayer(const DbMapLayer& layer)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("INSERT OR REPLACE INTO mapLayers (id, packageId, filename, name, category, active, opacity, symbol, lookup, sequence) VALUES (:id, :packageId, :filename, :name, :category, :active, :opacity, :symbol, :lookup, :sequence)");
    faultIf(!success, query, "Failed to save map layer 1");
    query.bindValue(":id", layer.id);
    query.bindValue(":packageId", layer.packageId);
    query.bindValue(":filename", layer.filename);
    query.bindValue(":name", layer.name);
    query.bindValue(":category", layer.category);
    query.bindValue(":active", layer.active ? 1 : 0);
    query.bindValue(":opacity", layer.opacity);
    query.bindValue(":symbol", layer.symbol.isEmpty() ? QString() : QString::fromUtf8(Utils::variantMapToJson(layer.symbol, QJsonDocument::Compact)));
    query.bindValue(":lookup", layer.lookup ? 1 : 0);
    query.bindValue(":sequence", layer.sequence);
    success = query.exec();
    faultIf(!success, query, "Failed to save map layer 2");
}

bool Database::loadMapLayer(const QString& id, DbMapLayer* layerOut) const
{
    *layerOut = DbMapLayer();

    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT id, packageId, filename, name, category, active, opacity, symbol, lookup, sequence FROM mapLayers WHERE (id = (:id))");
    faultIf(!success, query, "Failed to load map layer 1");
    query.bindValue(":id", id);
    success = query.exec();
    faultIf(!success, query, "Failed to load map layer 2");

    if (!query.next())
    {
        return false;
    }

    layerOut->id = query.value(0).toString();
    layerOut->packageId = query.value(1).toString();
    layerOut->filename = query.value(2).toString();
    layerOut->name = query.value(3).toString();
    layerOut->category = query.value(4).toString();
    layerOut->active = query.value(5).toInt() != 0;
    layerOut->opacity = query.value(6).toDouble();
    auto symbolJson = query.value(7).toString();
    if (!symbolJson.isEmpty())
    {
        layerOut->symbol = Utils::variantMapFromJson(symbolJson.toUtf8());
    }
    layerOut->lookup = query.value(8).toInt() != 0;
    layerOut->sequence = query.value(9).toInt();
    return true;
}

void Database::enumMapLayers(std::function<void(const DbMapLayer& layer)> callback) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT id, packageId, filename, name, category, active, opacity, symbol, lookup, sequence FROM mapLayers ORDER BY sequence");
    faultIf(!success, query, "Failed to enum map layers 1");
    success = query.exec();
    faultIf(!success, query, "Failed to enum map layers 2");

    while (query.next())
    {
        auto layer = DbMapLayer();
        layer.id = query.value(0).toString();
        layer.packageId = query.value(1).toString();
        layer.filename = query.value(2).toString();
        layer.name = query.value(3).toString();
        layer.category = query.value(4).toString();
        layer.active = query.value(5).toInt() != 0;
        layer.opacity = query.value(6).toDouble();
        auto symbolJson = query.value(7).toString();
        if (!symbolJson.isEmpty())
        {
            layer.symbol = Utils::variantMapFromJson(symbolJson.toUtf8());
        }
        layer.lookup = query.value(8).toInt() != 0;
        layer.sequence = query.value(9).toInt();
        callback(layer);
    }
}

void Database::enumMapLayersByPackage(const QString& packageId, std::function<void(const DbMapLayer& layer)> callback) const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT id, packageId, filename, name, category, active, opacity, symbol, lookup, sequence FROM mapLayers WHERE (packageId = (:packageId)) ORDER BY sequence");
    faultIf(!success, query, "Failed to enum map layers by package 1");
    query.bindValue(":packageId", packageId);
    success = query.exec();
    faultIf(!success, query, "Failed to enum map layers by package 2");

    while (query.next())
    {
        auto layer = DbMapLayer();
        layer.id = query.value(0).toString();
        layer.packageId = query.value(1).toString();
        layer.filename = query.value(2).toString();
        layer.name = query.value(3).toString();
        layer.category = query.value(4).toString();
        layer.active = query.value(5).toInt() != 0;
        layer.opacity = query.value(6).toDouble();
        auto symbolJson = query.value(7).toString();
        if (!symbolJson.isEmpty())
        {
            layer.symbol = Utils::variantMapFromJson(symbolJson.toUtf8());
        }
        layer.lookup = query.value(8).toInt() != 0;
        layer.sequence = query.value(9).toInt();
        callback(layer);
    }
}

void Database::deleteMapLayer(const QString& id)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM mapLayers WHERE (id = (:id))");
    faultIf(!success, query, "Failed to delete map layer 1");
    query.bindValue(":id", id);
    query.exec();
}

void Database::deleteMapLayersByPackage(const QString& packageId)
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("DELETE FROM mapLayers WHERE (packageId = (:packageId))");
    faultIf(!success, query, "Failed to delete map layers by package 1");
    query.bindValue(":packageId", packageId);
    query.exec();
}

int Database::getMaxMapLayerSequence() const
{
    Mutex lock;
    verifyThread();
    auto db = QSqlDatabase::database(m_connectionName);
    auto query = QSqlQuery(db);

    auto success = query.prepare("SELECT MAX(sequence) FROM mapLayers");
    faultIf(!success, query, "Failed to get max map layer sequence 1");
    success = query.exec();
    faultIf(!success, query, "Failed to get max map layer sequence 2");

    if (!query.next())
    {
        return 0;
    }

    auto value = query.value(0);
    if (value.isNull())
    {
        return 0;
    }
    return value.toInt();
}

} // namespace CyberTracker
