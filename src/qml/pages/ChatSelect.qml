import QtQuick 2.2
import Sailfish.Silica 1.0
import harbour.sailslack 1.0
import "Channel.js" as Channel

Page {
    id: page

    property Client slackClient

    SilicaFlickable {
        anchors.fill: parent

        PageHeader {
            id: header
            title: qsTr("Open chat")
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No available chats")
        }

        SilicaListView {
            id: listView
            spacing: Theme.paddingMedium

            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            VerticalScrollDecorator {}

            // Prevent losing focus from search field
            currentIndex: -1

            header: SearchField {
                id: searchField
                width: parent.width
                focus: true
                placeholderText: qsTr("Search")

                onTextChanged: {
                    channelListModel.update()
                }
            }

            model: ListModel {
                id: channelListModel

                property var available: slackClient.getChannels().filter(Channel.isJoinableChat)

                Component.onCompleted: update()

                function matchesSearch(channel) {
                    return listView.headerItem.text === "" || channel.name.toLowerCase().indexOf(listView.headerItem.text.toLowerCase()) >= 0
                }

                function update() {
                    var channels = available.filter(matchesSearch)
                    channels.sort(Channel.compareByName)

                    clear()
                    channels.forEach(function(c) {
                        append(c)
                    })
                }
            }

            delegate: BackgroundItem {
                id: delegate
                height: row.height + Theme.paddingLarge
                property color textColor: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

                Row {
                    id: row
                    width: parent.width - Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 4 : 2)
                    anchors.verticalCenter: parent.verticalCenter
                    x: Theme.paddingLarge * (Screen.sizeCategory >= Screen.Large ? 2 : 1)
                    spacing: Theme.paddingMedium

                    Image {
                        id: icon
                        source: "image://theme/" + Channel.getIcon(model) + "?" + textColor
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Label {
                        width: parent.width - icon.width - Theme.paddingMedium
                        wrapMode: Text.Wrap
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: Theme.fontSizeMedium
                        text: model.name
                        color: textColor
                    }
                }

                onClicked: {
                    slackClient.openChat(model.id)
                    pageStack.replace(Qt.resolvedUrl("Channel.qml"), {"slackClient": slackClient, "channelId": model.id})
                }
            }
        }
    }

    ConnectionPanel {
        slackClient: parent.slackClient
    }
}
