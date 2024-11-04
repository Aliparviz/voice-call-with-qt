#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QIODevice>
#include <QAudioSink>
#include <QByteArray>
#include <QMutex>
#include <QQueue>
#include <opus.h>

class AudioOutput : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = nullptr);
    ~AudioOutput();


    void addData(const QByteArray& encodedData);


    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;


    bool open(QIODevice::OpenMode mode) override;

signals:

    void newPacket();

private slots:

    void play();

private:

    void initializeAudio();


    void initializeOpusDecoder();


    void cleanup();


    void processPacket(const QByteArray &packet);

    void decodeAudioData(const QByteArray &packet, QByteArray &decodedData);

    void logPlaybackIssues() const;


    QAudioSink* m_audioSink;
    QIODevice* m_audioOutputDevice;
    OpusDecoder* m_opusDecoder;

    QAudioFormat m_audioFormat;
    QMutex m_mutex;

    QQueue<QByteArray> m_audioQueue;
};

#endif
