#ifndef EMOJIPROVIDER_H
#define EMOJIPROVIDER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QAtomicPointer>

class EmojiProvider : public QObject
{
    Q_OBJECT
public:
    explicit EmojiProvider();

    void setCustomEmojis(const QHash<QString, QString> &emojis);

public Q_SLOTS:
    QString urlForEmoji(const QString &emoji) const;

private:
    void loadEmojis();

    static QHash<QString, QString> sStandardEmojis;
    QHash<QString, QString> mCustomEmojis;
};

#endif // EMOJIPROVIDER_H
