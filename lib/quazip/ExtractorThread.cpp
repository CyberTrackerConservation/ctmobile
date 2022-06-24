#include "ExtractorThread.h"

ExtractorThread::ExtractorThread(const QString &source,
                                 const QString &destination, QObject */*parent*/) {
    m_extractorWorker = new ExtractorWorker(source, destination);
    m_extractorWorker->moveToThread(this);
    start();
    //    m_extractorWorker->start();
}

ExtractorWorker::ExtractorWorker(const QString &source,
                                 const QString &destination)
    : m_source(source), m_destination(destination) {}

void ExtractorWorker::start() {}
