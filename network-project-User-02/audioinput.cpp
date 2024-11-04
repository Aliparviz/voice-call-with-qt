#include "audioinput.h"
#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QDebug>

AudioInput::AudioInput(QObject *parent)
    : QIODevice(parent),
    opusEncoder(nullptr),
    m_audioSource(nullptr),
    m_audioInputDevice(nullptr)
{
    initializeAudio();
    initializeOpusEncoder();
}

AudioInput::~AudioInput()
{
    cleanup();
}

void AudioInput::initializeAudio()
{

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);
    format.setSampleFormat(QAudioFormat::Int16);


    QAudioDevice defaultAudioInput = QMediaDevices::defaultAudioInput();


    m_audioSource = new QAudioSource(defaultAudioInput, format, this);
    if (!m_audioSource) {
        qWarning() << "Failed to initialize audio source";
    }
}

void AudioInput::initializeOpusEncoder()
{

    int error;
    opusEncoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_AUDIO, &error);
    if (error != OPUS_OK) {
        qWarning() << "Opus encoder initialization failed with error code:" << error;
        opusEncoder = nullptr;
    }
}

void AudioInput::cleanup()
{

    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
    }

    if (opusEncoder) {
        opus_encoder_destroy(opusEncoder);
        opusEncoder = nullptr;
    }
}

bool AudioInput::startAudioCapture()
{
    if (!m_audioSource) {
        qWarning() << "Audio source not initialized";
        return false;
    }


    m_audioInputDevice = m_audioSource->start();
    if (!m_audioInputDevice) {
        qWarning() << "Failed to start audio source";
        return false;
    }


    connect(m_audioInputDevice, &QIODevice::readyRead, this, &AudioInput::processAudioInput);


    return QIODevice::open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

void AudioInput::stopAudioCapture()
{
    if (m_audioInputDevice) {
        disconnect(m_audioInputDevice, &QIODevice::readyRead, this, &AudioInput::processAudioInput);
        m_audioSource->stop();
        m_audioInputDevice = nullptr;
    }


    QIODevice::close();
}

void AudioInput::processAudioInput()
{
    if (!m_audioInputDevice)
        return;

    QByteArray inputData = m_audioInputDevice->readAll();
    if (!inputData.isEmpty()) {
        encodeAudioData(inputData);
    }
}

void AudioInput::encodeAudioData(const QByteArray &inputData)
{
    if (!opusEncoder) {
        qWarning() << "Opus encoder is not initialized";
        return;
    }


    QByteArray encodedData(4000, 0);


    int compressedSize = opus_encode(opusEncoder,
                                     reinterpret_cast<const opus_int16*>(inputData.data()),
                                     inputData.size() / 2,  // Sample count for encoding
                                     reinterpret_cast<unsigned char*>(encodedData.data()),
                                     encodedData.size());

    if (compressedSize < 0) {
        qWarning() << "Opus encoding error:" << compressedSize;
        return;
    }

    encodedData.resize(compressedSize);
    qDebug() << "Encoded audio data size:" << compressedSize;


    emit encodedAudioReady(encodedData);
}

qint64 AudioInput::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data)
    Q_UNUSED(maxlen)
    return -1;
}

qint64 AudioInput::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
}
