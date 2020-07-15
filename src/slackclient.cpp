#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QSize>
#include <QStringList>
#include <QRegularExpression>
#include <QFile>
#include <QHttpMultiPart>
#include <QtNetwork/QNetworkConfigurationManager>
#include <nemonotifications-qt5/notification.h>

#include "asyncfuture.h"
#include "requestutils.h"
#include "slackclient.h"

#include <algorithm>

SlackClient::SlackClient(QObject *parent)
    : QObject(parent)
    , messageFormatter(storage)
{}

SlackClient::SlackClient(const QString &team, QObject *parent)
    : QObject(parent)
    , team(team)
    , appActive(true)
    , activeWindow("init")
    , networkAccessManager(new QNetworkAccessManager(this))
    , config(new SlackClientConfig(team, this))
    , stream(new SlackStream(this))
    , reconnectTimer(new QTimer(this))
    , messageFormatter(storage)
    , initialized(false)
{
    qDebug() << "Creating SlackClient for" << config->getTeamName();

    networkAccessible = networkAccessManager->networkAccessible();

    connect(networkAccessManager, SIGNAL(networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)), this, SLOT(handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)));
    connect(reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnect()));

    connect(stream, SIGNAL(connected()), this, SLOT(handleStreamStart()));
    connect(stream, SIGNAL(disconnected()), this, SLOT(handleStreamEnd()));
    connect(stream, SIGNAL(messageReceived(QJsonObject)), this, SLOT(handleStreamMessage(QJsonObject)));
}

SlackClient::~SlackClient() {
    qDebug() << "Destroying SlackClient for " << config->getTeamName();
}

QString SlackClient::getTeam() const {
    return team;
}

QString SlackClient::toString(const QJsonObject &data) {
    QJsonDocument doc(data);
    return doc.toJson(QJsonDocument::Compact);
}

void SlackClient::setAppActive(bool active) {
    appActive = active;
    clearNotifications();
}

void SlackClient::setActiveWindow(QString windowId) {
    activeWindow = windowId;
    clearNotifications();
}

void SlackClient::clearNotifications() {
  foreach (QObject* object, Notification::notifications()) {
      Notification* n = qobject_cast<Notification*>(object);
      if (n->hintValue("x-sailslack-channel").toString() == activeWindow) {
          n->close();
      }

      delete n;
  }
}

void SlackClient::handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible) {
    qDebug() << "Network accessible changed" << accessible;
    networkAccessible = accessible;

    if (networkAccessible == QNetworkAccessManager::Accessible) {
        Q_EMIT networkAccessibleChanged(true);
    } else {
        Q_EMIT networkAccessibleChanged(false);
    }
}

void SlackClient::setConnectionStatus(ConnectionStatus status) {
    if (connectionStatus != status) {
        connectionStatus = status;
        Q_EMIT connectionStatusChanged(connectionStatus);
    }
}

void SlackClient::reconnect() {
    qDebug() << config->getTeamName() << ": Reconnecting";
    setConnectionStatus(Connecting);

    start();
}

void SlackClient::handleStreamStart() {
    qDebug() << config->getTeamName() << ": Stream started";
    setConnectionStatus(Connected);

    updatePresenceSubscription();
}

void SlackClient::updatePresenceSubscription() {
    QSet<QString> userIds;
    for (const auto &channelV : storage.channels()) {
        const QVariantMap channel = channelV.toMap();
        if (channel.value("type").toString() == "im" && !channel.value("is_user_deleted").toBool()) {
            userIds.insert(channel.value("userId").toString());
        } else if (channel.value("type").toString() == "mpim" || channel.value("type").toString() == "group") {
            // TODO: Does presence of mpim and group ever makes sense
            for (const auto &member : channel.value("memberIds").toList()) {
                userIds.insert(member.toString());
            }
        } else {
            // skip channels
        }

        // The upper limit is not documented, but if it's overstepped we get immediately disconnected from Slack.
        if (userIds.size() > 150) {
            break;
        }
    }

    QJsonArray userIdsJson;
    for (const auto &userId : userIds) {
        userIdsJson.push_back(QJsonValue::fromVariant(userId));
    }

    QJsonObject message;
    message.insert("type", QJsonValue(QString("presence_sub")));
    message.insert("ids", userIdsJson);
    this->stream->send(message);
}

void SlackClient::handleStreamEnd() {
    qDebug() << config->getTeamName() << ": Stream ended";

    if (!config->getAccessToken().isEmpty()) {
        qDebug() << config->getTeamName() << ": Stream reconnect scheduled";
        setConnectionStatus(Connecting);
        reconnectTimer->setSingleShot(true);
        reconnectTimer->start(1000);
    } else {
        setConnectionStatus(Disconnected);
    }
}

