#include "pch.h"
#include "Database.h"

namespace  {
QThreadStorage<QPair<QSqlDatabase, int>> s_databases;
bool s_preparedOnce = false;
}

Database::Database(QObject* parent): QObject(parent)
{
}

Database::Database(const QString& filePath, QObject* parent): QObject(parent)
{
    m_filePath = filePath;
    m_connectionName = "ct_db_" + QString::number((quint64)QThread::currentThread(), 16);

    auto db = s_databases.localData();
    if (!db.first.isValid())
    {
        db.first = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
        db.first.setDatabaseName(filePath);
        db.second = 0;
        qFatalIf(!db.first.open(), "Cannot open sqlite database: " + filePath);
    }

    db.second++;
    s_databases.setLocalData(db);

    m_db = db.first;

    if (!s_preparedOnce)
    {
        s_preparedOnce = true;
        QSqlQuery query = QSqlQuery(m_db);
        auto success = query.prepare("CREATE TABLE IF NOT EXISTS projects (projectUid TEXT, data BLOB)");
        qFatalIf(!success, "Failed to create projects table 1");
        success = query.exec();
        qFatalIf(!success, "Failed to create projects table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS sightings (projectUid TEXT, stateSpace TEXT, uid TEXT, flags INTEGER, data BLOB, attachments TEXT)");
        qFatalIf(!success, "Failed to create sightings table 1");
        success = query.exec();
        qFatalIf(!success, "Failed to create sightings table 2");
        success = query.prepare("CREATE UNIQUE INDEX IF NOT EXISTS sightingIndex ON sightings (uid)");
        qFatalIf(!success, "Failed to create sightings table 3");
        success = query.exec();
        qFatalIf(!success, "Failed to create sightings table 4");

        success = query.prepare("CREATE TABLE IF NOT EXISTS formState (projectUid TEXT, stateSpace TEXT, data BLOB)");
        qFatalIf(!success, "Failed to create formState table 1");
        success = query.exec();
        qFatalIf(!success, "Failed to create formState table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS exports (exportUid TEXT, data BLOB)");
        qFatalIf(!success, "Failed to create exports table 1");
        success = query.exec();
        qFatalIf(!success, "Failed to create exports table 2");

        success = query.prepare("CREATE TABLE IF NOT EXISTS tasks (projectUid TEXT, uid TEXT, parentUid TEXT, input BLOB, output BLOB, state INTEGER)");
        qFatalIf(!success, "Failed to create tasks table 1");
        success = query.exec();
        qFatalIf(!success, "Failed to create tasks table 2");

        success = query.prepare("DROP INDEX taskIndex");    // Tasks names can be duplicated.
        if (success)
        {
            success = query.exec();
            qFatalIf(!success, "Failed to create tasks table 3");
        }

        // Add attachments to sightings table.
        success = query.prepare("ALTER TABLE sightings ADD attachments TEXT");
        if (success)
        {
            query.exec();
        }

        // Add attachments to formState table.
        success = query.prepare("ALTER TABLE formState ADD attachments TEXT");
        if (success)
        {
            query.exec();
        }
    }
}

Database::~Database()
{
    auto db = s_databases.localData();
    qFatalIf(!db.first.isValid(), "Bad database reference 1");
    qFatalIf(db.second <= 0, "Bad database reference 2");
    db.second--;

    if (db.second == 0)
    {
        m_db.close();
        m_db = db.first = QSqlDatabase::database();
        s_databases.setLocalData(db);   // Must occur before removeDatabase.
        QSqlDatabase::removeDatabase(m_connectionName);
    }
    else
    {
        s_databases.setLocalData(db);
    }
}

void Database::verifyThread() const
{
    auto connectionName = "ct_db_" + QString::number((quint64)QThread::currentThread(), 16);
    qFatalIf(connectionName != m_connectionName, "Wrong thread");
}

void Database::compact()
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("VACUUM");
    qFatalIf(!success, "Failed compact 1");
    success = query.exec();
    qFatalIf(!success, "Failed compact 2");
}

