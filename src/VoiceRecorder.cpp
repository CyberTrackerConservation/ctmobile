#include "VoiceRecorder.h"
#include "App.h"
#include <QMediaDevices>

namespace CyberTracker {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AudioSourceFile.

class AudioSourceFile : public AudioSource
{
public:
    AudioSourceFile(const QString& filePath) : m_filePath(filePath) {}

    bool start(QString& errorMessage) override
    {
        m_file.setFileName(m_filePath);
        if (!m_file.open(QIODevice::ReadOnly))
        {
            errorMessage = QObject::tr("Failed to open file");
            return false;
        }

        if (!Utils::waveFileReadHeader(m_file, &m_sampleRate, &m_channelCount))
        {
            errorMessage = QObject::tr("Failed to read file");
            m_file.close();
            return false;
        }

        m_atEnd = false;
        return true;
    }

    void stop() override
    {
        m_file.close();
    }

    QByteArray read(int intervalMs) override
    {
        auto bytesPerInterval = m_sampleRate * intervalMs / 1000 * m_channelCount * (int)sizeof(short);
        auto data = m_file.read(bytesPerInterval);
        if (data.isEmpty())
        {
            m_atEnd = true;
        }
        return data;
    }

    bool atEnd() const override
    {
        return m_atEnd;
    }

private:
    QString m_filePath;
    QFile m_file;
    bool m_atEnd = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AudioSourceDevice: reads audio from the default microphone.

class AudioSourceDevice : public AudioSource
{
public:
    AudioSourceDevice(bool bluetoothAudio) : m_bluetoothAudio(bluetoothAudio) {}

    bool start(QString& errorMessage) override
    {
        if (m_bluetoothAudio)
        {
            if (!m_bluetoothHelper.start())
            {
                errorMessage = QObject::tr("Bluetooth not connected");
                return false;
            }

            emit App::instance()->showToast(QObject::tr("Bluetooth recording started"));
        }

        auto audioFormat = Utils::audioMakeFormat();
        m_sampleRate = audioFormat.sampleRate();
        m_channelCount = audioFormat.channelCount();

        m_audioSource.reset(new QAudioSource(QMediaDevices::defaultAudioInput(), audioFormat));
        m_audioDevice = m_audioSource->start();

        return true;
    }

    void stop() override
    {
        m_bluetoothHelper.stop();
        m_audioSource->stop();
        m_audioDevice = nullptr;
    }

    QByteArray read(int /*intervalMs*/) override
    {
        auto available = m_audioSource->bytesAvailable();
        if (available <= (int)sizeof(short))
        {
            return {};
        }

        return m_audioDevice->read(available);
    }

private:
    bool m_bluetoothAudio;
    Utils::AudioBluetoothHelper m_bluetoothHelper;
    std::unique_ptr<QAudioSource> m_audioSource;
    QIODevice* m_audioDevice = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VoiceRecorder.

VoiceRecorder::VoiceRecorder(QObject* parent): QObject(parent)
{
    m_listening = m_recording = false;
    m_duration = 0;
    m_silenceThresholdMs = 2000;
    m_minDurationMs = 1500;
    m_maxDurationMs = 60000;
    m_speed = 1;

    m_workPath = App::instance()->workPath() + "/voiceRecorder";
    qFatalIf(!Utils::ensurePath(m_workPath), "Failed to create voice recorder path");

    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, &VoiceRecorder::timeout);

    connect(this, &VoiceRecorder::startFailed, App::instance(), &App::showError);

