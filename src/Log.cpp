#include "Log.h"
#include "App.h"

namespace CyberTracker {

namespace {
static Log* s_instance;

void logHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (s_instance)
    {
        s_instance->log(type, context, msg);
    }
}

}

Log::Log(const QString& rootPath, QObject* parent)
{
    qFatalIf(s_instance != nullptr, "Already initialized");

    s_instance = this;

    m_filePath = rootPath + "/Log.txt";

    qSetMessagePattern("[%{time yyyyMMdd h:mm:ss.zzz} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{message}");
    m_lastMessageHandler = qInstallMessageHandler(logHandler);
}

Log::~Log()
{
    qInstallMessageHandler(m_lastMessageHandler);
    s_instance = nullptr;
}

Log* Log::instance()
{
    return s_instance;
}

void Log::init(const QString& rootPath)
{
    new Log(rootPath);
}

QString Log::filePath() const
{
    return m_filePath;
}

void Log::log(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (m_lastMessageHandler)
    {
        m_lastMessageHandler(type, context, msg);
    }

    emit consoleMessage(msg);

    auto timestamp = Utils::encodeTimestamp(Utils::currentDateTime());

    auto file = QFile(m_filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    auto stream = QTextStream(&file);

    switch (type)
    {
    case QtInfoMsg:
        stream << QString("[%1 I] %2").arg(timestamp, QString(msg)) << Qt::endl;
        break;

    case QtDebugMsg:
        stream << QString("[%1 D] %2").arg(timestamp, QString(msg)) << Qt::endl;
        break;

    case QtWarningMsg:
        stream << QString("[%1 W] %2").arg(timestamp, QString(msg)) << Qt::endl;
        break;

    case QtCriticalMsg:
        stream << QString("[%1 C] %2").arg(timestamp, QString(msg)) << Qt::endl;
        break;

    case QtFatalMsg:
        stream << QString("[%1 I] %2 %3 %4 %5").arg(timestamp, QString(msg), QString(context.file), QString::number(context.line), QString(context.function)) << Qt::endl;
        stream.flush();
        file.flush();

        if (App::instance() && App::instance()->telemetry())
        {
            App::instance()->telemetry()->aiSendEvent("exception", QVariantMap {{ "file", context.file }, { "line", context.line }, { "function", context.function }, { "message", msg }}, QVariantMap(), true);
        }

        abort();
    }

    stream.flush();
    file.flush();
}

} // namespace CyberTracker