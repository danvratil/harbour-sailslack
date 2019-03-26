#ifndef STORAGE_H
#define STORAGE_H

#include <QVariantMap>

class Storage
{
public:
    QVariantMap user(QVariant id);
    QVariantList users();
    void saveUser(QVariantMap user);

    QVariantMap channel(QVariant id);
    QVariantList channels();
    void saveChannel(QVariantMap channel);

    QVariantList channelMessages(QVariant channelId);
    bool channelMessagesExist(QVariant channelId);
    void setChannelMessages(QVariant channelId, QVariantList messages);
    void prependChannelMessages(QVariant channelId, QVariantList messages);
    void appendChannelMessage(QVariant channelId, QVariantMap message);
    void clearChannelMessages();

    void clear();

private:
    QVariantMap userMap;
    QVariantMap channelMap;
    QVariantMap channelMessageMap;
};

#endif // STORAGE_H
