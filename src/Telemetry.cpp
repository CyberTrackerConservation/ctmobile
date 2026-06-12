#include "Telemetry.h"
#include "App.h"

namespace CyberTracker {

namespace {
    Telemetry* g_instance = nullptr;
}

Telemetry::Telemetry(const QString& filePath, QObject *parent) : QObject(parent)
{
    qFatalIf(g_instance != nullptr, "Telemetry singleton already exists");
    g_instance = this;

    m_platform =
        #if defined(Q_OS_IOS)
            "ios";
        #elif defined(Q_OS_ANDROID)
            "android";
        #elif defined(Q_OS_WINDOWS)
            "windows";
        #elif defined(Q_OS_MACOS)
            "macos";
        #elif defined(Q_OS_LINUX)
            "linux";
        #else
            "unknown";
        #endif

    m_sku = App::instance()->config()["title"].toString();
    m_build = App::instance()->buildString();

    // Load the cumulative usage file once.
    QMutexLocker lock(&m_usageMutex);
    m_filePath = filePath;
    m_usage = Utils::variantMapFromJsonFile(m_filePath);
}

void Telemetry::setUsage(const QString& projectUid, const QString& stateSpace, const QString& name, const QVariant& value)
{
    QMutexLocker lock(&g_instance->m_usageMutex);

    auto project = g_instance->m_usage.value(projectUid).toMap();

    if (stateSpace.isEmpty())
    {
        if (project.value(name) == value)
        {
            return; // unchanged — do not rewrite the file
        }

        project.insert(name, Utils::variantFromJSValue(value));
    }
    else
    {
        auto state = project.value(stateSpace).toMap();
        if (state.value(name) == value)
        {
            return; // unchanged — do not rewrite the file
        }

        state.insert(name, Utils::variantFromJSValue(value));
        project.insert(stateSpace, state);
    }

    g_instance->m_usage.insert(projectUid, project);

    Utils::writeJsonToFile(g_instance->m_filePath, Utils::variantMapToJson(g_instance->m_usage));
}

QVariantMap Telemetry::getUsage(const QString& projectUid) const
{
    QMutexLocker lock(&m_usageMutex);
    return m_usage.value(projectUid).toMap();
}

void Telemetry::gaSendEvent(const QString& name, const QVariantMap& params, bool block) const
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
    auto reply = App::instance()->networkAccessManager()->post(request, QJsonDocument(payload).toJson());

    connect(reply, &QNetworkReply::finished, this, &Telemetry::replyFinished);

    if (block)
    {
        Utils::httpWait(reply);
    }
}

void Telemetry::aiSendEvent(const QString& name, const QVariantMap& props, const QVariantMap& measures, bool block) const
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
        else if (propValue.userType() != QMetaType::QString && !propValue.toList().isEmpty())
        {
            propString = QString(QJsonDocument(QJsonArray::fromVariantList(propValue.toList())).toJson(QJsonDocument::Compact));
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
    auto reply = App::instance()->networkAccessManager()->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, &Telemetry::replyFinished);

    if (block)
    {
        Utils::httpWait(reply);
    }
}

void Telemetry::replyFinished()
{
    auto reply = qobject_cast<QNetworkReply*>(sender());
    if (reply)
    {
        reply->deleteLater();
    }
}

} // namespace CyberTracker