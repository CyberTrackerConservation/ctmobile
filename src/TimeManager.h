#pragma once
#include "pch.h"

namespace CyberTracker {

class TimeManager: public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (bool, correctionEnabled)
    QML_READONLY_AUTO_PROPERTY (bool, corrected)
    QML_READONLY_AUTO_PROPERTY (QString, timeZoneId)
    QML_READONLY_AUTO_PROPERTY (QString, timeZoneText)

public:
    explicit TimeManager(QObject* parent = nullptr);
    ~TimeManager();

    void computeTimeZone(double x, double y, qint64 t);
    void computeTimeError(double x, double y, double s, qint64 t);

    Q_INVOKABLE void reset();

    Q_INVOKABLE qint64 correctTime(qint64 systemMSecsSinceEpoch) const;

    Q_INVOKABLE QDateTime currentDateTime() const;
    Q_INVOKABLE QString currentDateTimeISO() const;
    Q_INVOKABLE QString makeUniqueFilename(const QString& baseFilename) const;

    Q_INVOKABLE QString formatDateTime(qint64 mSecsSinceEpoch) const;
    Q_INVOKABLE QString formatDateTime(QDateTime dateTime) const;
    Q_INVOKABLE QString getDateText(const QString& isoDateTime, const QString& emptyValue = QString()) const;
    Q_INVOKABLE QString getTimeText(const QString& isoDateTime, const QString& emptyValue = QString()) const;

private:
    int m_correctCounter = 0;
    qint64 m_timeError = 0;
    QTimeZone m_timeZone;

    qint64 currentMSecsSinceEpoch() const;
    QString getTimeZoneText(qint64 mSecsSinceEpoch) const;
    QTimeZone findTimeZone(double x, double y) const;
    void loadState();
    void saveState();
};

} // namespace CyberTracker
