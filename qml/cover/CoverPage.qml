import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0 as Slack

CoverBackground {
    property alias model: repeater.model

    Column {
        anchors {
            fill: parent
            topMargin: Theme.paddingLarge
            bottomMargin: Theme.paddingLarge
            leftMargin: Theme.horizontalPageMargin
            rightMargin: Theme.horizontalPageMargin
        }
        spacing: Theme.paddingSmall

        Repeater {
            id: repeater

            Column {
                width: parent.width
                Label {
                    color: Theme.highlightColor
                    width: parent.width
                    text: model.client.config.teamName
                    truncationMode: TruncationMode.Fade
                }

                Row {
                    spacing: Theme.paddingMedium
                    Label {
                        id: messageCount
                        color: model.client.unreadCount === 0 ? Theme.secondaryColor : Theme.primaryColor
                        font.pixelSize: Theme.fontSizeHuge
                        text: model.client.unreadCount
                    }

                    Label {
                        text: qsTr("Unread\nmessages", "0", model.client.unreadCount)
                        font.pixelSize: Theme.fontSizeTiny
                        color: model.client.unreadCount === 0 ? Theme.secondaryColor : Theme.primaryColor
                        anchors.verticalCenter: messageCount.verticalCenter
                    }
                }
            }
        }
    }
}
