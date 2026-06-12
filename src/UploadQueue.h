#pragma once
#include "pch.h"
#include "Connector.h"

namespace CyberTracker {

constexpr char QUEUE_SUCCESS_TEXT[] = "Success";

class UploadQueueWorker : public QObject
{
    Q_OBJECT

public:
    explicit UploadQueueWorker(QObject* parent = nullptr);

    void initNetwork();
    void processItems(const QString& filterProjectUid = QString());

public slots:
    void start();
    void stop();
    void kick();

signals:
    void progress(int index, int total);
    void itemStarted(const QString& itemUid);
    void itemCompleted(const QString& itemUid);
    void processStarted();
    void processCompleted(const QMap<QString, CyberTracker::ApiResult>& results);

private:
    bool m_processing = false;
    bool m_kicked = false;
    std::unique_ptr<QTimer> m_timer;
    std::unique_ptr<QNetworkAccessManager> m_networkAccessManager;
    std::unique_ptr<Database> m_database;
    ApiResult uploadItem(const Database::DbQueueItem& item);
    ApiResult uploadPayload(const Database::DbQueueItem& item);
    ApiResult uploadWebhook(const Database::DbQueueItem& item);
};

class UploadQueue : public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY(bool, processing)
    QML_READONLY_AUTO_PROPERTY(QString, workingUid)

public:
    explicit UploadQueue(QObject *parent = nullptr);
    ~UploadQueue() override;

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

    void addItem(const Database::DbQueueItem& item);
    void enumItems(std::function<void(const Database::DbQueueItem& item)> callback);
    bool hasItems(const QString& projectUid);
    int itemCount(const QString& projectUid);

    Q_INVOKABLE QString createSnapshot();
    Q_INVOKABLE void resetQueue();
    Q_INVOKABLE void clearQueue();
    Q_INVOKABLE bool processSync(const QString& projectUid = QString());
    Q_INVOKABLE bool waitForQueue(const QString& projectUid = QString());
    Q_INVOKABLE void kick();

signals:
    void changed();
    void itemChanged(const QString& itemUid);

private:
    int m_block = 0;
    bool m_wasRunning = false;
    std::unique_ptr<QThread> m_thread;
    UploadQueueWorker* m_worker = nullptr;
};

} // namespace CyberTracker
