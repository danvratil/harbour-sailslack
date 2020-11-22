import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0

SilicaListView {
    property alias atBottom: listView.atYEnd
    property variant channel
    property variant thread

    property Client slackClient

    property bool appActive: Qt.application.state === Qt.ApplicationActive
    property bool inputEnabled: false
    property bool hasMoreMessages: false
    property bool loading: false
    property bool canLoadMore: hasMoreMessages && !loading
    property string latestRead: ""

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

    WorkerScript {
        id: loader
        source: "MessageLoader.js"

        onMessage: {
            if (messageObject.op === 'replace') {
                listView.positionViewAtEnd()
                inputEnabled = true
                loading = false
                loadCompleted()

                if (messageListModel.count) {
                    latestRead = messageListModel.get(messageListModel.count - 1).timestamp
                    readTimer.restart()
                }
            }
            else if (messageObject.op === 'prepend') {
                loading = false
            }
        }
    }

    header: PageHeader {
        title: thread ? qsTr("Thread in %1%2").arg("#").arg(channel.name) : channel.name
    }

    model: ListModel {
        id: messageListModel
    }

    delegate: MessageListItem {
        onClicked: {
            if (reply_count > 0 && !thread) {
                showThread(thread_ts)
            }
        }
        onOpenThread: {
            if (slackClient.createThread(channel.id, threadId, messageListModel.get(index))) {
                showThread(threadId)
            }
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
        placeholder: (thread ? qsTr("Message thread in %1%2") : qsTr("Message %1%2")).arg("#").arg(channel.name)
        onSendMessage: {
            var threadId = thread && thread.thread_ts;
            slackClient.postMessage(channel.id, threadId || "", content)
        }
    }

    onAppActiveChanged: {
        if (appActive && atBottom && messageListModel.count) {
            latestRead = messageListModel.get(messageListModel.count - 1).timestamp
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
        if (atBottom && messageListModel.count) {
            latestRead = messageListModel.get(messageListModel.count - 1).timestamp
            readTimer.restart()
        }
    }

    Component.onCompleted: {
        if (slackClient) {
            slackClient.onInitSuccess.connect(handleReload)
            slackClient.onLoadMessagesSuccess.connect(handleLoadSuccess)
            slackClient.onLoadHistorySuccess.connect(handleHistorySuccess)
            slackClient.onMessageReceived.connect(handleMessageReceived)
        }
    }

    Component.onDestruction: {
        slackClient.onInitSuccess.disconnect(handleReload)
        slackClient.onLoadMessagesSuccess.disconnect(handleLoadSuccess)
        slackClient.onLoadHistorySuccess.disconnect(handleHistorySuccess)
        slackClient.onMessageReceived.disconnect(handleMessageReceived)
    }

    function showThread(threadId) {
        pageStack.push(Qt.resolvedUrl("Thread.qml"), {"slackClient": slackClient, "channelId": channel.id, "threadId": threadId});
    }

    function markLatest() {
        if (latestRead != "") {
            slackClient.markChannel(channel.id, latestRead)
            latestRead = ""
        }
    }

    function handleReload() {
        inputEnabled = false
        loadStarted()
        loadMessages()
    }

    function loadMessages() {
        if (thread && thread.thread_ts) {
            if (!thread.transient) {
                loading = true
                slackClient.loadThreadMessages(thread.thread_ts, channel.id);
            } else {
                loading = false;
                var messages =[thread];
                loader.sendMessage({
                    op: 'replace',
                    model: messageListModel,
                    messages: messages
                })

            }
        } else {
            loading = true
            slackClient.loadMessages(channel.id)
        }
    }

    function loadHistory() {
        if (messageListModel.count) {
            loading = true
            slackClient.loadHistory(channel.id, messageListModel.get(0).timestamp)
        }
    }

    function handleLoadSuccess(channelId, threadId, messages, hasMore) {
        var isForThisThread = threadId && thread && threadId === thread.thread_ts;
        var isForThisChannel = !threadId && !thread && channelId === channel.id;
        if (isForThisChannel || isForThisThread) {
            hasMoreMessages = hasMore
            loader.sendMessage({
                op: 'replace',
                model: messageListModel,
                messages: messages
            })
        }
    }

    function handleHistorySuccess(channelId, messages, hasMore) {
        if (channelId === channel.id) {
            hasMoreMessages = hasMore
            loader.sendMessage({
                op: 'prepend',
                model: messageListModel,
                messages: messages
            })
        }
    }

    function handleMessageReceived(message, update) {
        if (message.type === "message" && message.channel === channel.id) {
            if ((message.thread_ts) && (message.thread_ts !== message.timestamp)) {
                // A message received a reply. Is it for this thread?
                if (message.thread_ts !== thread.thread_ts) {
                    return;
                }
            }

            if (update) {
                for (var i = 0; i < messageListModel.count; i++) {
                    if (messageListModel.get(i).timestamp === message.timestamp) {
                        messageListModel.set(i, message);
                        break;
                    }
                }
            } else {
                messageListModel.append(message)
            }

            var isAtBottom = atBottom
            if (isAtBottom) {
                listView.positionViewAtEnd()

                if (appActive) {
                    latestRead = message.timestamp
                    readTimer.restart()
                }
            }
        }
    }
}
