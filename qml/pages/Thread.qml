import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

Page {
    id: page

    property Client slackClient

    property string channelId
    property variant channel
    property string threadId
    property variant thread
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
            thread: page.thread
            channel: page.channel
            anchors.fill: parent
            slackClient: page.slackClient

            onLoadCompleted: {
                loader.visible = false
            }

            onLoadStarted: {
                loader.visible = true
            }
        }
    }

    ConnectionPanel {
        slackClient: parent.slackClient
    }

    Component.onCompleted: {
        page.channel = slackClient.getChannel(page.channelId)
        page.thread = slackClient.getThread(page.channelId, page.threadId)
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            slackClient.setActiveWindow(page.threadId)

            if (!initialized) {
                initialized = true
                listView.loadMessages()
            }
        }
        else if (status === PageStatus.Deactivating) {
            slackClient.setActiveWindow("")
            listView.markLatest()
        }
    }
}
