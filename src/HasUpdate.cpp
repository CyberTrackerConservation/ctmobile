#include "HasUpdate.h"
#include "App.h"

namespace CyberTracker {

namespace {
int INITIAL_TIMEOUT = 10000;
int DEFAULT_TIMEOUT = OS_DESKTOP ? 10000 : 40000;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HasUpdateWorker

HasUpdateWorker::HasUpdateWorker(QObject* parent): QObject(parent)
{
    m_timer.reset(new QTimer(this));
    m_timer->setSingleShot(true);

    connect(m_timer.get(), &QTimer::timeout, this, [&]()
    {
        processProjects();
        m_timer->start(DEFAULT_TIMEOUT);
    });
}

void HasUpdateWorker::start()
{
    m_networkAccessManager.reset(new QNetworkAccessManager(this));
    m_timer->start(INITIAL_TIMEOUT);
}

void HasUpdateWorker::stop()
{
    m_timer->stop();
}

void HasUpdateWorker::processProjects()
{
    auto projects = App::instance()->projectManager()->buildList();
    for (auto projectIt = projects.constBegin(); projectIt != projects.constEnd(); projectIt++)
    {
        if (QThread::currentThread()->isInterruptionRequested())
        {
            return;
        }

        if (!App::instance()->updateAllowed())
        {
            return;
        }

        auto project = App::instance()->projectManager()->loadPtr(*projectIt);
        if (!project)
        {
            continue;
        }

        if (project->version() == 0)
        {
            continue;
        }

        // Skip projects already known to need an update.
        if (project->hasUpdate())
        {
            continue;
        }

        // Create a connector.
        auto connector = App::instance()->createConnectorPtr(project->connector());
        if (!connector)
        {
            continue;
        }

        connector->setNetworkAccessManager(m_networkAccessManager.get());

        processProject(project.get(), connector.get());
    }
}

bool HasUpdateWorker::processProject(Project* project, Connector* connector)
{
    // hasUpdate.
    if (connector->canUpdate(project))
    {
        auto result = connector->hasUpdate(project);
        if (result == Connector::Unauthorized() && !project->refreshToken().isEmpty())
        {
            App::instance()->vault()->expireToken(project->accessToken());

            QString oldRefreshToken, newAccessToken, newRefreshToken;
            result = connector->refreshAccessToken(project, &oldRefreshToken, &newAccessToken, &newRefreshToken);
            if (result.success)
            {
                App::instance()->vault()->updateToken(oldRefreshToken, newAccessToken, newRefreshToken);
                result = connector->hasUpdate(project);
            }
            else if (result == Connector::Unauthorized())
            {
                App::instance()->vault()->expireToken(oldRefreshToken);
            }
        }

        if (result == Connector::UpdateAvailable())
        {
            emit projectHasUpdate(project->uid());
        }
    }


    // hasAnnouncements.
    {
        auto announcements = QVariantList();
        auto result = connector->hasAnnouncements(project, &announcements);
        if (result.success)
        {
            emit projectHasAnnouncements(project->uid(), announcements);
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HasUpdate

HasUpdate::HasUpdate(QObject *parent): QObject(parent)
{
}

HasUpdate::~HasUpdate()
{
    stop();
}

HasUpdate::Lock::Lock()
{
    App::instance()->hasUpdate()->pause();
}

HasUpdate::Lock::~Lock()
{
    App::instance()->hasUpdate()->resume();
}

void HasUpdate::start()
{
    stop();

    m_thread.reset(new QThread(this));

    auto worker = new HasUpdateWorker();
    worker->moveToThread(m_thread.get());

    connect(worker, &HasUpdateWorker::projectHasUpdate, this, &HasUpdate::projectHasUpdate);
    connect(worker, &HasUpdateWorker::projectHasAnnouncements, this, &HasUpdate::projectHasAnnouncements);

    connect(m_thread.get(), &QThread::started, worker, &HasUpdateWorker::start);
    connect(m_thread.get(), &QThread::finished, worker, &HasUpdateWorker::stop);
    connect(m_thread.get(), &QThread::finished, worker, &QObject::deleteLater);

    m_thread->start();
}

void HasUpdate::stop()
{
    if (m_thread)
    {
        m_thread->requestInterruption();
        m_thread->quit();
        m_thread->wait();
        m_thread.reset();
    }
}

void HasUpdate::pause()
{
    m_block++;
    if (m_block == 1)
    {
        m_wasRunning = m_thread.get() != nullptr;
        stop();
    }
}

void HasUpdate::resume()
{
    m_block--;
    qFatalIf(m_block < 0, "Bad scan block counter");
    if (m_block == 0 && m_wasRunning)
    {
        start();
    }
}

void HasUpdate::projectHasUpdate(const QString& projectUid)
{
    auto project = App::instance()->projectManager()->loadPtr(projectUid);
    if (project)
    {
        project->set_hasUpdate(true);
    }
}

void HasUpdate::projectHasAnnouncements(const QString& projectUid, const QVariantList& announcements)
{
    auto project = App::instance()->projectManager()->loadPtr(projectUid);
    if (project)
    {
        project->set_announcements(announcements);
    }
}

} // namespace CyberTracker