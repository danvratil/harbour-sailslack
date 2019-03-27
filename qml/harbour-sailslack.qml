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

    function activateChannel(teamId, channelId) {
        console.log("Navigate to", teamId, channelId)

        var client = teamsModel.clientForTeam(teamId)
        if (!client) {
            console.warn("Failed to find client for team", teamId)
            return
        }

        // FIXME: Don't pop the teamlist
        pageStack.clear()
        pageStack.push(Qt.resolvedUrl("pages/TeamList.qml"), {"model": teamsModel}, PageStack.Immediate)
        pageStack.push(Qt.resolvedUrl("pages/ChannelList.qml"), {"slackClient": client}, PageStack.Immediate)
        activate()
        pageStack.push(Qt.resolvedUrl("pages/Channel.qml"), {"slackClient": client, "channelId": channelId})
    }
}