void Database::addProject(const QString& projectUid)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT EXISTS(SELECT 1 FROM projects WHERE (projectUid = (:projectUid)))");
    qFatalIf(!success, "Failed to find project 1");
    query.bindValue(":projectUid", projectUid);
    success = query.exec() && query.next();
    qFatalIf(!success, "Failed to find project 2");
    auto exists = query.value(0).toInt() == 1;

    if (!exists)
    {
        query = QSqlQuery(m_db);
        success = query.prepare("INSERT INTO projects (projectUid) VALUES (:projectUid)");
        qFatalIf(!success, "Failed to add project 1");
        query.bindValue(":projectUid", projectUid);
        success = query.exec();
        qFatalIf(!success, "Failed to add project 2");
    }
}

void Database::getProjects(QStringList* projectUidsOut) const
{
    verifyThread();
    projectUidsOut->clear();

    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT projectUid FROM projects");
    qFatalIf(!success, "Failed to get projects 1");
    success = query.exec();
    qFatalIf(!success, "Failed to get projects 2");

    while (query.next())
    {
        projectUidsOut->append(query.value(0).toString());
    }
}

void Database::removeProject(const QString& projectUid)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("DELETE FROM projects WHERE (projectUid = (:projectUid))");
    qFatalIf(!success, "Failed to remove project 1");
    query.bindValue(":projectUid", projectUid);
    query.exec();

    success = query.prepare("DELETE FROM formState WHERE (projectUid = (:projectUid))");
    qFatalIf(!success, "Failed to remove project 2");
    query.bindValue(":projectUid", projectUid);
    query.exec();

    success = query.prepare("DELETE FROM sightings WHERE (projectUid = (:projectUid))");
    qFatalIf(!success, "Failed to remove project 3");
    query.bindValue(":projectUid", projectUid);
    query.exec();

    success = query.prepare("DELETE FROM tasks WHERE (projectUid = (:projectUid))");
    qFatalIf(!success, "Failed to remove project 4");
    query.bindValue(":projectUid", projectUid);
    query.exec();
}

void Database::clearProjects()
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("DELETE FROM projects");
    qFatalIf(!success, "Failed to delete projects");
    query.exec();
}

void Database::saveProject(const QString& projectUid, const QVariantMap& data)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("UPDATE projects SET data = :data WHERE (projectUid = (:projectUid))");
    qFatalIf(!success, "Failed to save project 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":data", Utils::variantMapToBlob(data));
    success = query.exec();
    qFatalIf(!success, "Failed to save project 2");
}

void Database::loadProject(const QString& projectUid, QVariantMap* dataOut) const
{
    verifyThread();
    dataOut->clear();

    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT data FROM projects WHERE (projectUid = (:projectUid))");
    qFatalIf(!success, "Failed to load project 1");
    query.bindValue(":projectUid", projectUid);
    success = query.exec();
    qFatalIf(!success, "Failed to load project 2");
    success = query.next();
    qFatalIf(!success, "Failed to load project 3");

    auto dataBlob = query.value(0).toByteArray();
    if (!dataBlob.isEmpty())
    {
        *dataOut = Utils::variantMapFromBlob(dataBlob);
    }
}

bool Database::hasSightings(const QString& projectUid, const QString& stateSpace, uint flags) const
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = false;

    if (stateSpace == "*")
    {
        success = query.prepare("SELECT EXISTS(SELECT 1 FROM sightings WHERE (projectUid = (:projectUid)) AND (flags & (:flags) <> 0))");
    }
    else
    {
        success = query.prepare("SELECT EXISTS(SELECT 1 FROM sightings WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace)) AND (flags & (:flags) <> 0))");
        query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    }

    qFatalIf(!success, "Failed to hasSightings 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":flags", flags);
    success = query.exec() && query.next();
    qFatalIf(!success, "Failed to hasSightings 2");

    return query.value(0).toInt() == 1;
}

void Database::saveSighting(const QString& projectUid, const QString& stateSpace, const QString& uid, uint flags, const QVariantMap& data, const QStringList& attachments)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT EXISTS(SELECT 1 FROM sightings WHERE (projectUid = (:projectUid)) AND (uid = (:uid)))");
    qFatalIf(!success, "Failed to save sighting 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec() && query.next();
    qFatalIf(!success, "Failed to save sighting 2");
    auto exists = query.value(0).toInt() == 1;

    if (exists)
    {
        success = query.prepare("UPDATE sightings SET data = :data, attachments = :attachments WHERE (uid = (:uid)) AND (projectUid = (:projectUid))");
    }
    else
    {
        success = query.prepare("INSERT INTO sightings (projectUid, stateSpace, uid, flags, data, attachments) VALUES (:projectUid, :stateSpace, :uid, :flags, :data, :attachments)");
        query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
        query.bindValue(":flags", flags);
    }

    qFatalIf(!success, "Failed to save sighting 3");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    query.bindValue(":data", Utils::variantMapToBlob(data));
    query.bindValue(":attachments", attachments.isEmpty() ? QVariant() : attachments.join(';'));

    success = query.exec();
    qFatalIf(!success, "Failed to save sighting 4");
}

