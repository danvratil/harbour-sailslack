import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

SilicaListView {
    property alias atBottom: listView.atYEnd
    property variant channel
    property variant threadId

    property Client slackClient

    property bool appActive: Qt.application.state === Qt.ApplicationActive
    property bool inputEnabled: false
    property bool hasMoreMessages: false
    property bool loading: false
    property bool canLoadMore: hasMoreMessages && !loading
    // TODO update this based on scroll
    property string furthestRead: ""
    property string currentLastRead: channel.lastRead

    // C++-ify this
    function timestampToIndex(timestamp) {
        // TODO: better than linear search for message to be updated
        for (var i = 0; i < model.count(); i++) {
            if (messageListModel.get(i).timestamp === timestamp) {
                return i
            }
        }
        return -1
    }

    function setLastRead(timestamp) {
        console.log("Setting lastRead to", timestamp)

        slackClient.markChannel(page.channel.id, timestamp)

        currentLastRead = channel.lastRead = timestamp
        if (model.count()) {
            furthestRead = model.get(model.count() - 1).timestamp
        }
        readTimer.restart()
    }

    signal loadCompleted()
    signal loadStarted()

    id: listView
    anchors.fill: parent
    spacing: Theme.paddingLarge
    currentIndex: -1

    BusyIndicator {
        visible: loading && hasMoreMessages
        running: visible
        size: BusyIndicatorSize.Medium
        anchors.topMargin: Theme.paddingLarge
        anchors.top: listView.top
        anchors.horizontalCenter: listView.horizontalCenter
    }

    PullDownMenu {
        enabled: channel.type === "im"
        MenuItem {
            text: qsTr("User Details")
            onClicked: {
                pageStack.push(Qt.resolvedUrl("UserView.qml"), {"slackClient": slackClient, "userId": channel.userId})
            }
        }
    }

    VerticalScrollDecorator {}

    Timer {
        id: readTimer
        interval: 5000
        triggeredOnStart: false
        running: false
        repeat: false
        onTriggered: {
            markLatest()
        }
    }

    header: PageHeader {
        title: threadId ? qsTr("Thread in #%1").arg(channel.name) : channel.name
    }

    delegate: MessageListItem {
        slackClient: listView.slackClient
        isUnread: message.timestamp > currentLastRead
        onClicked: {
            if (message.reply_count > 0 && !message.thread) {
                showThread(message.thread_ts)
            }
        }
        onOpenThread: {
            if (slackClient.createThread(channel.id, threadId)) {
                showThread(threadId)
            }
        }
        onMarkUnread: {
            if (index > 0) {
                var firstUnreadMessage = listView.model.get(index - 1);
                setLastRead(firstUnreadMessage.timestamp)
            } else {
                setLastRead("0000000000.000000")
            }
            readTimer.stop()
        }
    }

    section {
        property: "timegroup"
        criteria: ViewSection.FullString
        delegate: SectionHeader {
            text: section
        }
    }

    footer: MessageInput {
        visible: inputEnabled
        placeholder: (threadId ? qsTr("Message thread in %1%2") : qsTr("Message %1%2")).arg("#").arg(channel.name)
        onSendMessage: {
            slackClient.postMessage(channel.id, threadId || "", content)
        }
    }

    onAppActiveChanged: {
        if (appActive && atBottom && model.count()) {
            furthestRead = model.get(model.count() - 1).timestamp
            readTimer.restart()
        }
    }

    onContentYChanged: {
        var y = (contentY - originY) * (height / contentHeight)

        if (canLoadMore && y < Screen.height / 3) {
            loadHistory()
        }
    }

    onMovementEnded: {
        if (atBottom && model.count()) {
            furthestRead = model.get(model.count() - 1).timestamp
            readTimer.restart()
        }
    }

    Component.onCompleted: {
        if (model) {
            model.onRowsInserted.connect(handleRowsInserted)
        }
        if (slackClient) {
            slackClient.onLoadMessagesSuccess.connect(handleLoadSuccess)
            slackClient.onLoadHistorySuccess.connect(handleHistorySuccess)
        }
    }

    Component.onDestruction: {
        model.onRowsInserted.disconnect(handleRowsInserted)
        slackClient.onLoadMessagesSuccess.disconnect(handleLoadSuccess)
        slackClient.onLoadHistorySuccess.disconnect(handleHistorySuccess)
    }

    function showThread(threadId) {
        pageStack.push(Qt.resolvedUrl("Thread.qml"), {"slackClient": slackClient, "channel": channel, "threadId": threadId});
    }

    function markLatest() {
        if (furthestRead != "") {
            setLastRead(furthestRead)
            furthestRead = ""
        }
    }


    function loadHistory() {
        if (model.count()) {
            loading = true
            slackClient.loadHistory(channel.id, model.get(0).timestamp)
        }
    }

    function handleLoadSuccess(channelId, threadId, messages, hasMore) {
        var isForThisThread = threadId && threadId === listView.threadId;
        var isForThisChannel = !threadId && !listView.threadId && channelId === channel.id;
        if (isForThisChannel || isForThisThread) {
            loading = false
            currentLastRead = channel.lastRead
            hasMoreMessages = hasMore
            inputEnabled = true
            loadCompleted()

            var message = messages[messages.length - 1]
            maybeScrollToBottom(message.timestamp)
        }
    }

    function handleHistorySuccess(channelId, messages, hasMore) {
        loading = false
        hasMoreMessages = hasMore
    }

    function maybeScrollToBottom(timestamp) {
        var isAtBottom = atBottom
        if (isAtBottom) {
            listView.positionViewAtEnd()

            if (appActive) {
                furthestRead = timestamp
                readTimer.restart()
            }
        }
    }

    function handleRowsInserted(index, first, last) {
        maybeScrollToBottom(model.get(model.count() - 1).timestamp)
    }
}
