#include "TranscribeQueue.h"
#include "App.h"

namespace CyberTracker {

namespace {
int INITIAL_TIMEOUT = 5000;
int DEFAULT_TIMEOUT = OS_DESKTOP ? 10000 : 40000;
static constexpr char TRANSCRIBE_FILL_SERVER[] = "https://us-central1-formtranscriber.cloudfunctions.net/gemini-audio-json";
static constexpr char TRANSCRIBE_ONLY_SERVER[] = "https://us-central1-formtranscriber.cloudfunctions.net/gemini-audio-transcribeOnly";
constexpr char TRANSCRIBE_SUCCESS_TEXT[] = "Success";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TranscribeQueueWorker

TranscribeQueueWorker::TranscribeQueueWorker(QObject* parent): QObject(parent)
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

void TranscribeQueueWorker::start()
{
    m_networkAccessManager.reset(new QNetworkAccessManager(this));
    m_timer->start(INITIAL_TIMEOUT);
}

void TranscribeQueueWorker::stop()
{
    m_timer->stop();
}

void TranscribeQueueWorker::kick()
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

void TranscribeQueueWorker::processItems(const QString& filterProjectUid)
{
    if (!m_database)
    {
        m_database = App::instance()->createDatabasePtr();
    }

    // Get the items.
    auto pendingItems = QList<Database::DbTranscribeItem>();
    auto completedItems = QList<Database::DbTranscribeItem>();
    m_database->enumTranscribeItems([&](const Database::DbTranscribeItem& item)
    {
        if (!filterProjectUid.isEmpty() && filterProjectUid != item.projectUid)
        {
            return;
        }

        if (item.error == TRANSCRIBE_SUCCESS_TEXT)
        {
            completedItems.append(item);
            return;
        }

        pendingItems.append(item);
    });

    // Emit the completed items.
    for (auto itemIt = completedItems.constBegin(); itemIt != completedItems.constEnd(); itemIt++)
    {
        emit itemCompleted(*itemIt);
    }

    // Check for upload allowed.
    if (!App::instance()->uploadAllowed())
    {
        return;
    }

    emit workStarted();

    auto success = true;

    // Upload the pending items.
    for (auto itemIt = pendingItems.constBegin(); itemIt != pendingItems.constEnd(); itemIt++)
    {
        if (QThread::currentThread()->isInterruptionRequested())
        {
            break;
        }

        // Remove items where the file is gone. This can happen if the sighting is deleted.
        auto filePath = App::instance()->getMediaFilePath(itemIt->filename);
        if (!QFile::exists(filePath))
        {
            m_database->deleteTranscribeItem(itemIt->filename);
            continue;
        }

        // Upload the item and emit successful results.
        emit itemStarted(*itemIt);

        auto result = QVariantMap();
        auto error = QString();
        if (!uploadItem(*itemIt, &result, &error))
        {
            // Failed.
            qDebug() << "Failed to transcribe: " << itemIt->filename << error;
            success = false;
            continue;
        }

        // Success.
        auto item = *itemIt;
        item.result = result;
        item.error = error;
        m_database->transaction([&]() -> bool
        {
            m_database->updateTranscribeItem(item);
            if (!m_database->hasPendingTranscribeItems(item.projectUid, item.dataUid))
            {
                m_database->setDataFlags(item.projectUid, item.dataUid, Database::DataFlag::DB_TRANSCRIBING_FLAG, false);
                emit App::instance()->sightingFlagsChanged(item.projectUid, item.dataUid);
            }
            return true;
        });

        emit itemCompleted(item);
    }

    emit workCompleted(success);
}

bool TranscribeQueueWorker::uploadItem(const Database::DbTranscribeItem& item, QVariantMap* resultOut, QString* errorOut)
{
    if (!item.prompt.isEmpty() && !item.schema.isEmpty() && !item.fillRecordUid.isEmpty() && !item.fillFieldUids.isEmpty())
    {
        return uploadItemV1(item, resultOut, errorOut);
    }

    return uploadItemTranscribe(item, resultOut, errorOut);
}

bool TranscribeQueueWorker::uploadItemV1(const Database::DbTranscribeItem& item, QVariantMap* resultOut, QString* errorOut)
{
    auto filePath = App::instance()->getMediaFilePath(item.filename);
    auto file = QFile(filePath);
    qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to open audio file");

    auto multipart = QHttpMultiPart(QHttpMultiPart::FormDataType);
    multipart.setBoundary(Utils::makeMultiPartBoundary());

    auto prompt1 = R"(Transcribe the WAV file of a wildlife observation to text using the following vocabulary:)";
    auto prompt2 = item.prompt.toUtf8();
    auto prompt3 = R"(Then extract the data from the result and output it in JSON format. Put the raw text in the "_RAW_" property.)";
    auto prompt4 = R"(Do not emit data not in the transcribed string. Do not emit 0 for integer fields that are not explicitly mentioned.)";
    auto prompt5 = QString("The language might be %1").arg(App::instance()->languageName());
    auto prompt = QString("%1\n%2\n%3\n%4\n%5").arg(prompt1, prompt2, prompt3, prompt4, prompt5).toUtf8();

    auto part1 = QHttpPart();
    part1.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"prompt\"");
    part1.setBody(prompt);
    multipart.append(part1);