void Database::loadSighting(const QString& projectUid, const QString& uid, QVariantMap* dataOut, uint* flagsOut, QString* stateSpaceOut, QStringList* attachmentsOut) const
{
    verifyThread();
    bool success;

    QSqlQuery query = QSqlQuery(m_db);
    success = query.prepare("SELECT data, flags, stateSpace, attachments FROM sightings WHERE (uid = (:uid)) AND (projectUid = (:projectUid))");
    qFatalIf(!success, "Failed to load sighting 1");
    query.bindValue(":uid", uid);
    query.bindValue(":projectUid", projectUid);
    success = query.exec();
    qFatalIf(!success, "Failed to load sighting 2");
    success = query.next();
    qFatalIf(!success, "Failed to load sighting 3");

    if (dataOut)
    {
        dataOut->clear();
        auto dataBlob = query.value(0).toByteArray();
        qFatalIf(dataBlob.isEmpty(), "Failed to load sighting 4");
        *dataOut = Utils::variantMapFromBlob(dataBlob);
    }

    if (flagsOut)
    {
        *flagsOut = query.value(1).toUInt();
    }

    if (stateSpaceOut)
    {
        *stateSpaceOut = query.value(2).toString();
    }

    if (attachmentsOut)
    {
        *attachmentsOut = query.value(3).toString().split(';');
    }
}

bool Database::testSighting(const QString& projectUid, const QString& uid) const
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT uid FROM sightings WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    qFatalIf(!success, "Failed to test sighting 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec();
    qFatalIf(!success, "Failed to load sighting 2");
    success = query.next();

    return success;
}

