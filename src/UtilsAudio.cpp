#include "UtilsAudio.h"

namespace {
static const quint16 AUDIO_BITS_PER_SAMPLE = 16;
static const quint16 AUDIO_NUM_CHANNELS = 1;

struct Chunk
{
    char id[4] = {};
    quint32 size = 0;
};

struct RIFFHeader
{
    Chunk descriptor = {{ 'R','I','F','F' }};
    char type[4] = { 'W','A','V','E' };
};

struct WAVEHeader
{
    Chunk descriptor = { {'f','m','t',' '}, 16 };
    quint16 audioFormat = 1;
    quint16 numChannels = AUDIO_NUM_CHANNELS;
    quint32 sampleRate = AUDIO_SAMPLE_RATE;
    quint32 byteRate = AUDIO_SAMPLE_RATE * AUDIO_NUM_CHANNELS * AUDIO_BITS_PER_SAMPLE / 8;
    quint16 blockAlign = AUDIO_NUM_CHANNELS * AUDIO_BITS_PER_SAMPLE / 8;
    quint16 bitsPerSample = AUDIO_BITS_PER_SAMPLE;
};

struct DATAHeader
{
    Chunk descriptor = {{ 'd','a','t','a' }};
};

struct CombinedHeader
{
    RIFFHeader riff;
    WAVEHeader wave;
    DATAHeader data;
};

static const int HEADER_LENGTH = sizeof(CombinedHeader);
}

namespace CyberTracker {
namespace Utils {

QAudioFormat audioMakeFormat()
{
    auto result = QAudioFormat();

    // Default to AUDIO_SAMPLE_RATE, except for MacOS which does not support that.
    auto device = QMediaDevices::defaultAudioInput();
#ifdef Q_OS_MACOS
    auto sampleRate = device.preferredFormat().sampleRate();
#else
    auto sampleRate = AUDIO_SAMPLE_RATE;
#endif

    result.setSampleRate(sampleRate);
    result.setChannelCount(1);
    result.setSampleFormat(QAudioFormat::Int16);

    // Check before using it. Note, this test is not widely supported across platforms.
    if (!device.isFormatSupported(result))
    {
        qDebug() << "Error: Requested 16kHz/mono/int16 format not supported. Falling back.";
        result = device.preferredFormat();
    }

    return result;
}

int waveFileComputeDuration(qint64 fileSize, int sampleRate)
{
    auto dataLength = fileSize - HEADER_LENGTH;
    return 1000 * ((dataLength + sampleRate / 2) / sampleRate) / 2;
}

int waveFileComputeDuration(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly))
    {
        return 0;
    }

    auto header = CombinedHeader {};
    file.read((char*)&header, HEADER_LENGTH);

    return waveFileComputeDuration(file.size(), header.wave.sampleRate);
}

void waveFileRewriteHeader(const QString& filePath)
{
    auto file = QFile(filePath);
    if (!file.open(QFile::ReadWrite))
    {
        qDebug() << "Failed to load: " << filePath;
        return;
    }

    auto fileSize = file.size();
    auto header = CombinedHeader {};
    header.riff.descriptor.size = fileSize - 8;
    header.data.descriptor.size = fileSize - HEADER_LENGTH;

    if (file.write((const char *)&header, HEADER_LENGTH) != HEADER_LENGTH)
    {
        qDebug() << "Failed to write header: " << filePath;
        return;
    }

    file.close();
    file.flush();
}

void waveFileWriteHeader(QFile* file)
{
    auto header = CombinedHeader {};
    file->write((const char*)&header, HEADER_LENGTH);
}

bool waveFileReadHeader(QFile& file, int* sampleRateOut, int* channelCountOut)
{
    file.seek(0);

    // Validate RIFF/WAVE container.
    RIFFHeader riff {};
    if (file.read((char*)&riff, sizeof(riff)) != sizeof(riff)) return false;
    if (memcmp(riff.descriptor.id, "RIFF", 4) != 0) return false;
    if (memcmp(riff.type, "WAVE", 4) != 0) return false;

    auto localSampleRate = 0;
    auto localChannelCount = 0;

    // Scan chunks until the data chunk is found.
    while (!file.atEnd())
    {
        Chunk chunk {};
        if (file.read((char*)&chunk, sizeof(chunk)) != sizeof(chunk))
            break;

        if (memcmp(chunk.id, "fmt ", 4) == 0)
        {
            WAVEHeader wave {};
            auto payloadSize = (qint64)(sizeof(WAVEHeader) - sizeof(Chunk));
            auto readBytes = qMin(payloadSize, (qint64)chunk.size);
            if (file.read((char*)&wave.audioFormat, readBytes) != readBytes) return false;
            if (wave.audioFormat != 1 || wave.bitsPerSample != 16) return false;
            if (wave.numChannels < 1 || wave.numChannels > 2) return false;
            localSampleRate = (int)wave.sampleRate;
            localChannelCount = (int)wave.numChannels;

            // Skip any remaining bytes in this chunk plus alignment padding.
            auto skip = (qint64)chunk.size - readBytes + (chunk.size % 2);
            if (skip > 0)
            {
                file.seek(file.pos() + skip);
            }
        }
        else if (memcmp(chunk.id, "data", 4) == 0)
        {
            // File is now positioned at the start of audio data.
            if (localSampleRate == 0) return false;
            *sampleRateOut = localSampleRate;
            *channelCountOut = localChannelCount;
            return true;
        }
        else
        {
            // Unknown chunk — skip it (plus alignment padding).
            file.seek(file.pos() + (qint64)chunk.size + (chunk.size % 2));
        }
    }

    return false;
}

