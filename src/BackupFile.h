#pragma once
#include "pch.h"

namespace CyberTracker {

class BackupFile : public QObject
{
    Q_OBJECT

public:
    explicit BackupFile(QObject* parent = nullptr);

    void registerFile(const QString& projectTitle, const QString& filePath);
    void registerData(const QString& projectTitle, const QByteArray& data, const QString& prefix, const QString& suffix);

    Q_INVOKABLE void reset();
    Q_INVOKABLE QVariantList buildModel() const;

    Q_INVOKABLE QString createZip() const;

signals:
    void changed();

private:
    void enforceQuota();
}; 

} // namespace CyberTracker