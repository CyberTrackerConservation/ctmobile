#pragma once
#include "pch.h"

class WaveFileRecorder : public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (bool, recording)
    QML_READONLY_AUTO_PROPERTY (int, duration)

public:
    WaveFileRecorder(QObject *parent = nullptr);
    ~WaveFileRecorder();

    Q_INVOKABLE bool start(const QString& filePath, int sampleRate = 16000);
    Q_INVOKABLE int stop();

private:
    bool writeHeader(QFile& file, const QAudioFormat& format);
    bool writeDataLength(QFile& file, qint64 dataLength);

    QAudioInput* m_input = nullptr;
    QFile m_file;
    int m_sampleRate;
    bool m_useAudioRecorder = true;
    QAudioRecorder m_audioRecorder;

    QTimer m_durationTimer;
};
