#pragma once
#include "pch.h"
#include "Connector.h"

namespace CyberTracker {

class HasUpdateWorker : public QObject
{
    Q_OBJECT

public:
    explicit HasUpdateWorker(QObject* parent = nullptr);

    void processProjects();

public slots:
    void start();
    void stop();

signals:
    void projectHasUpdate(const QString& projectUid);
    void projectHasAnnouncements(const QString& projectUid, const QVariantList& announcements);

private:
    std::unique_ptr<QTimer> m_timer;
    std::unique_ptr<QNetworkAccessManager> m_networkAccessManager;
    bool processProject(Project* project, Connector* connector);
};

class HasUpdate : public QObject
{
    Q_OBJECT

public:
    explicit HasUpdate(QObject *parent = nullptr);
    ~HasUpdate() override;

    class Lock
    {
    public:
        Lock();
        ~Lock();
    };

    void start();
    void stop();
    void pause();
    void resume();

private slots:
    void projectHasUpdate(const QString& projectUid);
    void projectHasAnnouncements(const QString& projectUid, const QVariantList& announcements);

private:
    int m_block = 0;
    bool m_wasRunning = false;
    std::unique_ptr<QThread> m_thread;
};

} // namespace CyberTracker