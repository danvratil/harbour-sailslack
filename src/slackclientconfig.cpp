#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QAtomicPointer>

#include "slackclientconfig.h"

static QAtomicPointer<const SlackClientConfig> s_lastToken{};

QString SlackClientConfig::lastToken() {
    auto lastToken = s_lastToken.load();
    if (lastToken) {
        return lastToken->getAccessToken();
    } else {
        return QString();
    }
}

SlackClientConfig::SlackClientConfig(const QString &team, QObject *parent)
    : QObject(parent), settings(this) {
    // FIXME: Technically we could read those on demand each time, but I've been getting weird
    // crashes inside of the QML engine when reading from the properties, possibly due to the
    // returned strings being temporaries
    settings.beginGroup(QStringLiteral("team/%1").arg(team));
    accessToken = settings.value("accessToken").toString();
    userId = settings.value("userId").toString();
    teamId = settings.value("teamId").toString();
    teamName = settings.value("teamName").toString();
}

QString SlackClientConfig::getAccessToken() const {
    if (!accessToken.isEmpty()) {
        s_lastToken.store(this);
    }
    return accessToken;
}

void SlackClientConfig::setAccessToken(QString accessToken) {
    this->accessToken = accessToken;
    settings.setValue("accessToken", this->accessToken);
}

void SlackClientConfig::clear() {
    settings.remove("accessToken");
    accessToken.clear();
    settings.remove("userId");
    userId.clear();
    settings.remove("teamId");
    teamId.clear();
    settings.remove("teamName");
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
        settings.setValue("userId", this->userId);
        Q_EMIT userIdChanged(this->userId);
    }
}

QString SlackClientConfig::getTeamId() const {
    return teamId;
}

void SlackClientConfig::setTeamId(const QString &teamId) {
    if (this->teamId != teamId) {
        this->teamId = teamId;
        settings.setValue("teamId", this->teamId);
        Q_EMIT teamIdChanged(this->teamId);
    }
}

QString SlackClientConfig::getTeamName() const {
    return teamName;
}

void SlackClientConfig::setTeamName(const QString &teamName) {
    if (this->teamName != teamName) {
        this->teamName = teamName;
        settings.setValue("teamName", this->teamName);
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
