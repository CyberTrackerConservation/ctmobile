#pragma once
#include "pch.h"
#include "UtilsAudio.h"

namespace CyberTracker {

class AudioSource
{
public:
    virtual ~AudioSource() = default;
    virtual bool start(QString& errorMessage) = 0;
    virtual void stop() = 0;
    virtual QByteArray read(int intervalMs) = 0;
    virtual bool atEnd() const { return false; }
    int sampleRate() const { return m_sampleRate; }
    int channelCount() const { return m_channelCount; }

protected:
    int m_sampleRate = 0;
    int m_channelCount = 1;
};

class VoiceRecorder : public QObject
{
    Q_OBJECT

    QML_WRITABLE_AUTO_PROPERTY (QString, workPath)
    QML_WRITABLE_AUTO_PROPERTY (int, silenceThresholdMs)
    QML_WRITABLE_AUTO_PROPERTY (int, minDurationMs)
    QML_WRITABLE_AUTO_PROPERTY (int, maxDurationMs)
    QML_WRITABLE_AUTO_PROPERTY (int, speed)
    QML_WRITABLE_AUTO_PROPERTY (QString, sourceFilePath)
    QML_READONLY_AUTO_PROPERTY (bool, listening)
    QML_READONLY_AUTO_PROPERTY (bool, recording)
    QML_READONLY_AUTO_PROPERTY (int, duration)
    QML_READONLY_AUTO_PROPERTY (QString, durationText)
    QML_READONLY_AUTO_PROPERTY (QString, stateText)

public:
    VoiceRecorder(QObject* parent = nullptr);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void voiceStarted(const QString& filePath);
    void voiceCompleted(const QString& filePath, int duration, bool deleted);
    void startFailed(const QString& reason);

private slots:
    void timeout();

private:
    Utils::AudioResampler m_resampler;
    Utils::AudioVoiceDetector m_voiceDetector;

    std::unique_ptr<AudioSource> m_source;
    std::unique_ptr<QFile> m_audioFile;

    QTimer m_timer;
    QString m_currentFilePath;

    int m_silenceMs = 0;
    qint64 m_lastVoiceFilePos = 0;

    void internalStart();
    void startRecording();
    void finishRecording();
};

} // namespace CyberTracker
