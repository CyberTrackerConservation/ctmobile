#include "WaveFileRecorder.h"

struct Chunk
{
    char id[4];
    quint32 size;
};

struct RIFFHeader
{
    Chunk descriptor; // "RIFF"
    char type[4];     // "WAVE"
};

struct WAVEHeader
{
    Chunk descriptor;
    quint16 audioFormat;
    quint16 numChannels;
    quint32 sampleRate;
    quint32 byteRate;
    quint16 blockAlign;
    quint16 bitsPerSample;
};

struct DATAHeader
{
    Chunk descriptor;
};

struct CombinedHeader
{
    RIFFHeader riff;
    WAVEHeader wave;
    DATAHeader data;
};

static const int HeaderLength = sizeof(CombinedHeader);

WaveFileRecorder::WaveFileRecorder(QObject *parent): QObject(parent)
{
    m_recording = false;
    m_duration = 0;

#if defined(Q_OS_ANDROID)
    m_useAudioRecorder = false;
#endif
}

WaveFileRecorder::~WaveFileRecorder()
{
    stop();
}

bool WaveFileRecorder::start(const QString& filePath, int frequency)
{
    stop();

    if (m_useAudioRecorder)
    {
        QAudioEncoderSettings audioSettings;
        audioSettings.setCodec("audio/pcm");
        audioSettings.setQuality(QMultimedia::HighQuality);
        m_audioRecorder.setEncodingSettings(audioSettings);
        m_audioRecorder.setOutputLocation(QUrl::fromLocalFile(filePath));
        m_audioRecorder.record();

        connect(&m_audioRecorder, &QAudioRecorder::durationChanged, [&](qint64 duration)
        {
            update_duration(duration);
        });
    }
    else
    {
        auto format = QAudioFormat();
        format.setSampleRate(frequency);
        format.setChannelCount(1);
        format.setSampleSize(16);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::UnSignedInt);

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultInputDevice());
        format = info.nearestFormat(format);
        m_sampleRate = format.sampleRate();

        m_file.setFileName(filePath);
        if (!m_file.open(QIODevice::WriteOnly))
        {
            qDebug() << "Failed to open audio file" << m_file.errorString();
            return false;
        }

        if (!writeHeader(m_file, format))
        {
            qDebug() << "Failed to write audio header";
            return false;
        }

        m_input = new QAudioInput(info, format);
        m_input->start(&m_file);

        m_durationTimer.start(250);
        m_durationTimer.connect(&m_durationTimer, &QTimer::timeout, [&]()
        {
            update_duration(m_input->elapsedUSecs() / 1000);
        });
    }

    update_recording(true);

    return true;
}

int WaveFileRecorder::stop()
{
    auto duration = 0;
    m_durationTimer.stop();

    if (m_useAudioRecorder)
    {
        duration = m_audioRecorder.duration();
        m_audioRecorder.stop();
    }
    else
    {
        if (m_input)
        {
            m_input->stop();
            delete m_input;
            m_input = nullptr;
        }

        if (m_file.isOpen())
        {
            auto dataLength = m_file.size() - HeaderLength;
            writeDataLength(m_file, dataLength);
            m_file.close();

            duration = 1000 * ((dataLength + m_sampleRate / 2) / m_sampleRate) / 2;
        }
    }

    update_duration(duration);
    update_recording(false);

    return duration;
}

bool WaveFileRecorder::writeHeader(QFile& file, const QAudioFormat& format)
{
    auto header = CombinedHeader {};

    // RIFF header.
    memcpy(header.riff.descriptor.id, "RIFF", 4);
    header.riff.descriptor.size = 0; // this will be updated with correct duration:
                                     // m_dataLength + HeaderLength - 8
    // WAVE header.
    memcpy(header.riff.type, "WAVE", 4);
    memcpy(header.wave.descriptor.id, "fmt ", 4);
    header.wave.descriptor.size = quint32(16);
    header.wave.audioFormat = quint16(1);
    header.wave.numChannels = quint16(format.channelCount());
    header.wave.sampleRate = quint32(format.sampleRate());
    header.wave.byteRate = quint32(format.sampleRate() * format.channelCount() * format.sampleSize() / 8);
    header.wave.blockAlign = quint16(format.channelCount() * format.sampleSize() / 8);
    header.wave.bitsPerSample = quint16(format.sampleSize());

    // DATA header.
    memcpy(header.data.descriptor.id,"data", 4);

    // Will be updated with correct data length: m_dataLength after recording.
    header.data.descriptor.size = 0;

    return (file.write(reinterpret_cast<const char *>(&header), HeaderLength) == HeaderLength);
}

bool WaveFileRecorder::writeDataLength(QFile& file, qint64 dataLength)
{
    // Seek to RIFF header size, see header.riff.descriptor.size above.
    if (!m_file.seek(4))
    {
        return false;
    }

    auto length = dataLength + HeaderLength - 8;
    if (file.write(reinterpret_cast<const char *>(&length), 4) != 4)
    {
        return false;
    }

    // Seek to DATA header size, see header.data.descriptor.size above.
    if (!file.seek(40))
    {
        return false;
    }

    return m_file.write(reinterpret_cast<const char *>(&dataLength), 4) == 4;
}
