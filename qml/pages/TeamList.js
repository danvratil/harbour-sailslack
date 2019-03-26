.import harbour.sailslack 1.0 as Slack

function init() {
    Slack.Config.onTeamAdded.connect(handleTeamAdded)
    Slack.Config.onTeamRemoved.connect(handleTeamRemoved)

    teamsModel.clear()
    Slack.Config.teams.forEach(function(team) {
        teamsModel.append({
            uuid: team,
            client: Slack.ClientFactory.createClient(team),
            unreadCount: 0
         })
    })
}

function handleTeamAdded(team) {
    var page = pageStack.push(Qt.resolvedUrl("LoginPage.qml"))
    page.onLoginSuccess.connect(function(userId, teamId, teamName, accessToken) {
        var client = Slack.ClientFactory.createClient(team)
        client.config.userId = userId
        client.config.teamId = teamId
        client.config.teamName = teamName
        client.config.accessToken = accessToken
        teamsModel.append({
            uuid: team,
            client: client,
            unreadCount: 0
        })
        pageStack.replace(Qt.resolvedUrl("Loader.qml"), {"slackClient": client})
    })

}

function handleTeamRemoved(team) {
    for (var i = 0; i < teamsModel.count; i++) {
        var current = teamsModel.get(i)
        if (current.uuid === team) {
            current.client.logout()
            current.client.destroy()
            teamsModel.remove(i)
            return
        }
    }
}
