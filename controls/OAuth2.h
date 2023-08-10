#pragma once
#include <App.h>
#include <QOAuth2AuthorizationCodeFlow>

class OAuth2 : public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY(int, timeoutSeconds)
    QML_WRITABLE_AUTO_PROPERTY(QString, scope)
    QML_WRITABLE_AUTO_PROPERTY(QString, authorizationUrl)
    QML_WRITABLE_AUTO_PROPERTY(QString, accessTokenUrl)
    QML_WRITABLE_AUTO_PROPERTY(QString, clientId)
    QML_WRITABLE_AUTO_PROPERTY(QString, clientSecret)
    QML_WRITABLE_AUTO_PROPERTY(QString, redirectUri)
    QML_WRITABLE_AUTO_PROPERTY(bool, googleNative)

public:
    explicit OAuth2(QObject *parent = nullptr);

    Q_INVOKABLE void start();

signals:
    void granted(const QString& accessToken, const QString& refreshToken);
    void failed(const QString& errorString);

private:
    QOAuth2AuthorizationCodeFlow* m_flow = nullptr;
    QTimer m_timer;
};
