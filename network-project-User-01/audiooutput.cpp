#include "audiooutput.h"
#include <QAudioFormat>
#include <QMediaDevices>
#include <QDebug>


AudioOutput::AudioOutput(QObject *parent)
    : QIODevice(parent),
    m_audioSink(nullptr),
    m_audioOutputDevice(nullptr),
    m_opusDecoder(nullptr)
{
    initializeAudio();
    initializeOpusDecoder();


    connect(this, &AudioOutput::newPacket, this, &AudioOutput::play);
}


AudioOutput::~AudioOutput()
{
    cleanup();
}


void AudioOutput::initializeAudio()
{

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);


    QAudioDevice defaultAudioOutput = QMediaDevices::defaultAudioOutput();


    m_audioSink = new QAudioSink(defaultAudioOutput, format, this);
    if (!m_audioSink) {
        qWarning() << "Failed to initialize audio sink";
    }
}


void AudioOutput::initializeOpusDecoder()
{

    int error;
    m_opusDecoder = opus_decoder_create(48000, 1, &error);
    if (error != OPUS_OK) {
        qWarning() << "Failed to initialize Opus decoder with error code:" << error;
        m_opusDecoder = nullptr;
    }
}


void AudioOutput::cleanup()
{

    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }

    if (m_opusDecoder) {
        opus_decoder_destroy(m_opusDecoder);
        m_opusDecoder = nullptr;
    }
}


bool AudioOutput::open(QIODevice::OpenMode mode)
{
    if (!QIODevice::open(mode))
        return false;

    m_audioOutputDevice = m_audioSink->start();
    if (!m_audioOutputDevice) {
        qWarning() << "Failed to start audio output device";
        return false;
    }

    return true;
}

void AudioOutput::addData(const QByteArray &encodedData)
{
    QMutexLocker locker(&m_mutex);
    m_audioQueue.enqueue(encodedData);
    emit newPacket();
}


void AudioOutput::play()
{
    QMutexLocker locker(&m_mutex);


    if (m_audioQueue.isEmpty() || !m_opusDecoder || !m_audioOutputDevice) {
        logPlaybackIssues();
        return;
    }

    QByteArray packet = m_audioQueue.dequeue();
    processPacket(packet);
}


void AudioOutput::processPacket(const QByteArray &packet)
{
    QByteArray decodedData;
    decodeAudioData(packet, decodedData);

    if (!decodedData.isEmpty() && m_audioOutputDevice) {
        qint64 bytesWritten = m_audioOutputDevice->write(decodedData);
        if (bytesWritten != decodedData.size()) {
            qWarning() << "Failed to write all decoded data to audio output device";
        }
    } else {
        qWarning() << "Failed to write decoded data to audio output device";
    }
}


void AudioOutput::decodeAudioData(const QByteArray &packet, QByteArray &decodedData)
{
    if (!m_opusDecoder) {
        qWarning() << "Opus decoder is not initialized";
        return;
    }

    int maxFrameSize = 480;
    decodedData.resize(maxFrameSize * 2);

    int frameSize = opus_decode(m_opusDecoder,
                                reinterpret_cast<const unsigned char*>(packet.data()),
                                static_cast<opus_int32>(packet.size()),
                                reinterpret_cast<opus_int16*>(decodedData.data()),
                                maxFrameSize, 0);

    if (frameSize < 0) {
        qWarning() << "Opus decoding error:" << opus_strerror(frameSize);
        decodedData.clear();
        return;
    }

    decodedData.resize(frameSize * 2);
}


void AudioOutput::logPlaybackIssues() const
{
    if (m_audioQueue.isEmpty()) {
        qDebug() << "Audio queue is empty";
    }
    if (!m_opusDecoder) {
        qWarning() << "Opus decoder is not initialized";
    }
    if (!m_audioOutputDevice) {
        qWarning() << "Audio output device is not available";
    }
}


qint64 AudioOutput::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data)
    Q_UNUSED(maxlen)
    return -1;
}


qint64 AudioOutput::writeData(const char *data, qint64 len)
{
    addData(QByteArray(data, static_cast<int>(len)));
    return len;
}
