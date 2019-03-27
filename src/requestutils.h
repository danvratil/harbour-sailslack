#ifndef REQUESTUTILS_H
#define REQUESTUTILS_H

class QNetworkReply;
class QJsonObject;
class QString;

namespace Request {

bool isOk(const QNetworkReply *reply);
bool isError(const QJsonObject &data);

QJsonObject getResult(QNetworkReply *reply);

QString nextCursor(const QJsonObject &data);

}

#endif // REQUESTUTILS_H
