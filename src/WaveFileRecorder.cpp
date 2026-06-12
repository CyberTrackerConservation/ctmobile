#include "WaveFileRecorder.h"
#include "UtilsAudio.h"
#include "App.h"
#include <QMediaDevices>

namespace CyberTracker {

WaveFileRecorder::WaveFileRecorder(QObject *parent): QObject(parent)
{
    m_recording = false;
    m_duration = 0;

    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, &WaveFileRecorder::timeout);

    connect(this, &WaveFileRecorder::startFailed, App::instance(), &App::showError);
}

void WaveFileRecorder::timeout()
{
    auto available = m_audioSource->bytesAvailable();
    if (available <= sizeof(short))
    {
        return;
    }

    auto data = m_audioDevice->read(available);

    if (m_sampleRate == AUDIO_SAMPLE_RATE)
    {
        m_audioFile->write(data);
    }
    else
    {
        m_audioFile->write(m_resampler.resample(data, m_sampleRate));
    }

    update_duration(Utils::waveFileComputeDuration(m_audioFile->size()));
}

void WaveFileRecorder::start(const QString& filePath)
{
    m_filePath = filePath;

    QMicrophonePermission microphonePermission;

    switch (qApp->checkPermission(microphonePermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, &WaveFileRecorder::internalStart);
        break;

    case Qt::PermissionStatus::Denied:
        emit startFailed(tr("Microphone permission not granted"));
        break;

    case Qt::PermissionStatus::Granted:
        internalStart();
        break;
    }
}

void WaveFileRecorder::internalStart()
{
    // Check Bluetooth.
    if (App::instance()->settings()->bluetoothAudio())
    {
        if (!m_bluetoothHelper.start())
        {
            emit startFailed(tr("Bluetooth not connected"));
            return;
        }

        emit App::instance()->showToast(tr("Bluetooth recording started"));
    }

    // Check the output file.
    m_audioFile.reset(new QFile(m_filePath));
    if (!m_audioFile->open(QIODevice::WriteOnly))
    {
        qDebug() << "Failed to open audio file for write: " << m_audioFile->errorString();
        emit startFailed(tr("Cannot save audio file"));
        return;
    }

    Utils::waveFileWriteHeader(m_audioFile.get());

    // Initialize resampler.
    m_resampler.reset();

    // Success.
    auto audioFormat = Utils::audioMakeFormat();
    m_sampleRate = audioFormat.sampleRate();

    // Start recording.
    m_audioSource.reset(new QAudioSource(QMediaDevices::defaultAudioInput(), audioFormat));
    m_audioDevice = m_audioSource->start();
    update_recording(true);
    update_duration(0);

    m_timer.start();

    emit started();
}

int WaveFileRecorder::stop()
{
    if (!m_recording)
    {
        return 0;
    }

    m_timer.stop();
    timeout();

    m_resampler.reset();
    m_bluetoothHelper.stop();

    m_audioSource->stop();
    m_audioDevice = nullptr;

    auto duration = Utils::waveFileComputeDuration(m_audioFile->size());

    m_audioFile->close();
    m_audioFile->flush();
    m_audioFile.reset();

    Utils::waveFileRewriteHeader(m_filePath);

    update_recording(false);

    emit stopped();

    return duration;
}

} // namespace CyberTracker
