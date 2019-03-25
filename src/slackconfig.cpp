#include "slackconfig.h"

#include <QUuid>

SlackConfig::SlackConfig(QObject *parent)
    : QObject(parent)
    , settings(this) {
    settings.beginGroup(QStringLiteral("team"));
    teams = settings.childGroups();
    // don't end the group here, the rest of the code expects to be inside the "team" group
}

QStringList SlackConfig::getTeams() const {
    return teams;
}

void SlackConfig::addNewTeam() {
    // 36 is the number of alphanum characters in uuid plus the four hypens
    const auto newid = QUuid::createUuid().toString().mid(1, 36);
    teams.push_back(newid);
    Q_EMIT teamAdded(newid);
    Q_EMIT teamsChanged();
}

void SlackConfig::removeTeam(const QString &team) {
    auto it = std::find(teams.begin(), teams.end(), team);
    if (it != teams.end()) {
        teams.erase(it);
        Q_EMIT teamRemoved(team);
        Q_EMIT teamsChanged();

        // this removes all entries for the group and the group itself
        settings.beginGroup(team);
        settings.remove(QStringLiteral(""));
        settings.endGroup();
        settings.sync();
    }
}
