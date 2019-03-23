#ifndef REQUESTUTILS_H
#define REQUESTUTILS_H

class QNetworkReply;
class QJsonObject;

namespace Request {

bool isOk(const QNetworkReply *reply);
bool isError(const QJsonObject &data);

QJsonObject getResult(QNetworkReply *reply);

}

#endif // REQUESTUTILS_H
