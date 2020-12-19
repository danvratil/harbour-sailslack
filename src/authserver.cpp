#include "authserver.h"
#include <QHostAddress>
#include <QTcpSocket>

AuthServer::AuthServer(QObject *parent) : QObject(parent)
    , server(this)
{
    connect(&server, SIGNAL(newConnection()), this, SLOT(clientConnected()));
}

bool AuthServer::listen(quint16 port) {
    Q_ASSERT(!server.isListening());
    bool result = server.listen(QHostAddress::LocalHost, port);
    Q_EMIT listening(server.isListening());
    if (!result) {
        qDebug("Server error listening: %d", server.serverError());
    }
    return result;
}

bool AuthServer::isListening() {
    return server.isListening();
}

void AuthServer::clientConnected() {
    auto socket = server.nextPendingConnection();
    Q_ASSERT(socket);
    socket->waitForReadyRead();
    QByteArray resultBytes = socket->readLine();
    if (server.serverError() > 0) {
        qDebug("Server error reading: %d", server.serverError());
    }
    QString result(resultBytes);

    // Replace "GET " with http://localhost:3000 and " HTTP 1.x" with nothing
    int verbEnd = result.indexOf(' ') + 1;
    int httpStart = result.lastIndexOf(" HTTP/");
    auto urlWithoutHost = result.mid(verbEnd, httpStart - verbEnd);
    emit resultUrlAvailable(QString("http://localhost:") + QString::number(server.serverPort()) + urlWithoutHost);

    socket->write("HTTP/1.0 200 OK\nContent-type:text/html\n\n<head><title>Sailslack connected</title><meta name='viewport' content='width=device-width, initial-scale=1'/> </head><body><header>You can now switch back to Sailslack</header></body>");
}

void AuthServer::close() {
    server.close();
    Q_EMIT listening(server.isListening());
    if (server.serverError() > 0) {
        qDebug("Server error closing: %d", server.serverError());
    }
}

AuthServer::~AuthServer() {
    close();
}
