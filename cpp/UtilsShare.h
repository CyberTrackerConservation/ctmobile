#pragma once
#include "pch.h"

class PlatformShareUtils : public QObject
{
    Q_OBJECT

public:
    PlatformShareUtils(QObject* parent) : QObject(parent)
    {}

    virtual void share(const QUrl& /*url*/, const QString& /*title*/)
    {}

    virtual void sendFile(const QString& /*filePath*/, const QString& /*title*/, const QString& /*mimeType*/, int /*requestId*/)
    {}
};

#if defined(Q_OS_IOS)
class IosShareUtils : public PlatformShareUtils
{
    Q_OBJECT

public:
    explicit IosShareUtils(QObject* parent);

    void share(const QUrl& url, const QString& title) override;
    void sendFile(const QString& filePath, const QString& title, const QString& mimeType, int requestId) override;
};

#elif defined(Q_OS_ANDROID)
class AndroidShareUtils : public PlatformShareUtils
{
    Q_OBJECT

public:
    explicit AndroidShareUtils(QObject* parent) : PlatformShareUtils(parent)
    {}

    void share(const QUrl& url, const QString& title) override
    {
        QAndroidJniObject jsUrl = QAndroidJniObject::fromString(url.toString());
        QAndroidJniObject jsTitle = QAndroidJniObject::fromString(title);
        QtAndroid::androidActivity().callMethod<void>(
            "share", "(Ljava/lang/String;Ljava/lang/String;)V",
            jsUrl.object<jstring>(), jsTitle.object<jstring>());
    }

    void sendFile(const QString& filePath, const QString& title, const QString& mimeType, int /*requestId*/) override
    {
        // Create the shared path.
        auto sharePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Share";  // See "file_paths.xml"
        qFatalIf(!Utils::ensurePath(sharePath), "Failed to create share path: " + sharePath);

        // Copy to the shared path.
        auto fileInfo = QFileInfo(filePath);
        auto shareFilePath = sharePath + '/' + fileInfo.fileName();
        QFile::remove(shareFilePath);
        QFile::copy(filePath, shareFilePath);
        qFatalIf(!QFile::exists(shareFilePath), "Failed to copy file to share");

        // Send the file.
        QAndroidJniObject jsPath = QAndroidJniObject::fromString(shareFilePath);
        QAndroidJniObject jsTitle = QAndroidJniObject::fromString(title);
        QAndroidJniObject jsMimeType = QAndroidJniObject::fromString(mimeType);
        QtAndroid::androidActivity().callMethod<void>(
            "sendFile", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
            jsPath.object<jstring>(),
            jsTitle.object<jstring>(),
            jsMimeType.object<jstring>());
    }
};
#endif

class ShareUtils : public QObject
{
    Q_OBJECT

public:
    explicit ShareUtils(QObject* parent) : QObject(parent)
    {
#if defined(Q_OS_IOS)
        m_platformShareUtils = new IosShareUtils(this);
#elif defined(Q_OS_ANDROID)
        m_platformShareUtils = new AndroidShareUtils(this);
#else
        m_platformShareUtils = new PlatformShareUtils(this);
#endif
    }

    void share(const QUrl& url, const QString& title)
    {
        m_platformShareUtils->share(url, title);
    }

    void sendFile(const QString& filePath, const QString& title, const QString& mimeType, int requestId)
    {
        m_platformShareUtils->sendFile(filePath, title, mimeType, requestId);
    }

private:
    PlatformShareUtils* m_platformShareUtils = nullptr;
};
