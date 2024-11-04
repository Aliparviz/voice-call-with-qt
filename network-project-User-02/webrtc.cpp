#include "webrtc.h"
#include <QtEndian>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtWebSockets/QWebSocket>
#include <QTimer>


static_assert(true);

#pragma pack(push, 1)
struct RtpHeader {
    uint8_t version:2;
    uint8_t padding:1;
    uint8_t extension:1;
    uint8_t csrcCount:4;
    uint8_t marker:1;
    uint8_t payloadType:7;
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint32_t ssrc;
};
#pragma pack(pop)


WebRTC::WebRTC(QObject *parent)
    : QObject{parent},
    m_timestamp(0),
    m_ssrc(0),
    m_isOfferer(false),
    audioInput(nullptr),
    audioOutput(nullptr)
{

    connect(this, &WebRTC::gatheringCompleted, [this](const QString &peerID) {
        m_localDescription = descriptionToJson(m_peerConnections[peerID]->localDescription().value());
        emit localDescriptionGenerated(peerID, m_localDescription);

        if (m_isOfferer)
            emit this->offerIsReady(peerID, m_localDescription);
        else
            emit this->answerIsReady(peerID, m_localDescription);
    });


    audioInput = new AudioInput(this);
    audioOutput = new AudioOutput(this);


    connect(audioInput, &AudioInput::encodedAudioReady, this, [this](const QByteArray& encodedData){

        for(auto it = m_peerConnections.begin(); it != m_peerConnections.end(); ++it){
            sendTrack(it.key(), encodedData);
        }
    });


    connect(this, &WebRTC::incommingPacket, this, &WebRTC::handleIncommingAudioData);

}

WebRTC::~WebRTC()
{

    if(audioInput){
        audioInput->stopAudioCapture();
    }
}

/**
 * ====================================================
 * ================= public methods ===================
 * ====================================================
 */

void WebRTC::init(bool isOfferer, const QString &localId)
{
    m_localId = localId;
    m_isOfferer = isOfferer;

    rtc::Configuration config;
    config.iceServers.push_back(rtc::IceServer("stun:stun.l.google.com:19302"));
    m_config = config;

    m_sequenceNumber = 0;
    m_timestamp = 0;

    setBitRate(48000);
    setPayloadType(111);
    setSsrc(2);


    m_signalingClient = new SignalingClient("ws://localhost:3000", m_localId, this);

    connect(this, &WebRTC::offerIsReady, m_signalingClient, &SignalingClient::sendSdp);
    connect(this, &WebRTC::answerIsReady, m_signalingClient, &SignalingClient::sendSdp);
    connect(this, &WebRTC::localCandidateGenerated, m_signalingClient, &SignalingClient::sendIceCandidate);
    connect(m_signalingClient, &SignalingClient::sdpReceived, this, &WebRTC::setRemoteDescription);
    connect(m_signalingClient, &SignalingClient::iceCandidateReceived, this, &WebRTC::setRemoteCandidate);

}


void WebRTC::startCall(const QString &peerId)
{
    m_isOfferer = true;
    addPeer(peerId);
    generateOfferSDP(peerId);


    if(audioInput){
        if(!audioInput->startAudioCapture()){
            qWarning() << "Failed to start audio capture.";
        }
    }
}


void WebRTC::addPeer(const QString &peerId)
{
    qDebug() << "addPeer called for peerId:" << peerId;

    try {

        auto newPeer = std::make_shared<rtc::PeerConnection>(m_config);
        m_peerConnections[peerId] = newPeer;
        qDebug() << "PeerConnection created for peerId:" << peerId;


        auto dataChannel = newPeer->createDataChannel("data_channel");
        if (dataChannel) {
            qDebug() << "DataChannel created for peerId:" << peerId;


            dataChannel->onOpen([peerId]() {
                qDebug() << "DataChannel opened for peerId:" << peerId;
            });


            dataChannel->onMessage([peerId](const rtc::message_variant &message) {
                if (std::holds_alternative<rtc::string>(message))
                    qDebug() << "Message received on DataChannel for peerId:" << peerId << ": " << std::get<rtc::string>(message).c_str();
                else
                    qDebug() << "Binary message received on DataChannel for peerId:" << peerId;
            });
        } else {
            qWarning() << "Failed to create DataChannel for peerId:" << peerId;
        }



        addAudioTrack(peerId, "audio_track");


        newPeer->onLocalDescription([this, peerId](const rtc::Description &description) {

            m_localDescription = descriptionToJson(description);
            qDebug() << "SDP generated for peer:" << peerId << "\n"
                     << QJsonDocument(m_localDescription).toJson(QJsonDocument::Compact);


            if (description.type() == rtc::Description::Type::Offer) {
                emit offerIsReady(peerId, m_localDescription);
            } else if (description.type() == rtc::Description::Type::Answer) {
                emit answerIsReady(peerId, m_localDescription);
            } else {
                qWarning() << "Unknown description type";
            }
        });


        newPeer->onLocalCandidate([this, peerId](const rtc::Candidate &candidate) {
            QString candidateStr = QString::fromStdString(candidate.candidate());
            QString sdpMid = QString::fromStdString(candidate.mid());
            emit localCandidateGenerated(peerId, candidateStr, sdpMid);
        });




        newPeer->onStateChange([this, peerId](rtc::PeerConnection::State state) {
            if (state == rtc::PeerConnection::State::Connected) {
                qDebug() << "Peer connected for peerId:" << peerId;
                Q_EMIT connected(peerId);
            } else if (state == rtc::PeerConnection::State::Disconnected) {
                qDebug() << "Peer disconnected for peerId:" << peerId;
                Q_EMIT disconnected(peerId);
            }
        });


        newPeer->onGatheringStateChange([this, peerId](rtc::PeerConnection::GatheringState state) {
            if (state == rtc::PeerConnection::GatheringState::Complete) {
                qDebug() << "Gathering completed for peerId:" << peerId;
                emit gatheringCompleted(peerId);
            }
        });



        newPeer->onTrack([this, peerId](std::shared_ptr<rtc::Track> track) {
            qDebug() << "Track received for peerId:" << peerId;
            track->onMessage([this, peerId](rtc::message_variant data) {
                QByteArray audioData = readVariant(data);
                qDebug() << "Incoming audio packet for peerId:" << peerId << ", size:" << audioData.size();
                Q_EMIT incommingPacket(audioData);
            });
        });

    } catch (const std::exception &e) {
        qWarning() << "Exception in addPeer:" << e.what();
    }

    qDebug() << "addPeer finished for peerId:" << peerId;
}





