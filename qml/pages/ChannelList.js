.import "Channel.js" as Channel

function init(slackClient) {
    reloadChannels(slackClient)
    slackClient.onInitSuccess.connect(reloadChannels)
    slackClient.onChannelUpdated.connect(handleChannelUpdate)
    slackClient.onChannelJoined.connect(handleChannelJoined)
    slackClient.onChannelLeft.connect(handleChannelLeft)
}

function disconnect(slackClient) {
    slackClient.onInitSuccess.disconnect(reloadChannels)
    slackClient.onChannelUpdated.disconnect(handleChannelUpdate)
    slackClient.onChannelJoined.disconnect(handleChannelJoined)
    slackClient.onChannelLeft.disconnect(handleChannelLeft)
}

function reloadChannels(slackClient) {
    var channels = slackClient.getChannels().filter(Channel.isOpen)
    channels.sort(compareChannels)

    channelListModel.clear()
    channels.forEach(function(c) {
        c.section = getChannelSection(c)
        channelListModel.append(c)
    })
}

function handleChannelUpdate(channel) {
    // FIXME: Find some way to pass around the client explicitly
    reloadChannels(slackClient)
}

function handleChannelJoined(channel) {
    reloadChannels(slackClient)
}

function handleChannelLeft(channel) {
    for (var i = 0; i < channelListModel.count; i++) {
        var current = channelListModel.get(i)

        if (channel.id === current.id) {
            channelListModel.remove(i)
        }
    }
}

function getChannelSection(channel) {
    if (channel.unreadCount > 0) {
        return "unread"
    }
    else {
        return channel.category
    }
}

function compareChannels(a, b) {
    if (a.unreadCount === 0 && b.unreadCount === 0) {
        return compareByCategory(a, b)
    }
    else {
        if (a.unreadCount > 0 && b.unreadCount > 0) {
            return Channel.compareByName(a, b)
        }
        else if (a.unreadCount > 0) {
            return -1
        }
        else if (b.unreadCount > 0) {
            return 1
        }
        else {
            return Channel.compareByName(a, b)
        }
    }
}

function compareByCategory(a, b) {
    if (a.category === b.category) {
        return Channel.compareByName(a, b)
    }
    else {
        if (a.category === "channel") {
            return -1
        }
        else if (b.category === "channel") {
            return 1
        }
        else {
            return Channel.compareByName(a, b)
        }
    }
}
