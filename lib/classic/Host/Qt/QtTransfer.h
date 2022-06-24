#pragma once
#include "pch.h"
#include "ArcGisClient.h"

class QtTransfer: public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY(bool, active)

public:
    QtTransfer(QObject* parent = nullptr);
    ~QtTransfer();

    void start(const QString& queuePath);
    void stop();

    bool sendDataHTTP(const QString& filename, const QString& url, const QString& username, const QString& password, bool compressed);
    bool sendDataESRI(const QString& filename, const QString& url, const QString& username, const QString& password);
    bool sendDataFTP(const QString& filename, const QString& url, const QString& username, const QString& password);
    bool sendDataUNC(const QString& filename, const QString& url, const QString& username, const QString& password);

signals:
    void stateChange(int state);

private:
    QThread* m_thread = nullptr;
    ArcGisClient m_arcGisClient;
};
