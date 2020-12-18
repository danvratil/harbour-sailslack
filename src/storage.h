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

    QVariantList channelMessages(const QString &channelId) const;
    bool channelMessagesExist(const QString &channelId);
    QVariantMap channelMessage(const QString &channelId, const QString &messageId) const;
    void setChannelMessages(const QString &channelId, QVariantList messages);
    QVariantList::const_iterator findSortedMessages(const QVariantList& messages, const QString& timestamp);

    QVariantList threadMessages(const QString &threadId);
    bool threadMessagesExist(const QString &threadId);
    void setThreadMessages(const QString& threadId, QVariantList messages);
    void createOrUpdateThread(const QString &threadId, QVariantMap message);

    void prependChannelMessages(const QString &channelId, QVariantList messages);
    void appendChannelMessage(const QString &channelId, QVariantMap message);
    void updateChannelMessage(const QString &channelId, const QVariantMap &message);
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
