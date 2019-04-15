import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

DockedPanel {
    id: connectionPanel

    property Client slackClient

    width: parent.width
    height: content.height + Theme.paddingLarge

    dock: Dock.Bottom

    visible: slackClient.connectionStatus !== Client.Connected || !slackClient.networkAccessible

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
                visible: slackClient.connectionStatus === Client.Connecting
                text: qsTr("Reconnecting")
            }
        }

        Label {
            id: disconnectedMessage
            visible: slackClient.connectionStatus === Client.Disconnected || !slackClient.networkAccessible
            anchors.horizontalCenter: parent.horizontalCenter
            text: slackClient.networkAccessible ? qsTr("Disconnected") : qsTr("No network connection")
        }

        Button {
            id: reconnectButton
            visible: slackClient.connectionStatus === Client.Disconnected && slackClient.networkAccessible
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Reconnect")
            onClicked: {
                slackClient.reconnect()
            }
        }
    }
}