    connect(this, &VoiceRecorder::durationChanged, this, [&]()
    {
        update_durationText(QTime::fromMSecsSinceStartOfDay(m_duration).toString("mm:ss"));
    });
}

void VoiceRecorder::start()
{
    if (!m_sourceFilePath.isEmpty())
    {
        internalStart();
        return;
    }

    QMicrophonePermission microphonePermission;

    switch (qApp->checkPermission(microphonePermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(microphonePermission, this, &VoiceRecorder::internalStart);
        break;

    case Qt::PermissionStatus::Denied:
        emit startFailed(tr("Microphone permission not granted"));
        break;

    case Qt::PermissionStatus::Granted:
        internalStart();
        break;
    }
}

void VoiceRecorder::internalStart()
{
    if (!m_sourceFilePath.isEmpty())
    {
        m_source = std::make_unique<AudioSourceFile>(m_sourceFilePath);
    }
    else
    {
        m_source = std::make_unique<AudioSourceDevice>(App::instance()->settings()->bluetoothAudio());
    }

    auto errorMessage = QString();
    if (!m_source->start(errorMessage))
    {
        m_source.reset();
        emit startFailed(errorMessage);
        return;
    }

    update_listening(true);
    update_stateText(tr("Listening"));
    m_timer.start();
}

void VoiceRecorder::stop()
{
    if (!m_listening)
    {
        return;
    }

    m_timer.stop();
    if (!m_source->atEnd())
    {
        timeout(); // drain remaining audio
    }

    // Abandon any in-progress recording.
    if (m_audioFile)
    {
        m_audioFile->close();
        m_audioFile->flush();
        m_audioFile.reset();
        QFile::remove(m_currentFilePath);
        m_silenceMs = 0;
    }

    m_resampler.reset();
    m_voiceDetector.reset();

    if (m_source)
    {
        m_source->stop();
        m_source.reset();
    }

    update_listening(false);
    update_recording(false);
    update_stateText("");
    update_duration(0);
}

void VoiceRecorder::timeout()
{
    auto rawData = m_source->read(m_timer.interval() * m_speed);
    if (rawData.isEmpty())
    {
        if (m_source->atEnd())
        {
            if (m_audioFile)
            {
                finishRecording();
            }
            stop();
        }
        return;
    }

    // Resample to AUDIO_SAMPLE_RATE (also handles stereo-to-mono downmix).
    auto data = (m_source->sampleRate() == (int)AUDIO_SAMPLE_RATE && m_source->channelCount() == 1)
        ? rawData
        : m_resampler.resample(rawData, m_source->sampleRate(), m_source->channelCount());
    if (data.isEmpty())
    {
        return;
    }

    // Run VAD on resampled data.
    auto voiceDetected = m_voiceDetector.detect(data);
    if (voiceDetected)
    {
        m_silenceMs = 0;
        if (!m_audioFile)
        {
            startRecording();
        }
    }
    else if (m_audioFile)
    {
        m_silenceMs += m_timer.interval() * m_speed;
        if (m_silenceMs >= m_silenceThresholdMs)
        {
            finishRecording();
        }
    }

    // Write to file or wait for voice detection.
    if (m_audioFile)
    {
        m_audioFile->write(data);
        if (voiceDetected)
        {
            m_lastVoiceFilePos = m_audioFile->size();
        }

        update_duration(Utils::waveFileComputeDuration(m_audioFile->size()));

        if (m_duration >= m_maxDurationMs)
        {
            finishRecording();
        }
    }
    else
    {
        update_duration(0);
    }
}

void VoiceRecorder::startRecording()
{
    auto filename = Utils::makeUniqueFilename("VOICE_", QDateTime::currentDateTime(), ".wav");
    m_currentFilePath = m_workPath + "/" + filename;

    m_audioFile.reset(new QFile(m_currentFilePath));
    if (!m_audioFile->open(QIODevice::WriteOnly))
    {
        qDebug() << "VoiceRecorder: failed to open file:" << m_audioFile->errorString();
        m_audioFile.reset();
        return;
    }

    Utils::waveFileWriteHeader(m_audioFile.get());

    m_silenceMs = 0;
    m_lastVoiceFilePos = 0;

    update_recording(true);
    update_duration(0);
    update_stateText(tr("Recording"));

    emit voiceStarted(m_currentFilePath);
}

void VoiceRecorder::finishRecording()
{
    m_audioFile->close();
    m_audioFile->flush();
    m_audioFile.reset();

    // Trim the silence tail: truncate to the last position where voice was detected.
    QFile(m_currentFilePath).resize(m_lastVoiceFilePos);

    Utils::waveFileRewriteHeader(m_currentFilePath);
    auto duration = Utils::waveFileComputeDuration(m_currentFilePath);

    m_silenceMs = 0;
    m_lastVoiceFilePos = 0;

    update_recording(false);
    update_stateText(tr("Listening"));
    update_duration(0);

    if (duration < m_minDurationMs)
    {
        QFile::remove(m_currentFilePath);
        emit voiceCompleted(m_currentFilePath, duration, true);
    }
    else
    {
        emit voiceCompleted(m_currentFilePath, duration, false);
    }
}

} // namespace CyberTracker
