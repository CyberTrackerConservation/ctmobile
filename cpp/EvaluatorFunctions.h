#pragma once
#include "pch.h"
#include "App.h"
#include <regex>

//--------------------------------------------------------------------------------------------------
// Helpers.

QVariantList unpackParams(const QVariantList& params, QVariant defaultValue = QVariant())
{
    auto result = QVariantList();

    for (auto paramIt = params.constBegin(); paramIt != params.constEnd(); paramIt++)
    {
        auto param = *paramIt;
        if (param.userType() == QMetaType::QStringList || param.userType() == QMetaType::QVariantList)
        {
            auto items = param.toList();
            for (auto itemIt = items.constBegin(); itemIt != items.constEnd(); itemIt++)
            {
                result.append(*itemIt);
            }
        }
        else if (param.isValid())
        {
            result.append(param);
        }
    }

    if (result.isEmpty() && defaultValue.isValid())
    {
        result.append(defaultValue);
    }

    return result;
}

QStringList toStringList(const QVariantList& params)
{
    auto result = QStringList();
    for (auto paramIt = params.constBegin(); paramIt != params.constEnd(); paramIt++)
    {
        result.append(paramIt->toString());
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
// Functions.

// if(expression, then, else)
bool f_if(const QVariantList& params, QVariant& result)
{
    if (params.count() != 3)
    {
        qDebug("if expects 3 parameters");
        return false;
    }

    result = params[0].toBool() ? params[1] : params[2];
    return true;
}

// position(xpath)
bool f_position(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("position expects 1 parameter");
        return false;
    }

    qDebug() << "Not implemented";
    result = QVariant();

    return true;
}

// once(expression)
bool f_once(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("once expects 1 parameter");
        return false;
    }

    result = params[0];

    return true;
}

// selected(space_delimited_array, string)
bool f_selected(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("selected expects 2 parameters");
        return false;
    }

    auto arrayString = params[0].toString();
    auto searchString = params[1].toString();
    auto strings = arrayString.split(' ');
    result = strings.contains(searchString);

    return true;
}

// selected-at(space_delimited_array, n)
bool f_selected_at(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("selected-at expects 1 parameter");
        return false;
    }

    auto arrayString = params[0].toString();
    auto searchIndex = params[1].toInt();
    auto strings = arrayString.split(' ');

    if (searchIndex >= strings.count())
    {
        qDebug("selected-at requires a valid index");
        return false;
    }

    result = strings[searchIndex];

    return true;
}

// indexed-repeat(name, group, i[, sub_grp, sub_i[, sub_sub_grp, sub_sub_i]])
bool f_indexed_repeat(const QVariantList& params, QVariant& /*result*/)
{
    if (params.count() < 3)
    {
        qDebug("indexed-repeat expects at least 3 parameters");
        return false;
    }

    qDebug("Not implemented");

    return false;
}

// count-selected(multi_select_question)
bool f_count_selected(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("count-selected expects 1 parameter");
        return false;
    }

    result = unpackParams(params, "").constFirst().toString().split(' ').count();

    return true;
}

// count(nodeset)
bool f_count(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("count expects 1 parameter");
        return false;
    }

    result = unpackParams(params).count();

    return true;
}

// count-non-empty(nodeset)
bool f_count_non_empty(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("count-non-empty expects 1 parameter");
        return false;
    }

    auto count = 0;
    for (auto param: unpackParams(params))
    {
        if (!param.toString().isEmpty())
        {
            count++;
        }
    }

    result = count;

    return true;
}

// sum(nodeset)
bool f_sum(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("sum expects 1 parameter");
        return false;
    }

    auto sum = 0;
    for (auto param: unpackParams(params))
    {
        sum += param.toDouble();
    }

    result = sum;

    return true;
}

// max(nodeset)
bool f_max(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("max expects 1 parameter");
        return false;
    }

    auto maxValue = -DBL_MAX;
    for (auto param: unpackParams(params))
    {
        maxValue = qMax(maxValue, param.toDouble());
    }

    result = maxValue != -DBL_MAX ? maxValue : std::numeric_limits<double>::quiet_NaN();

    return true;
}

