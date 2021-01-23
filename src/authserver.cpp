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
    const bool result = server.listen(QHostAddress::LocalHost, port);
    Q_EMIT listening(server.isListening());
    if (!result) {
        qDebug() << "Server error listening " << server.serverError() << ": " << server.errorString();
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
    const QByteArray resultBytes = socket->readLine();
    if (server.serverError() > 0) {
        qDebug("Server error reading: %d", server.serverError());
    }
    QString result(resultBytes);

    // Replace "GET " with http://localhost:3000 and " HTTP 1.x" with nothing
    const int verbEnd = result.indexOf(' ') + 1;
    const int httpStart = result.lastIndexOf(" HTTP/");
    const auto urlWithoutHost = result.mid(verbEnd, httpStart - verbEnd);
    const QString replyHTML = "<head><title>Sailslack connected</title><meta name='viewport' content='width=device-width, initial-scale=1'/> </head><body><header>You can now switch back to Sailslack</header></body>";

    socket->write(QStringLiteral("HTTP/1.0 200 OK\r\nContent-type:text/html\r\nContent-Length:%1\r\n\r\n%2").arg(replyHTML.length()).arg(replyHTML).toUtf8());
    socket->flush();
    socket->close();
    emit resultUrlAvailable(QStringLiteral("http://localhost:%1%2").arg(server.serverPort()).arg(urlWithoutHost));
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