void Database::deleteSighting(const QString& projectUid, const QString& uid)
{
    verifyThread();
    // Delete existing sighting if it exists.
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("DELETE FROM sightings WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    qFatalIf(!success, "Failed to delete sighting 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec();
    qFatalIf(!success, "Failed to delete sighting 2");
}

void Database::deleteSightings(const QString& projectUid, const QString& stateSpace, uint flags)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("DELETE FROM sightings WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace)) AND (flags & (:flags) = (:flags))");
    qFatalIf(!success, "Failed to delete sightings 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    query.bindValue(":flags", flags);
    success = query.exec();
    qFatalIf(!success, "Failed to delete sightings 2");
}

void Database::enumSightings(const QString& projectUid, const QString& stateSpace, uint flagsAny, std::function<void(const QString& uid, uint flags, const QVariantMap& data, const QStringList& attachments)> callback) const
{
    verifyThread();

    auto query = QSqlQuery(m_db);
    auto success = false;
    auto flagsSql = QString("(flags & (:flagsAny) <> 0)");

    if (stateSpace == "*")
    {
        success = query.prepare("SELECT uid, flags, data, attachments FROM sightings WHERE (projectUid = (:projectUid)) AND " + flagsSql + " ORDER BY rowid");
    }
    else
    {
        success = query.prepare("SELECT uid, flags, data, attachments FROM sightings WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace)) AND " + flagsSql + " ORDER BY rowid");
        query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    }

    qFatalIf(!success, "Failed to get sightings 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":flagsAny", flagsAny);
    success = query.exec();
    qFatalIf(!success, "Failed to get sightings 2");

    while (query.next())
    {
        auto uid = query.value(0).toString();
        auto flags = query.value(1).toUInt();

        auto dataBlob = query.value(2).toByteArray();
        qFatalIf(dataBlob.isEmpty(), "Failed to read sighting");
        auto data = Utils::variantMapFromBlob(dataBlob);

        auto attachments = query.value(3).toString().split(';');

        callback(uid, flags, data, attachments);
    }
}

void Database::enumSightings(const QString& projectUid, const QString& stateSpace, uint flagsOn, uint flagsOff, std::function<void(const QString& uid, uint flags, const QVariantMap& data, const QStringList& attachments)> callback) const
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = false;
    auto flagsSql = QString("(flags & (:flagsOn) = (:flagsOn)) AND (flags & (:flagsOff) = 0)");

    if (stateSpace == "*")
    {
        success = query.prepare("SELECT uid, flags, data, attachments FROM sightings WHERE (projectUid = (:projectUid)) AND " + flagsSql + " ORDER BY rowid");
    }
    else
    {
        success = query.prepare("SELECT uid, flags, data, attachments FROM sightings WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace)) AND " + flagsSql + " ORDER BY rowid");
        query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    }

    qFatalIf(!success, "Failed to get sightings 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":flagsOn", flagsOn);
    query.bindValue(":flagsOff", flagsOff);
    success = query.exec();
    qFatalIf(!success, "Failed to get sightings 2");

    while (query.next())
    {
        auto uid = query.value(0).toString();
        auto flags = query.value(1).toUInt();

        auto dataBlob = query.value(2).toByteArray();
        qFatalIf(dataBlob.isEmpty(), "Failed to read sighting");
        auto data = Utils::variantMapFromBlob(dataBlob);

        auto attachments = query.value(3).toString().split(';');

        callback(uid, flags, data, attachments);
    }
}

void Database::getSightings(const QString& projectUid, const QString& stateSpace, uint flagsAny, QStringList* uidsOut) const
{
    uidsOut->clear();
    enumSightings(projectUid, stateSpace, flagsAny, [&](const QString& uid, uint /*flags*/, const QVariantMap& /*data*/, const QStringList& /*attachments*/)
    {
        uidsOut->append(uid);
    });
}

void Database::getSightings(const QString& projectUid, const QString& stateSpace, uint flagsOn, uint flagsOff, QStringList* uidsOut) const
{
    uidsOut->clear();
    enumSightings(projectUid, stateSpace, flagsOn, flagsOff, [&](const QString& uid, uint /*flags*/, const QVariantMap& /*data*/, const QStringList& /*attachments*/)
    {
        uidsOut->append(uid);
    });
}

uint Database::getSightingFlags(const QString& projectUid, const QString& uid) const
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT flags FROM sightings WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    qFatalIf(!success, "Failed to get sighting flags 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec();
    qFatalIf(!success, "Failed to get sighting flags 2");
    success = query.next();
    qFatalIf(!success, "Failed to get sighting flags 3");

    return query.value(0).toUInt();
}

void Database::setSightingFlags(const QString& projectUid, const QString& uid, uint flags, bool on)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = false;

    if (on)
    {
        success = query.prepare("UPDATE sightings SET flags = flags | :flags WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
        qFatalIf(!success, "Failed to set sighting flags 1");
    }
    else
    {
        flags = ~flags;
        success = query.prepare("UPDATE sightings SET flags = flags & :flags WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
        qFatalIf(!success, "Failed to set sighting flags 2");
    }

    query.bindValue(":flags", flags);
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);

    success = query.exec();
    qFatalIf(!success, "Failed to set sighting flags 3");
}

void Database::setSightingFlags(const QString& projectUid, const QString& stateSpace, uint matchFlagsOn, uint matchFlagsOff, uint flags)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = false;
    auto flagsSql = QString("(flags & (:flagsOn) = (:flagsOn)) AND (flags & (:flagsOff) = 0)");

    success = query.prepare("UPDATE sightings SET flags = flags | :flags WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace)) AND " + flagsSql);
    qFatalIf(!success, "Failed to set sighting flags 1");

    query.bindValue(":projectUid", projectUid);
    query.bindValue(":stateSpace", stateSpace);
    query.bindValue(":flagsOn", matchFlagsOn);
    query.bindValue(":flagsOff", matchFlagsOff);
    query.bindValue(":flags", flags);

    success = query.exec();
    qFatalIf(!success, "Failed to set sighting flags 2");
}

void Database::setSightingFlagsAll(const QString& projectUid, uint flags, bool on)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = false;

    if (on)
    {
        success = query.prepare("UPDATE sightings SET flags = flags | :flags WHERE (projectUid = (:projectUid))");
        qFatalIf(!success, "Failed to set all sighting flags 1");
    }
    else
    {
        flags = ~flags;
        success = query.prepare("UPDATE sightings SET flags = flags & :flags WHERE (projectUid = (:projectUid))");
        qFatalIf(!success, "Failed to set all sighting flags 2");
    }

    query.bindValue(":flags", flags);
    query.bindValue(":projectUid", projectUid);

    success = query.exec();
    qFatalIf(!success, "Failed to set all sighting flags 3");
}

