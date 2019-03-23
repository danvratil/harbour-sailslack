#include "requestutils.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>

bool Request::isOk(const QNetworkReply *reply) {
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (statusCode / 100 == 2) {
        return true;
    }
    else {
        return false;
    }
}

bool Request::isError(const QJsonObject &data) {
    if (data.isEmpty()) {
        return true;
    }
    else {
        return !data.value("ok").toBool(false);
    }
}

QJsonObject Request::getResult(QNetworkReply *reply) {
    if (isOk(reply)) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &error);

        if (error.error == QJsonParseError::NoError) {
            return document.object();
        }
        else {
            return QJsonObject();
        }
    }
    else {
        return QJsonObject();
    }
}

