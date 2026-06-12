#pragma once
#include "pch.h"
#include "Database.h"
#include "UploadQueue.h"

namespace CyberTracker {

class UploadQueueModel: public VariantListModel, public QQmlParserStatus
{
    Q_OBJECT

    Q_INTERFACES (QQmlParserStatus)

public:
    UploadQueueModel(QObject* parent = nullptr);
    ~UploadQueueModel() override;

    void classBegin() override;
    void componentComplete() override;

private slots:
    void queueItemChanged(const QString& itemUid);
    void rebuild();

private:
    UploadQueue* m_uploadQueue = nullptr;
    QStringList m_cacheItemUids;
    QMap<QString, QString> m_projectTitles;
    QString m_workingUid;
    QVariantMap makeItem(const Database::DbQueueItem& queueItem);
    void setWorking(const QString& itemUid, bool working);
};

} // namespace CyberTracker