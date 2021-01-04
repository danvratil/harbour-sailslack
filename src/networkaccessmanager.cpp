#include "networkaccessmanager.h"
#include <QRegularExpression>

NetworkAccessManager::NetworkAccessManager(QObject *parent): QNetworkAccessManager(parent) {
}

QNetworkReply* NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData) {
    if (request.url().host() == "files.slack.com") {
        QNetworkRequest copy(request);

        const QString token = getToken(copy.url());
        if (!token.isEmpty()) {
            copy.setRawHeader(QString("Authorization").toUtf8(), QString("Bearer " + token).toUtf8());
        }

        return QNetworkAccessManager::createRequest(op, copy, outgoingData);
    }
    else {
        return QNetworkAccessManager::createRequest(op, request, outgoingData);
    }
}

QString NetworkAccessManager::getToken(QUrl url) {
    QRegularExpression re("^.*/([A-Z0-9]+)-.*$");
    QRegularExpressionMatch match = re.match(url.path());
    if (match.hasMatch()) {
        const QString team = match.captured(1);
        SlackClientConfig config(team);
        const QString token = config.getAccessToken();
        if (!token.isEmpty()){
            return config.getAccessToken();
        }
    }
    return SlackClientConfig::lastToken();
}
