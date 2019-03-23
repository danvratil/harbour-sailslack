#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "slackclientconfig.h"

SlackClientConfig::SlackClientConfig(QObject *parent)
    : QObject(parent), settings(this) {
    // FIXME: Technically we could read those on demand each time, but I've been getting weird
    // crashes inside of the QML engine when reading from the properties, possibly due to the
    // returned strings being temporaries
    accessToken = settings.value("user/accessToken").toString();
    userId = settings.value("user/userId").toString();
    teamId = settings.value("user/teamId").toString();
    teamName = settings.value("user/teamName").toString();
}

QString SlackClientConfig::getAccessToken() const {
    return accessToken;
}

void SlackClientConfig::setAccessToken(QString accessToken) {
    this->accessToken = accessToken;
    settings.setValue("user/accessToken", this->accessToken);
}

void SlackClientConfig::clear() {
    settings.remove("user/accessToken");
    accessToken.clear();
    settings.remove("user/userId");
    userId.clear();
    settings.remove("user/teamId");
    teamId.clear();
    settings.remove("user/teamName");
    teamName.clear();
    Q_EMIT userIdChanged(userId);
    Q_EMIT accessTokenChanged(accessToken);
    Q_EMIT teamIdChanged(teamId);
    Q_EMIT teamNameChanged(teamName);
}

QString SlackClientConfig::getUserId() const {
    return userId;
}

void SlackClientConfig::setUserId(QString userId) {
    if (this->userId != userId) {
        this->userId = userId;
        settings.setValue("user/userId", this->userId);
        Q_EMIT userIdChanged(this->userId);
    }
}

QString SlackClientConfig::getTeamId() const {
    return teamId;
}

void SlackClientConfig::setTeamId(const QString &teamId) {
    if (this->teamId != teamId) {
        this->teamId = teamId;
        settings.setValue("user/teamId", this->teamId);
        Q_EMIT teamIdChanged(this->teamId);
    }
}

QString SlackClientConfig::getTeamName() const {
    return teamName;
}

void SlackClientConfig::setTeamName(const QString &teamName) {
    if (this->teamName != teamName) {
        this->teamName = teamName;
        settings.setValue("user/teamName", this->teamName);
        Q_EMIT teamNameChanged(this->teamName);
    }
}

void SlackClientConfig::clearWebViewCache() {
    QStringList dataPaths = QStandardPaths::standardLocations(QStandardPaths::DataLocation);

    if (dataPaths.size()) {
        QDir webData(QDir(dataPaths.at(0)).filePath(".QtWebKit"));
        if (webData.exists()) {
            webData.removeRecursively();
        }
    }
}
