#include "UploadQueueModel.h"
#include "App.h"

namespace CyberTracker {

UploadQueueModel::UploadQueueModel(QObject *parent): VariantListModel(parent)
{
}

UploadQueueModel::~UploadQueueModel()
{

}

void UploadQueueModel::classBegin()
{

}

void UploadQueueModel::componentComplete()
{
    m_uploadQueue = App::instance()->uploadQueue();

    // Cache project names.
    {
        auto projects = App::instance()->projectManager()->buildListPtr();
        for (auto projectIt = projects.constBegin(); projectIt != projects.constEnd(); projectIt++)
        {
            m_projectTitles[projectIt->get()->uid()] = projectIt->get()->title();
        }
    }

    connect(m_uploadQueue, &UploadQueue::changed, this, &UploadQueueModel::rebuild);

    connect(m_uploadQueue, &UploadQueue::itemChanged, this, &UploadQueueModel::queueItemChanged);

    connect(m_uploadQueue, &UploadQueue::workingUidChanged, this, [&]() {
        setWorking(m_workingUid, false);
        setWorking(m_uploadQueue->workingUid(), true);
    });

    rebuild();
}

QVariantMap UploadQueueModel::makeItem(const Database::DbQueueItem& queueItem)
{
    auto subtitle = queueItem.payloadError == QUEUE_SUCCESS_TEXT ? QString("%1 - %2").arg(tr("Success"), queueItem.uploadTime) : queueItem.payloadError;
    if (queueItem.payloadError == QUEUE_SUCCESS_TEXT && queueItem.webhookError != QUEUE_SUCCESS_TEXT)
    {
        subtitle = QString("%1%2").arg(tr("Webhook error"), queueItem.webhookError);
    }

    return QVariantMap {
        { "uid", queueItem.uid },
        { "projectUid", queueItem.projectUid },
        { "sightingUid", queueItem.dataUid },
        { "error", queueItem.payloadError },
        { "projectTitle", m_projectTitles.value(queueItem.projectUid, tr("Unknown project")) },
        { "uploadTime", queueItem.uploadTime },
        { "summary", queueItem.summary },
        { "subtitle", subtitle },
        { "working", false },
        { "attachmentCount", queueItem.attachments.count() },
        { "complete", queueItem.flags.testFlag(Database::QueueFlag::DB_COMPLETE_FLAG) },
    };
}

void UploadQueueModel::setWorking(const QString& itemUid, bool working)
{
    if (working)
    {
        m_workingUid = itemUid;
    }

    if (!itemUid.isEmpty())
    {
        auto index = m_cacheItemUids.indexOf(itemUid);
        if (index != -1)
        {
            auto itemData = get(index).toMap();
            itemData["working"] = working;
            replace(index, itemData);
        }
    }
}

void UploadQueueModel::queueItemChanged(const QString& itemUid)
{
    auto index = m_cacheItemUids.indexOf(itemUid);
    if (index != -1)
    {
        auto dbItem = Database::DbQueueItem();
        App::instance()->database()->loadQueueItem(itemUid, &dbItem);
        replace(index, makeItem(dbItem));
    }
}

void UploadQueueModel::rebuild()
{
    clear();
    m_cacheItemUids.clear();

    auto queueItems = QVariantList();

    m_uploadQueue->enumItems([&](const Database::DbQueueItem& queueItem)
    {
        if (queueItem.flags.testFlag(Database::QueueFlag::DB_AUTO_DELETE))
        {
            return;
        }

        queueItems.append(makeItem(queueItem));
        m_cacheItemUids.append(queueItem.uid);
    });

    appendList(queueItems);

    setWorking(m_uploadQueue->workingUid(), true);
}

} // namespace CyberTracker