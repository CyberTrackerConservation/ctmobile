#pragma once
#include "pch.h"
#include "UtilsAudio.h"

namespace CyberTracker {

class WaveFileRecorder : public QObject
{
    Q_OBJECT

    QML_READONLY_AUTO_PROPERTY (bool, recording)
    QML_READONLY_AUTO_PROPERTY (int, duration)

public:
    WaveFileRecorder(QObject* parent = nullptr);

    Q_INVOKABLE void start(const QString& filePath);
    Q_INVOKABLE int stop();

signals:
    void started();
    void stopped();
    void startFailed(const QString& reason);

private slots:
    void timeout();

private:
    QString m_filePath;
    Utils::AudioBluetoothHelper m_bluetoothHelper;
    Utils::AudioResampler m_resampler;

    std::unique_ptr<QFile> m_audioFile;
    std::unique_ptr<QAudioSource> m_audioSource;
    QIODevice* m_audioDevice = nullptr;

    QTimer m_timer;
    int m_sampleRate = 0;

    void internalStart();
};

} // namespace CyberTracker