// min(nodeset)
bool f_min(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("min expects 1 parameter");
        return false;
    }

    auto minValue = DBL_MAX;
    for (auto param: unpackParams(params))
    {
        minValue = qMin(minValue, param.toDouble());
    }

    result = minValue != DBL_MAX ? minValue : std::numeric_limits<double>::quiet_NaN();

    return true;
}

// regex(string, expression)
bool f_regex(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("regex expects 2 parameters");
        return false;
    }

    if (params[0].isValid())
    {
        auto expr = params[1].toString();
        if (!expr.contains("\\u")) // Default to PCRE2, unless "\uFFFF" is present.
        {
            auto re = QRegularExpression(expr, QRegularExpression::PatternOption::DotMatchesEverythingOption);
            auto match = re.match(params[0].toString());
            result = match.hasMatch();
        }
        else // Use std regex for support of "\uFFFF" format.
        {
            auto re = std::wregex(expr.toStdWString());
            auto m = std::wsmatch();
            auto v = params[0].toString().toStdWString();
            result = std::regex_match(v, m, re);
        }
    }

    return true;
}

// contains(string, substring)
bool f_contains(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("contains expects 2 parameters");
        return false;
    }

    result = params[0].toString().contains(params[1].toString());

    return true;
}

// starts-with(string, substring)
bool f_starts_with(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("starts-with expects 2 parameters");
        return false;
    }

    result = params[0].toString().startsWith(params[1].toString());

    return true;
}

// ends-with(string, substring)
bool f_ends_with(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("ends-with expects 2 parameters");
        return false;
    }

    result = params[0].toString().endsWith(params[1].toString());

    return true;
}

// substr(string, start[, end])
bool f_substr(const QVariantList& params, QVariant& result)
{
    if (params.count() < 2 || params.count() > 3)
    {
        qDebug("substr expects 2 or 3 parameters");
        return false;
    }

    auto start = params[1].toInt();
    auto end = params.count() == 3 ? params[2].toInt() : -1;
    result = params[0].toString().mid(start, end);

    return true;
}

// substring-before(string, target)
bool f_substring_before(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("substring-before expects 2 parameters");
        return false;
    }

    auto string = params[0].toString();
    auto target = params[1].toString();
    auto index = string.indexOf(target);
    result = index == -1 ? "" : string.mid(0, index);

    return true;
}

// substring-after(string, target)
bool f_substring_after(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("substring-after expects 2 parameters");
        return false;
    }

    auto string = params[0].toString();
    auto target = params[1].toString();
    auto index = string.indexOf(target);
    result = index == -1 ? "" : string.mid(index + target.length());

    return true;
}

// translate(string, fromchars, tochars)
bool f_translate(const QVariantList& params, QVariant& /*result*/)
{
    if (params.count() != 3)
    {
        qDebug("translate expects 3 parameters");
        return false;
    }

    qDebug() << "Not implemented";

    return false;
}

// string-length(string)
bool f_string_length(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("string-length expects 1 parameter");
        return false;
    }

    result = params[0].toString().count();

    return true;
}

// normalize-space(string)
bool f_normalize_space(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("normalize-space expects 1 parameter");
        return false;
    }

    auto resultString = QString();
    auto lastWasSpace = true;
    for (auto c: params[0].toString())
    {
        if (c.isSpace() && lastWasSpace)
        {
            continue;
        }

        lastWasSpace = c.isSpace();
        resultString += c;
    }

    result = resultString.trimmed();

    return true;
}

// concat(arg [, arg [, arg [, arg [...]]]])
bool f_concat(const QVariantList& params, QVariant& result)
{
    auto unpackedParams = unpackParams(params);

    auto strings = QString();
    for (auto param: unpackedParams)
    {
        strings.append(param.toString());
    }

    result = strings;

    return true;
}

