import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.sailslack 1.0 as Slack
import "pages"
import "cover"
import "pages/TeamList.js" as TeamList

ApplicationWindow {
    id: appWindow

    Slack.TeamsModel {
        id: teamsModel
        slackConfig: Slack.Config
    }

    initialPage: Component {
        TeamList {
            model: teamsModel
        }
    }

    // FIXME: Make it a component
    cover: CoverPage {
        model: teamsModel
    }

    allowedOrientations: Orientation.All
    _defaultPageOrientations: Orientation.All

    Component.onCompleted: {
        TeamList.init()
    }

    function activateChannel(channelId) {
        console.log("Navigate to", channelId);
        pageStack.clear()
        pageStack.push(Qt.resolvedUrl("pages/ChannelList.qml"), {}, true)
        activate()
        pageStack.push(Qt.resolvedUrl("pages/Channel.qml"), {"channelId": channelId})
    }
}
