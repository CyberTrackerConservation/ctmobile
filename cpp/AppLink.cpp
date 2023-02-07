#include "AppLink.h"

AppLink::AppLink(QObject* parent): QObject(parent)
{
    //
    // NOTE: only add to the end of this list, otherwise you will break outstanding QR codes.
    //

    m_dictionary = QStringList
    {
        "true",
        "false",
        "connector",
        "auth",
        "launch",
        "server",
        "webUpdateUrl",
        "surveyId",
        "formId",
        "projectId",
        "token",
        "packageUuid",
        "account",
        "\"https://kf.kobotoolbox.org\"",
        "\"https://kobo.humanitarianresponse.info\"",
        "\"trilliontrees\"",
        "\"consosci\"",
        "\"EarthRanger\"",
        "\"Classic\"",
        "\"Esri\"",
        "\"ODK\"",
        "\"Native\"",
        "\"KoBo\"",
        "\"SMART\"",
        "accessToken",
        "refreshToken",
        "username",
        "WebUpdate",
        "url",
        "packageUrl"
    };

    m_shorteners = QMap<QString, QString>
    {
        { "\"https://cybertrackerwiki.org/assets/", "\"wiki:/" },
    };

    qFatalIf(m_dictionary.count() > 254, "Dictionary too large");
}

QString AppLink::encode(const QVariantMap& value)
{
    auto bytes = QByteArray();

    auto append = [&](const QString& value)
    {
        // Dictionary.
        auto dictIndex = m_dictionary.indexOf(value);
        if (dictIndex != -1)
        {
            bytes.append((char)dictIndex);
            return;
        }

        // Shorteners.
        auto v = value;
        for (auto it = m_shorteners.constKeyValueBegin(); it != m_shorteners.constKeyValueEnd(); it++)
        {
            if (v.startsWith(it->first))
            {
                v.replace(it->first, it->second);
                break;
            }
        }

        bytes.append((char)0xff);
        bytes.append(v.toLatin1());
        bytes.append('\0');
        //qDebug() << "Dictionary miss: " << value;
    };

    for (auto it = value.constKeyValueBegin(); it != value.constKeyValueEnd(); it++)
    {
        append(it->first);

        auto value = QString(QJsonDocument(QJsonObject::fromVariantMap(QVariantMap {{ "v", it->second }})).toJson(QJsonDocument::Compact));
        value.remove(0, 5);
        value.remove(value.length() - 1, 1);
        append(value);
    }

    return bytes.toBase64(QByteArray::Base64UrlEncoding);
}

QVariantMap AppLink::decode(const QString& code)
{
    try
    {
        auto bytes = QByteArray::fromBase64(code.toUtf8(), QByteArray::Base64UrlEncoding);
        auto string = QString(bytes).trimmed();

        if (string.at(0) == '{' && string.right(1) == '}')
        {
            return Utils::variantMapFromJson(bytes);
        }

        auto retrieve = [&](int* index) -> QString
        {
            // Dictionary.
            auto i = *index;
            auto n = bytes[i++];
            if (n != (char)0xff)
            {
                *index = i;
                return m_dictionary.value(n, "");
            }

            auto string = QByteArray();
            for (; bytes[i] != '\0'; i++)
            {
                string.append(bytes[i]);
            }

            *index = i + 1;

            // Shorteners.
            auto result = QString(string);
            for (auto it = m_shorteners.constKeyValueBegin(); it != m_shorteners.constKeyValueEnd(); it++)
            {
                if (result.startsWith(it->second))
                {
                    result.remove(0, it->second.length());
                    result.prepend(it->first);
                    break;
                }
            }

            return result;
        };

        auto lines = QStringList({"{"});

        auto i = 0;
        for (;;)
        {
            auto name = retrieve(&i);
            if (i == bytes.length())
            {
                break;
            }

            auto value = retrieve(&i);
            auto end = i >= bytes.length();
            lines.append(QString("\"%1\":%2%3").arg(name, value, end ? "" : ","));
            if (end)
            {
                break;
            }
        }

        lines.append("}");

        return Utils::variantMapFromJson(lines.join("").toUtf8());
    }
    catch (...)
    {
        return QVariantMap();
    }
}
