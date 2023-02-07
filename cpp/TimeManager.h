#pragma once
#include "pch.h"
#include "zonedetect.h"

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

    void computeTimeError(double x, double y, double s, qint64 t);

    Q_INVOKABLE void reset();

    Q_INVOKABLE qint64 correctTime(qint64 systemMSecsSinceEpoch) const;

    Q_INVOKABLE QDateTime currentDateTime() const;
    Q_INVOKABLE QString currentDateTimeISO() const;
    Q_INVOKABLE QString formatDateTime(qint64 mSecsSinceEpoch) const;
    Q_INVOKABLE QString formatDateTime(QDateTime dateTime) const;
    Q_INVOKABLE QString getDateText(const QString& isoDateTime, const QString& emptyValue = QString()) const;
    Q_INVOKABLE QString getTimeText(const QString& isoDateTime, const QString& emptyValue = QString()) const;

private:
    int m_correctCounter = 0;
    QFile m_zoneFile;
    uchar* m_zoneFileMap = nullptr;
    ZoneDetect* m_zoneDb = nullptr;
    qint64 m_timeError = 0;

    qint64 currentMSecsSinceEpoch() const;
    QString getTimeZoneText(const QString& timeZoneId, qint64 mSecsSinceEpoch) const;
    void loadState();
    void saveState();
};
