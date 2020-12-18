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
    static EmojiProvider *self();

public Q_SLOTS:
    QString urlForEmoji(const QString &emoji) const;

private:
    explicit EmojiProvider();

    void loadEmojis();

    QHash<QString, QString> mEmojiStore;

    static EmojiProvider *sEmojiProvider;
};

#endif // EMOJIPROVIDER_H
