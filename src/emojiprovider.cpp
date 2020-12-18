#include "emojiprovider.h"

#include <QFile>
#include <QtConcurrent>

EmojiProvider *EmojiProvider::sEmojiProvider = nullptr;

EmojiProvider::EmojiProvider()
{
    loadEmojis();
}

EmojiProvider *EmojiProvider::self()
{
    if (!sEmojiProvider) {
        sEmojiProvider = new EmojiProvider{};
    }

    return sEmojiProvider;
}

QString EmojiProvider::urlForEmoji(const QString &emoji) const
{
    auto it = mEmojiStore.find(emoji);
    if (it == mEmojiStore.cend()) {
        qDebug() << "No such emoji:" << emoji;
        return {};
    }
    qDebug() << "Found image name for emoji" << emoji << ":" << it.value();
    return QStringLiteral("https://a.slack-edge.com/production-standard-emoji-assets/10.2/google-large/%1").arg(it.value());
}

void EmojiProvider::loadEmojis()
{
    Q_INIT_RESOURCE(data);

    QFile file;
    file.setFileName((":/data/data/emoji.dat"));
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    while (!file.atEnd()) {
        const auto line = file.readLine();
        const int split = line.indexOf(':');

        mEmojiStore.insert(QString::fromLatin1(line.constData(), split),
                           QString::fromLatin1(line.constData() + split + 1, line.size() - split - 2)); // -2 to also remove \n
    }
}
