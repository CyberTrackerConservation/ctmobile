#pragma once
#include "pch.h"

class Database: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, filePath)

public:
    explicit Database(QObject* parent = nullptr);
    explicit Database(const QString& filePath, QObject* parent = nullptr);
    virtual ~Database();

    void compact();

    void addProject(const QString& projectUid);
    void getProjects(QStringList* projectUidsOut) const;
    void saveProject(const QString& projectUid, const QVariantMap& data);
    void loadProject(const QString& projectUid, QVariantMap* dataOut) const;
    void removeProject(const QString& projectUid);
    void clearProjects();

    bool hasSightings(const QString& projectUid, const QString& stateSpace, uint flags) const;
    void saveSighting(const QString& projectUid, const QString& stateSpace, const QString& uid, uint flags, const QVariantMap& data, const QStringList& attachments);
    void loadSighting(const QString& projectUid, const QString& uid, QVariantMap* dataOut, uint* flagsOut = nullptr, QString* stateSpaceOut = nullptr, QStringList* attachmentsOut = nullptr) const;
    bool testSighting(const QString& projectUid, const QString& uid) const;
    void deleteSighting(const QString& projectUid, const QString& uid);
    void deleteSightings(const QString& projectUid, const QString& stateSpace, uint flags);
    void getSightings(const QString& projectUid, const QString& stateSpace, uint flags, QStringList* uidsOut) const;
    uint getSightingFlags(const QString& projectUid, const QString& uid) const;
    void setSightingFlags(const QString& projectUid, const QString& uid, uint flags, bool on = true);
    void setSightingFlagsAll(const QString& projectUid, uint flags, bool on = true);

    void saveFormState(const QString& projectUid, const QString& stateSpace, const QVariantMap& data, const QStringList& attachments);
    void loadFormState(const QString& projectUid, const QString& stateSpace, QVariantMap* dataOut, QStringList* attachmentsOut = nullptr) const;
    void deleteFormState(const QString& projectUid, const QString& stateSpace);

    void addExport(const QString& exportUid, const QVariantMap& data);
    void getExports(QStringList* exportUidsOut) const;
    void loadExport(const QString& exportUid, QVariantMap* dataOut) const;
    void removeExport(const QString& exportUid);

    void addTask(const QString& projectUid, const QString& uid, const QString& parentUid, const QVariantMap& input, int state);
    void getTasks(const QString& projectUid, int stateMask, QStringList* uidsOut, QStringList* parentUidsOut) const;
    void getTaskProperty(const QString& projectUid, const QString& uid, const QString& propertyName, QVariant* valueOut) const;
    void getTaskProperty(const QString& projectUid, const QString& uid, const QString& propertyName, QVariantMap* valueOut) const;
    void setTaskProperty(const QString& projectUid, const QString& uid, const QString& propertyName, const QVariant& value);
    void setTaskProperty(const QString& projectUid, const QString& uid, const QString& propertyName, const QVariantMap& value);
    void deleteTasks(const QString& projectUid, uint state);

    QStringList getAttachments();

private:
    QSqlDatabase m_db;
    QString m_connectionName;
    void verifyThread() const;
};
