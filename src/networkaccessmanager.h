#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <QPointer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include "slackclientconfig.h"

class NetworkAccessManager : public QNetworkAccessManager
{
public:
    explicit NetworkAccessManager(QObject *parent = 0);

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = 0);

private:
    QString getToken(QUrl url);
};

#endif // NETWORKACCESSMANAGER_H
