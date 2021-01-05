#ifndef STORAGE_H
#define STORAGE_H

#include <QVariantMap>
#include <QVariantHash>

class Storage
{
public:
    QVariantMap user(const QString &id) const;
    QVariantList users() const;
    void saveUser(const QVariantMap &user);

    QVariantMap channel(const QString &channelId) const;
    QVariantMap thread(const QString &channelId, const QString &id) const;
    QVariantList channels() const;
    void saveChannel(const QVariantMap &channel);

    QVariantList channelMessages(const QString &channelId) const;
    bool channelHasMessages(const QString &channelId) const;
    QVariantMap channelMessage(const QString &channelId, const QString &messageId) const;
    void setChannelMessages(const QString &channelId, const QVariantList &messages);

    QVariantList::const_iterator findSortedMessages(const QVariantList& messages, const QString& timestamp) const;
    QVariantList::iterator findSortedMessages(QVariantList &messages, const QString &timestamp);

    QVariantList threadMessages(const QString &channelId, const QString &threadId) const;
    bool threadHasMessages(const QString &channelId, const QString &threadId) const;
    void setThreadMessages(const QString &channelId, const QString& threadId, const QVariantList &messages);
    void createOrUpdateThread(const QString &channelId, const QString &threadId, const QVariantMap &message);

    void prependChannelMessages(const QString &channelId, const QVariantList &messages);
    void appendChannelMessage(const QString &channelId, const QVariantMap &message);
    void updateChannelMessage(const QString &channelId, const QVariantMap &message);
    void clearChannelMessages(const QString &channelId = {});

    void appendThreadMessage(const QString &channelId, const QString &threadId, const QVariantMap &message);

    void clear();

private:
    struct Channel {
        QVariantMap info;
        QVariantList messages; // top level messages
        QHash<QString /*threadId*/, QVariantList> threadMessages; // thread messages (incl. thread root)
    };
    QHash<QString /*userId*/, QVariantMap> userMap;
    QHash<QString /*channelId*/, Channel> channelMap;


    auto findChannelMessage(const QString &channelId, const QString &messageId) const
        -> std::pair<bool,  decltype(Channel::messages)::const_iterator>;
    auto findChannelMessage(const QString &channelId, const QString &messageId)
        -> std::pair<bool, decltype(Channel::messages)::iterator>;
};

#endif // STORAGE_H
