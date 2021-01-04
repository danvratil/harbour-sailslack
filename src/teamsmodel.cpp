#include "teamsmodel.h"

#include <QQmlEngine>
#include <functional>

#include <functional>

Q_DECLARE_METATYPE(SlackClient*)

TeamsModel::TeamsModel(QObject *parent)
    : QAbstractListModel(parent) {}

void TeamsModel::setSlackConfig(SlackConfig *newConfig) {
    if (config) {
        config->disconnect(this);
    }
    config = newConfig;
    if (!config) {
        return;
    }

    beginResetModel();
    teams.clear();
    for (const auto &team : config->getTeams()) {
        teams.emplace_back(team, std::unique_ptr<SlackClient>(new SlackClient(team)));
    }
    endResetModel();
}

SlackConfig *TeamsModel::getSlackConfig() const {
    return config;
}

QHash<int, QByteArray> TeamsModel::roleNames() const {
    return {{ UuidRole, "uuid" }, { ClientRole, "client" }};
}

int TeamsModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : teams.size();
}

QVariant TeamsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() && std::size_t(index.row()) >= teams.size()) {
        return {};
    }

    auto &team = teams[index.row()];
    switch (role) {
    case UuidRole:
        return team.uuid;
    case ClientRole:
        return QVariant::fromValue(team.client.get());
    }

    return {};
}

SlackClient *TeamsModel::addTeam(const QString &userId, const QString &teamId, const QString &teamName, const QString &accessToken) {
    std::unique_ptr<SlackClient> client(new SlackClient(teamId));
    QQmlEngine::setObjectOwnership(client.get(), QQmlEngine::CppOwnership);
    auto config = client->getConfig();
    config->setUserId(userId);
    config->setTeamId(teamId);
    config->setTeamName(teamName);
    config->setAccessToken(accessToken);
    beginInsertRows({}, teams.size(), teams.size());
    teams.emplace_back(teamName, std::move(client));
    endInsertRows();
    return teams.back().client.get();
}

namespace {

std::function<bool(const TeamsModel::Team &team)> findTeam(const QString &teamId) {
    return [teamId](const TeamsModel::Team &team) {
        return team.uuid == teamId;
    };
}

}

void TeamsModel::removeTeam(const QString &teamId) {
    auto team = std::find_if(teams.begin(), teams.end(), findTeam(teamId));
    if (team == teams.end()) {
        return;
    }
    const int row = std::distance(teams.begin(), team);
    beginRemoveRows({}, row, row);
    teams.erase(team);
    endRemoveRows();
}

SlackClient *TeamsModel::clientForTeam(const QString &teamId) const {
    auto team = std::find_if(teams.begin(), teams.end(), findTeam(teamId));
    if (team == teams.end()) {
        return nullptr;
    }
    return team->client.get();
}
