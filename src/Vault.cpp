#include "Vault.h"
#include "App.h"

#if !OS_DESKTOP
#define QTKEYCHAIN_NO_EXPORT
#include "qtkeychain/qtkeychain/keychain.h"

constexpr char VAULT_NAME[] = "vaultName";
constexpr char VAULT_SETTINGS[] = "vaultSettings";
#endif

namespace CyberTracker {

constexpr char VAULT_TOKENS[] = "tokens";
constexpr char VAULT_ACCESS_TOKEN[] = "accessToken";
constexpr char VAULT_UPLOAD_TOKEN[] = "uploadToken";
constexpr char VAULT_REFRESH_TOKEN[] = "refreshToken";

Vault::Vault(const QString& filePath, QObject *parent)
    : QObject(parent), m_filePath(filePath)
{
    load();
}

QVariant Vault::getSetting(const QString& name, const QVariant& defaultValue) const
{
    QMutexLocker locker(&m_lock);
    return m_cache.value(name);
}

void Vault::setSetting(const QString& name, const QVariant& value)
{
    QMutexLocker locker(&m_lock);

    if (!value.isValid())
    {
        m_cache.remove(name);
    }
    else
    {
        m_cache.insert(name, value);
    }

    save();
}

QString Vault::getAccessToken(const QString& projectUid) const
{
    return getTokenValue(projectUid, VAULT_ACCESS_TOKEN);
}

void Vault::setAccessToken(const QString& projectUid, const QString& token)
{
    return setTokenValue(projectUid, VAULT_ACCESS_TOKEN, token);
}

QString Vault::getUploadToken(const QString& projectUid) const
{
    return getTokenValue(projectUid, VAULT_UPLOAD_TOKEN);
}

void Vault::setUploadToken(const QString& projectUid, const QString& token)
{
    return setTokenValue(projectUid, VAULT_UPLOAD_TOKEN, token);
}

QString Vault::getRefreshToken(const QString& projectUid) const
{
    return getTokenValue(projectUid, VAULT_REFRESH_TOKEN);
}

void Vault::setRefreshToken(const QString& projectUid, const QString& token)
{
    return setTokenValue(projectUid, VAULT_REFRESH_TOKEN, token);
}

void Vault::expireToken(const QString& token)
{
    if (token.isEmpty())
    {
        return;
    }

    QMutexLocker locker(&m_lock);

    auto oldTokens = m_cache.value(VAULT_TOKENS).toMap();
    auto newTokens = QVariantMap();

    for (auto it = oldTokens.constKeyValueBegin(); it != oldTokens.constKeyValueEnd(); it++)
    {
        auto data = it->second.toMap();

        // Cleanup expired tokens.
        if (data.value(VAULT_ACCESS_TOKEN).toString() == token)
        {
            data.remove(VAULT_ACCESS_TOKEN);
        }
        else if (data.value(VAULT_UPLOAD_TOKEN).toString() == token)
        {
            data.remove(VAULT_UPLOAD_TOKEN);
        }
        else if (data.value(VAULT_REFRESH_TOKEN).toString() == token)
        {
            data.remove(VAULT_ACCESS_TOKEN);
            data.remove(VAULT_REFRESH_TOKEN);
        }

        // Put back whats left.
        if (!data.isEmpty())
        {
            newTokens.insert(it->first, data);
        }
    }

    m_cache.remove(VAULT_TOKENS);
    m_cache.insert(VAULT_TOKENS, newTokens);

    save();
}

void Vault::updateToken(const QString& oldRefreshToken, const QString& newAccessToken, const QString& newRefreshToken)
{
    if (oldRefreshToken.isEmpty())
    {
        qDebug() << "Failed to update old refresh token";
        return;
    }

    QMutexLocker locker(&m_lock);

    auto oldTokens = m_cache.value(VAULT_TOKENS).toMap();
    auto newTokens = QVariantMap();

    for (auto it = oldTokens.constKeyValueBegin(); it != oldTokens.constKeyValueEnd(); it++)
    {
        auto data = it->second.toMap();

        // Update matching token.
        if (data.value(VAULT_REFRESH_TOKEN).toString() == oldRefreshToken)
        {
            data.insert(VAULT_ACCESS_TOKEN, newAccessToken);
            data.insert(VAULT_REFRESH_TOKEN, newRefreshToken);
        }

        newTokens.insert(it->first, data);
    }

    m_cache.remove(VAULT_TOKENS);
    m_cache.insert(VAULT_TOKENS, newTokens);

    save();
}

QStringList Vault::getTokenRefreshProjects() const
{
    QMutexLocker locker(&m_lock);

    auto refreshMap = QMap<QString, QString>();
    auto tokens = m_cache.value(VAULT_TOKENS).toMap();

    for (auto it = tokens.constKeyValueBegin(); it != tokens.constKeyValueEnd(); it++)
    {
        auto data = it->second.toMap();
        auto refreshToken = data.value(VAULT_REFRESH_TOKEN).toString();

        if (!refreshToken.isEmpty() && !data.contains(VAULT_ACCESS_TOKEN))
        {
            refreshMap[data.value(VAULT_REFRESH_TOKEN).toString()] = it->first;
        }
    }

    return refreshMap.values();
}

QString Vault::getTokenValue(const QString& projectUid, const QString& subKey) const
{
    QMutexLocker locker(&m_lock);
    return m_cache.value(VAULT_TOKENS).toMap().value(projectUid).toMap().value(subKey).toString();
}

void Vault::setTokenValue(const QString& projectUid, const QString& subKey, const QString& value)
{
    QMutexLocker locker(&m_lock);

    auto tokens = m_cache.value(VAULT_TOKENS).toMap();
    auto entry = tokens.value(projectUid).toMap();

    if (value.isEmpty())
    {
        entry.remove(subKey);
    }
    else
    {
        entry.insert(subKey, value);
    }

    tokens.remove(projectUid);
    if (!entry.isEmpty())
    {
        tokens.insert(projectUid, entry);
    }

    if (m_cache.value(VAULT_TOKENS) != tokens)
    {
        m_cache.remove(VAULT_TOKENS);
        m_cache.insert(VAULT_TOKENS, tokens);
        save();
    }
}

void Vault::load()
{
#if OS_DESKTOP
    m_cache = Utils::variantMapFromJsonFile(m_filePath);

#else
    #if defined(Q_OS_ANDROID)
        QJniObject jsName = QJniObject::fromString(VAULT_TOKENS);
        auto result = QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("getVaultValue", "(Ljava/lang/String;)Ljava/lang/String;", jsName.object<jstring>()).toString();
        if (!result.isEmpty())
        {
            m_cache = Utils::variantMapFromJson(result.toLatin1());
            return;
        }

        // BUGBUG: Hack on Android - fall throught to try to recover tokens.
        // On Android, the maximum vault size is 256, so we did not catch that in time.
        // Happened around build 535, so this can be moved into a #else later as below with save().
    #endif

    auto job = new QKeychain::ReadPasswordJob(VAULT_NAME);
    job->setAutoDelete(false);
    job->setKey(VAULT_SETTINGS);

    connect(job, &QKeychain::ReadPasswordJob::finished, this, [this, job]()
    {
        if (job->error())
        {
            qDebug() << "Vault read failed: " << job->errorString();
            m_cache.clear();
        }
        else
        {
            m_cache = Utils::variantMapFromJson(job->textData().toLatin1());
        }

        job->deleteLater();
    });

    job->start();
#endif
}

void Vault::save() const
{
#if OS_DESKTOP
    Utils::writeJsonToFile(m_filePath, QJsonDocument::fromVariant(m_cache).toJson());

#elif defined(Q_OS_ANDROID)
    QJniObject jsName = QJniObject::fromString(VAULT_TOKENS);
    QJniObject jsData = QJniObject::fromString(Utils::variantMapToString(m_cache));
    QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("setVaultValue", "(Ljava/lang/String;Ljava/lang/String;)V", jsName.object<jstring>(), jsData.object<jstring>());

#else
    auto job = new QKeychain::WritePasswordJob(VAULT_NAME);
    job->setAutoDelete(false);
    job->setKey(VAULT_SETTINGS);

    connect(job, &QKeychain::WritePasswordJob::finished, this, [job]()
    {
        if (job->error())
        {
            qDebug() << "Vault write failed: " << job->errorString();
        }

        job->deleteLater();
    });

    job->setTextData(Utils::variantMapToString(m_cache));
    job->start();
#endif
}

} // namespace CyberTracker
