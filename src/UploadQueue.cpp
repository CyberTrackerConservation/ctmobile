#include "UploadQueue.h"
#include "App.h"

namespace CyberTracker {

namespace {
int INITIAL_TIMEOUT = 5000;
int DEFAULT_TIMEOUT = OS_DESKTOP ? 10000 : 40000;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UploadQueueWorker

UploadQueueWorker::UploadQueueWorker(QObject* parent): QObject(parent)
{
    m_timer.reset(new QTimer(this));
    m_timer->setSingleShot(true);

    connect(m_timer.get(), &QTimer::timeout, this, [&]()
    {
        m_kicked = false;
        m_processing = true;
        processItems();
        m_processing = false;

        m_timer->start(m_kicked ? 1 : DEFAULT_TIMEOUT);
    });
}

void UploadQueueWorker::initNetwork()
{
    m_networkAccessManager.reset(new QNetworkAccessManager(this));
}

void UploadQueueWorker::start()
{
    initNetwork();
    m_timer->start(INITIAL_TIMEOUT);
}

void UploadQueueWorker::stop()
{
    m_timer->stop();
}

void UploadQueueWorker::kick()
{
    if (m_processing)
    {
        // When done, immediately timeout again.
        m_kicked = true;
    }
    else
    {
        // Speed up the timeout.
        m_timer->stop();
        m_timer->start(1);
    }
}

void UploadQueueWorker::processItems(const QString& filterProjectUid)
{
    emit processStarted();

    if (!m_database)
    {
        m_database = App::instance()->createDatabasePtr();
    }

    // Get the items.
    auto items = QList<Database::DbQueueItem>();
    m_database->enumQueueItems([&](const Database::DbQueueItem& item)
    {
        if (item.flags.testFlag(Database::QueueFlag::DB_COMPLETE_FLAG))
        {
            return;
        }

        if (!filterProjectUid.isEmpty() && filterProjectUid != item.projectUid)
        {
            return;
        }

        items.append(item);
    });

    // Upload the items.
    auto results = QMap<QString, ApiResult>();
    for (auto i = 0; i < items.count(); i++)
    {
        if (QThread::currentThread()->isInterruptionRequested())
        {
            break;
        }

        emit progress(i + 1, items.count());

        auto item = items[i];

        emit itemStarted(item.uid);

        if (results.value(item.projectUid, ApiResult::Success()).success)
        {
            results[item.projectUid] = uploadItem(item);
        }
        else
        {
            qDebug() << "UploadQueue skipping failed projectUid: " << item.projectUid << results[item.projectUid].errorString;
        }

        emit itemCompleted(item.uid);
    }

    // Remove attachments so that large files can be cleaned up.
    m_database->deleteQueueAttachments(Database::QueueFlag::DB_COMPLETE_FLAG);

    // Remove completed items which are marked for auto deletion.
    m_database->deleteQueueItems(Database::QueueFlag::DB_AUTO_DELETE | Database::QueueFlag::DB_COMPLETE_FLAG);

    emit processCompleted(results);
}

ApiResult UploadQueueWorker::uploadItem(const Database::DbQueueItem& item)
{
    // Payload.
    auto payloadResult = uploadPayload(item);
    if (item.payloadError != QUEUE_SUCCESS_TEXT)
    {
        m_database->transaction([&]() -> bool
        {
            m_database->setQueueItemPayloadError(item.uid,
                payloadResult.success ? QUEUE_SUCCESS_TEXT : payloadResult.errorString,
                payloadResult.success ? App::instance()->timeManager()->currentDateTimeISO() : QString());

            if (payloadResult.success && !item.dataUid.isEmpty())
            {
                m_database->setDataFlags(item.projectUid, item.dataUid, Database::DataFlag::DB_UPLOADED_FLAG);
                emit App::instance()->sightingFlagsChanged(item.projectUid, item.dataUid);
            }

            return true;
        });
    }

    // Webhook.
    auto webhookResult = uploadWebhook(item);
    if (item.webhookError != QUEUE_SUCCESS_TEXT)
    {
        m_database->setQueueItemWebhookError(item.uid, webhookResult.success ? QUEUE_SUCCESS_TEXT : webhookResult.errorString);
    }

    // Mark as complete.
    if (payloadResult.success && webhookResult.success)
    {
        m_database->setQueueItemFlags(item.uid, Database::QueueFlag::DB_COMPLETE_FLAG);
    }

    return payloadResult;
}

ApiResult UploadQueueWorker::uploadPayload(const Database::DbQueueItem& item)
{
    // Already uploaded.
    if (item.payloadError == QUEUE_SUCCESS_TEXT)
    {
        return ApiResult::Success();
    }

    // No WiFi.
    if (!App::instance()->uploadAllowed())
    {
        return ApiResult::Error(tr("No WiFi"));
    }

    auto project = App::instance()->projectManager()->loadPtr(item.projectUid);
    if (!project)
    {
        return ApiResult::Error("Project load failure");
    }

    auto connector = App::instance()->createConnectorPtr(project->connector());
    if (!connector)
    {
        return ApiResult::Error("Connector failure");
    }

    connector->setNetworkAccessManager(m_networkAccessManager.get());

    // Get initial state.
    auto state = item.state;
    if (!item.dataUid.isEmpty())
    {
        m_database->getDataQueueState(item.projectUid, item.dataUid, &state);
    }

    // Commit state.
    auto commitState = [&]()
    {
        if (!item.dataUid.isEmpty())
        {
            m_database->setDataQueueState(item.projectUid, item.dataUid, state);
        }
        else
        {
            m_database->setQueueItemState(item.uid, state);
        }
    };

    // Upload data.
    auto result = connector->uploadData(project.get(), item, &state, commitState);

    // Refresh token.
    if (result == Connector::Unauthorized() && !project->refreshToken().isEmpty())
    {
        auto vault = App::instance()->vault();
        auto projectManager = App::instance()->projectManager();

        qDebug() << "Expire token for " << item.projectUid;
        vault->expireToken(project->accessToken());

        QString oldRefreshToken, newAccessToken, newRefreshToken;
        auto refreshAccessTokenResult = connector->refreshAccessToken(project.get(), &oldRefreshToken, &newAccessToken, &newRefreshToken);
        if (refreshAccessTokenResult.success)
        {
            qDebug() << "Success: refresh token for " << item.projectUid;
            vault->updateToken(oldRefreshToken, newAccessToken, newRefreshToken);
            result = connector->uploadData(project.get(), item, &state, commitState);
        }
        else if (refreshAccessTokenResult == Connector::Unauthorized())
        {
            qDebug() << "Failure: refresh token for " << item.projectUid;
            vault->expireToken(oldRefreshToken);
            emit projectManager->projectTriggerLogin(project->uid());
        }
        else if (result == Connector::NotSupported())
        {
            emit projectManager->projectTriggerLogin(project->uid());
        }
    }

    return result;
}

ApiResult UploadQueueWorker::uploadWebhook(const Database::DbQueueItem& item)
{
    // Already complete.
    if (item.webhookError == QUEUE_SUCCESS_TEXT)
    {
        return ApiResult::Success();
    }

    // No WiFi.
    if (!App::instance()->uploadAllowed())
    {
        return ApiResult::Error(tr("No WiFi"));
    }

    // Send if present and there is a target.
    auto result = ApiResult::Success();
    auto webhookUrl = App::instance()->settings()->webhookUrl();
    if (!webhookUrl.isEmpty() && !item.webhook.isEmpty())
    {
        auto multipart = QHttpMultiPart(QHttpMultiPart::FormDataType);

        // Add JSON payload.
        auto jsonPart = QHttpPart();
        jsonPart.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        jsonPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"json_submission_file\"; filename=\"json_submission_file\"");
        jsonPart.setBody(item.webhook);
        multipart.append(jsonPart);

        // Add attachments.
        auto mimeDatabase = QMimeDatabase();
        auto attachmentFiles = QList<QFile*>();
        auto cleanupFilesOnExit = qScopeGuard([&] { qDeleteAll(attachmentFiles); });
        for (auto attachmentIt = item.attachments.constBegin(); attachmentIt != item.attachments.constEnd(); attachmentIt++)
        {
            auto file = new QFile(App::instance()->getMediaFilePath(*attachmentIt));
            qFatalIf(!file->open(QIODevice::ReadOnly), "Failed to read attachment: " + *attachmentIt);
            attachmentFiles.append(file);

            auto part = QHttpPart();
            auto fileInfo = QFileInfo(file->fileName());
            auto partName = fileInfo.fileName();
            part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + partName + "\"; filename=\"" + partName + "\"");
            part.setHeader(QNetworkRequest::ContentTypeHeader, mimeDatabase.mimeTypeForFile(file->fileName()).name());
            part.setBodyDevice(file);
            multipart.append(part);
        }