// checklist_match(arg [, arg [, arg [, arg [...]]]])
bool f_checklist_match(const QVariantList& params, QVariant& result)
{
    if (params.count() != 3)
    {
        qDebug("checklist_match expects 3 parameters");
        return false;
    }

    auto fieldValues = params[0].toString().split(' ');
    auto matchMode = params[1].toString();
    auto matches = params[2].toStringList();

    if (matchMode == "exact")
    {
        result = false;

        auto sameCount = matches.count() == fieldValues.count();
        if (sameCount)
        {
            result = true;
            for (auto fieldValue: fieldValues)
            {
                if (!matches.contains(fieldValue))
                {
                    result = false;
                    break;
                }
            }
        }
    }
    else if (matchMode == "and")
    {
        result = true;
        for (auto match: matches)
        {
            if (!fieldValues.contains(match))
            {
                result = false;
                break;
            }
        }
    }
    else if (matchMode == "or")
    {
        result = false;
        for (auto match: matches)
        {
            if (fieldValues.contains(match))
            {
                result = true;
                break;
            }
        }
    }

    return true;
}

// make_list(arg [, arg [, arg [, arg [...]]]])
bool f_make_list(const QVariantList& params, QVariant& result)
{
    result = unpackParams(params);

    return true;
}

// join(separator, nodeset)
bool f_join(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("join expects 2 parameters");
        return false;
    }

    auto strings = toStringList(unpackParams(params));
    auto separator = strings[0];
    strings.removeFirst();

    result = strings.join(separator);

    return true;
}

// boolean-from-string(string)
bool f_boolean_from_string(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("boolean-from-string expects 1 parameter");
        return false;
    }

    auto string = params[0].toString();
    result = string == "true" || string == "1";

    return true;
}

// string(arg)
bool f_string(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("string expects 1 parameter");
        return false;
    }

    result = params[0].toString();

    return true;
}

// round(number, places)
bool f_round(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("round expects 2 parameters");
        return false;
    }

    auto decimals = params[1].toInt();
    int multiply = qPow(10, decimals);

    auto value = params[0].toDouble() * multiply;
    result = round(value) / multiply;

    return true;
}

// int(number)
bool f_int(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("int expects 1 parameter");
        return false;
    }

    result = params[0].toInt();

    return true;
}

// number(arg)
bool f_number(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("number expects 1 parameter");
        return false;
    }

    bool ok = false;
    result = params[0].toDouble(&ok);
    if (!ok)
    {
        result = std::numeric_limits<double>::quiet_NaN();
    }

    return true;
}

// digest(data, algorithm, encoding method (optional))
bool f_digest(const QVariantList& params, QVariant& result)
{
    if (params.count() < 2 || params.count() > 3)
    {
        qDebug("digest expects 2 or 3 parameters");
        return false;
    }

    auto algorithmName = params[1].toString();
    auto algorithmEnum = QCryptographicHash::Md5;
    if (algorithmName == "MD5")
    {
        algorithmEnum = QCryptographicHash::Md5;
    }
    else if (algorithmName == "SHA-1")
    {
        algorithmEnum = QCryptographicHash::Sha1;
    }
    else if (algorithmName == "SHA-256")
    {
        algorithmEnum = QCryptographicHash::Sha256;
    }
    else if (algorithmName == "SHA-384")
    {
        algorithmEnum = QCryptographicHash::Sha384;
    }
    else if (algorithmName == "SHA-512")
    {
        algorithmEnum = QCryptographicHash::Sha512;
    }

    auto digest = QCryptographicHash::hash(params[0].toString().toLatin1(), algorithmEnum);

    auto encodingName = params.count() == 3 ? params[2].toString() : "base64";
    if (encodingName == "hex")
    {
        result = QString(digest.toHex());
    }
    else if (encodingName == "base64")
    {
        result = QString(digest.toBase64());
    }
    else
    {
        result = "";
    }

    return true;
}

// pow(number, power)
bool f_pow(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("pow expects 2 parameters");
        return false;
    }

    result = qPow(params[0].toDouble(), params[1].toDouble());

    return true;
}

// log(number)
bool f_log(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("log expects 1 parameter");
        return false;
    }

    result = qLn(params[0].toDouble());

    return true;
}

// log10(number)
bool f_log10(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("log10 expects 1 parameter");
        return false;
    }

    result = log10(params[0].toDouble());

    return true;
}

// abs(number)
bool f_abs(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("abs expects 1 parameter");
        return false;
    }

    result = qAbs(params[0].toDouble());

    return true;
}

// sin(number)
bool f_sin(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("sin expects 1 parameter");
        return false;
    }

    result = qSin(params[0].toDouble());

    return true;
}

