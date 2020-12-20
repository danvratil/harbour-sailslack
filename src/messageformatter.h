#ifndef MESSAGEFORMATTER_H
#define MESSAGEFORMATTER_H

#include <QMap>
#include <QString>

class Storage;
class SlackClient;

class MessageFormatter
{
public:
    MessageFormatter(SlackClient &client, Storage &storage);

    void replaceUserInfo(QString &message);
    void replaceTargetInfo(QString &message);
    void replaceChannelInfo(QString &message);
    void replaceSpecialCharacters(QString &message);
    void replaceLinks(QString &message);
    void replaceMarkdown(QString &message);
    void replaceEmoji(QString &message);

private:
    Storage &storage;
    SlackClient &client;
};

#endif // MESSAGEFORMATTER_H
