.import harbour.sailslack 1.0 as Slack

function init() {
    Slack.Config.onTeamAdded.connect(handleTeamAdded)
    Slack.Config.onTeamRemoved.connect(handleTeamRemoved)
}

function handleTeamAdded(team) {
    var page = pageStack.push(Qt.resolvedUrl("LoginLink.qml"));
    page.onLoginSuccess.connect(function(userId, teamId, teamName, accessToken) {
        var client = teamsModel.addTeam(userId, teamId, teamName, accessToken)
        pageStack.replace(Qt.resolvedUrl("Loader.qml"), {"slackClient": client})
    })
}

function handleTeamRemoved(team) {
    teamsModel.removeTeam(team)
}
