#pragma once
#include "pch.h"

namespace CyberTracker {

class Vault : public QObject
{
    Q_OBJECT

    QML_SETTING(QString, googleEmail, QString())
    QML_SETTING(QString, googleAccessToken, QString())
    QML_SETTING(QString, googleRefreshToken, QString())

    QML_SETTING(QString, webhookToken, QString())

public:
    explicit Vault(const QString& filePath, QObject *parent = nullptr);

    QVariant getSetting(const QString& name, const QVariant& defaultValue) const;
    void setSetting(const QString& name, const QVariant& value);

    QString getAccessToken(const QString& projectUid) const;
    void setAccessToken(const QString& projectUid, const QString& token);

    QString getUploadToken(const QString& projectUid) const;
    void setUploadToken(const QString& projectUid, const QString& token);

    QString getRefreshToken(const QString& projectUid) const;
    void setRefreshToken(const QString& projectUid, const QString& token);

    void expireToken(const QString& token);
    void updateToken(const QString& oldRefreshToken, const QString& newAccessToken, const QString& newRefreshToken);

    QStringList getTokenRefreshProjects() const;

private:
    mutable QMutex m_lock;
    QString m_filePath;
    QVariantMap m_cache;

    QString getTokenValue(const QString& projectUid, const QString& subKey) const;
    void setTokenValue(const QString& projectUid, const QString& subKey, const QString& value);

    void load();
    void save() const;
};

} // namespace CyberTracker