// cos(number)
bool f_cos(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("cos expects 1 parameter");
        return false;
    }

    result = qCos(params[0].toDouble());

    return true;
}

// tan(number)
bool f_tan(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("tan expects 1 parameter");
        return false;
    }

    result = qTan(params[0].toDouble());

    return true;
}

// asin(number)
bool f_asin(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("asin expects 1 parameter");
        return false;
    }

    result = qAsin(params[0].toDouble());

    return true;
}

// acos(number)
bool f_acos(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("acos expects 1 parameter");
        return false;
    }

    result = qAcos(params[0].toDouble());

    return true;
}

// atan(number)
bool f_atan(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("atan expects 1 parameter");
        return false;
    }

    result = qAtan(params[0].toDouble());

    return true;
}

// atan2(y, x)
bool f_atan2(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("atan2 expects 2 parameters");
        return false;
    }

    result = qAtan2(params[0].toDouble(), params[1].toDouble());

    return true;
}

// sqrt(number)
bool f_sqrt(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("sqrt expects 1 parameter");
        return false;
    }

    result = qSqrt(unpackParams(params, 0.0).constFirst().toDouble());

    return true;
}

// exp(x)
bool f_exp(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("exp expects 1 parameter");
        return false;
    }

    result = qExp(unpackParams(params, 0.0).constFirst().toDouble());

    return true;
}

// exp10(x)
bool f_exp10(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("exp10 expects 1 parameter");
        return false;
    }

    result = qPow(10, unpackParams(params, 0.0).constFirst().toDouble());

    return true;
}

// pi()
bool f_pi(const QVariantList& params, QVariant& result)
{
    if (params.count() != 0)
    {
        qDebug("pi expects no parameters");
        return false;
    }

    result = M_PI;

    return true;
}

// today()
bool f_today(const QVariantList& params, QVariant& result)
{
    if (params.count() != 0)
    {
        qDebug("today expects no parameters");
        return false;
    }

    result = App::instance()->timeManager()->currentDateTime().date().toString(Qt::DateFormat::ISODate);

    return true;
}

// now()
bool f_now(const QVariantList& params, QVariant& result)
{
    if (params.count() != 0)
    {
        qDebug("now expects no parameters");
        return false;
    }

    result = App::instance()->timeManager()->currentDateTimeISO();

    return true;
}

// decimal-date-time(dateTime)
bool f_decimal_date_time(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("decimal-date-time expects 1 parameter");
        return false;
    }

    result = (double)Utils::decodeTimestampSecs(unpackParams(params, "1970-01-01").constFirst().toString()) / (60 * 60 * 24);

    return true;
}

// date(days)
bool f_date(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("date expects 1 parameter");
        return false;
    }

    result = QDateTime::fromSecsSinceEpoch(unpackParams(params, 0).constFirst().toInt() * 60 * 60 * 24).date().toString(Qt::DateFormat::ISODate);

    return true;
}

// date-time(days)
bool f_date_time(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("date expects 1 parameter");
        return false;
    }

    auto value = QDateTime::fromSecsSinceEpoch(unpackParams(params, 0).constFirst().toDouble() * 60 * 60 * 24);

    result = App::instance()->timeManager()->formatDateTime(value);

    return true;
}
// decimal-time(time)
bool f_decimal_time(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("decimal-time expects 1 parameter");
        return false;
    }

    auto t = Utils::decodeTimestamp(unpackParams(params, "1970-01-01").constFirst().toString()).time();
    result = (double)t.msecsSinceStartOfDay() / (1000 * 60 * 60 * 24);

    return true;
}

