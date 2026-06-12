#include "TimeManager.h"
#include "App.h"

namespace CyberTracker {

TimeManager::TimeManager(QObject* parent): QObject(parent)
{
    m_correctionEnabled = true;
    reset();
    loadState();
}

TimeManager::~TimeManager()
{
}

void TimeManager::reset()
{
    update_corrected(false);
    m_correctCounter = 0;
    m_timeError = 0;
    update_timeZoneId(QDateTime::currentDateTime().timeZone().id());
    m_timeZone = QTimeZone(m_timeZoneId.toLatin1());
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
        m_timeZone = QTimeZone(m_timeZoneId.toLatin1());
        m_timeZoneText = getTimeZoneText(currentMSecsSinceEpoch());
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

QString TimeManager::getTimeZoneText(qint64 mSecsSinceEpoch) const
{
    auto dateTime = QDateTime::fromMSecsSinceEpoch(mSecsSinceEpoch, m_timeZone);
    auto zoneDiff = m_timeZone.offsetFromUtc(dateTime);

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

void TimeManager::computeTimeZone(double x, double y, qint64 t)
{
    m_timeZone = Utils::detectTimeZone(x, y);

    update_timeZoneId(m_timeZone.id());
    update_timeZoneText(getTimeZoneText(t));

    saveState();
}

void TimeManager::computeTimeError(double x, double y, double s, qint64 t)
{
    computeTimeZone(x, y, t);

    // Compute the time error.
    if (std::isnan(s))
    {
        return;
    }

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
    auto result = QDateTime::fromMSecsSinceEpoch(correctTime(QDateTime::currentMSecsSinceEpoch()), m_timeZone);

    // Hack if failing to get current date and time. This seems to happen on some devices.
    if (!result.isValid())
    {
        result = Utils::currentDateTime();
    }

    qFatalIf(!result.isValid(), "Bad time");

    return result;
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
    return Utils::encodeTimestamp(QDateTime::fromMSecsSinceEpoch(mSecsSinceEpoch, m_timeZone));
}

QString TimeManager::formatDateTime(QDateTime dateTime) const
{
    dateTime.setTimeZone(m_timeZone);
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

QString TimeManager::makeUniqueFilename(const QString& baseFilename) const
{
    auto fileInfo = QFileInfo(baseFilename);
    return Utils::makeUniqueFilename(fileInfo.baseName() + '_', currentDateTime(), '.' + fileInfo.suffix());
}

} // namespace CyberTracker