        result = Connector::ResultFromResponse(Utils::httpPostMultiPart(m_networkAccessManager.get(), webhookUrl, &multipart));
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UploadQueue

UploadQueue::UploadQueue(QObject *parent): QObject(parent)
{
    m_processing = false;
}

UploadQueue::~UploadQueue()
{
    stop();
}

UploadQueue::Lock::Lock()
{
    App::instance()->uploadQueue()->pause();
}

UploadQueue::Lock::~Lock()
{
    App::instance()->uploadQueue()->resume();
}

void UploadQueue::start()
{
    stop();

    m_thread.reset(new QThread(this));

    m_worker = new UploadQueueWorker();
    m_worker->moveToThread(m_thread.get());

    connect(m_thread.get(), &QThread::started, m_worker, &UploadQueueWorker::start);
    connect(m_thread.get(), &QThread::finished, m_worker, &UploadQueueWorker::stop);
    connect(m_thread.get(), &QThread::finished, m_worker, [this]()
    {
        delete m_worker;
        m_worker = nullptr;
    });

    connect(m_worker, &UploadQueueWorker::processStarted, this, [&]() { update_processing(true); });
    connect(m_worker, &UploadQueueWorker::processCompleted, this, [&]() { update_processing(false); });

    connect(m_worker, &UploadQueueWorker::itemStarted, this,
        [&](const QString& itemUid) { update_workingUid(itemUid); });

    connect(m_worker, &UploadQueueWorker::itemCompleted, this,
        [&](const QString& itemUid) { update_workingUid(""); emit itemChanged(itemUid); });

    connect(App::instance()->projectManager(), &ProjectManager::projectLoggedInChanged, this,
        [&](const QString& projectUid, bool loggedIn) { if (loggedIn) { kick(); } });

    m_thread->start();
}

void UploadQueue::stop()
{
    if (m_thread)
    {
        m_thread->requestInterruption();
        m_thread->quit();
        m_thread->wait();
        m_thread.reset();
    }
}

void UploadQueue::pause()
{
    m_block++;
    if (m_block == 1)
    {
        m_wasRunning = m_thread.get() != nullptr;
        stop();
    }
}

void UploadQueue::resume()
{
    m_block--;
    qFatalIf(m_block < 0, "Bad scan block counter");
    if (m_block == 0 && m_wasRunning)
    {
        start();
    }
}

void UploadQueue::addItem(const Database::DbQueueItem& item)
{
    auto dbItem = item;

    if (dbItem.uid.isEmpty())
    {
        dbItem.uid = Utils::createUuid();
    }

    if (dbItem.submitTime.isEmpty())
    {
        dbItem.submitTime = App::instance()->timeManager()->currentDateTimeISO();
    }

    App::instance()->database()->appendQueueItem(dbItem);

    emit changed();
}

void UploadQueue::enumItems(std::function<void(const Database::DbQueueItem& item)> callback)
{
    App::instance()->database()->enumQueueItems([&](const Database::DbQueueItem& item)
    {
        callback(item);
    });
}

bool UploadQueue::hasItems(const QString& projectUid)
{
    return itemCount(projectUid) > 0;
}

int UploadQueue::itemCount(const QString& projectUid)
{
    auto count = 0;

    enumItems([&](const Database::DbQueueItem& item)
    {
        if (item.projectUid == projectUid && !item.flags.testFlag(Database::QueueFlag::DB_COMPLETE_FLAG))
        {
            count++;
        }
    });

    return count;
}

QString UploadQueue::createSnapshot()
{
    stop();
    auto startOnExit = qScopeGuard([&] { start(); });

    auto app = App::instance();

    auto filePath = app->sendFilePath() + "/QueueSnapshot.zip";
    qDebug() << "Create upload queue snapshot";
    qFatalIf(QFile::exists(filePath) && !QFile::remove(filePath), "Failed to delete old snapshot");

    auto snapshotPath = QDir::cleanPath(app->tempPath() + "/QueueSnapshot");
    QDir(snapshotPath).removeRecursively();
    qFatalIf(!Utils::ensurePath(snapshotPath), "Failed to create temp folder");
    auto cleanupOnExit = qScopeGuard([&] { QDir(snapshotPath).removeRecursively(); });

    auto itemCount = 0;
    enumItems([&](const Database::DbQueueItem& item)
    {
        // Skip already completed items. They have lost their payloads anyway.
        if (item.flags.testFlags(Database::QueueFlag::DB_COMPLETE_FLAG))
        {
            return;
        }

        // Create a path.
        auto prefix = QString("%1-").arg(itemCount++, 4, 10, QChar('0'));
        auto itemPath = QString("%1/%2").arg(snapshotPath, Utils::makeUniqueFilename(prefix, item.submitTime));
        qFatalIf(!Utils::ensurePath(itemPath), "Failed to create item path folder");

        auto queueItem = QVariantMap
        {
            { "uid", item.uid },
            { "projectUid", item.projectUid },
            { "dataUid", item.dataUid },
            { "summary", item.summary },
            { "submitTime", item.submitTime },
            { "uploadTime", item.uploadTime },
            { "state", item.state },
            { "type", item.type },
            { "url", item.url },
            { "payloadError", item.payloadError },
            { "webhookError", item.webhookError },
            { "attachments", item.attachments },
            { "completed", item.flags.testFlag(Database::QueueFlag::DB_COMPLETE_FLAG) },
        };

        Utils::variantMapToJsonFile(itemPath + "/metadata.json", queueItem);
        Utils::writeDataToFile(itemPath + "/payload", item.payload);
        Utils::writeDataToFile(itemPath + "/webhook.json", item.webhook);

        if (!item.webhook.isEmpty())
        {
            Utils::writeJsonToFile(itemPath + "/webhook.json", item.webhook);
        }

        for (auto attachmentIt = item.attachments.constBegin(); attachmentIt != item.attachments.constEnd(); attachmentIt++)
        {
            if (!QFile::copy(App::instance()->getMediaFilePath(*attachmentIt), itemPath + "/" + *attachmentIt))
            {
                qDebug() << "Failed to archive attachment: " << *attachmentIt;
            }
        }
    });

    // Remove data folder if empty.
    if (itemCount == 0)
    {
        emit App::instance()->showError(tr("No unsent data to export"));
        return QString();
    }

    // Zip the file.
    if (!Utils::zipCompressDir(filePath, snapshotPath))
    {
        emit App::instance()->showError(tr("Failed to create snapshot"));
        return QString();
    }

    // Return the package.
    Utils::mediaScan(filePath);

    return filePath;
}

void UploadQueue::clearQueue()
{
    Audit::add("Clear upload queue");

    stop();
    auto startOnExit = qScopeGuard([&] { start(); });

    App::instance()->database()->deleteQueueItems(Database::QueueFlag::DB_COMPLETE_FLAG);
    
    App::instance()->garbageCollectMedia();

    emit changed();
}

void UploadQueue::resetQueue()
{
    Audit::add("Reset upload queue");

    stop();
    auto startOnExit = qScopeGuard([&] { start(); });

    App::instance()->database()->resetQueueItems();

    App::instance()->garbageCollectMedia();

    emit changed();
}

void UploadQueue::kick()
{
    update_processing(true);
    if (m_worker)
    {
        QMetaObject::invokeMethod(m_worker, "kick", Qt::QueuedConnection);
    }
}

bool UploadQueue::processSync(const QString& projectUid)
{
    qFatalIf(m_thread.get(), "UploadQueue should be stopped");

    auto worker = std::unique_ptr<UploadQueueWorker>(new UploadQueueWorker());
    worker->initNetwork();
    auto result = ApiResult::Success();

    connect(worker.get(), &UploadQueueWorker::processCompleted, this, [&, projectUid](const QMap<QString, ApiResult>& results)
    {
        result = results.value(projectUid, ApiResult::Success());
    });

    connect(worker.get(), &UploadQueueWorker::progress, this, [&, projectUid](int index, int count)
    {
        if (index == 1)
        {
            App::instance()->initProgress(projectUid, "UploadData", { count });
        }

        App::instance()->sendProgress(index);
    });

    worker->processItems(projectUid);

    if (!result.success)
    {
        emit App::instance()->showError(result.errorString);
    }

    emit changed();

    return result.success;
}

bool UploadQueue::waitForQueue(const QString& projectUid)
{
    stop();
    auto startOnExit = qScopeGuard([&] { start(); });

    return processSync(projectUid);
}

} // namespace CyberTracker
