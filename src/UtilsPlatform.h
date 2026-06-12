#pragma once
#include "pch.h"

class PlatformUtils : public QObject
{
    Q_OBJECT

public:
    explicit PlatformUtils(QObject* parent);
    virtual void init();
    virtual void share(const QUrl& url, const QString& title);
    virtual void sendFile(const QString& filePath, const QString& title, const QString& mimeType, int requestId);
    virtual bool saveMedia(const QString& filePath, const QString& filename, const QString& mimeType, const QString& directoryName);
    virtual bool deleteMedia(const QString& contentUri);
    virtual QString copyContent(const QString& contentUri, const QString& targetPath);
    virtual QString copyToDownload(const QString& filePath, const QString& folder);
    virtual bool hasContent(const QString& uri);
    virtual void requestReview();

signals:
    void mediaSaved(const QString& filename, const QString& mediaFilePath);

private:
    PlatformUtils* m_platformUtils = nullptr;
};

#if defined(Q_OS_IOS)
class IosUtils : public PlatformUtils
{
public:
    explicit IosUtils(QObject* parent);

    void init() override;
    void share(const QUrl& url, const QString& title) override;
    void sendFile(const QString& filePath, const QString& title, const QString& mimeType, int requestId) override;
    bool saveMedia(const QString& filePath, const QString& filename, const QString& mimeType, const QString& directoryName) override;
    bool deleteMedia(const QString& contentUri) override;
    QString copyContent(const QString& contentUri, const QString& targetPath) override;
    QString copyToDownload(const QString& filePath, const QString& folder) override;
    bool hasContent(const QString& uri) override;
    void requestReview() override;
};
#endif