    auto part2 = QHttpPart();
    part2.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"schema\"");
    part2.setBody(item.schema.toUtf8());
    multipart.append(part2);

    auto part3 = QHttpPart();
    part3.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=audio.wav");
    part3.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    part3.setBodyDevice(&file);
    multipart.append(part3);

    auto response = Utils::httpPostMultiPart(m_networkAccessManager.get(), TRANSCRIBE_FILL_SERVER, &multipart);

    *errorOut = response.success ? TRANSCRIBE_SUCCESS_TEXT : response.errorString;
    *resultOut = response.success ? QJsonDocument::fromJson(response.data).object().toVariantMap() : QVariantMap();

    return response.success;
}

bool TranscribeQueueWorker::uploadItemTranscribe(const Database::DbTranscribeItem& item, QVariantMap* resultOut, QString* errorOut)
{
    auto filePath = App::instance()->getMediaFilePath(item.filename);
    auto file = QFile(filePath);
    qFatalIf(!file.open(QIODevice::ReadOnly), "Failed to open audio file");

    auto multipart = QHttpMultiPart(QHttpMultiPart::FormDataType);
    multipart.setBoundary(Utils::makeMultiPartBoundary());

    auto part1 = QHttpPart();
    part1.setHeader(QNetworkRequest::KnownHeaders::ContentDispositionHeader, "form-data; name=\"language\"");
    part1.setBody(App::instance()->languageName().toUtf8());
    multipart.append(part1);

    auto part2 = QHttpPart();
    part2.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=audio.wav");
    part2.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    part2.setBodyDevice(&file);
    multipart.append(part2);

    auto response = Utils::httpPostMultiPart(m_networkAccessManager.get(), TRANSCRIBE_ONLY_SERVER, &multipart);

    *errorOut = response.success ? TRANSCRIBE_SUCCESS_TEXT : response.errorString;
    *resultOut = response.success ? QJsonDocument::fromJson(response.data).object().toVariantMap() : QVariantMap();

    return response.success;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TranscribeQueue

TranscribeQueue::TranscribeQueue(QObject *parent): QObject(parent)
{
    m_working = false;
    m_lastSuccess = true;
}

TranscribeQueue::~TranscribeQueue()
{
    stop();
}

TranscribeQueue::Lock::Lock()
{
    App::instance()->transcribeQueue()->pause();
}

TranscribeQueue::Lock::~Lock()
{
    App::instance()->transcribeQueue()->resume();
}

void TranscribeQueue::start()
{
    stop();

    m_thread.reset(new QThread(this));

    m_worker = new TranscribeQueueWorker();
    m_worker->moveToThread(m_thread.get());

    connect(m_thread.get(), &QThread::started, m_worker, &TranscribeQueueWorker::start);
    connect(m_thread.get(), &QThread::finished, m_worker, &TranscribeQueueWorker::stop);
    connect(m_thread.get(), &QThread::finished, m_worker, [this]()
    {
        delete m_worker;
        m_worker = nullptr;
    });

    connect(m_worker, &TranscribeQueueWorker::workStarted, this, [&]() { update_working(true); });
    connect(m_worker, &TranscribeQueueWorker::workCompleted, this, [&](bool success) { update_working(false); update_lastSuccess(success); update_workingDataUid(QString()); });
    connect(m_worker, &TranscribeQueueWorker::itemStarted, this, [&](const Database::DbTranscribeItem& item) { update_workingDataUid(item.dataUid); });

    connect(m_worker, &TranscribeQueueWorker::itemCompleted, this, &TranscribeQueue::itemCompleted);

    m_thread->start();
}

void TranscribeQueue::stop()
{
    if (m_thread)
    {
        m_thread->requestInterruption();
        m_thread->quit();
        m_thread->wait();
        m_thread.reset();
    }
}

void TranscribeQueue::pause()
{
    m_block++;
    if (m_block == 1)
    {
        m_wasRunning = m_thread.get() != nullptr;
        stop();
    }
}

void TranscribeQueue::resume()
{
    m_block--;
    qFatalIf(m_block < 0, "Bad scan block counter");
    if (m_block == 0 && m_wasRunning)
    {
        start();
    }
}

void TranscribeQueue::request(const Database::DbTranscribeItem& item)
{
    App::instance()->database()->appendTranscribeItem(item);

    kick();
}

void TranscribeQueue::consume(const QString& filename)
{
    App::instance()->database()->deleteTranscribeItem(filename);
}

void TranscribeQueue::kick()
{
    if (m_worker)
    {
        QMetaObject::invokeMethod(m_worker, "kick", Qt::QueuedConnection);
    }
}

bool TranscribeQueue::hasWork() const
{
    auto result = false;
    App::instance()->database()->enumTranscribeItems([&](const Database::DbTranscribeItem& item)
    {
        result = true;
    });

    return result;
}

} // namespace CyberTracker
