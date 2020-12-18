#ifndef SLACKCLIENT_H
#define SLACKCLIENT_H

#include <QObject>
#include <QPointer>
#include <QImage>
#include <QFile>
#include <QJsonObject>
#include <QUrl>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "slackclientconfig.h"
#include "slackstream.h"
#include "storage.h"
#include "messageformatter.h"

class SlackClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(SlackClientConfig* config READ getConfig CONSTANT)
    Q_PROPERTY(QString team READ getTeam CONSTANT)
    Q_PROPERTY(bool initialized READ getInitialized NOTIFY initializedChanged)
    Q_PROPERTY(ConnectionStatus connectionStatus READ getConnectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(bool networkAccessible READ getNetworkAccessible NOTIFY networkAccessibleChanged)
    Q_PROPERTY(int unreadCount READ getUnreadCount NOTIFY unreadCountChanged)

public:
    enum ConnectionStatus {
        Disconnected,
        Connecting,
        Connected
    };
    Q_ENUM(ConnectionStatus)

    explicit SlackClient(QObject *parent = 0); // only to make qmlRegisterType happy, don't use
    explicit SlackClient(const QString &team, QObject *parent = 0);
    ~SlackClient() override;

    Q_INVOKABLE void setAppActive(bool active);
    Q_INVOKABLE void setActiveWindow(QString windowId);

    Q_INVOKABLE QVariantList getChannels();
    Q_INVOKABLE QVariant getChannel(QString channelId);

    SlackClientConfig *getConfig() const { return this->config; }
    bool getInitialized() const { return initialized; }

    void setTeam(const QString &team);
    QString getTeam() const;

    int getUnreadCount() const { return unreadCount; }

    ConnectionStatus getConnectionStatus() const { return connectionStatus; }
    bool getNetworkAccessible() const { return networkAccessible == QNetworkAccessManager::Accessible; }

signals:
    void teamChanged();

    void testConnectionFail();
    void testLoginSuccess();
    void testLoginFail();

    void loadUsersSuccess();
    void loadUsersFail();

    void loadMessagesSuccess(QString channelId, QVariantList messages, bool hasMore);
    void loadMessagesFail();
    void loadHistorySuccess(QString channelId, QVariantList messages, bool hasMore);
    void loadHistoryFail();

    void loadUserInfoSuccess(QString userId, QVariantMap user);
    void loadUserInfoFail(QString userId);

    void initFail();
    void initSuccess();
    void initializedChanged();

    void reconnectFail();
    void reconnectAccessTokenFail();

    void messageReceived(QVariantMap message);
    void channelUpdated(QVariantMap channel);
    void channelJoined(QVariantMap channel);
    void channelLeft(QVariantMap channel);
    void userUpdated(QVariantMap user);

    void postImageSuccess();
    void postImageFail();

    void connectionStatusChanged(SlackClient::ConnectionStatus status);
    void networkAccessibleChanged(bool hasNetwork);

    void unreadCountChanged(int count);

public slots:
    void init();
    void start();
    void reconnect();

    void testLogin();
    void handleTestLoginReply();

    void loadHistory(QString type, QString channelId, QString latest);
    void loadMessages(QString type, QString channelId);
    void handleLoadMessagesReply();

    void logout();
    void loadUsers(const QString &cursor = {});
    void markChannel(QString type, QString channelId, QString time);
    void joinChannel(QString channelId);
    void leaveChannel(QString channelId);
    void leaveGroup(QString groupId);
    void openChat(QString chatId);
    void closeChat(QString chatId);
    void postMessage(QString channelId, QString content);
    void postImage(QString channelId, QString imagePath, QString title, QString comment);
    void loadUserInfo(QString userId);

    void handleNetworkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible);

    void handleStreamStart();
    void handleStreamEnd();
    void handleStreamMessage(QJsonObject message);

    void updatePresenceSubscription();

private:
    QString team;

    bool appActive;
    QString activeWindow;

    void setConnectionStatus(ConnectionStatus status);

    QNetworkReply* executePost(QString method, const QMap<QString, QString> &data);
    QNetworkReply *executePostWithFile(QString method, const QMap<QString, QString>&, QFile *file);

    QNetworkReply* executeGet(QString method, QMap<QString,QString> params = QMap<QString,QString>());

    static QString toString(const QJsonObject &data);

    void loadConversations(QString cursor = QString());

    void parseMessageUpdate(QJsonObject message);
    void parseChannelUpdate(QJsonObject message);
    void parseChannelJoin(QJsonObject message);
    void parseChannelLeft(QJsonObject message);
    void parseGroupJoin(QJsonObject message);
    void parseChatOpen(QJsonObject message);
    void parseChatClose(QJsonObject message);
    void parsePresenceChange(QJsonObject message);
    void parseNotification(QJsonObject message);

    QVariantList parseMessages(const QJsonObject data);
    QVariantMap getMessageData(const QJsonObject message);

    QString getContent(QJsonObject message);
    QVariantList getAttachments(QJsonObject message);
    QVariantList getImages(QJsonObject message);
    QString getAttachmentColor(QJsonObject attachment);
    QVariantList getAttachmentFields(QJsonObject attachment);
    QVariantList getAttachmentImages(QJsonObject attachment);

    QVariantMap parseChannel(QJsonObject data);
    QVariantMap parseGroup(QJsonObject group);
    QVariantMap parseChat(QJsonObject chat);

    void parseUsers(QJsonObject data);
    void findNewUsers(const QString &message);

    void sendNotification(QString channelId, QString title, QString content);
    void clearNotifications(QString channelId);

    void updateUnreadCount();

    QVariantMap user(const QJsonObject &data);

    QString historyMethod(QString type);
    QString markMethod(QString type);

    QPointer<QNetworkAccessManager> networkAccessManager;
    QPointer<SlackClientConfig> config;
    QPointer<SlackStream> stream;
    QPointer<QTimer> reconnectTimer;
    Storage storage;
    MessageFormatter messageFormatter;

    QNetworkAccessManager::NetworkAccessibility networkAccessible = QNetworkAccessManager::UnknownAccessibility;

    bool initialized = false;
    ConnectionStatus connectionStatus = Disconnected;
    int unreadCount = 0;
};

#endif // SLACKCLIENT_H