void Database::saveFormState(const QString& projectUid, const QString& stateSpace, const QVariantMap& data, const QStringList& attachments)
{
    verifyThread();

    deleteFormState(projectUid, stateSpace);

    auto query = QSqlQuery(m_db);
    auto success = query.prepare("INSERT INTO formState (projectUid, stateSpace, data, attachments) VALUES (:projectUid, :stateSpace, :data, :attachments)");
    qFatalIf(!success, "Failed to save formState 2");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":stateSpace", stateSpace.isNull() ? "" : stateSpace);
    query.bindValue(":data", Utils::variantMapToBlob(data));
    query.bindValue(":attachments", attachments.isEmpty() ? QVariant() : attachments.join(';'));
    success = query.exec();
    qFatalIf(!success, "Failed to save global 3");
}

void Database::loadFormState(const QString& projectUid, const QString& stateSpace, QVariantMap* dataOut, QStringList* attachmentsOut) const
{
    verifyThread();

    if (dataOut)
    {
        dataOut->clear();
    }

    if (attachmentsOut)
    {
        attachmentsOut->clear();
    }

    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT data, attachments FROM formState WHERE (projectUid = (:projectUid)) AND (stateSpace = (:stateSpace))");
    qFatalIf(!success, "Failed to load formState 1");
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
            *attachmentsOut = query.value(1).toString().split(';');
        }
    }
}

void Database::deleteFormState(const QString& projectUid, const QString& stateSpace)
{
    verifyThread();

    auto query = QSqlQuery(m_db);
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

    qFatalIf(!success, "Failed to delete formState 1");
    query.bindValue(":projectUid", projectUid);
    query.exec();   // Will fail if the formState is not present.
}

void Database::addExport(const QString& exportUid, const QVariantMap& data)
{
    verifyThread();
    removeExport(exportUid);
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("INSERT INTO exports (exportUid, data) VALUES (:exportUid, :data)");
    qFatalIf(!success, "Failed to add export 1");
    query.bindValue(":exportUid", exportUid);
    query.bindValue(":data", Utils::variantMapToBlob(data));
    success = query.exec();
    qFatalIf(!success, "Failed to add export 2");
}

void Database::getExports(QStringList* exportUidsOut) const
{
    verifyThread();
    exportUidsOut->clear();

    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT exportUid FROM exports");
    qFatalIf(!success, "Failed to get exports 1");
    success = query.exec();
    qFatalIf(!success, "Failed to get exports 2");

    while (query.next())
    {
        exportUidsOut->append(query.value(0).toString());
    }
}

void Database::loadExport(const QString& exportUid, QVariantMap* dataOut) const
{
    verifyThread();
    dataOut->clear();

    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT data FROM exports WHERE (exportUid = (:exportUid))");
    qFatalIf(!success, "Failed to load export 1");
    query.bindValue(":exportUid", exportUid);
    success = query.exec();
    qFatalIf(!success, "Failed to load export 2");
    success = query.next();
    qFatalIf(!success, "Failed to load export 3");

    auto dataBlob = query.value(0).toByteArray();
    if (!dataBlob.isEmpty())
    {
        *dataOut = Utils::variantMapFromBlob(dataBlob);
    }
}

void Database::removeExport(const QString& exportUid)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("DELETE FROM exports WHERE (exportUid = (:exportUid))");
    qFatalIf(!success, "Failed to remove export 1");
    query.bindValue(":exportUid", exportUid);
    query.exec();
}

void Database::addTask(const QString& projectUid, const QString& uid, const QString& parentUid, const QVariantMap& input, int state)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("INSERT INTO tasks (projectUid, uid, parentUid, input, output, state) VALUES (:projectUid, :uid, :parentUid, :input, :output, :state)");
    qFatalIf(!success, "Failed to save task 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    query.bindValue(":parentUid", parentUid);
    query.bindValue(":input", Utils::variantMapToBlob(input));
    query.bindValue(":output", QVariant());
    query.bindValue(":state", state);
    success = query.exec();
    qFatalIf(!success, "Failed to save task 2");
}

