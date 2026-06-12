#pragma once
#include "pch.h"
#include "Database.h"

namespace CyberTracker {

class TranscribeQueueWorker : public QObject
{
    Q_OBJECT

public:
    explicit TranscribeQueueWorker(QObject* parent = nullptr);

    void processItems(const QString& filterProjectUid = QString());

public slots:
    void start();
    void stop();
    void kick();

signals:
    void workStarted();
    void workCompleted(bool success);
    void itemStarted(const CyberTracker::Database::DbTranscribeItem& item);
    void itemCompleted(const CyberTracker::Database::DbTranscribeItem& item);

private:
    bool m_processing = false;
    bool m_kicked = false;
    std::unique_ptr<QTimer> m_timer;
    std::unique_ptr<QNetworkAccessManager> m_networkAccessManager;
    std::unique_ptr<Database> m_database;
    bool uploadItem(const Database::DbTranscribeItem& item, QVariantMap* resultOut, QString* errorOut);
    bool uploadItemV1(const Database::DbTranscribeItem& item, QVariantMap* resultOut, QString* errorOut);
    bool uploadItemTranscribe(const Database::DbTranscribeItem& item, QVariantMap* resultOut, QString* errorOut);
};

class TranscribeQueue : public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (bool, working)
    QML_READONLY_AUTO_PROPERTY (bool, lastSuccess)
    QML_READONLY_AUTO_PROPERTY (QString, workingDataUid)

public:
    explicit TranscribeQueue(QObject *parent = nullptr);
    ~TranscribeQueue() override;

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

    void request(const Database::DbTranscribeItem& item);
    void consume(const QString& filename);

    Q_INVOKABLE bool hasWork() const;
    Q_INVOKABLE void kick();

signals:
    void itemCompleted(const CyberTracker::Database::DbTranscribeItem& transcribeItem);

private:
    int m_block = 0;
    bool m_wasRunning = false;
    std::unique_ptr<QThread> m_thread;
    TranscribeQueueWorker* m_worker = nullptr;
};

} // namespace CyberTracker