// format-date(date, format)
bool f_format_date(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("format-date expects 2 parameters");
        return false;
    }

    auto d = Utils::decodeTimestamp(unpackParams(params, "1970-01-01").constFirst().toString());
    auto f = QString(params[1].toString());
    f.replace("%Y", "yyyy");    //    %Y: 4-digit year
    f.replace("%y", "yy");      //    %y: 2-digit year
    f.replace("%m", "MM");      //    %m 0-padded month
    f.replace("%n", "M");       //    %n numeric month
    f.replace("%b", "MMM");     //    %b short text month (Jan, Feb, etc)*
    f.replace("%d", "dd");      //    %d 0-padded day of month
    f.replace("%e", "d");       //    %e day of month
    f.replace("%a", "ddd");     //    %a short text day (Sun, Mon, etc).*
    f.replace("%H", "HH");      //    %H 0-padded hour (24-hr time)
    f.replace("%h", "H");       //    %h hour (24-hr time)
    f.replace("%M", "mm");      //    %M 0-padded minute
    f.replace("%S", "ss");      //    %S 0-padded second
    f.replace("%3", "zzz");     //    %3 0-padded millisecond ticks.*

    result = d.toString(QString(f));

    return true;
}

// format-date-time(dateTime, format)
bool f_format_date_time(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("format-date-time expects 2 parameters");
        return false;
    }

    auto d = Utils::decodeTimestamp(params[0].toString());
    auto f = QString(params[1].toString());

    f.replace("%Y", "yyyy");    //    %Y: 4-digit year
    f.replace("%y", "yy");      //    %y: 2-digit year
    f.replace("%m", "MM");      //    %m 0-padded month
    f.replace("%n", "M");       //    %n numeric month
    f.replace("%b", "MMM");     //    %b short text month (Jan, Feb, etc)*
    f.replace("%d", "dd");      //    %d 0-padded day of month
    f.replace("%e", "d");       //    %e day of month
    f.replace("%a", "ddd");     //    %a short text day (Sun, Mon, etc).*

    f.replace("%H", "HH");      //    %H 0-padded hour (24-hr time)
    f.replace("%h", "H");       //    %h hour (24-hr time)
    f.replace("%M", "mm");      //    %M 0-padded minute
    f.replace("%S", "ss");      //    %S 0-padded second
    f.replace("%3", "zzz");     //    %3 0-padded millisecond ticks.*

    result = d.toString(QString(f));

    return true;
}

// area(nodeset | geoshape)
bool f_area(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("area expects 1 parameter");
        return false;
    }

    result = Utils::areaInSquareMeters(Utils::pointsFromXml(params[0].toString()));

    return true;
}

// distance(nodeset | geoshape | geotrace)
bool f_distance(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("distance expects 1 parameter");
        return false;
    }

    result = Utils::distanceInMeters(Utils::pointsFromXml(params[0].toString()));

    return true;
}

// random()
bool f_random(const QVariantList& params, QVariant& result)
{
    if (params.count() != 0)
    {
        qDebug("random expects 0 parameters");
        return false;
    }

    result = QRandomGenerator::global()->bounded(1.0);

    return true;
}

// randomize(nodeset[, seed])
bool f_randomize(const QVariantList& params, QVariant& /*result*/)
{
    if (params.count() < 1 || params.count() > 2)
    {
        qDebug("randomize expects 1 or 2 parameters");
        return false;
    }

    qDebug() << "Not implemented";

    return true;
}

// uuid([length])
bool f_uuid(const QVariantList& params, QVariant& result)
{
    if (params.count() > 1)
    {
        qDebug("uuid expects 0 or 1 parameters");
        return false;
    }

    auto uuid = QString();
    auto length = params.count() == 0 ? 16 : params[0].toInt();

    while (uuid.length() < length)
    {
        uuid.append(QUuid::createUuid().toString(QUuid::Id128));
    }

    result = uuid.mid(0, length);

    return true;
}

// boolean(arg)
bool f_boolean(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("boolean expects 1 parameter");
        return false;
    }

    result = params[0].toBool();

    return true;
}

// not(arg)
bool f_not(const QVariantList& params, QVariant& result)
{
    if (params.count() != 1)
    {
        qDebug("not expects 1 parameter");
        return false;
    }

    f_boolean(params, result);
    result = !result.toBool();

    return true;
}

// coalesce(arg, arg)
bool f_coalesce(const QVariantList& params, QVariant& result)
{
    if (params.count() != 2)
    {
        qDebug("coalesce expects 2 parameters");
        return false;
    }

    auto string1 = params[0].toString();
    auto string2 = params[1].toString();

    result = !string1.isEmpty() ? string1 : string2;

    return true;
}