// AudioResampler.
AudioResampler::AudioResampler()
{
    auto srcError = (int)0;
    m_srcState = src_new(SRC_SINC_FASTEST, 1, &srcError);
    qFatalIf(!m_srcState, "Failed to create libsamplerate state: " + QString(src_strerror(srcError)));
}

AudioResampler::~AudioResampler()
{
    src_delete(m_srcState);
    m_srcState = nullptr;
}

void AudioResampler::reset()
{
    auto srcResetError = src_reset(m_srcState);
    qFatalIf(srcResetError != 0, "Failed to reset libsamplerate state: " + QString(src_strerror(srcResetError)));
}

QByteArray AudioResampler::resample(const QByteArray& data, int fromSampleRate, int fromChannelCount)
{
    // Downmix stereo to mono if needed.
    QByteArray monoData;
    if (fromChannelCount == 2)
    {
        auto frames = data.size() / (2 * (int)sizeof(short));
        monoData.resize(frames * (int)sizeof(short));
        auto* src = reinterpret_cast<const short*>(data.constData());
        auto* dst = reinterpret_cast<short*>(monoData.data());
        for (int i = 0; i < frames; i++)
        {
            dst[i] = (int(src[2 * i]) + int(src[2 * i + 1])) / 2;
        }
    }

    const QByteArray& inputData = (fromChannelCount == 2) ? monoData : data;

    // If already at the target sample rate, return the (possibly downmixed) data.
    if (fromSampleRate == (int)AUDIO_SAMPLE_RATE)
    {
        return inputData;
    }

    auto rawData = reinterpret_cast<const short*>(inputData.constData());
    auto samples = static_cast<int>(inputData.length() / sizeof(short));
    auto inputFloats = std::vector<float>(samples);

    src_short_to_float_array(rawData, inputFloats.data(), samples);
    auto ratio = (double)AUDIO_SAMPLE_RATE / (double)fromSampleRate;
    auto outputFramesCap = (long)std::ceil((double)samples * ratio) + 16;
    auto outputFloats = std::vector<float>(outputFramesCap);

    auto srcData = SRC_DATA();
    srcData.data_in = inputFloats.data();
    srcData.input_frames = samples;
    srcData.data_out = outputFloats.data();
    srcData.output_frames = outputFramesCap;
    srcData.src_ratio = ratio;
    srcData.end_of_input = 0;

    auto srcResult = src_process(m_srcState, &srcData);
    if (srcResult != 0)
    {
        qDebug() << "src_process error:" << src_strerror(srcResult);
        return {};
    }

    auto outFrames = srcData.output_frames_gen;
    if (outFrames == 0)
    {
        return {};
    }

    auto output = QByteArray(outFrames * (long)sizeof(short), Qt::Uninitialized);
    src_float_to_short_array(outputFloats.data(), reinterpret_cast<short*>(output.data()), outFrames);

    return output;
}

// AudioVoiceDetector.
AudioVoiceDetector::AudioVoiceDetector()
{
    m_vad = fvad_new();
    qFatalIf(!m_vad, "Failed to create libfvad");

    fvad_set_sample_rate(m_vad, AUDIO_SAMPLE_RATE);
    fvad_set_mode(m_vad, 3);
}

AudioVoiceDetector::~AudioVoiceDetector()
{
    fvad_free(m_vad);
    m_vad = nullptr;
}

void AudioVoiceDetector::reset()
{
    m_buffer.clear();
}

bool AudioVoiceDetector::detect(const QByteArray& data)
{
    static const int FRAME_SAMPLES = (AUDIO_SAMPLE_RATE * 30) / 1000;
    static const int FRAME_BYTES = FRAME_SAMPLES * (int)sizeof(int16_t);

    m_buffer.append(data);

    auto result = false;

    while (m_buffer.size() >= FRAME_BYTES)
    {
        auto frame = reinterpret_cast<const int16_t*>(m_buffer.constData());
        if (fvad_process(m_vad, frame, FRAME_SAMPLES) == 1)
        {
            result = true;
        }

        m_buffer.remove(0, FRAME_BYTES);
    }

    return result;
}

// AudioBluetoothHelper.
AudioBluetoothHelper::AudioBluetoothHelper()
{
}

AudioBluetoothHelper::~AudioBluetoothHelper()
{
    stop();
}

bool AudioBluetoothHelper::start()
{
    auto result = true;

#if defined(Q_OS_ANDROID)
    auto androidJni = QJniObject(QNativeInterface::QAndroidApplication::context());
    androidJni.callMethod<void>("startSco", "()V");

    auto timer = QElapsedTimer();
    timer.start();

    result = false;
    while (timer.elapsed() < 5000)
    {
        result = androidJni.callMethod<jboolean>("isScoConnected");
        if (result)
        {
            break;
        }

        QThread::msleep(50);
    }

#endif

    m_started = result;
    return result;
}

void AudioBluetoothHelper::stop()
{
    if (m_started)
    {
#if defined(Q_OS_ANDROID)
        QJniObject(QNativeInterface::QAndroidApplication::context()).callMethod<void>("stopSco", "()V");
#endif
    }

    m_started = false;
}

} // namespace Utils

} // namespace CyberTracker