void SlackClient::handleStreamMessage(QJsonObject message) {
    QString type = message.value("type").toString();

    if (type == "message") {
        parseMessageUpdate(message);
    }
    else if (type == "group_marked" || type == "channel_marked" || type == "im_marked" || type == "mpim_marked") {
        parseChannelUpdate(message);
    }
    else if (type == "channel_joined") {
        parseChannelJoin(message);
    }
    else if (type == "group_joined") {
        parseGroupJoin(message);
    }
    else if (type == "im_open") {
        parseChatOpen(message);
    }
    else if (type == "im_close") {
        parseChatClose(message);
    }
    else if (type == "channel_left" || type == "group_left") {
        parseChannelLeft(message);
    }
    else if (type == "presence_change") {
        parsePresenceChange(message);
    }
    else if (type == "desktop_notification") {
        parseNotification(message);
    }
}

void SlackClient::parseChatOpen(QJsonObject message) {
    QString id = message.value("channel").toString();
    QVariantMap channel = storage.channel(id);
    channel.insert("isOpen", QVariant(true));
    storage.saveChannel(channel);
    emit channelJoined(channel);

    updatePresenceSubscription();
}

void SlackClient::parseChatClose(QJsonObject message) {
    QString id = message.value("channel").toString();
    QVariantMap channel = storage.channel(id);
    channel.insert("isOpen", QVariant(false));
    storage.saveChannel(channel);
    emit channelLeft(channel);

    updatePresenceSubscription();
}

void SlackClient::parseChannelJoin(QJsonObject message) {
    QVariantMap data = parseChannel(message.value("channel").toObject());
    storage.saveChannel(data);
    emit channelJoined(data);
}

void SlackClient::parseChannelLeft(QJsonObject message) {
    QString id = message.value("channel").toString();
    QVariantMap channel = storage.channel(id);
    channel.insert("isOpen", QVariant(false));
    storage.saveChannel(channel);
    emit channelLeft(channel);
}

void SlackClient::parseGroupJoin(QJsonObject message) {
    QVariantMap data = parseGroup(message.value("channel").toObject());
    storage.saveChannel(data);
    emit channelJoined(data);

    updatePresenceSubscription();
}

void SlackClient::parseChannelUpdate(QJsonObject message) {
    QString id = message.value("channel").toString();
    QVariantMap channel = storage.channel(id);
    channel.insert("lastRead", message.value("ts").toVariant());
    channel.insert("unreadCount", message.value("unread_count_display").toVariant());
    storage.saveChannel(channel);
    emit channelUpdated(channel);

    updateUnreadCount();
}

void SlackClient::parseMessageUpdate(QJsonObject message) {
    QVariantMap data = getMessageData(message);

    QString channelId = message.value("channel").toString();
    if (storage.channelMessagesExist(channelId)) {
        storage.appendChannelMessage(channelId, data);
    }

    QVariantMap channel = storage.channel(channelId);

    QString messageTime = data.value("timestamp").toString();
    QString latestRead = channel.value("lastRead").toString();

    if (messageTime > latestRead) {
        int channelUnreadCount = channel.value("unreadCount").toInt() + 1;
        channel.insert("unreadCount", channelUnreadCount);
        storage.saveChannel(channel);
        emit channelUpdated(channel);

        updateUnreadCount();
    }

    if (channel.value("isOpen").toBool() == false) {
        if (channel.value("type").toString() == "im") {
            openChat(channelId);
        }
    }

    emit messageReceived(data);
}

void SlackClient::parsePresenceChange(QJsonObject message) {
    QVariant presence = message.value("presence").toVariant();
    QVariantList userIds;
    if (message.contains("user")) {
        userIds << message.value("user").toVariant();
    }
    else {
        userIds = message.value("users").toArray().toVariantList();
    }

    foreach (QVariant userId, userIds) {
        QVariantMap user = storage.user(userId);
        if (!user.isEmpty()) {
            user.insert("presence", presence);
            storage.saveUser(user);
            emit userUpdated(user);
        }

        foreach (QVariant item, storage.channels()) {
            QVariantMap channel = item.toMap();

            if (channel.value("type") == QVariant("im") && channel.value("userId") == userId) {
                channel.insert("presence", presence);
                storage.saveChannel(channel);
                emit channelUpdated(channel);
            }
        }
    }
}

void SlackClient::parseNotification(QJsonObject message) {
  QString channel = message.value("subtitle").toString();
  QString content = message.value("content").toString();

  QString channelId = message.value("channel").toString();
  QString title;

  if (channelId.startsWith("C") || channelId.startsWith("G")) {
      title = QString(tr("New message in %1")).arg(channel);
  }
  else if (channelId.startsWith("D")) {
      title = QString(tr("New message from %1")).arg(channel);
  }
  else {
      title = QString(tr("New message"));
  }

  qDebug() << config->getTeamName() << ": App state" << appActive << activeWindow;

  if (!appActive || activeWindow != channelId) {
      sendNotification(channelId, title, content);
  }
}

