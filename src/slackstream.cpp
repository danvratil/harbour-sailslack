#include "slackstream.h"

#include <QJsonDocument>
#include <QJsonObject>

SlackStream::SlackStream(QObject *parent) : QObject(parent), isConnected(false), helloReceived(false), lastMessageId(1) {
    checkTimer = new QTimer(this);

    connect(checkTimer, SIGNAL(timeout()), this, SLOT(checkConnection()));
}

void SlackStream::initSocket(const QUrl& url) {
    webSocket = new QWebSocket("app.slack.com", QWebSocketProtocol::VersionLatest, this);
    connect(webSocket, SIGNAL(connected()), this, SLOT(handleListerStart()));
    connect(webSocket, SIGNAL(disconnected()), this, SLOT(handleListerEnd()));
    connect(webSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(handleMessage(QString)));
    connect(webSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError(QAbstractSocket::SocketError)));
    connect(webSocket, SIGNAL(pong(quint64, QByteArray)), this, SLOT(pong(quint64, QByteArray)));

    webSocket->open(url);
}

SlackStream::~SlackStream() {
    disconnect(webSocket, SIGNAL(disconnected()), this, SLOT(handleListerEnd()));

    if (!webSocket.isNull()) {
        webSocket->close();
    }
}

void SlackStream::disconnectFromHost() {
    if (!webSocket.isNull()) {
        qDebug() << "Disconnecting socket which is valid: " << webSocket->isValid();
        webSocket->close();
        webSocket->deleteLater();
    }
}

void SlackStream::listen(QUrl url) {
    helloReceived = false;
    initSocket(url);
}

void SlackStream::send(QJsonObject message) {
    message.insert("id", QJsonValue(lastMessageId.fetchAndAddRelaxed(1)));
    QJsonDocument document(message);
    QByteArray data = document.toJson(QJsonDocument::Compact);
    qDebug() << "Send" << data;

    webSocket->sendTextMessage(QString(data));
}

void SlackStream::checkConnection() {
    if (isConnected) {
        QJsonObject values;
        values.insert("type", QJsonValue(QString("ping")));
        values.insert("lastMessageId", QJsonValue(lastMessageId));
        lastMessageId.fetchAndAddRelaxed(1);
        qDebug() << "Check connection" << lastMessageId << " state: " << webSocket->state() << "valid: " << webSocket->isValid();
        QJsonDocument document(values);
        QByteArray data = document.toJson(QJsonDocument::Compact);
        webSocket->ping(data);
    }
    else {
        qDebug() << "Socket not connected, skiping connection check";
    }
}

void SlackStream::pong(quint64 elapsedTime, const QByteArray &payload) {
    qDebug() << elapsedTime << payload;
}

void SlackStream::handleListerStart() {
    qDebug() << "Socket connected";
    isConnected = true;
    checkTimer->start(15000);
}

void SlackStream::handleListerEnd() {
    qDebug() << "Socket disconnected";
    checkTimer->stop();
    isConnected = false;
    lastMessageId = 0;
    emit disconnected();
}

namespace {

bool isHelloMessage(const QJsonObject &msg) {
    return msg.value(QStringLiteral("type")).toString() == QLatin1String("hello");
}

}

void SlackStream::handleMessage(QString message) {
    qDebug() << "Got message" << message;

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Failed to parse message" << message;
        return;
    }

    if (!helloReceived && isHelloMessage(document.object())) {
        qDebug() << "Hello received";
        helloReceived = true;
        emit connected();
        return;
    }

    emit messageReceived(document.object());
}

void SlackStream::handleError(QAbstractSocket::SocketError error) {
    qDebug() << "Socket error" << error;
}
