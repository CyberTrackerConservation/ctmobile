#pragma once
#include "pch.h"

namespace CyberTracker {

class ExportFile : public QObject
{
    Q_OBJECT

public:
    explicit ExportFile(QObject* parent = nullptr);

    Q_INVOKABLE void registerFile(const QString& filePath, const QVariantMap& data);
    Q_INVOKABLE void removeFile(const QString& filePath);
    Q_INVOKABLE QVariantList buildModel(const QString& projectUid = QString(), const QString& filter = QString()) const;

    Q_INVOKABLE void syncWithDownloads();

signals:
    void changed();
}; 

} // namespace CyberTracker