QNetworkReply* SlackClient::executeGet(QString method, QMap<QString, QString> params) {
    QUrlQuery query;

    QString token = config->getAccessToken();
    if (!token.isEmpty()) {
        query.addQueryItem("token", token);
    }

    foreach (const QString key, params.keys()) {
        query.addQueryItem(key, params.value(key));
    }

    QUrl url("https://slack.com/api/" + method);
    url.setQuery(query);
    QNetworkRequest request(url);

    qDebug() << config->getTeamName() << ": GET" << url.toString();
    return networkAccessManager->get(request);
}

QNetworkReply* SlackClient::executePost(QString method, const QMap<QString, QString>& data) {
    QUrlQuery query;

    QString token = config->getAccessToken();
    if (!token.isEmpty()) {
        query.addQueryItem("token", token);
    }

    foreach (const QString key, data.keys()) {
        query.addQueryItem(key, data.value(key));
    }

    QUrl params;
    params.setQuery(query);
    QByteArray body = params.toEncoded(QUrl::EncodeUnicode | QUrl::EncodeReserved);
    body.remove(0,1);

    QUrl url("https://slack.com/api/" + method);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::ContentLengthHeader, body.length());

    qDebug() << config->getTeamName() << ": POST" << url.toString() << body;
    return networkAccessManager->post(request, body);
}

QNetworkReply* SlackClient::executePostWithFile(QString method, const QMap<QString, QString>& formdata, QFile* file) {
    QHttpMultiPart* dataParts = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart tokenPart;
    tokenPart.setHeader(QNetworkRequest::ContentDispositionHeader, "from-data; name=\"token\"");
    tokenPart.setBody(config->getAccessToken().toUtf8());
    dataParts->append(tokenPart);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, "from-data; name=\"file\"; filename=\"" + file->fileName() + "\"");
    filePart.setBodyDevice(file);
    dataParts->append(filePart);

    foreach (const QString key, formdata.keys()) {
        QHttpPart data;
        data.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"" + key.toUtf8() + "\"");
        data.setBody(formdata.value(key).toUtf8());
        dataParts->append(data);
    }

    QUrl url("https://slack.com/api/" + method);
    QNetworkRequest request(url);

    qDebug() << config->getTeamName() << ": POST" << url << dataParts;

    QNetworkReply* reply = networkAccessManager->post(request, dataParts);
    connect(reply, SIGNAL(finished()), dataParts, SLOT(deleteLater()));

    return reply;
}

void SlackClient::logout() {
    config->clear();
    stream->disconnectFromHost();
    storage.clear();
}

void SlackClient::testLogin() {
    if (networkAccessible != QNetworkAccessManager::Accessible) {
        qDebug() << config->getTeamName() << ": Login failed no network" << networkAccessible;
        emit testConnectionFail();
        return;
    }

    QString token = config->getAccessToken();
    if (token.isEmpty()) {
        emit testLoginFail();
        return;
    }

    QNetworkReply *reply = executeGet("auth.test");
    connect(reply, SIGNAL(finished()), this, SLOT(handleTestLoginReply()));
}

void SlackClient::handleTestLoginReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QJsonObject data = Request::getResult(reply);

    if (Request::isError(data)) {
        config->clear();
        reply->deleteLater();
        emit testLoginFail();
        return;
    }

    config->setTeamId(data.value("team_id").toString());
    config->setUserId(data.value("user_id").toString());
    config->setTeamName(data.value("team").toString());
    qDebug() << config->getTeamName() << ": Login success" << config->getUserId() << config->getTeamId() << config->getTeamName();

    emit testLoginSuccess();
    reply->deleteLater();
}

void SlackClient::init() {
    setConnectionStatus(Connecting);
    qDebug() << config->getTeamName() << ": Start init";
    loadUsers();
}

void SlackClient::loadUsers(const QString &cursor) {
  qDebug() << config->getTeamName() << ": Start load users";
  QMap<QString, QString> params;
  if (!cursor.isEmpty()) {
      params.insert("cursor", cursor);
  }
  QNetworkReply* reply = executeGet("users.list", params);
  connect(reply, &QNetworkReply::finished, [reply,this]() {
    QJsonObject data = Request::getResult(reply);
    if (Request::isError(data)) {
      qDebug() << config->getTeamName() << ": User load failed";
      emit loadUsersFail();
    } else {
      parseUsers(data);
      reply->deleteLater();
      const QString nextCursor = Request::nextCursor(data);
      if (!nextCursor.isEmpty()) {
          loadUsers(nextCursor);
      } else {
        qDebug() << config->getTeamName() << ": Load users completed";
        emit loadUsersSuccess();
        loadConversations();
      }
    }
  });
}

void SlackClient::start() {
    qDebug() << config->getTeamName() << ": Connect start";

    QMap<QString,QString> params;
    params.insert("batch_presence_aware", "1");
    QNetworkReply *reply = executeGet("rtm.connect", params);

    connect(reply, &QNetworkReply::finished, [reply,this]() {
        QJsonObject data = Request::getResult(reply);

        if (Request::isError(data)) {
            qDebug() << config->getTeamName() << ": Connect result error";
            setConnectionStatus(Disconnected);
            emit initFail();
        }
        else {
            QUrl url(data.value("url").toString());
            stream->listen(url);
            qDebug() << config->getTeamName() << ": Connect completed";

            storage.clearChannelMessages();
            initialized = true;
            Q_EMIT initializedChanged();
            emit initSuccess();
        }

        reply->deleteLater();
    });
}

