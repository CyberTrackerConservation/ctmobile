#include "Telemetry.h"
#include "App.h"

Telemetry::Telemetry(QObject *parent) : QObject(parent)
{
    m_platform =
        #if defined(Q_OS_IOS)
            "ios";
        #elif defined(Q_OS_ANDROID)
            "android";
        #elif defined(Q_OS_WIN)
            "windows";
        #elif defined(Q_OS_MACOS)
            "macos";
        #else
            "unknown";
        #endif

    m_sku = App::instance()->config()["title"].toString();
    m_build = App::instance()->buildString();
}

void Telemetry::gaSendEvent(const QString& name, const QVariantMap& params) const
{
    if (m_gaMeasurementId.isEmpty() || m_gaSecret.isEmpty() || !App::instance()->uploadAllowed())
    {
        // Google Analytics disabled.
        return;
    }

    auto languageCode = App::instance()->settings()->languageCode();

    auto paramsJson = QJsonObject
    {
        { "deviceId", App::instance()->settings()->deviceId() },
        { "platform", m_platform },
        { "sku", m_sku },
        { "build", m_build },
        { "locale", languageCode != "system" ? languageCode : QLocale::system().name() }
    };

    for (auto it = params.constKeyValueBegin(); it != params.constKeyValueEnd(); it++)
    {
        paramsJson.insert(it->first, QJsonValue::fromVariant(it->second));
    }

    auto payload = QJsonObject
    {
        { "client_id", App::instance()->settings()->deviceId() },
        { "timestamp_micros", QDateTime::currentMSecsSinceEpoch() * 1000 },
        { "events", QJsonArray { QJsonObject {{ "name", name }, {"params", paramsJson }} }}
    };

    QNetworkRequest request(QUrl("https://www.google-analytics.com/mp/collect?&measurement_id=" + m_gaMeasurementId + "&api_secret=" + m_gaSecret));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    App::instance()->networkAccessManager()->post(request, QJsonDocument(payload).toJson());
}

void Telemetry::aiSendEvent(const QString& name, const QVariantMap& props, const QVariantMap& measures) const
{
    if (m_aiKey.isEmpty() || !App::instance()->uploadAllowed())
    {
        // Azure Application Insights disabled.
        return;
    }

    auto languageCode = App::instance()->settings()->languageCode();

    auto propsJson = QJsonObject {
        { "deviceId", App::instance()->settings()->deviceId() },
        { "platform", m_platform },
        { "sku", m_sku },
        { "build", m_build },
        { "locale", languageCode != "system" ? languageCode : QLocale::system().name() }
    };

    for (auto it = props.constKeyValueBegin(); it != props.constKeyValueEnd(); it++)
    {
        auto propValue = it->second;
        auto propString = QString();

        if (!propValue.toMap().isEmpty())
        {
            propString = QString(QJsonDocument(QJsonObject::fromVariantMap(propValue.toMap())).toJson(QJsonDocument::Compact));
        }
        else
        {
            propString = propValue.toString();
        }

        propsJson[it->first] = propString;
    }

    auto measuresJson = QJsonObject();
    for (auto it = measures.constKeyValueBegin(); it != measures.constKeyValueEnd(); it++)
    {
        measuresJson[it->first] = it->second.toDouble();
    }

    auto payload = QJsonObject
    {
        { "name", "AppEvents" },
        { "time", Utils::encodeTimestamp(QDateTime::currentDateTimeUtc()) },
        { "iKey", m_aiKey },
        { "data", QJsonObject {
            { "baseType", "EventData" },
            { "baseData", QJsonObject {
                { "ver", 2 },
                { "name", name },
                { "properties", propsJson },
                { "measurements", measuresJson } } } } },
    };

    QNetworkRequest request(QUrl("https://dc.services.visualstudio.com/v2/track"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-json-stream");
    App::instance()->networkAccessManager()->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
}
