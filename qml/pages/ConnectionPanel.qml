import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

DockedPanel {
    id: connectionPanel

    property Client slackClient

    width: parent.width
    height: content.height + Theme.paddingLarge

    dock: Dock.Bottom

    Column {
        id: content
        width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
        anchors.centerIn: parent
        spacing: Theme.paddingMedium

        Row {
            id: reconnectingMessage
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.paddingMedium

            BusyIndicator {
                running: reconnectingMessage.visible
                anchors.verticalCenter: parent.verticalCenter
                size: BusyIndicatorSize.ExtraSmall
            }

            Label {
                text: qsTr("Reconnecting")
            }
        }

        Label {
            id: disconnectedMessage
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Disconnected")
        }

        Button {
            id: reconnectButton
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Reconnect")
            onClicked: {
                slackClient.reconnect()
            }
        }
    }

    Component.onCompleted: {
        slackClient.onConnected.connect(hideConnectionPanel)
        slackClient.onReconnecting.connect(showReconnectingMessage)
        slackClient.onDisconnected.connect(showDisconnectedMessage)
        slackClient.onNetworkOff.connect(showNoNetworkMessage)
        slackClient.onNetworkOn.connect(hideConnectionPanel)
    }

    function hideConnectionPanel() {
        connectionPanel.hide()
    }

    function showReconnectingMessage() {
        disconnectedMessage.visible = false
        reconnectButton.visible = false
        reconnectingMessage.visible = true
        connectionPanel.show()
    }

    function showDisconnectedMessage() {
        disconnectedMessage.text = qsTr("Disconnected")
        disconnectedMessage.visible = true
        reconnectButton.visible = true
        reconnectingMessage.visible = false
        connectionPanel.show()
    }

    function showNoNetworkMessage() {
        disconnectedMessage.text = qsTr("No network connection")
        disconnectedMessage.visible = true
        reconnectButton.visible = false
        reconnectingMessage.visible = false
        connectionPanel.show()
    }
}