QVariantMap SlackClient::parseChannel(QJsonObject channel) {
    QVariantMap data;
    data.insert("id", channel.value("id").toVariant());
    data.insert("type", QVariant("channel"));
    data.insert("category", QVariant("channel"));
    data.insert("name", channel.value("name").toVariant());
    data.insert("presence", QVariant("none"));
    data.insert("isOpen", channel.value("is_member").toVariant());
    data.insert("lastRead", channel.value("last_read").toVariant());
    data.insert("unreadCount", channel.value("unread_count_display").toVariant());
    data.insert("userId", QVariant());
    return data;
}

QVariantMap SlackClient::parseGroup(QJsonObject group) {
    QVariantMap data;

    if (group.value("is_mpim").toBool()) {
        data.insert("type", QVariant("mpim"));
        data.insert("category", QVariant("chat"));

        QStringList members;
        QVariantList memberIds;
        QJsonArray memberList = group.value("members").toArray();
        foreach (const QJsonValue &member, memberList) {
            QVariant memberId = member.toVariant();
            if (memberId != config->getUserId()) {
                members << storage.user(memberId).value("name").toString();
                memberIds << memberId.toString();
            }
        }
        data.insert("name", QVariant(members.join(", ")));
        data.insert("memberIds", memberIds);
    }
    else {
        data.insert("type", QVariant("group"));
        data.insert("category", QVariant("channel"));
        data.insert("name", group.value("name").toVariant());
    }

    data.insert("id", group.value("id").toVariant());
    data.insert("presence", QVariant("none"));
    data.insert("isOpen", group.value("is_open").toVariant());
    data.insert("lastRead", group.value("last_read").toVariant());
    data.insert("unreadCount", group.value("unread_count_display").toVariant());
    return data;
}

QVariantMap SlackClient::parseChat(QJsonObject chat) {
  QVariantMap data;

  QVariant userId = chat.value("user").toVariant();
  QVariantMap user = storage.user(userId);

  QString name;
  if (userId.toString() == config->getUserId()) {
    name = user.value("name").toString() + " (you)";
  }
  else {
    name = user.value("name").toString();
  }

  data.insert("type", QVariant("im"));
  data.insert("category", QVariant("chat"));
  data.insert("id", chat.value("id").toVariant());
  data.insert("userId", userId);
  data.insert("name", QVariant(name));
  data.insert("presence", user.value("presence"));
  data.insert("isOpen", chat.value("is_open").toVariant());
  data.insert("lastRead", chat.value("last_read").toVariant());
  data.insert("unreadCount", chat.value("unread_count_display").toVariant());

  return data;
}

void SlackClient::parseUsers(QJsonObject data) {
    foreach (const QJsonValue &value, data.value("members").toArray()) {
        QJsonObject user = value.toObject();
        QJsonObject profile = user.value("profile").toObject();
        QVariant presence;
        if (profile.value("always_active").toBool()) {
            presence = QVariant("active");
        }
        else {
            presence = QVariant("away");
        }

        QVariantMap data;
        data.insert("id", user.value("id").toVariant());
        const auto name = user.value("name").toString();
        const auto realName = profile.value("real_name").toString();
        const auto displayName = profile.value("display_name").toString();

        if (displayName == name && !realName.isEmpty()) {
            data.insert("name", realName);
        } else if (!displayName.isEmpty()) {
            data.insert("name", displayName);
        } else if (!realName.isEmpty()) {
            data.insert("name", realName);
        } else {
            data.insert("name", name);
        }
        data.insert("presence", presence);
        storage.saveUser(data);
    }
}

QVariantList SlackClient::getChannels() {
    return storage.channels();
}

QVariant SlackClient::getChannel(QString channelId) {
    return storage.channel(QVariant(channelId));
}

QString SlackClient::historyMethod(QString type) {
    if (type == "channel") {
        return "channels.history";
    }
    else if (type == "group") {
        return "groups.history";
    }
    else if (type == "mpim") {
        return "mpim.history";
    }
    else if (type == "im") {
        return "im.history";
    }
    else {
        return "";
    }
}

