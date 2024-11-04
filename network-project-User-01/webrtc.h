#ifndef WEBRTC_H
#define WEBRTC_H

#include <QObject>
#include <QMap>
#include <rtc/rtc.h>
#include <rtc/rtc.hpp>
#include "signalingclient.h"


#include "AudioInput.h"
#include "AudioOutput.h"

class WebRTC : public QObject
{
    Q_OBJECT

public:
    explicit WebRTC(QObject *parent = nullptr);
    virtual ~WebRTC();

    Q_INVOKABLE void init(bool isOfferer, const QString &localId);
    Q_INVOKABLE void startCall(const QString &peerId);
    Q_INVOKABLE void addPeer(const QString &peerId);
    Q_INVOKABLE void generateOfferSDP(const QString &peerId);
    Q_INVOKABLE void generateAnswerSDP(const QString &peerId);
    Q_INVOKABLE void addAudioTrack(const QString &peerId, const QString &trackName);
    Q_INVOKABLE void sendTrack(const QString &peerId, const QByteArray &buffer);


    bool isOfferer() const;
    Q_INVOKABLE void setIsOfferer(bool newIsOfferer);

    rtc::SSRC ssrc() const;
    void setSsrc(rtc::SSRC newSsrc);
    void resetSsrc();

    int payloadType() const;
    void setPayloadType(int newPayloadType);
    void resetPayloadType();

    int bitRate() const;
    void setBitRate(int newBitRate);
    void resetBitRate();


signals:
    void openedDataChannel(const QString &peerId);
    void closedDataChannel(const QString &peerId);
    void incommingPacket(const QByteArray &data);
    void localDescriptionGenerated(const QString &peerID, const QJsonObject &sdp);
    void localCandidateGenerated(const QString &peerID, const QString &candidate, const QString &sdpMid);
    void isOffererChanged();
    void gatheringCompleted(const QString &peerID);
    void offerIsReady(const QString &peerID, const QJsonObject &description);
    void answerIsReady(const QString &peerID, const QJsonObject &description);
    void ssrcChanged(rtc::SSRC newSsrc);
    void payloadTypeChanged(int newPayloadType);
    void bitRateChanged(int newBitRate);
    void connected(const QString &peerID);
    void disconnected(const QString &peerID);
    void incomingFrame(const rtc::binary &frame, const rtc::FrameInfo &info);

public slots:
    void setRemoteDescription(const QString &peerID, const QJsonObject &sdp);
    void setRemoteCandidate(const QString &peerID, const QString &candidate, const QString &sdpMid);
    void handleIncommingAudioData(const QByteArray &data);

private:
    QByteArray readVariant(const rtc::message_variant &data);
    QJsonObject descriptionToJson(const rtc::Description &description);

    inline uint32_t getCurrentTimestamp() {
        using namespace std::chrono;
        auto now = steady_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
        return static_cast<uint32_t>(ms);
    }

private:
    static inline uint16_t m_sequenceNumber = 0;
    uint32_t m_timestamp = 0;
    bool m_gatheringCompleted = false;
    int m_bitRate = 48000;
    int m_payloadType = 111;
    rtc::SSRC m_ssrc = 2;
    bool m_isOfferer = false;
    QString m_localId;
    rtc::Configuration m_config;
    QMap<QString, rtc::Description> m_peerSdps;
    QMap<QString, std::shared_ptr<rtc::PeerConnection>> m_peerConnections;
    QMap<QString, std::shared_ptr<rtc::Track>> m_peerTracks;
    QJsonObject m_localDescription;
    QString m_remoteDescription;

    SignalingClient *m_signalingClient;


    AudioInput* audioInput;
    AudioOutput* audioOutput;
    bool isTrackOpen = false;
    bool iceGatheringComplete = false;


    Q_PROPERTY(bool isOfferer READ isOfferer WRITE setIsOfferer NOTIFY isOffererChanged FINAL)
    Q_PROPERTY(rtc::SSRC ssrc READ ssrc WRITE setSsrc RESET resetSsrc NOTIFY ssrcChanged FINAL)
    Q_PROPERTY(int payloadType READ payloadType WRITE setPayloadType RESET resetPayloadType NOTIFY payloadTypeChanged FINAL)
    Q_PROPERTY(int bitRate READ bitRate WRITE setBitRate RESET resetBitRate NOTIFY bitRateChanged FINAL)
};

#endif
