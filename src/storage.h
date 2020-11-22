#ifndef STORAGE_H
#define STORAGE_H

#include <QVariantMap>

class Storage
{
public:
    QVariantMap user(const QString &id);
    QVariantList users();
    void saveUser(QVariantMap user);

    QVariantMap channel(const QString &id);
    QVariantMap thread(const QString &id);
    QVariantList channels();
    void saveChannel(QVariantMap channel);

    QVariantList channelMessages(const QString &channelId);
    bool channelMessagesExist(const QString &channelId);
    void setChannelMessages(const QString &channelId, QVariantList messages);

    QVariantList threadMessages(const QString &threadId);
    bool threadMessagesExist(const QString &threadId);
    void setThreadMessages(const QString& threadId, QVariantList messages);
    void createOrUpdateThread(const QString &threadId, QVariantMap message);

    void prependChannelMessages(const QString &channelId, QVariantList messages);
    bool appendChannelMessage(const QString &channelId, QVariantMap message);
    void clearChannelMessages();

    void appendThreadMessage(const QString &threadId, QVariantMap message);

    void clear();

private:

    QVariantMap userMap;
    QVariantMap channelMap;
    QVariantMap channelMessageMap;
    QVariantMap threadMap;
    QVariantMap threadMessageMap;
};

#endif // STORAGE_H