void SlackClient::loadConversations(QString cursor) {
  qDebug() << config->getTeamName() << ": Conversation load start" << cursor;

  QMap<QString,QString> params;
  params.insert("types", "public_channel,private_channel,mpim,im");
  params.insert("limit", "1000"); // the API limit

  if (!cursor.isEmpty()) {
      params.insert("cursor", cursor);
  }

  QNetworkReply* reply = executeGet("users.conversations", params);
  connect(reply, &QNetworkReply::finished, [reply,this]() {
    QJsonObject data = Request::getResult(reply);

    if (Request::isError(data)) {
      qDebug() << config->getTeamName() << ": Conversation load failed";
    }
    else {
      auto combinator = AsyncFuture::combine();
      QString nextCursor = Request::nextCursor(data);

      foreach (const QJsonValue &value, data.value("channels").toArray()) {
        QJsonObject channel = value.toObject();

        // Skip private group chats that are closed, we don't want those listed or
        // counted towards our unread count. If we receive a message in those
        if (channel.value("is_mpim").toBool() && !channel.value("is_open").toBool()) {
            continue;
        }

        QString infoMethod;
        if (channel.value("is_channel").toBool()) {
          infoMethod = "channels.info";
        }
        else if (channel.value("is_group").toBool()) {
          infoMethod = "groups.info";
        }
        else {
          infoMethod = "conversations.info";
        }

        QMap<QString,QString> params;
        params.insert("channel", channel.value("id").toString());
        QNetworkReply* infoReply = executeGet(infoMethod, params);

        combinator << AsyncFuture::observe(infoReply, &QNetworkReply::finished).future();
        connect(infoReply, &QNetworkReply::finished, [infoReply,infoMethod,this]() {
            QJsonObject infoData = Request::getResult(infoReply).value(infoMethod == "groups.info" ? "group" : "channel").toObject();
            QVariantMap channel;

            if (infoData.value("is_im").toBool()) {
              channel = parseChat(infoData);
            }
            else if (infoData.value("is_channel").toBool()) {
              channel = parseChannel(infoData);
            }
            else {
              channel = parseGroup(infoData);
            }

            storage.saveChannel(channel);
            infoReply->deleteLater();
        });
      }

      AsyncFuture::observe(combinator.future()).subscribe([nextCursor,this]() {
          if (nextCursor.isEmpty()) {
              updateUnreadCount();
              start();
          }
          else {
              loadConversations(nextCursor);
          }
      });
    }

    reply->deleteLater();
  });
}

void SlackClient::joinChannel(QString channelId) {
    QVariantMap channel = storage.channel(QVariant(channelId));

    QMap<QString,QString> params;
    params.insert("name", channel.value("name").toString());

    QNetworkReply* reply = executeGet("channels.join", params);
    connect(reply, &QNetworkReply::finished, [reply,this]() {
        QJsonObject data = Request::getResult(reply);

        if (Request::isError(data)) {
            qDebug() << config->getTeamName() << ": Channel join failed";
        }

        reply->deleteLater();
    });
}

void SlackClient::leaveChannel(QString channelId) {
    QMap<QString,QString> params;
    params.insert("channel", channelId);

    QNetworkReply* reply = executeGet("channels.leave", params);
    connect(reply, &QNetworkReply::finished, [reply,this]() {
        QJsonObject data = Request::getResult(reply);

        if (Request::isError(data)) {
            qDebug() << config->getTeamName() << ": Channel leave failed";
        }

        reply->deleteLater();
    });
}

void SlackClient::leaveGroup(QString groupId) {
    QMap<QString,QString> params;
    params.insert("channel", groupId);

    QNetworkReply* reply = executeGet("groups.leave", params);
    connect(reply, &QNetworkReply::finished, [reply,this]() {
        QJsonObject data = Request::getResult(reply);

        if (Request::isError(data)) {
            qDebug() << config->getTeamName() << ": Group leave failed";
        }

        reply->deleteLater();
    });
}

void SlackClient::openChat(QString chatId) {
    QVariantMap channel = storage.channel(QVariant(chatId));

    QMap<QString,QString> params;
    params.insert("user", channel.value("userId").toString());

    QNetworkReply* reply = executeGet("im.open", params);
    connect(reply, &QNetworkReply::finished, [reply,this]() {
        QJsonObject data = Request::getResult(reply);

        if (Request::isError(data)) {
            qDebug() << config->getTeamName() << ": Chat open failed";
        }

        reply->deleteLater();
    });
}

void SlackClient::closeChat(QString chatId) {
    QMap<QString,QString> params;
    params.insert("channel", chatId);

    QNetworkReply* reply = executeGet("im.close", params);
    connect(reply, &QNetworkReply::finished, [reply,this]() {
        QJsonObject data = Request::getResult(reply);

        if (Request::isError(data)) {
            qDebug() << config->getTeamName() << ": Chat close failed";
        }

        reply->deleteLater();
    });
}

void SlackClient::loadHistory(QString type, QString channelId, QString latest) {
  QMap<QString,QString> params;
  params.insert("channel", channelId);
  params.insert("count", "20");
  params.insert("latest", latest);
  params.insert("inclusive", "0");

  QNetworkReply* reply = executeGet(historyMethod(type), params);
  connect(reply, &QNetworkReply::finished, [reply,channelId,this]() {
      QJsonObject data = Request::getResult(reply);

      if (Request::isError(data)) {
          reply->deleteLater();
          emit loadHistoryFail();
          return;
      }

      QVariantList messages = parseMessages(data);
      bool hasMore = data.value("has_more").toBool();
      storage.prependChannelMessages(channelId, messages);

      emit loadHistorySuccess(channelId, messages, hasMore);
      reply->deleteLater();
  });
}

