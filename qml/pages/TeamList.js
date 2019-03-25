.import harbour.slackfish 1.0 as Slack

function init() {
    Slack.Config.onTeamAdded.connect(handleTeamAdded)
    Slack.Config.onTeamRemoved.connect(handleTeamRemoved)

    teamsModel.clear()
    console.log(Slack.Config.teams)
    Slack.Config.teams.forEach(function(team) {
        teamsModel.append({
            uuid: team,
            client: Slack.ClientFactory.createClient(team),
            unreadCount: 0
         })
    })
}

function handleTeamAdded(team) {
    var client = Slack.ClientFactory.createClient(team)
    teamsModel.append({
        uuid: team,
        client: client,
        unreadCount: 0
    })
    pageStack.push(Qt.resolvedUrl("Loader.qml"), {"slackClient": client})
}

function handleTeamRemoved(team) {
    for (var i = 0; i < teamsModel.count; i++) {
        var current = teamsModel.get(i)
        if (current.uuid === team) {
            teamsModel.remove(i)
            return
        }
    }
}
