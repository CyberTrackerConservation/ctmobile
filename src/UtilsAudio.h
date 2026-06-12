#pragma once
#include "pch.h"
#include "samplerate.h"
#include "fvad.h"

static const quint32 AUDIO_SAMPLE_RATE = 16000;

namespace CyberTracker {
namespace Utils {

QAudioFormat audioMakeFormat();

int waveFileComputeDuration(qint64 fileSize, int sampleRate = AUDIO_SAMPLE_RATE);
int waveFileComputeDuration(const QString& filePath);
bool waveFileReadHeader(QFile& file, int* sampleRateOut, int* channelCountOut);
void waveFileRewriteHeader(const QString& filePath);
void waveFileWriteHeader(QFile* file);

class AudioResampler
{
private:
    SRC_STATE* m_srcState = nullptr;

public:
    AudioResampler();
    ~AudioResampler();
    void reset();
    QByteArray resample(const QByteArray& data, int fromSampleRate, int fromChannelCount = 1);
};

class AudioVoiceDetector
{
private:
    Fvad* m_vad = nullptr;
    QByteArray m_buffer;

public:
    AudioVoiceDetector();
    ~AudioVoiceDetector();
    void reset();
    bool detect(const QByteArray& data);
};

class AudioBluetoothHelper
{
private:
    bool m_started = false;

public:
    AudioBluetoothHelper();
    ~AudioBluetoothHelper();

    bool start();
    void stop();
};

} // namespace Utils
} // namespace CyberTracker
