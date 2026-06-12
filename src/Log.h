#pragma once
#include "pch.h"

namespace CyberTracker {

class Log: public QObject
{
    Q_OBJECT

public:
    Log(const QString& rootPath, QObject* parent = nullptr);
    ~Log();

    static Log* instance();
    static void init(const QString& rootPath);

    QString filePath() const;

    void log(QtMsgType type, const QMessageLogContext& context, const QString& msg);

signals:
    void consoleMessage(const QString& message);

private:
    QString m_filePath;
    QtMessageHandler m_lastMessageHandler = nullptr;
};

} // namespace CyberTracker