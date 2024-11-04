#include "signalingclient.h"
#include <QJsonDocument>
#include <QDebug>

SignalingClient::SignalingClient(const QString &serverUrl, const QString &localId, QObject *parent)
    : QObject(parent), m_localId(localId)
{
    connect(&m_socket, &QWebSocket::connected, this, &SignalingClient::onConnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &SignalingClient::onMessageReceived);
    connect(&m_socket, &QWebSocket::disconnected, this, &SignalingClient::onDisconnected);
    connect(&m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &SignalingClient::onError);

    qDebug() << "Connecting to signaling server at:" << serverUrl;
    m_socket.open(QUrl(serverUrl));
}

void SignalingClient::sendSdp(const QString &peerID, const QJsonObject &sdp)
{
    QJsonObject message;
    message["type"] = "sdp";
    message["from"] = m_localId;
    message["to"] = peerID;
    message["sdp"] = sdp;

    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    qDebug() << "Sending SDP to peerId:" << peerID << ", message:" << jsonString;
    m_socket.sendTextMessage(jsonString);
}

void SignalingClient::sendIceCandidate(const QString &peerID, const QString &candidate, const QString &sdpMid)
{
    QJsonObject message;
    message["type"] = "candidate";
    message["from"] = m_localId;
    message["to"] = peerID;
    message["candidate"] = candidate;
    message["sdpMid"] = sdpMid;

    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    qDebug() << "Sending ICE Candidate to peerId:" << peerID << ", message:" << jsonString;
    m_socket.sendTextMessage(jsonString);
}




void SignalingClient::onMessageReceived(const QString &message)
{
    qDebug() << "Message received:" << message;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull()) {
        qWarning() << "Received invalid JSON message:" << message;
        return;
    }

    QJsonObject obj = doc.object();
    QString to = obj["to"].toString();
    QString from = obj["from"].toString();


    if (to == m_localId || to.isEmpty()) {
        QString peerId = from;
        QString type = obj["type"].toString();

        if (type == "sdp") {
            QJsonObject sdpObj = obj["sdp"].toObject();
            qDebug() << "SDP received from peerId:" << peerId;

            emit sdpReceived(peerId, sdpObj);
        } else if (type == "candidate") {
            QString candidate = obj["candidate"].toString();
            QString sdpMid = obj["sdpMid"].toString();
            qDebug() << "ICE Candidate received from peerId:" << peerId;

            emit iceCandidateReceived(peerId, candidate, sdpMid);
        } else {
            qDebug() << "Unknown message type received.";
        }
    } else {
        qDebug() << "Message not intended for this client. Ignoring.";
    }
}



void SignalingClient::onConnected()
{
    qDebug() << "Connected to signaling server.";

    QJsonObject message;
    message["type"] = "register";
    message["from"] = m_localId;
    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    qDebug() << "Sending registration message:" << jsonString;
    m_socket.sendTextMessage(jsonString);
}


void SignalingClient::onDisconnected()
{
    qDebug() << "Disconnected from signaling server.";
}

void SignalingClient::onError(QAbstractSocket::SocketError error)
{
    qWarning() << "WebSocket error occurred:" << error << "-" << m_socket.errorString();
}