// checklist(min, max, response[, response[, response[, ...]]])
bool f_checklist(const QVariantList& params, QVariant& /*result*/)
{
    if (params.count() < 3)
    {
        qDebug("checklist expects at least 3 parameters");
        return false;
    }

    qDebug() << "Not implemented";

    return true;
}

// weighted-checklist(min, max, reponse, weight[, response, weight[, response, weight[, response, weight[, ... ]]])
bool f_weighted_checklist(const QVariantList& params, QVariant& /*result*/)
{
    if (params.count() < 4)
    {
        qDebug("weighted-checklist expects at least 3 parameters");
        return false;
    }

    qDebug() << "Not implemented";

    return true;
}

// true()
bool f_true(const QVariantList& params, QVariant& result)
{
    if (params.count() != 0)
    {
        qDebug("true expects 0 parameters");
        return false;
    }

    result = true;

    return true;
}

// false()
bool f_false(const QVariantList& params, QVariant& result)
{
    if (params.count() != 0)
    {
        qDebug("false expects 0 parameters");
        return false;
    }

    result = false;

    return true;
}

//--------------------------------------------------------------------------------------------------
// Function table.

struct FunctionTable
{
    const char* name;
    std::function<bool(const QVariantList& params, QVariant& result)> function;
};

QMap<QString, std::function<bool(const QVariantList& params, QVariant& result)>> s_functionTable =
{
    // Control flow.
    { "if",                  f_if                  },
    { "position",            f_position            },
    { "once",                f_once                },

    // Accessing response values.
    { "selected",            f_selected            },
    { "selected-at",         f_selected_at         },
    { "count-selected",      f_count_selected      },

    // Repeat groups.
    { "indexed-repeat",      f_indexed_repeat      },
    { "count",               f_count               },
    { "count-non-empty",     f_count_non_empty     },
    { "sum",                 f_sum                 },
    { "max",                 f_max                 },
    { "min",                 f_min                 },

    // Strings.
    { "regex",               f_regex               },
    { "contains",            f_contains            },
    { "starts-with",         f_starts_with         },
    { "ends-with",           f_ends_with           },
    { "substr",              f_substr              },
    { "substring-before",    f_substring_before    },
    { "substring-after",     f_substring_after     },
    { "translate",           f_translate           },
    { "string-length",       f_string_length       },
    { "normalize-space",     f_normalize_space     },

    // Combining strings.
    { "concat",              f_concat              },
    { "join",                f_join                },

    // Converting to and from strings.
    { "boolean-from-string", f_boolean_from_string },
    { "string",              f_string              },

    // Math.
    { "round",               f_round               },
    { "int",                 f_int                 },
    { "number",              f_number              },
    { "digest",              f_digest              },

    // Calculation.
    { "pow",                 f_pow                 },
    { "log",                 f_log                 },
    { "log10",               f_log10               },
    { "abs",                 f_abs                 },
    { "sin",                 f_sin                 },
    { "cos",                 f_cos                 },
    { "tan",                 f_tan                 },
    { "asin",                f_asin                },
    { "acos",                f_acos                },
    { "atan",                f_atan                },
    { "atan2",               f_atan2               },
    { "sqrt",                f_sqrt                },
    { "exp",                 f_exp                 },
    { "exp10",               f_exp10               },
    { "pi",                  f_pi                  },

    // Date and time.
    { "today",               f_today               },
    { "now",                 f_now                 },

    // Converting dates and times.
    { "decimal-date-time",   f_decimal_date_time   },
    { "date",                f_date                },
    { "date-time",           f_date_time           },
    { "decimal-time",        f_decimal_time        },
    { "format-date",         f_format_date         },
    { "format-date-time",    f_format_date_time    },

    // Geography.
    { "area",                f_area                },
    { "distance",            f_distance            },

    // Utility.
    { "checklist_match",     f_checklist_match     },
    { "make_list",           f_make_list           },
    { "random",              f_random              },
    { "randomize",           f_randomize           },
    { "uuid",                f_uuid                },
    { "boolean",             f_boolean             },
    { "not",                 f_not                 },
    { "coalesce",            f_coalesce            },
    { "checklist",           f_checklist           },
    { "weighted-checklist",  f_weighted_checklist  },
    { "true",                f_true                },
    { "false",               f_false               },
};
