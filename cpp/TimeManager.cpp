#include "TimeManager.h"
#include "App.h"

TimeManager::TimeManager(QObject* parent): QObject(parent)
{
    m_correctionEnabled = true;
    m_zoneFile.setFileName(":/lib/zonedetect/timezone21.bin");

    if (m_zoneFile.open(QIODevice::ReadOnly))
    {
        auto fileSize = static_cast<uint>(m_zoneFile.size());
        m_zoneFileMap = m_zoneFile.map(0, fileSize);
        m_zoneDb = ZDOpenDatabaseFromMemory(m_zoneFileMap, fileSize);
    }

    reset();
    loadState();
}

TimeManager::~TimeManager()
{
    if (m_zoneDb)
    {
        ZDCloseDatabase(m_zoneDb);
        m_zoneDb = nullptr;
    }

    if (m_zoneFileMap)
    {
        m_zoneFile.unmap(m_zoneFileMap);
        m_zoneFileMap = nullptr;
    }

    m_zoneFile.close();
}

void TimeManager::reset()
{
    update_corrected(false);
    m_correctCounter = 0;
    m_timeError = 0;
    update_timeZoneId(QDateTime::currentDateTime().timeZone().id());
    update_timeZoneText("??");
}

void TimeManager::loadState()
{
    auto state = App::instance()->settings()->timeManager();
    m_corrected = state.value("corrected", false).toBool();

    if (m_corrected)
    {
        m_timeError = state.value("timeError", 0).toLongLong();
        m_timeZoneId = state.value("timeZoneId", QDateTime::currentDateTime().timeZone().id()).toString();
        m_timeZoneText = getTimeZoneText(m_timeZoneId, currentMSecsSinceEpoch());
    }
}

void TimeManager::saveState()
{
    App::instance()->settings()->set_timeManager(QVariantMap
    {
        { "corrected", m_corrected },
        { "timeError", m_timeError },
        { "timeZoneId", m_timeZoneId }
    });
}

QString TimeManager::getTimeZoneText(const QString& timeZoneId, qint64 mSecsSinceEpoch) const
{
    auto timeZone = QTimeZone(timeZoneId.toLatin1());
    auto dateTime = QDateTime::fromMSecsSinceEpoch(mSecsSinceEpoch, timeZone);
    auto zoneDiff = timeZone.offsetFromUtc(dateTime);

    auto result = QString("UTC");
    if (zoneDiff != 0)
    {
        result += zoneDiff < 0 ? "-" : "+";
        result += QString::number(abs(static_cast<float>(zoneDiff / 3600)));
        auto minutes = abs(static_cast<long>(zoneDiff)) % 3600;
        if (minutes != 0)
        {
            result += ":" + QString::number(minutes / 60);
        }
    }

    return result;
}

void TimeManager::computeTimeError(double x, double y, double s, qint64 t)
{
    // Compute the time zone based on the location.
    auto safeZone = static_cast<float>(0);
    auto results = ZDLookup(m_zoneDb, static_cast<float>(y), static_cast<float>(x), &safeZone);
    auto result = results[0];
    auto ianaId = QString();

    if ((result.lookupResult != ZD_LOOKUP_END) && (result.numFields >= 2))
    {
        auto zonePrefix = QString(result.data[0]);
        auto zoneId = QString(result.data[1]);
        ianaId = zonePrefix + zoneId;

        static bool once = true;
        if (once)
        {
            once = false;
            qDebug() << "computeTimeError zone found: " << ianaId;
        }
    }
    else
    {
        ianaId = QDateTime::currentDateTime().timeZone().id();
        qDebug() << "computeTimeError zone *not* found: " << ianaId;
    }

    ZDFreeResults(results);
    update_timeZoneId(ianaId);
    update_timeZoneText(getTimeZoneText(ianaId, t));

    // Compute the time error.
    if (!std::isnan(s))
    {
        m_timeError = t - QDateTime::currentMSecsSinceEpoch();

        // Suppress this message which happens a lot.
        //qDebug() << "computeTimeError found time error " << m_timeError / 1000 << " seconds";

        // Complete the correction.
        m_correctCounter++;
        if (m_correctCounter >= 5)
        {
            update_corrected(true);

            // Sanity test for the corrected time.
            if (currentDateTime().date().year() < 2022)
            {
                qDebug() << "computeTimeError found year less than 2022: " << currentDateTime();
                m_timeError = 0;
                m_correctCounter = 0;
                update_corrected(false);
            }
        }
    }

    saveState();
}

qint64 TimeManager::correctTime(qint64 systemMSecsSinceEpoch) const
{
    if (m_correctionEnabled && m_corrected)
    {
        return systemMSecsSinceEpoch + m_timeError;
    }
    else
    {
        return systemMSecsSinceEpoch;
    }
}

QDateTime TimeManager::currentDateTime() const
{
    return QDateTime::fromMSecsSinceEpoch(correctTime(QDateTime::currentMSecsSinceEpoch()), QTimeZone(m_timeZoneId.toLatin1()));
}

QString TimeManager::currentDateTimeISO() const
{
    return Utils::encodeTimestamp(currentDateTime());
}

qint64 TimeManager::currentMSecsSinceEpoch() const
{
    return correctTime(QDateTime::currentMSecsSinceEpoch());
}

QString TimeManager::formatDateTime(qint64 mSecsSinceEpoch) const
{
    return Utils::encodeTimestamp(QDateTime::fromMSecsSinceEpoch(mSecsSinceEpoch, QTimeZone(m_timeZoneId.toLatin1())));
}

QString TimeManager::formatDateTime(QDateTime dateTime) const
{
    dateTime.setTimeZone(QTimeZone(m_timeZoneId.toLatin1()));
    return Utils::encodeTimestamp(dateTime);
}

QString TimeManager::getDateText(const QString& isoDateTime, const QString& emptyValue) const
{
    if (isoDateTime.isEmpty())
    {
        return emptyValue;
    }

    auto date = Utils::decodeTimestamp(isoDateTime).date();
    return App::instance()->locale().toString(date, QLocale::ShortFormat);
}

QString TimeManager::getTimeText(const QString& isoDateTime, const QString& emptyValue) const
{
    if (isoDateTime.isEmpty())
    {
        return emptyValue;
    }

    auto time = Utils::decodeTimestamp(isoDateTime).time();
    return App::instance()->locale().toString(time, QLocale::ShortFormat);
}
