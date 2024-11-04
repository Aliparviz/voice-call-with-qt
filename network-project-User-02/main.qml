import QtQuick 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Layouts 1.12
import com.example.network 1.0

Window {
    width: 280
    height: 520
    visible: true
    title: qsTr("P2P Call (User2)")

    WebRTC {
        id: webrtc
    }

    Item {
        anchors.fill: parent

        ColumnLayout {
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 20
            }

            Label {
                text: "IP: 172.16.142.176"
                Layout.fillWidth: true
                Layout.preferredHeight: 40
            }
            Label {
                text: "IceCandidate: 172.16.142.176"
                Layout.fillWidth: true
                Layout.preferredHeight: 40
            }


            TextField {
                id: callerIdField
                placeholderText: "Your CallerID"
                text: "User2"
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                enabled: !connectButton.connected
            }

            // "Connect" button
            Button {
                id: connectButton
                property bool connected: false

                text: connected ? "Connected" : "Connect"
                enabled: !connected
                Material.background: connected ? "gray" : "blue"
                Material.foreground: "white"
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                onClicked: {
                    webrtc.init(false, callerIdField.text)
                    connected = true
                }
            }


            TextField {
                id: targetIdField
                placeholderText: "Target CallerID"
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                enabled: connectButton.connected && !callButton.callInProgress
            }
        }

        Button {
            id: callButton

            property bool callInProgress: false

            height: 47
            text: callInProgress ? "End Call" : "Call"
            Material.background: callInProgress ? "red" : "green"
            Material.foreground: "white"
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                margins: 20
            }
            enabled: connectButton.connected

            onClicked: {
                    callInProgress = !callInProgress
                    if (callInProgress) {
                        webrtc.setIsOfferer(true)
                        webrtc.startCall(targetIdField.text)
                    } else {

                        targetIdField.clear()
                }
            }
        }
    }
}
