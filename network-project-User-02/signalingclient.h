#ifndef SIGNALINGCLIENT_H
#define SIGNALINGCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>

class SignalingClient : public QObject
{
    Q_OBJECT
public:
    explicit SignalingClient(const QString &serverUrl, const QString &localId, QObject *parent = nullptr);

    void sendSdp(const QString &peerID, const QJsonObject &sdp);
    void sendIceCandidate(const QString &peerId, const QString &candidate, const QString &sdpMid);

signals:
    void sdpReceived(const QString &from, const QJsonObject &sdp);
    void iceCandidateReceived(const QString &from, const QString &candidate, const QString &sdpMid);

private slots:
    void onMessageReceived(const QString &message);
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

private:
    QWebSocket m_socket;
    QString m_localId;
};

#endif
