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
    return SlackClientConfig::lastToken();
g}
