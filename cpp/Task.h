#pragma once
#include "pch.h"
#include "Database.h"

class Task: public QObject, public QRunnable
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (QString, projectUid)
    QML_READONLY_AUTO_PROPERTY (QString, uid)
    QML_READONLY_AUTO_PROPERTY (QVariantMap, parentOutput)
    QML_READONLY_AUTO_PROPERTY (QVariantMap, input)
    QML_READONLY_AUTO_PROPERTY (QVariantMap, output)
    QML_READONLY_AUTO_PROPERTY (bool, runAlone)

public:
    explicit Task(QObject* parent = nullptr);
    virtual ~Task();

    void init(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput, bool runAlone = false);

    virtual bool doWork() = 0;

    enum State
    {
        Active   = 1,
        Paused   = 2,
        Complete = 4,
    };

    static QString taskNameFromUid(const QString& uid);
    static QString makeUid(const QString& taskName, const QString& taskUid);

private:
    virtual void run() override;

signals:
    void completed(const QString& projectUid, const QString& uid, const QVariantMap& output, bool success);
};

class TaskManager;

typedef Task* (*TaskFactory)(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput);
typedef void (*TaskCompleted)(TaskManager* taskManager, const QString& projectUid, const QString& uid, const QVariantMap& output, bool success);

class TaskManager: public QObject
{
    Q_OBJECT

public:
    explicit TaskManager(QObject* parent = nullptr);
    ~TaskManager();

    void registerTask(const QString& name, TaskFactory taskFactory, TaskCompleted taskCompleted = nullptr);
    void addTask(const QString& projectUid, const QString& uid, const QString& parentUid, const QVariantMap& input);
    void getTasks(const QString& projectUid, const QString& uidStartsWith, int stateMask, QStringList* foundUidsOut) const;
    void pauseTask(const QString& projectUid, const QString& uid);
    void resumeTask(const QString& projectUid, const QString& uid, const QString& inputName = QString(), const QVariant& inputValue = QVariant());
    void rundownTasks(const QString& projectUid) const;

    Q_INVOKABLE void resumePausedTasks(const QString& projectUid);
    Q_INVOKABLE bool isTaskComplete(const QString& projectUid, const QString& uid) const;
    Q_INVOKABLE bool waitForTasks(const QString& projectUid);
    Q_INVOKABLE int getIncompleteTaskCount(const QString& projectUid) const;

private:
    QMap<QString, TaskFactory> m_taskFactory;
    QMap<QString, TaskCompleted> m_taskCompleted;
    QMap<QString, bool> m_runningTasks;
    Database* m_database = nullptr;

    void setRunning(const QString& projectUid, const QString& uid, bool value);
    bool getRunning(const QString& projectUid, const QString& uid) const;
    int getRunningCount(const QString& projectUid) const;

    void startTask(const QString& projectUid, const QString& uid);
    void processTasks(const QString& projectUid);

private slots:
    void onTaskCompleted(const QString& projectUid, const QString& uid, const QVariantMap& output, bool success);

signals:
    void taskCompleted(const QString& projectUid, const QString& uid, const QVariantMap& output, bool success);
    void runningCountChanged();
};

