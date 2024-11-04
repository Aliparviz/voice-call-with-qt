Report

---

**Title:** Distributed Voice Call System Using Qt and WebRTC

**1. Introduction**

This project is a distributed voice call application, allowing real-time voice communication directly between users over a peer-to-peer (P2P) connection without requiring a central server. Implemented in C++ using Qt and WebRTC, the application leverages the Opus codec to ensure high-quality audio while keeping bandwidth requirements low.

**2. Tools and Libraries**

- **Qt Framework**: Provides the graphical user interface (GUI) and manages audio devices.
- **WebRTC (libdatachannel)**: Supports P2P connection setup and audio data transmission.
- **Opus Codec**: Encodes and decodes audio, reducing bandwidth usage while preserving quality.

**3. System Architecture**

The project consists of these main components:

- **Main Application (`main.cpp`)**: This file initializes the Qt application and registers the `WebRTC` class, setting up the GUI and allowing QML to control call functionalities.
    ```cpp
    int main(int argc, char *argv[]) {
        QGuiApplication app(argc, argv);
        qmlRegisterType<WebRTC>("com.example.network", 1, 0, "WebRTC");
        QQmlApplicationEngine engine;
        engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
        return app.exec();
    }
    ```
    This code initializes the application, registers the WebRTC class for QML, and loads the main QML file to display the UI.

- **Signaling Client (`signalingclient.cpp`)**: Handles WebSocket-based signaling to facilitate P2P connection setup by exchanging Session Description Protocol (SDP) and ICE candidates.
    ```cpp
    void SignalingClient::sendSdp(const QString &peerID, const QJsonObject &sdp) {
        QJsonObject message;
        message["type"] = "sdp";
        message["from"] = m_localId;
        message["to"] = peerID;
        message["sdp"] = sdp;
        m_socket.sendTextMessage(QJsonDocument(message).toJson(QJsonDocument::Compact));
    }
    ```
    This function constructs an SDP message, assigning the sender and receiver IDs and including the SDP offer or answer. It sends the message to the signaling server, allowing both peers to establish a connection.

- **WebRTC (`webrtc.cpp`)**: Manages connection setup, audio track addition, and real-time audio packet handling.
    ```cpp
    void WebRTC::startCall(const QString &peerId) {
        m_isOfferer = true;
        addPeer(peerId);
        generateOfferSDP(peerId);
        if(audioInput && !audioInput->startAudioCapture()) {
            qWarning() << "Failed to start audio capture.";
        }
    }
    ```
    This function initiates a call by setting the local peer as the offerer, creating a new peer connection, generating an SDP offer, and starting audio capture.

**4. Implementation Details**

- **Signaling Setup** (`signalingclient.cpp`):
  - The `SignalingClient` class establishes a WebSocket connection with a signaling server, enabling initial peer-to-peer setup by exchanging SDP and ICE candidates.
    ```cpp
    void SignalingClient::onConnected() {
        QJsonObject message;
        message["type"] = "register";
        message["from"] = m_localId;
        m_socket.sendTextMessage(QJsonDocument(message).toJson(QJsonDocument::Compact));
    }
    ```
    This function registers the client with the signaling server by sending a JSON message with the client’s ID, allowing it to participate in SDP and ICE exchanges.

- **Peer Management and Audio Streaming** (`webrtc.cpp`):
  - `addPeer`: Creates a new peer connection, sets up the necessary callbacks for handling SDP and ICE candidates, and adds an audio track.
    ```cpp
    void WebRTC::addPeer(const QString &peerId) {
        auto newPeer = std::make_shared<rtc::PeerConnection>(m_config);
        m_peerConnections[peerId] = newPeer;
        newPeer->onLocalDescription([this, peerId](const rtc::Description &description) {
            m_localDescription = descriptionToJson(description);
            emit offerIsReady(peerId, m_localDescription);
        });
        // Set up additional callbacks for ICE candidates, state changes, and audio track management
    }
    ```
    This code creates a new peer connection and binds callbacks to handle local descriptions (SDP offers or answers), ICE candidates, and connection state changes. These connections are necessary for initiating and maintaining P2P audio calls.

- **Audio Input and Output Management**:
  - **Audio Capture** (`audioinput.cpp`): Captures audio from the microphone, encodes it using Opus, and prepares it for transmission.
    ```cpp
    void AudioInput::processAudioInput() {
        QByteArray inputData = m_audioInputDevice->readAll();
        if (!inputData.isEmpty()) {
            encodeAudioData(inputData);
        }
    }
    ```
    This function reads audio data from the input device, checks if the data is valid, and then passes it to the Opus encoder for compression.

  - **Audio Playback** (`audiooutput.cpp`): Receives, decodes, and plays audio data through the default output device.
    ```cpp
    void AudioOutput::processPacket(const QByteArray &packet) {
        QByteArray decodedData;
        decodeAudioData(packet, decodedData);
        m_audioOutputDevice->write(decodedData);
    }
    ```
    This function decodes received audio packets and plays them through the audio output device, ensuring smooth audio playback.

- **RTP Packetization** (`webrtc.cpp`):
  - `sendTrack`: Creates and sends Real-time Transport Protocol (RTP) packets for each outgoing audio buffer.
    ```cpp
    void WebRTC::sendTrack(const QString &peerId, const QByteArray &buffer) {
        RtpHeader rtpHeader;
        rtpHeader.sequenceNumber = qToBigEndian(static_cast<uint16_t>(m_sequenceNumber++));
        rtpHeader.timestamp = qToBigEndian(m_timestamp += 160);
        QByteArray packet = QByteArray(reinterpret_cast<char*>(&rtpHeader), sizeof(RtpHeader)) + buffer;
        if (m_peerTracks.contains(peerId)) {
            m_peerTracks[peerId]->send(reinterpret_cast<const std::byte*>(packet.data()), packet.size());
        }
    }
    ```
    This function constructs RTP headers to manage packet sequencing and timing. It attaches each RTP header to an audio packet before sending, helping the receiver maintain audio stream order and playback continuity.

**5. User Interface (`main.qml`)**

The UI in QML provides an interface for entering caller and target IDs, connecting, and initiating calls. Buttons and text fields guide users through the process, from connecting to ending calls, with visual feedback based on connection status.

**6. Challenges and Solutions**

- **Signaling and P2P Setup**: SDP and ICE candidate exchange is handled by the `SignalingClient` to set up a stable P2P connection.
- **Audio Quality and Latency**: Using Opus for encoding ensures minimal bandwidth consumption and good quality. RTP packetization in `sendTrack` guarantees audio data integrity and smooth playback.

**7. Conclusion**

This project demonstrates a distributed voice call system using Qt and WebRTC, providing efficient real-time voice communication over P2P connections. The code efficiently manages signaling, audio processing, and P2P connection setup, delivering an optimized solution for voice transmission without centralized servers.

---
Fall 2024