void WebRTC::generateOfferSDP(const QString &peerId)
{
    if (m_peerConnections.contains(peerId)) {
        auto description = m_peerConnections[peerId]->localDescription();
        if (description.has_value()) {

            QJsonObject sdpObj = descriptionToJson(description.value());


            emit offerIsReady(peerId, sdpObj);
        } else {
            qWarning() << "No local description available for peerId:" << peerId;
        }
    } else {
        qWarning() << "No peer connection found for peerId:" << peerId;
    }
}





void WebRTC::generateAnswerSDP(const QString &peerId)
{
    if (m_peerConnections.contains(peerId)) {
        m_peerConnections[peerId]->setLocalDescription(rtc::Description::Type::Answer);
    }
}


void WebRTC::addAudioTrack(const QString &peerId, const QString &trackName)
{
    qDebug() << "addAudioTrack called for peerId:" << peerId << ", trackName:" << trackName;

    if (m_peerConnections.contains(peerId)) {
        auto peerConnection = m_peerConnections[peerId];

        try {

            std::string mline = "audio 9 UDP/TLS/RTP/SAVPF 111";
            std::string mid = "0";
            rtc::Description::Media audio(mline, mid, rtc::Description::Direction::SendRecv);


            audio.addAttribute("rtpmap:111 opus/48000/2");
            audio.addAttribute("fmtp:111 minptime=10;useinbandfec=1");
            audio.addAttribute("rtcp-mux");
            audio.addAttribute("rtcp-rsize");


            std::string msid = "msid:stream_id " + trackName.toStdString();
            audio.addAttribute(msid);


            auto track = peerConnection->addTrack(audio);

            if (track) {
                qDebug() << "Audio track successfully added for peerId:" << peerId;


                track->onOpen([this, peerId]() {
                    qDebug() << "Audio track is open for peerId:" << peerId;
                    isTrackOpen = true;


                    if (iceGatheringComplete) {
                        qDebug() << "Starting to send audio data to peerId:" << peerId;

                    } else {
                        qDebug() << "ICE gathering not complete yet for peerId:" << peerId;
                    }
                });


                peerConnection->onGatheringStateChange([this, peerId](rtc::PeerConnection::GatheringState newState) {
                    qDebug() << "Gathering state changed for peerId:" << peerId << ", newState:" << static_cast<int>(newState);
                    if (newState == rtc::PeerConnection::GatheringState::Complete) {
                        qDebug() << "ICE gathering is now complete for peerId:" << peerId;
                        iceGatheringComplete = true;


                        if (isTrackOpen) {
                            qDebug() << "Both track open and ICE complete for peerId:" << peerId << ". Ready to send audio.";

                        } else {
                            qDebug() << "Track is not open yet for peerId:" << peerId;
                        }
                    }
                });


                track->onMessage([this, peerId](rtc::message_variant data) {
                    QByteArray audioData = readVariant(data);
                    qDebug() << "Incoming audio packet for peerId:" << peerId << ", size:" << audioData.size();
                    Q_EMIT incommingPacket(audioData);
                });


                m_peerTracks[peerId] = track;
            } else {
                qWarning() << "Failed to add audio track for peerId:" << peerId;
            }
        } catch (const std::exception &e) {
            qWarning() << "Exception while adding audio track:" << e.what();
        }
    } else {
        qWarning() << "PeerConnection not found for peerId:" << peerId;
    }

    qDebug() << "addAudioTrack finished for peerId:" << peerId;
}










