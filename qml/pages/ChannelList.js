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
    if (channel.is_starred > 0) {
        return "starred"
    }
    else {
        return channel.category
    }
}

function compareChannels(a, b) {
    var result = compareByBool(a, b, "is_starred", true);
    if (result) return result;

    result = compareByCategory(a, b);
    if (result) return result;

    result = compareByBool(a, b, "is_private", false);
    if (result) return result;

    result = compareByBool(a, b, "is_mpim", true);
    if (result) return result;

    return Channel.compareByName(a, b)
}

var categories = ["starred", "channel", "chat"];
function compareByCategory(a, b) {
    if (a.category !== b.category) {
        var diff = categories.indexOf(a.category) - categories.indexOf(b.category);
        return diff / Math.abs(diff);
    }
    return 0;
}

function compareByBool(a, b, prop, desired) {
    if (a[prop] !== b[prop]) {
        if (Boolean(b[prop]) === desired) {
            return 1;
        } else if (Boolean(a[prop]) === desired){
            return -1;
        }
    }
    return 0;
}
