#include "OAuth2.h"
#include <QOAuthHttpServerReplyHandler>
#include <QDesktopServices>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Android

#if defined(Q_OS_ANDROID)
namespace {
OAuth2* s_instance = nullptr;
}

extern "C"
{
    JNIEXPORT void JNICALL Java_org_cybertracker_mobile_MainActivity_authGranted(JNIEnv* env, jobject, jstring code)
    {
        auto cleanup = qScopeGuard([&] { s_instance = nullptr; });
        if (s_instance == nullptr)
        {
            return;
        }

        auto authCode = env->GetStringUTFChars(code, 0);

        QNetworkAccessManager networkAccessManager;
        auto accessToken = QString();
        auto refreshToken = QString();
        auto response = Utils::googleAcquireOAuthTokenFromCode(&networkAccessManager, authCode, &accessToken, &refreshToken);
        if (!response.success)
        {
            qDebug() << "Failed to get token: " << authCode << response.errorString;
            emit s_instance->failed(response.errorString);
            return;
        }

        emit s_instance->granted(accessToken, refreshToken);
    }

    JNIEXPORT void JNICALL Java_org_cybertracker_mobile_MainActivity_authFailed(JNIEnv* env, jobject, jstring message)
    {
        auto cleanup = qScopeGuard([&] { s_instance = nullptr; });
        if (s_instance == nullptr)
        {
            return;
        }

        emit s_instance->failed(env->GetStringUTFChars(message, 0));
    }
}
#endif

OAuth2::OAuth2(QObject *parent): QObject(parent)
{
    m_timeoutSeconds = 60 * 5;
    m_redirectUri = "http://127.0.0.1:1234";
    m_googleNative = false;
}

void OAuth2::start()
{
    if (m_googleNative)
    {
     #if defined(Q_OS_ANDROID)
         s_instance = this;
         QAndroidJniObject clientId = QAndroidJniObject::fromString(App::instance()->googleCredentials()["clientId"].toString());
         QAndroidJniObject requestedScope = QAndroidJniObject::fromString(scope());
         QtAndroid::androidActivity().callMethod<void>("googleLogin", "(Ljava/lang/String;Ljava/lang/String;)V", clientId.object<jstring>(), requestedScope.object<jstring>());
         return;
     #endif
    }

    if (m_flow)
    {
        delete m_flow;
        m_flow = nullptr;
    }

    m_flow = new QOAuth2AuthorizationCodeFlow(this);
    m_flow->setScope(m_scope);
    m_flow->setAuthorizationUrl(m_authorizationUrl);
    m_flow->setAccessTokenUrl(m_accessTokenUrl);
    m_flow->setClientIdentifier(m_clientId);
    m_flow->setClientIdentifierSharedKey(m_clientSecret);

    m_flow->setModifyParametersFunction([&](QAbstractOAuth::Stage stage, QVariantMap* parameters)
    {
        if (stage == QAbstractOAuth::Stage::RequestingAuthorization)
        {
            parameters->insert("redirect_uri", m_redirectUri);
            return;
        }

        // Percent-decode the "code" parameter so Google can match it.
        if (stage == QAbstractOAuth::Stage::RequestingAccessToken)
        {
            QByteArray code = parameters->value("code").toByteArray();
            parameters->insert("code", QUrl::fromPercentEncoding(code));
            return;
        }
    });

    auto replyHandler = new QOAuthHttpServerReplyHandler(1234, m_flow);
    m_flow->setReplyHandler(replyHandler);

    connect(m_flow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser, this, [&](QUrl url)
    {
        QUrlQuery query(url);

        query.addQueryItem("prompt", "consent");
        query.addQueryItem("access_type", "offline");
        url.setQuery(query);

        QDesktopServices::openUrl(url);
    });

    connect(m_flow, &QOAuth2AuthorizationCodeFlow::granted, this, [&]()
    {
        m_timer.stop();
        emit granted(m_flow->token(), m_flow->refreshToken());
        delete m_flow;
        m_flow = nullptr;
    });

    connect(m_flow, &QOAuth2AuthorizationCodeFlow::error, this, [&](const QString &error, const QString& /*errorDescription*/, const QUrl& /*uri*/)
    {
        m_timer.stop();
        emit failed(error);
        delete m_flow;
        m_flow = nullptr;
    });

    connect(&m_timer, &QTimer::timeout, this, [&]()
    {
        emit failed(tr("Timeout"));
        delete m_flow;
        m_flow = nullptr;
    });

    m_timer.start(m_timeoutSeconds * 1000);

    m_flow->grant();
}
