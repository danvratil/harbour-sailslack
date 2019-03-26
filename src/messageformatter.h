#ifndef MESSAGEFORMATTER_H
#define MESSAGEFORMATTER_H

#include <QMap>
#include <QString>

class Storage;

class MessageFormatter
{
public:
    MessageFormatter(Storage &storage);

    void replaceUserInfo(QString &message);
    void replaceTargetInfo(QString &message);
    void replaceChannelInfo(QString &message);
    void replaceSpecialCharacters(QString &message);
    void replaceLinks(QString &message);
    void replaceMarkdown(QString &message);
    void replaceEmoji(QString &message);

private:
    Storage &storage;

    static QMap<QString,QString> emojis;
};

#endif // MESSAGEFORMATTER_H