void SlackClient::loadMessages(QString type, QString channelId) {
    if (storage.channelMessagesExist(channelId)) {
        QVariantList messages = storage.channelMessages(channelId);
        emit loadMessagesSuccess(channelId, messages, true);
        return;
    }

    QMap<QString,QString> params;
    params.insert("channel", channelId);
    params.insert("count", "20");

    QNetworkReply* reply = executeGet(historyMethod(type), params);
    reply->setProperty("channelId", channelId);
    connect(reply, SIGNAL(finished()), this, SLOT(handleLoadMessagesReply()));
}

void SlackClient::handleLoadMessagesReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    QJsonObject data = Request::getResult(reply);

    if (Request::isError(data)) {
        reply->deleteLater();
        emit loadMessagesFail();
        return;
    }

    QVariantList messages = parseMessages(data);
    bool hasMore = data.value("has_more").toBool();
    QString channelId = reply->property("channelId").toString();
    storage.setChannelMessages(channelId, messages);

    emit loadMessagesSuccess(channelId, messages, hasMore);
    reply->deleteLater();
}

QVariantList SlackClient::parseMessages(const QJsonObject data) {
    QJsonArray messageList = data.value("messages").toArray();
    QVariantList messages;

    foreach (const QJsonValue &value, messageList) {
        QJsonObject message = value.toObject();
        messages << getMessageData(message);
    }
    std::sort(messages.begin(), messages.end(), [](const QVariant &a, const QVariant &b) -> bool {
        return a.toMap().value("time").toDateTime() < b.toMap().value("time").toDateTime();
    });

    return messages;
}

QString SlackClient::markMethod(QString type) {
    if (type == "channel") {
        return "channels.mark";
    }
    else if (type == "group") {
        return "groups.mark";
    }
    else if (type == "mpim") {
        return "mpim.mark";
    }
    else if (type == "im") {
        return "im.mark";
    }
    else {
        return "";
    }
}

void SlackClient::markChannel(QString type, QString channelId, QString time) {
    QMap<QString,QString> params;
    params.insert("channel", channelId);
    params.insert("ts", time);

    QNetworkReply* reply = executeGet(markMethod(type), params);
    connect(reply, &QNetworkReply::finished, [reply,this]() {
        QJsonObject data = Request::getResult(reply);

        if (Request::isError(data)) {
            qDebug() << config->getTeamName() << ": Mark conversation failed";
        }

        reply->deleteLater();
    });
}

void SlackClient::postMessage(QString channelId, QString content) {
    content.replace(QRegularExpression("&"), "&amp;");
    content.replace(QRegularExpression(">"), "&gt;");
    content.replace(QRegularExpression("<"), "&lt;");

    QMap<QString,QString> data;
    data.insert("channel", channelId);
    data.insert("text", content);
    data.insert("as_user", "true");
    data.insert("parse", "full");

    QNetworkReply* reply = executePost("chat.postMessage", data);
    connect(reply, &QNetworkReply::finished, [reply,this]() {
        QJsonObject data = Request::getResult(reply);

        if (Request::isError(data)) {
            qDebug() << config->getTeamName() << ": Post message failed";
        }

        reply->deleteLater();
    });
}

void SlackClient::postImage(QString channelId, QString imagePath, QString title, QString comment) {
    QMap<QString,QString> data;
    data.insert("channels", channelId);

    if (!title.isEmpty()) {
        data.insert("title", title);
    }

    if (!comment.isEmpty()) {
        data.insert("initial_comment", comment);
    }

    QFile* imageFile = new QFile(imagePath);
    if (!imageFile->open(QFile::ReadOnly)) {
        qWarning() << "image file not readable" << imagePath;
        emit postImageFail();
        return;
    }

    qDebug() << config->getTeamName() << ": sending image" << imagePath;
    QNetworkReply* reply = executePostWithFile("files.upload", data, imageFile);

    connect(reply, SIGNAL(finished()), imageFile, SLOT(deleteLater()));
    connect(reply, &QNetworkReply::finished, [reply,this]() {
        QJsonObject data = Request::getResult(reply);
        qDebug() << config->getTeamName() << ": Post image result" << data;

        if (Request::isError(data)) {
            emit postImageFail();
        }
        else {
            emit postImageSuccess();
        }

        reply->deleteLater();
    });
}

void SlackClient::loadUserInfo(QString userId) {
    QMap<QString, QString> params;
    params["user"] = userId;

    QNetworkReply *reply = executeGet("users.info", params);
    connect(reply, &QNetworkReply::finished, [userId, reply, this]() {
        QJsonObject data = Request::getResult(reply);
        if (Request::isError(data)) {
            qDebug() << config->getTeamName() << ": User fetch failed:" << data.toVariantMap();
            Q_EMIT loadUserInfoFail(userId);
            return;
        }

        const auto user = data["user"].toObject();
        Q_EMIT loadUserInfoSuccess(userId, user.toVariantMap());
    });
}

