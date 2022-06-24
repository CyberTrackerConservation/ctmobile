#include "Task.h"
#include "App.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Task

Task::Task(QObject* parent): QObject(parent)
{
}

Task::~Task()
{
}

void Task::init(const QString& projectUid, const QString& uid, const QVariantMap& input, const QVariantMap& parentOutput, bool runAlone)
{
    m_projectUid = projectUid;
    m_uid = uid;
    m_input = input;
    m_parentOutput = parentOutput;
    m_runAlone = runAlone;
}

void Task::run()
{
    auto workResult = doWork();
    emit completed(m_projectUid, m_uid, m_output, workResult);
}

QString Task::taskNameFromUid(const QString& uid)
{
    return uid.split('/')[0];
}

QString Task::makeUid(const QString& taskName, const QString& taskUid)
{
    return taskName + '/' + taskUid;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TaskManager

TaskManager::TaskManager(QObject* parent): QObject(parent)
{
    m_database = App::instance()->database();
}

TaskManager::~TaskManager()
{
}

bool TaskManager::waitForTasks(const QString& projectUid)
{
    auto totalTaskCount = getIncompleteTaskCount(projectUid);
    if (totalTaskCount == 0)
    {
        return true;
    }

    QEventLoop eventLoop;
    App::instance()->initProgress(projectUid, "EngineWaitForTasks", { totalTaskCount });

    auto listener = connect(this, &TaskManager::taskCompleted, this, [&](const QString& taskProjectUid, const QString& /*uid*/, QVariantMap /*output*/, bool success)
    {
        // Ignore tasks from other projects.
        if (taskProjectUid != projectUid)
        {
            return;
        }

        // Any failed task means the wait failed.
        if (!success)
        {
            eventLoop.exit();
        }

        // Count the remaining tasks.
        auto tasks = QStringList();
        getTasks(projectUid, "", Task::State::Active | Task::State::Paused, &tasks);
        App::instance()->sendProgress(totalTaskCount - tasks.count());

        // No more running tasks means we are done.
        if (m_runningTasks.count() == 0)
        {
            eventLoop.exit();
        }
    });

    resumePausedTasks(projectUid);
    eventLoop.exec();
    disconnect(listener);

    return getIncompleteTaskCount(projectUid) == 0;
}

void TaskManager::rundownTasks(const QString& projectUid) const
{
    if (getRunningCount(projectUid) == 0)
    {
        return;
    }

    QEventLoop eventLoop;
    auto listener = connect(this, &TaskManager::runningCountChanged, this, [&]()
    {
        if (getRunningCount(projectUid) == 0)
        {
            eventLoop.exit();
        }
    });

    eventLoop.exec();
    disconnect(listener);
}

void TaskManager::setRunning(const QString& projectUid, const QString& uid, bool value)
{
    auto taskId = projectUid + '/' + uid;
    if (value)
    {
        m_runningTasks[taskId] = value;
    }
    else
    {
        m_runningTasks.remove(taskId);
    }

    emit runningCountChanged();
}

bool TaskManager::getRunning(const QString& projectUid, const QString& uid) const
{
    return m_runningTasks.value(projectUid + '/' + uid, false);
}

int TaskManager::getRunningCount(const QString& projectUid) const
{
    auto result = 0;

    for (auto it = m_runningTasks.constKeyValueBegin(); it != m_runningTasks.constKeyValueEnd(); it++)
    {
        if (it->first.startsWith(projectUid))
        {
            result++;
        }
    }

    return result;
}

void TaskManager::registerTask(const QString& name, TaskFactory taskFactory, TaskCompleted taskCompleted)
{
    if (!m_taskFactory.contains(name))
    {
        m_taskFactory[name] = taskFactory;
    }

    if (!m_taskCompleted.contains(name))
    {
        m_taskCompleted[name] = taskCompleted;
    }
}

void TaskManager::addTask(const QString& projectUid, const QString& uid, const QString& parentUid, const QVariantMap& input)
{
    m_database->addTask(projectUid, uid, parentUid, input, Task::State::Paused);
    resumePausedTasks(projectUid);
}

void TaskManager::getTasks(const QString& projectUid, const QString& uidStartsWith, int stateMask, QStringList* outFoundUids) const
{
    QStringList uids, parentUids;
    m_database->getTasks(projectUid, stateMask, &uids, &parentUids);

    outFoundUids->clear();
    for (auto it = uids.constBegin(); it != uids.constEnd(); it++)
    {
        if (it->startsWith(uidStartsWith))
        {
            outFoundUids->append(*it);
        }
    }
}

void TaskManager::pauseTask(const QString& projectUid, const QString& uid)
{
    QVariant state;
    m_database->getTaskProperty(projectUid, uid, "state", &state);
    qFatalIf(state == Task::State::Complete, "Task is already complete");

    m_database->setTaskProperty(projectUid, uid, "state", Task::State::Paused);
}

void TaskManager::resumeTask(const QString& projectUid, const QString& uid, const QString& inputName, const QVariant& inputValue)
{
    QVariant state;
    m_database->getTaskProperty(projectUid, uid, "state", &state);
    qFatalIf(state.toInt() != Task::State::Paused, "Task is not paused");

    if (!inputName.isEmpty())
    {
        QVariantMap input;
        m_database->getTaskProperty(projectUid, uid, "input", &input);
        input[inputName] = inputValue;
        m_database->setTaskProperty(projectUid, uid, "input", input);
    }

    m_database->setTaskProperty(projectUid, uid, "state", Task::State::Active);

    processTasks(projectUid);
}

void TaskManager::resumePausedTasks(const QString& projectUid)
{
    QStringList uids, parentUids;
    m_database->getTasks(projectUid, Task::State::Paused, &uids, &parentUids);

    for (auto it = uids.constBegin(); it != uids.constEnd(); it++)
    {
        m_database->setTaskProperty(projectUid, *it, "state", Task::State::Active);
    }

    processTasks(projectUid);
}

bool TaskManager::isTaskComplete(const QString& projectUid, const QString& uid) const
{
    QVariant state;
    m_database->getTaskProperty(projectUid, uid, "state", &state);

    return state == Task::State::Complete;
}

int TaskManager::getIncompleteTaskCount(const QString& projectUid) const
{
    auto pausedTasks = QStringList();
    getTasks(projectUid, "", Task::State::Active | Task::State::Paused, &pausedTasks);
    return pausedTasks.count();
}

void TaskManager::startTask(const QString& projectUid, const QString& uid)
{
    QVariantMap input;
    m_database->getTaskProperty(projectUid, uid, "input", &input);
    auto taskName = Task::taskNameFromUid(uid);

    QVariant parentUid;
    m_database->getTaskProperty(projectUid, uid, "parentUid", &parentUid);

    QVariantMap parentOutput;
    if (!parentUid.toString().isEmpty())
    {
        m_database->getTaskProperty(projectUid, parentUid.toString(), "output", &parentOutput);
    }

    auto task = m_taskFactory[taskName](projectUid, uid, input, parentOutput);
    task->setAutoDelete(true);

    // Skip tasks that want to run alone if others are running.
    if (task->runAlone() && !m_runningTasks.isEmpty())
    {
        return;
    }

    // Start the task.
    connect(task, &Task::completed, this, &TaskManager::onTaskCompleted);

    setRunning(projectUid, uid, true);
    QThreadPool::globalInstance()->start(task);
}

void TaskManager::processTasks(const QString& projectUid)
{
    // Get tasks that aren't done
    QStringList uids, parentUids;
    m_database->getTasks(projectUid, Task::State::Active, &uids, &parentUids);

    for (auto i = 0; i < uids.length(); i++)
    {
        // Skip if already running.
        auto uid = uids[i];
        if (getRunning(projectUid, uid))
        {
            continue;
        }

        // Start if no parent.
        auto parentUid = parentUids[i];
        if (parentUid.isEmpty())
        {
            startTask(projectUid, uid);
            continue;
        }

        // Skip if parent is not done.
        QVariant parentState;
        m_database->getTaskProperty(projectUid, parentUid, "state", &parentState);
        if (parentState != Task::State::Complete)
        {
            continue;
        }

        // Parent is done, update the input and start.
        startTask(projectUid, uid);
    }
}

void TaskManager::onTaskCompleted(const QString& projectUid, const QString& uid, const QVariantMap& output, bool success)
{
    // Update the output and state.
    if (success)
    {
        m_database->setTaskProperty(projectUid, uid, "output", output);
        m_database->setTaskProperty(projectUid, uid, "state", Task::State::Complete);
    }

    // Do task completion code.
    auto taskName = Task::taskNameFromUid(uid);
    if (m_taskCompleted.contains(taskName))
    {
        m_taskCompleted[taskName](this, projectUid, uid, output, success);
    }

    // Remove from the running list.
    setRunning(projectUid, uid, false);

    // Reprocess the task list.
    processTasks(projectUid);

    // Pass the event on.
    emit taskCompleted(projectUid, uid, output, success);
}
