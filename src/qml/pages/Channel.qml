import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

Page {
    id: page

    property Client slackClient

    property variant channel
    property bool initialized: false

    SilicaFlickable {
        anchors.fill: parent

        BusyIndicator {
            id: loader
            visible: true
            running: visible
            size: BusyIndicatorSize.Large
            anchors.centerIn: parent
        }

        MessageListView {
            id: listView
            channel: page.channel
            anchors.fill: parent
            slackClient: page.slackClient
            model: ChannelMessagesModel {
                channelId: channel.id
                sourceModel: slackClient.model
            }

            onLoadCompleted: {
                loader.visible = false
            }

            onLoadStarted: {
                loader.visible = true
            }

            PushUpMenu {
                id: bottomMenu

                MenuItem {
                    text: qsTr("Upload image")
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("FileSend.qml"), { "slackClient": page.slackClient, "channelId": page.channelId})
                    }
                }
            }
        }
    }

    ConnectionPanel {
        slackClient: parent.slackClient
    }

    Component.onCompleted: {
        slackClient.openChannel(page.channel.id)
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            slackClient.setActiveWindow(page.channelId)

            if (!initialized) {
                initialized = true
            }
        }
        else if (status === PageStatus.Deactivating) {
            slackClient.setActiveWindow("")
        }
    }
}