void Database::getTasks(const QString& projectUid, int stateMask, QStringList* outUids, QStringList* outParentUids) const
{
    verifyThread();
    outUids->clear();

    auto query = QSqlQuery(m_db);
    auto success = false;

    auto statement = QString();
    if (stateMask != -1)
    {
        statement = "SELECT uid, parentUid FROM tasks WHERE (projectUid = (:projectUid)) AND (state & (:stateMask) <> 0) ORDER BY rowid";
    }
    else
    {
        statement = "SELECT uid, parentUid FROM tasks WHERE (projectUid = (:projectUid)) ORDER BY rowid";
    }

    success = query.prepare(statement);
    qFatalIf(!success, "Failed to get tasks 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":stateMask", stateMask);
    success = query.exec();
    qFatalIf(!success, "Failed to get tasks 2");

    while (query.next())
    {
        outUids->append(query.value(0).toString());
        outParentUids->append(query.value(1).toString());
    }
}

void Database::getLastTask(const QString& projectUid, QString* outUid) const
{
    verifyThread();
    outUid->clear();

    auto query = QSqlQuery(m_db);
    auto success = false;

    success = query.prepare("SELECT uid FROM tasks WHERE (projectUid = (:projectUid)) ORDER BY rowid DESC LIMIT 1");
    qFatalIf(!success, "Failed to get last task 1");
    query.bindValue(":projectUid", projectUid);
    success = query.exec();
    qFatalIf(!success, "Failed to get last task 2");

    if (query.next())
    {
        *outUid = query.value(0).toString();
    }
}

void Database::getTaskProperty(const QString& projectUid, const QString& uid, const QString& propertyName, QVariant* outValue) const
{
    verifyThread();
    bool success;

    QSqlQuery query = QSqlQuery(m_db);
    success = query.prepare("SELECT " + propertyName + " FROM tasks WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    qFatalIf(!success, "Failed to get tasks property 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec() && query.next();
    qFatalIf(!success, "Failed to get task property 2");

    *outValue = query.value(0);
}

void Database::getTaskProperty(const QString& projectUid, const QString& uid, const QString& propertyName, QVariantMap* outValue) const
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT " + propertyName + " FROM tasks WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    qFatalIf(!success, "Failed to get tasks property map 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    success = query.exec() && query.next();
    qFatalIf(!success, "Failed to get task property map 2");

    auto valueBlob = query.value(0).toByteArray();
    *outValue = Utils::variantMapFromBlob(valueBlob);
}

void Database::setTaskProperty(const QString& projectUid, const QString& uid, const QString& propertyName, const QVariant& value)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("UPDATE tasks SET " + propertyName + " = :value WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    qFatalIf(!success, "Failed to set task property 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    query.bindValue(":value", value);
    success = query.exec();
    qFatalIf(!success, "Failed to set task property 2");
}

void Database::setTaskProperty(const QString& projectUid, const QString& uid, const QString& propertyName, const QVariantMap& value)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("UPDATE tasks SET " + propertyName + " = :value WHERE (projectUid = (:projectUid)) AND (uid = (:uid))");
    qFatalIf(!success, "Failed to set blob task property map 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":uid", uid);
    query.bindValue(":value", Utils::variantMapToBlob(value));
    success = query.exec();
    qFatalIf(!success, "Failed to set blob task property map 2");
}

void Database::deleteTasks(const QString& projectUid, uint state)
{
    verifyThread();
    auto query = QSqlQuery(m_db);
    auto success = query.prepare("DELETE FROM tasks WHERE (projectUid = (:projectUid)) AND (state & (:state) = (:state))");
    qFatalIf(!success, "Failed to delete tasks 1");
    query.bindValue(":projectUid", projectUid);
    query.bindValue(":state", state);
    success = query.exec();
    qFatalIf(!success, "Failed to delete tasks 2");
}

QStringList Database::getAttachments()
{
    verifyThread();

    auto attachments = QStringList();

    auto query = QSqlQuery(m_db);
    auto success = query.prepare("SELECT attachments FROM sightings WHERE attachments is not null");
    qFatalIf(!success, "Failed to get attachments 1");
    success = query.exec();
    qFatalIf(!success, "Failed to get attachments 2");
    while (query.next())
    {
        attachments.append(query.value(0).toString().split(';'));
    }

    success = query.prepare("SELECT attachments FROM formState WHERE attachments is not null");
    qFatalIf(!success, "Failed to get attachments 3");
    success = query.exec();
    qFatalIf(!success, "Failed to get attachments 4");
    while (query.next())
    {
        attachments.append(query.value(0).toString().split(';'));
    }

    return attachments;
}
