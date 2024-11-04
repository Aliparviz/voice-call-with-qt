#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include <QIODevice>
#include <QAudioSource>
#include <QByteArray>
#include <QMutex>
#include <opus.h>

class AudioInput : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioInput(QObject *parent = nullptr);
    ~AudioInput();

    bool startAudioCapture();
    void stopAudioCapture();

signals:
    void encodedAudioReady(const QByteArray& encodedData);

protected:

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private slots:
    void processAudioInput();

private:
    void initializeAudio();
    void initializeOpusEncoder();
    void cleanup();
    void encodeAudioData(const QByteArray &inputData);

    QByteArray buffer;
    OpusEncoder* opusEncoder;
    QAudioSource* m_audioSource;
    QIODevice* m_audioInputDevice;

    const int sampleRate = 48000;
    const int channels = 1;
    const int bitrate = 64000;

    QMutex mutex;
};

#endif