void WebRTC::sendTrack(const QString &peerId, const QByteArray &buffer)
{

    RtpHeader rtpHeader;
    rtpHeader.version = 2;
    rtpHeader.padding = 0;
    rtpHeader.extension = 0;
    rtpHeader.csrcCount = 0;
    rtpHeader.marker = 0;
    rtpHeader.payloadType = payloadType();
    rtpHeader.sequenceNumber = qToBigEndian(static_cast<uint16_t>(m_sequenceNumber++));
    rtpHeader.timestamp = qToBigEndian(m_timestamp += 160);
    rtpHeader.ssrc = qToBigEndian(ssrc());


    QByteArray headerData(reinterpret_cast<char*>(&rtpHeader), sizeof(RtpHeader));


    QByteArray packet = headerData + buffer;

    try {
        if (m_peerTracks.contains(peerId)) {
            auto &track = m_peerTracks[peerId];
            track->send(reinterpret_cast<const std::byte*>(packet.data()), packet.size());
        } else {
            qWarning() << "Audio track not found for peer:" << peerId;
        }
    } catch (const std::exception &e) {
        qWarning() << "Failed to send RTP packet over audio track:" << e.what();
    }
}



/**
 * ====================================================
 * ================= public slots =====================
 * ====================================================
 */

void WebRTC::setRemoteDescription(const QString &peerID, const QJsonObject &sdpObj)
{

    if (!m_peerConnections.contains(peerID)) {
        addPeer(peerID);
    }

    if (m_peerConnections.contains(peerID)) {
        QString typeStr = sdpObj["type"].toString();
        QString sdpStr = sdpObj["sdp"].toString();

        rtc::Description::Type descType;
        if (typeStr == "offer") {
            descType = rtc::Description::Type::Offer;
            setIsOfferer(false);
        } else if (typeStr == "answer") {
            descType = rtc::Description::Type::Answer;
        } else {
            qWarning() << "Unknown SDP type:" << typeStr;
            return;
        }

        rtc::Description description(sdpStr.toStdString(), descType);
        m_peerConnections[peerID]->setRemoteDescription(description);

        if (!isOfferer() && descType == rtc::Description::Type::Offer) {

            generateAnswerSDP(peerID);
        }
    } else {
        qWarning() << "Failed to create peer connection for peerId:" << peerID;
    }
}




void WebRTC::setRemoteCandidate(const QString &peerID, const QString &candidate, const QString &sdpMid)
{
    if (m_peerConnections.contains(peerID)) {
        rtc::Candidate rtcCandidate(candidate.toStdString());
        m_peerConnections[peerID]->addRemoteCandidate(rtcCandidate);
        qDebug() << "Remote ICE candidate added for peerId:" << peerID;
    } else {
        qWarning() << "No peer connection found for peerId:" << peerID;
    }
}



/*
 * ====================================================
 * ================= private methods ==================
 * ====================================================
 */

QByteArray WebRTC::readVariant(const rtc::message_variant &data)
{
    if (auto binaryData = std::get_if<rtc::binary>(&data)) {
        return QByteArray(reinterpret_cast<const char*>(binaryData->data()), binaryData->size());
    }
    return QByteArray();
}


QJsonObject WebRTC::descriptionToJson(const rtc::Description &description)
{
    QJsonObject jsonObject;
    jsonObject.insert("type", QString::fromStdString(description.typeString()));
    jsonObject.insert("sdp", QString::fromStdString(description.generateSdp()));
    return jsonObject;
}




int WebRTC::bitRate() const
{
    return m_bitRate;
}

void WebRTC::setBitRate(int newBitRate)
{
    m_bitRate = newBitRate;
    Q_EMIT bitRateChanged(newBitRate);
}

void WebRTC::resetBitRate()
{
    setBitRate(48000);
}

int WebRTC::payloadType() const
{
    return m_payloadType;
}

void WebRTC::setPayloadType(int newPayloadType)
{
    m_payloadType = newPayloadType;
    Q_EMIT payloadTypeChanged(newPayloadType);
}

void WebRTC::resetPayloadType()
{
    setPayloadType(111);
}

rtc::SSRC WebRTC::ssrc() const
{
    return m_ssrc;
}

void WebRTC::setSsrc(rtc::SSRC newSsrc)
{
    m_ssrc = newSsrc;
    Q_EMIT ssrcChanged(newSsrc);
}

void WebRTC::resetSsrc()
{
    setSsrc(2);
}

/**
 * ====================================================
 * ================= getters setters ==================
 * ====================================================
 */

bool WebRTC::isOfferer() const
{
    return m_isOfferer;
}

void WebRTC::setIsOfferer(bool isOfferer)
{
    m_isOfferer = isOfferer;
}



void WebRTC::handleIncommingAudioData(const QByteArray &data) {
    audioOutput->addData(data);
}
