#ifndef AUTHSERVER_H
#define AUTHSERVER_H

#include <QObject>
#include <QTcpServer>

class AuthServer
        : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AuthServer)
    QTcpServer server;

public:
    explicit AuthServer(QObject *parent = nullptr);
    virtual ~AuthServer();

public Q_SLOTS:
    bool listen(quint16 port);
    void close();

private Q_SLOTS:
    void clientConnected();

signals:
    void resultUrlAvailable(const QString& url);

};

#endif // AUTHSERVER_H