QVariantMap SlackClient::getMessageData(const QJsonObject message) {
    qlonglong multiplier = 1000;
    QStringList timeParts = message.value("ts").toString().split(".");
    QString timePart = timeParts.value(0);
    QString indexPart = timeParts.value(1);

    qlonglong timestamp = timePart.toLongLong() * multiplier + indexPart.toLongLong();
    QDateTime time = QDateTime::fromMSecsSinceEpoch(timestamp);

    QVariantMap data;
    data.insert("type", message.value("type").toVariant());
    data.insert("time", QVariant::fromValue(time));
    data.insert("timegroup", QVariant::fromValue(time.toString("MMMM d, yyyy")));
    data.insert("timestamp", message.value("ts").toVariant());
    data.insert("channel", message.value("channel").toVariant());
    data.insert("user", user(message));
    data.insert("attachments", getAttachments(message));
    data.insert("images", getImages(message));
    data.insert("content", QVariant(getContent(message)));

    return data;
}

QVariantMap SlackClient::user(const QJsonObject &data) {
    QString type = data.value("subtype").toString("default");
    QVariant userId;

    if (type == "bot_message") {
        userId = data.value("bot_id").toVariant();
    }
    else if (type == "file_comment") {
        userId = data.value("comment").toObject().value("user").toVariant();
    }
    else {
        userId = data.value("user").toVariant();
    }

    if (!userId.isValid()) {
        qDebug() << config->getTeamName() << ": User not found for message";
    }

    QVariantMap userData = storage.user(userId);

    if (userData.isEmpty()) {
        userData.insert("id", data.value("user").toVariant());
        userData.insert("name", QVariant("Unknown"));
        userData.insert("presence", QVariant("away"));
    }

    QString username = data.value("username").toString();
    if (!username.isEmpty()) {
        QRegularExpression newUserPattern("<@([A-Z0-9]+)\\|([^>]+)>");
        username.replace(newUserPattern, "\\2");
        userData.insert("name", username);
    }

    return userData;
}

QString SlackClient::getContent(QJsonObject message) {
    QString content = message.value("text").toString();

    findNewUsers(content);
    messageFormatter.replaceUserInfo(content);
    messageFormatter.replaceTargetInfo(content);
    messageFormatter.replaceChannelInfo(content);
    messageFormatter.replaceLinks(content);
    messageFormatter.replaceSpecialCharacters(content);
    messageFormatter.replaceMarkdown(content);
    messageFormatter.replaceEmoji(content);

    return content;
}

QVariantList SlackClient::getImages(QJsonObject message) {
    QVariantList images;
    QStringList imageTypes;
    imageTypes << "jpg";
    imageTypes << "png";
    imageTypes << "gif";

    if (message.value("subtype").toString() == "file_share") {
        QJsonObject file = message.value("file").toObject();
        QString fileType = file.value("filetype").toString();

        if (imageTypes.contains(fileType)) {
            getImageData(file, images);
        }
    } else if (message.contains("files")) {
        QJsonArray files = message.value("files").toArray();
        foreach (const QJsonValue &value, files) {
            QJsonObject file = value.toObject();
            QString fileType = file.value("filetype").toString();

            if (imageTypes.contains(fileType)) {
                getImageData(file, images);
            }
        }
    }

    return images;
}

void SlackClient::getImageData(const QJsonObject &file, QVariantList &list) {
    QString thumbItem = file.contains("thumb_480") ? "480" : "360";

    QVariantMap thumbSize;
    thumbSize.insert("width", file.value("thumb_" + thumbItem + "_w").toVariant());
    thumbSize.insert("height", file.value("thumb_" + thumbItem + "_h").toVariant());

    QVariantMap imageSize;
    imageSize.insert("width", file.value("original_w").toVariant());
    imageSize.insert("height", file.value("original_h").toVariant());

    QVariantMap fileData;
    fileData.insert("name", file.value("name").toVariant());
    fileData.insert("url", file.value("url_private").toVariant());
    fileData.insert("size", imageSize);
    fileData.insert("thumbSize", thumbSize);
    fileData.insert("thumbUrl", file.value("thumb_" + thumbItem).toVariant());

    if (file.contains("title")) {
        fileData.insert("title", file.value("title").toVariant());
    }

    list.append(fileData);
}

