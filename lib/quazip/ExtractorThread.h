#ifndef EXTRACTORTHREAD_H
#define EXTRACTORTHREAD_H

#include "jlcompress.h"
#include <QObject>
#include <QThread>

class ExtractorWorker;
class ExtractorThread : public QThread {
    Q_OBJECT
public:
    explicit ExtractorThread(const QString &source, const QString &destination,
                             QObject *parent = nullptr);

signals:

public slots:
private:
    ExtractorWorker *m_extractorWorker;
};

class ExtractorWorker : public QObject {
    Q_OBJECT
public:
    explicit ExtractorWorker(const QString &source, const QString &destination);

public slots:
    void start();

private:
    QString m_source;
    QString m_destination;
};

#endif // EXTRACTORTHREAD_H
