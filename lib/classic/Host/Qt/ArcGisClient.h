#pragma once
#include "pch.h"

class ArcGisClient: public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY(QString, clientId)
    QML_WRITABLE_AUTO_PROPERTY(QString, clientSecret)
    QML_WRITABLE_AUTO_PROPERTY(QString, redirectUri)
    QML_WRITABLE_AUTO_PROPERTY(QString, username)
    QML_WRITABLE_AUTO_PROPERTY(QString, password)

    QML_WRITABLE_AUTO_PROPERTY(QString, authorizationCode)
    QML_WRITABLE_AUTO_PROPERTY(QString, refreshToken)
    QML_WRITABLE_AUTO_PROPERTY(QString, accessToken)

public:
    ArcGisClient(QObject* parent = nullptr);

    struct SendResponse
    {
        bool success = false;
        int errorCode = 0;
        QString errorMessage;
    };

    struct SendResult
    {
        int objectId = 0;
        bool success = false;
    };

    struct SendDataResponse
    {
        bool success = false;
        int errorCode = 0;
        QString errorMessage;
        QList<SendResult> results;
    };

    bool authorizeOAuth();
    bool authorizeToken();

    void reset();

    bool refreshAccessToken();

    SendDataResponse sendData(const QString& serviceUrl, int layerId, const QString& filename);
    SendDataResponse sendData(const QString& serviceUrl, int layerId, const QByteArray& data);
    SendResponse sendAttachment(const QString& serviceUrl, int layerId, int objectId, const QString& filename);

private:
    QString extractText(const QString& value, const QString& prefix, const QString& suffix);
    int httpPost(const QString& url, const QUrlQuery& query, QByteArray* responseBodyOut = nullptr, QString* responseLocationOut = nullptr);
};
