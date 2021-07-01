import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

import "Channel.js" as Channel

SilicaListView {
    id: listView

    property Client slackClient

    spacing: Theme.paddingMedium

    VerticalScrollDecorator {}

    header: PageHeader {
        title: slackClient === null ? "" : slackClient.config.teamName
    }

    model: ChannelSortModel {
        sourceModel: ChannelListModel {
            sourceModel: slackClient.model
        }
    }

    section {
        property: "section"
        criteria: ViewSection.FullString
        delegate: SectionHeader {
            text: getSectionName(section)
        }
    }

    delegate: ListItem {
        id: delegate
        contentHeight: row.height + Theme.paddingLarge
        property color infoColor: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        property color textColor: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
        property color currentColor: channel.unreadCount > 0 ? textColor : infoColor

        Row {
            id: row
            width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
            anchors.verticalCenter: parent.verticalCenter
            x: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)
            spacing: Theme.paddingMedium

            Image {
                id: icon
                source: "image://theme/" + Channel.getIcon(channel) + "?" + (delegate.highlighted ? currentColor : Channel.getIconColor(channel, currentColor))
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                width: parent.width - icon.width - Theme.paddingMedium
                wrapMode: Text.Wrap
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: Theme.fontSizeMedium
                font.bold: model.unreadCount > 0
                text: channel.name
                color: currentColor
            }
        }

        onClicked: {
            pageStack.push(Qt.resolvedUrl("Channel.qml"), {"slackClient": slackClient, "channel": channel})
        }

        menu: ContextMenu {
            hasContent: channel.category === "channel" || channel.type === "im"

            MenuItem {
                text: channel.category === "channel" ? qsTr("Leave") : qsTr("Close")
                onClicked: {
                    switch (channel.type) {
                        case "channel":
                            slackClient.leaveChannel(channel.id)
                            break

                        case "group":
                            var dialog = pageStack.push(Qt.resolvedUrl("GroupLeaveDialog.qml"), {"name": channel.name})
                            dialog.accepted.connect(function() {
                                slackClient.leaveGroup(channel.id)
                            })
                            break

                        case "im":
                            slackClient.closeChat(channel.id)
                    }
                }
            }
        }
    }

    function getSectionName(section) {
        switch (section) {
            case "unread":
                return qsTr("Unread");
            case "starred":
                return qsTr("Starred")
            case "channel":
                return qsTr("Channels")
            case "chat":
                return qsTr("Direct messages")
        }
    }
}
