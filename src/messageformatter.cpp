#include "messageformatter.h"

#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "storage.h"
#include "slackclient.h"

MessageFormatter::MessageFormatter(SlackClient &slackClient, Storage &storage)
    : storage(storage), client(slackClient)
{}

void MessageFormatter::replaceUserInfo(QString &message) {
    foreach (const QVariant &value, storage.users()) {
        QVariantMap user = value.toMap();
        QString id = user.value("id").toString();
        QString name = user.value("name").toString();

        QRegularExpression userIdPattern("<@" + id + "(\\|[^>]+)?>");
        QString displayName = "<a href=\"sailslack://user/"+ id +"\">@" + name + "</a>";

        message.replace(userIdPattern, displayName);
    }
}

void MessageFormatter::replaceChannelInfo(QString &message) {
    foreach (const QVariant &value, storage.channels()) {
        QVariantMap channel = value.toMap();
        QString id = channel.value("id").toString();
        QString name = channel.value("name").toString();

        QRegularExpression channelIdPattern("<#" + id + "(\\|[^>]+)?>");
        QString displayName = "<a href=\"sailslack://channel/"+ id +"\">#" + name + "</a>";

        message.replace(channelIdPattern, displayName);
    }
}

void MessageFormatter::replaceLinks(QString &message) {
    QRegularExpression labelPattern("<(http[^\\|>]+)\\|([^>]+)>");
    message.replace(labelPattern, "<a href=\"\\1\">\\2</a>");

    QRegularExpression plainPattern("<(http[^>]+)>");
    message.replace(plainPattern, "<a href=\"\\1\">\\1</a>");

    QRegularExpression mailtoPattern("<(mailto:[^\\|>]+)\\|([^>]+)>");
    message.replace(mailtoPattern, "<a href=\"\\1\">\\2</a>");
}

void MessageFormatter::replaceMarkdown(QString &message) {
    QRegularExpression italicPattern("(^|\\s)_([^_]+)_(\\s|\\.|\\?|!|,|$)");
    message.replace(italicPattern, "\\1<i>\\2</i>\\3");

    QRegularExpression boldPattern("(^|\\s)\\*([^\\*]+)\\*(\\s|\\.|\\?|!|,|$)");
    message.replace(boldPattern, "\\1<b>\\2</b>\\3");

    QRegularExpression strikePattern("(^|\\s)~([^~]+)~(\\s|\\.|\\?|!|,|$)");
    message.replace(strikePattern, "\\1<s>\\2</s>\\3");

    QRegularExpression codePattern("(^|\\s)`([^`]+)`(\\s|\\.|\\?|!|,|$)");
    message.replace(codePattern, "\\1<code>\\2</code>\\3");

    QRegularExpression codeBlockPattern("```([^`]+)```");
    message.replace(codeBlockPattern, "<br/><code>\\1</code><br/>");

    QRegularExpression newLinePattern("\n");
    message.replace(newLinePattern, "<br/>");
}

void MessageFormatter::replaceEmoji(QString &message) {
    QRegularExpression emojiPattern(":([\\w\\+\\-]+):(:[\\w\\+\\-]+:)?[\\?\\.!,]?");
    QRegularExpressionMatchIterator i = emojiPattern.globalMatch(message);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString name = match.captured(1);

        const auto image = client.getEmojiProvider()->urlForEmoji(name);
        if (!image.isEmpty()) {
            const QString emoji = QStringLiteral("<img src=\"%1\" alt=\"%2\" align=\"bottom\" width=\"64\" height=\"64\" />").arg(image, name);
            message.replace(":" + name + ":", emoji);
        } else {
          qDebug() << "Missing emoji" << name;
        }
    }
}

void MessageFormatter::replaceTargetInfo(QString &message) {
    QRegularExpression variableLabelPattern("<!(here|channel|group|everyone)\\|([^>]+)>");
    message.replace(variableLabelPattern, "<a href=\"sailslack://target/\\1\">\\2</a>");

    QRegularExpression variablePattern("<!(here|channel|group|everyone)>");
    message.replace(variablePattern, "<a href=\"sailslack://target/\\1\">@\\1</a>");
}

void MessageFormatter::replaceSpecialCharacters(QString &message) {
    message.replace(QRegularExpression("&gt;"), ">");
    message.replace(QRegularExpression("&lt;"), "<");
    message.replace(QRegularExpression("&amp;"), "&");
}
