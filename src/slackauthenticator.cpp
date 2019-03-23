#include "slackauthenticator.h"
#include "requestutils.h"

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>

SlackAuthenticator::SlackAuthenticator(QObject *parent)
    : QObject(parent)
    , networkAccess(new QNetworkAccessManager(this))
{}

void SlackAuthenticator::fetchAccessToken(const QUrl &resultUrl) {
    QUrlQuery resultQuery(resultUrl);
    QString code = resultQuery.queryItemValue("code");

    if (code.isEmpty()) {
        emit accessTokenFail();
        return;
    }

    QUrlQuery query;
    query.addQueryItem("client_id", SLACK_CLIENT_ID);
    query.addQueryItem("client_secret", SLACK_CLIENT_SECRET);
    query.addQueryItem("code", code);

    QUrl requestUrl("https://slack.com/api/oauth.access");
    requestUrl.setQuery(query);

    qDebug() << "GET" << requestUrl.toString();
    QNetworkReply* reply = networkAccess->get(QNetworkRequest(requestUrl));
    connect(reply, SIGNAL(finished()), this, SLOT(handleAccessTokenReply()));
}

void SlackAuthenticator::handleAccessTokenReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QJsonObject data = Request::getResult(reply);

    if (Request::isError(data)) {
        reply->deleteLater();
        emit accessTokenFail();
        return;
    }

    QString accessToken = data.value("access_token").toString();
    QString teamId = data.value("team_id").toString();
    QString userId = data.value("user_id").toString();
    QString teamName = data.value("team_name").toString();
    qDebug() << "Access token success" << accessToken << userId << teamId << teamName;

    emit accessTokenSuccess(userId, teamId, teamName, accessToken);

    reply->deleteLater();
}