QVariantList SlackClient::getAttachments(QJsonObject message) {
    QJsonArray attachementList = message.value("attachments").toArray();
    QVariantList attachments;

    foreach (const QJsonValue &value, attachementList) {
        QJsonObject attachment = value.toObject();
        QVariantMap data;

        QString titleLink = attachment.value("title_link").toString();
        QString title = attachment.value("title").toString();
        QString pretext = attachment.value("pretext").toString();
        QString text = attachment.value("text").toString();
        QString fallback = attachment.value("fallback").toString();
        QString color = getAttachmentColor(attachment);
        QVariantList fields = getAttachmentFields(attachment);
        QVariantList images = getAttachmentImages(attachment);

        messageFormatter.replaceLinks(pretext);
        messageFormatter.replaceLinks(text);
        messageFormatter.replaceLinks(fallback);
        messageFormatter.replaceEmoji(text);

        int index = text.indexOf(' ', 250);
        if (index > 0) {
            text = text.left(index) + "...";
        }

        if (!title.isEmpty() && !titleLink.isEmpty()) {
            title = "<a href=\""+ titleLink + "\">" + title +"</a>";
        }

        data.insert("title", QVariant(title));
        data.insert("pretext", QVariant(pretext));
        data.insert("content", QVariant(text));
        data.insert("fallback", QVariant(fallback));
        data.insert("indicatorColor", QVariant(color));
        data.insert("fields", QVariant(fields));
        data.insert("images", QVariant(images));

        attachments.append(data);
    }

    return attachments;
}

QString SlackClient::getAttachmentColor(QJsonObject attachment) {
    QString color = attachment.value("color").toString();

    if (color.isEmpty()) {
        color = "theme";
    }
    else if (color == "good") {
        color = "#6CC644";
    }
    else if (color == "warning") {
        color = "#E67E22";
    }
    else if (color == "danger") {
        color = "#D00000";
    }
    else if (!color.startsWith('#')) {
        color = "#" + color;
    }

    return color;
}

QVariantList SlackClient::getAttachmentFields(QJsonObject attachment) {
    QVariantList fields;
    if (attachment.contains("fields")) {
        QJsonArray fieldList = attachment.value("fields").toArray();

        foreach (const QJsonValue &fieldValue, fieldList) {
            QJsonObject field = fieldValue.toObject();
            QString title = field.value("title").toString();
            QString value = field.value("value").toString();
            bool isShort = field.value("short").toBool();

            if (!title.isEmpty()) {
                messageFormatter.replaceLinks(title);
                messageFormatter.replaceMarkdown(title);

                QVariantMap titleData;
                titleData.insert("isTitle", QVariant(true));
                titleData.insert("isShort", QVariant(isShort));
                titleData.insert("content", QVariant(title));
                fields.append(titleData);
            }

            if (!value.isEmpty()) {
                messageFormatter.replaceLinks(value);
                messageFormatter.replaceMarkdown(value);

                QVariantMap valueData;
                valueData.insert("isTitle", QVariant(false));
                valueData.insert("isShort", QVariant(isShort));
                valueData.insert("content", QVariant(value));
                fields.append(valueData);
            }
        }
    }

    return fields;
}

QVariantList SlackClient::getAttachmentImages(QJsonObject attachment) {
    QVariantList images;

    if (attachment.contains("image_url")) {
        QVariantMap size;
        size.insert("width", attachment.value("image_width"));
        size.insert("height", attachment.value("image_height"));

        QVariantMap image;
        image.insert("url", attachment.value("image_url"));
        image.insert("size", size);

        images.append(image);
    }

    return images;
}

void SlackClient::findNewUsers(const QString &message) {
    QRegularExpression newUserPattern("<@([A-Z0-9]+)\\|([^>]+)>");

    QRegularExpressionMatchIterator i = newUserPattern.globalMatch(message);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString id = match.captured(1);

        if (storage.user(id).isEmpty()) {
            QString name = match.captured(2);
            QVariantMap data;
            data.insert("id", QVariant(id));
            data.insert("name", QVariant(name));
            data.insert("presence", QVariant("active"));
            storage.saveUser(data);
        }
    }
}

void SlackClient::sendNotification(QString channelId, QString title, QString text) {
    QString body = text.length() > 100 ? text.left(97) + "..." : text;
    QString preview = text.length() > 40 ? text.left(37) + "..." : text;

    QVariantList arguments;
    arguments.append(team);
    arguments.append(channelId);

    Notification notification;
    notification.setAppName("Sailslack");
    notification.setAppIcon("harbour-sailslack");
    notification.setBody(body);
    notification.setPreviewSummary(title);
    notification.setPreviewBody(preview);
    notification.setCategory("chat");
    notification.setHintValue("x-sailslack-team", team);
    notification.setHintValue("x-sailslack-channel", channelId);
    notification.setHintValue("x-nemo-feedback", "chat_exists");
    notification.setHintValue("x-nemo-priority", 100);
    notification.setHintValue("x-nemo-display-on", true);
    notification.setRemoteAction(Notification::remoteAction("default", "", "harbour.sailslack", "/", "harbour.sailslack", "activate", arguments));
    notification.publish();
}

void SlackClient::updateUnreadCount() {
    const auto channels = storage.channels();
    unreadCount = std::accumulate(channels.cbegin(), channels.cend(), 0,
                                  [](int count, const QVariant &channel) {
        const auto map = channel.toMap();
        return count + map.value(QStringLiteral("unreadCount"), 0).toInt();
    });
    Q_EMIT unreadCountChanged(unreadCount);
}
