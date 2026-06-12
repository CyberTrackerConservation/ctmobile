#pragma once

#include "UtilsPlatform.h"

using namespace CyberTracker;

#if defined(Q_OS_ANDROID)

class AndroidUtils : public PlatformUtils
{
public:
    explicit AndroidUtils(QObject* parent) : PlatformUtils(parent)
    {}

    void init() override
    {}

    void share(const QUrl& url, const QString& title) override
    {
        QJniObject jsUrl = QJniObject::fromString(url.toString());
        QJniObject jsTitle = QJniObject::fromString(title);
        QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>(
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
        QJniObject jsPath = QJniObject::fromString(shareFilePath);
        QJniObject jsTitle = QJniObject::fromString(title);
        QJniObject jsMimeType = QJniObject::fromString(mimeType);
        QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>(
            "sendFile", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
            jsPath.object<jstring>(),
            jsTitle.object<jstring>(),
            jsMimeType.object<jstring>());
    }

    bool saveMedia(const QString& filePath, const QString& filename, const QString& mimeType, const QString& directoryName) override
    {
        QJniObject jsFilePath = QJniObject::fromString(filePath);
        QJniObject jsFilename = QJniObject::fromString(filename);
        QJniObject jsMimeType = QJniObject::fromString(mimeType);
        QJniObject jsDirectoryName = QJniObject::fromString(directoryName);

        auto contentUri = QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("saveMedia", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
                                                                        jsFilePath.object<jstring>(),
                                                                        jsFilename.object<jstring>(),
                                                                        jsMimeType.object<jstring>(),
                                                                        jsDirectoryName.object<jstring>()).toString();

        if (!contentUri.isEmpty())
        {
            emit mediaSaved(filename, contentUri);
            return true;
        }

        return false;
    }

    bool deleteMedia(const QString& contentUri) override
    {
        QJniObject jsContentUri = QJniObject::fromString(contentUri);
        auto result = QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("deleteMedia", "(Ljava/lang/String;)Z", jsContentUri.object<jstring>());
        qDebug() << "Media deleted 1 " << contentUri << result;

        result = QFile::remove(contentUri);
        qDebug() << "Media deleted 2 " << contentUri << result;

        Utils::mediaScan(contentUri);

        return true;
    }

    QString copyContent(const QString& contentUri, const QString& targetPath) override
    {
        auto contentName = contentUri;
        if (contentUri.startsWith("content:"))
        {
            QJniObject jsPath = QJniObject::fromString(contentUri);
            auto result = QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("resolveContentUri", "(Ljava/lang/String;)Ljava/lang/String;", jsPath.object<jstring>()).toString();
            contentName = result.isEmpty() ? contentUri : result;
        }

        if (contentName.isEmpty())
        {
            qDebug() << "Failed to resolve content: " << contentUri;
            return "";
        }

        auto sourceFilePath = Utils::urlToLocalFile(contentUri);
        auto targetFilePath = targetPath + "/" + QFileInfo(contentName).fileName();

        QFile::remove(targetFilePath);

        if (!QFile::copy(sourceFilePath, targetFilePath))
        {
            qDebug() << "Failed to copy content: " << contentUri << targetFilePath;
            return "";
        }

        QFile::setPermissions(targetFilePath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther);

        return targetFilePath;
    }

    QString copyToDownload(const QString& filePath, const QString& folder) override
    {
        QJniObject jsFilePath = QJniObject::fromString(filePath);
        QJniObject jsFolder = QJniObject::fromString(folder);
        QJniObject jsMimeType = QJniObject::fromString(Utils::mimeDatabase()->mimeTypeForFile(filePath).name());

        return QJniObject(QNativeInterface::QAndroidApplication::context()).callObjectMethod("copyToDownload", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", jsFilePath.object<jstring>(), jsFolder.object<jstring>(), jsMimeType.object<jstring>()).toString();
    }

    bool hasContent(const QString& uri) override
    {
        QJniObject jsUri = QJniObject::fromString(uri);

        return QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<jboolean>("hasContentUri", "(Ljava/lang/String;)Z", jsUri.object<jstring>());
    }

    void requestReview() override
    {
        QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("requestReview", "()V");
    }
};
#endif

#if OS_DESKTOP
class DesktopUtils : public PlatformUtils
{
public:
    explicit DesktopUtils(QObject* parent): PlatformUtils(parent)
    {}

    void init() override
    {}

    void share(const QUrl& /*url*/, const QString& /*title*/) override
    {
        qDebug() << "share not supported";
    }

    void sendFile(const QString& /*filePath*/, const QString& /*title*/, const QString& /*mimeType*/, int /*requestId*/) override
    {
        qDebug() << "sendFile not supported";
    }

    bool saveMedia(const QString& filePath, const QString& filename, const QString& /*mimeType*/, const QString& directoryName) override
    {
        auto mediaPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        if (!Utils::ensurePath(mediaPath))
        {
            qDebug() << "Failed to create media path: " << mediaPath;
            return false;
        }

        if (!directoryName.isEmpty())
        {
            mediaPath = mediaPath + "/" + directoryName;
            if (!Utils::ensurePath(mediaPath))
            {
                qDebug() << "Failed to create media sub path: " << mediaPath;
                return false;
            }
        }

        auto mediaFilePath = mediaPath + "/" + filename;

        if (!QFile::copy(filePath, mediaFilePath))
        {
            qDebug() << "Failed to copy media from: " << filePath << " to " << mediaPath;
            return false;
        }

        emit mediaSaved(filename, mediaFilePath);

        return true;
    }

    bool deleteMedia(const QString& contentUri) override
    {
        if (!QFile::exists(contentUri))
        {
            qDebug() << "Media not found: " << contentUri;
            return false;
        }

        if (!QFile::remove(contentUri))
        {
            qDebug() << "Media not deleted: " << contentUri;
            return false;
        }

        return true;
    }

    QString copyContent(const QString& contentUri, const QString& targetPath) override
    {
        auto sourceFilePath = Utils::urlToLocalFile(contentUri);
        auto targetFilePath = targetPath + "/" + QFileInfo(contentUri).fileName();

        QFile::remove(targetFilePath);

        if (!QFile::copy(sourceFilePath, targetFilePath))
        {
            qDebug() << "Failed to copy content: " << contentUri << targetFilePath;
            return "";
        }

        QFile::setPermissions(targetFilePath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther);

        return targetFilePath;
    }

    QString copyToDownload(const QString& filePath, const QString& folder) override
    {
        auto targetPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        auto targetFilePath = targetPath + "/" + QFileInfo(filePath).fileName();

        if (!QFile::copy(filePath, targetFilePath))
        {
            qDebug() << "Failed to copy to Download: " << filePath << targetFilePath;
            return "";
        }
        
        return targetFilePath;
    }

    bool hasContent(const QString& uri) override
    {
        return QFile::exists(Utils::urlToLocalFile(uri));
    }

    void requestReview() override
    {
        qDebug() << "requestReview not supported";
    }
};
#endif

PlatformUtils::PlatformUtils(QObject* parent) : QObject(parent)
{
}

void PlatformUtils::init()
{
#if defined(Q_OS_IOS)
    m_platformUtils = new IosUtils(this);
#elif defined(Q_OS_ANDROID)
    m_platformUtils = new AndroidUtils(this);
#else
    m_platformUtils = new DesktopUtils(this);
#endif

    m_platformUtils->init();

    connect(m_platformUtils, &PlatformUtils::mediaSaved, this, &PlatformUtils::mediaSaved);
}

void PlatformUtils::share(const QUrl& url, const QString& title)
{
    m_platformUtils->share(url, title);
}

void PlatformUtils::sendFile(const QString& filePath, const QString& title, const QString& mimeType, int requestId)
{
    m_platformUtils->sendFile(filePath, title, mimeType, requestId);
}

bool PlatformUtils::saveMedia(const QString& filePath, const QString& filename, const QString& mimeType, const QString& directoryName)
{
    return m_platformUtils->saveMedia(filePath, filename, mimeType, directoryName);
}

bool PlatformUtils::deleteMedia(const QString& contentUri)
{
    return m_platformUtils->deleteMedia(contentUri);
}

QString PlatformUtils::copyContent(const QString& contentUri, const QString& targetPath)
{
    return m_platformUtils->copyContent(contentUri, targetPath);
}

QString PlatformUtils::copyToDownload(const QString& filePath, const QString& folder)
{
    return m_platformUtils->copyToDownload(filePath, folder);
}

bool PlatformUtils::hasContent(const QString& uri)
{
    return m_platformUtils->hasContent(uri);
}

void PlatformUtils::requestReview()
{
    m_platformUtils->requestReview();
}
