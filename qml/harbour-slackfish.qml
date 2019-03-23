import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.slackfish 1.0
import "pages"
import "dialogs"

ApplicationWindow {
    id: appWindow

    // SlackClient
    Client {
        id: slackClient
    }

    Component.onCompleted: {
        pageStack.push(Qt.resolvedUrl("pages/Loader.qml"), { "slackClient": slackClient })
    }

    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: Orientation.All
    _defaultPageOrientations: Orientation.All

    function activateChannel(channelId) {
        console.log("Navigate to", channelId);
        pageStack.clear()
        pageStack.push(Qt.resolvedUrl("pages/ChannelList.qml"), {}, true)
        activate()
        pageStack.push(Qt.resolvedUrl("pages/Channel.qml"), {"channelId": channelId})
    }
}
