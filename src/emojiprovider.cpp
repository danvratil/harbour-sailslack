#include "emojiprovider.h"

#include <QFile>
#include <QtConcurrent>

static QHash<QString, QString> loadDefaultEmojis() {
    Q_INIT_RESOURCE(data);

    QFile file;
    file.setFileName((":/data/data/emoji.dat"));
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QHash<QString, QString> store;
    while (!file.atEnd()) {
        const auto line = file.readLine();
        const int split = line.indexOf(':');

        store.insert(QString::fromLatin1(line.constData(), split),
                     QString::fromLatin1(line.constData() + split + 1, line.size() - split - 2)); // -2 to also remove \n
    }

    return store;
}

QHash<QString, QString> EmojiProvider::sStandardEmojis = loadDefaultEmojis();

EmojiProvider::EmojiProvider() = default;

QString EmojiProvider::urlForEmoji(const QString &emoji) const
{
    auto sit = sStandardEmojis.constFind(emoji);
    if (sit != sStandardEmojis.constEnd()) {
        return QStringLiteral("https://a.slack-edge.com/production-standard-emoji-assets/10.2/google-large/%1").arg(sit.value());
    }

    auto cit = mCustomEmojis.constFind(emoji);
    if (cit != mCustomEmojis.constEnd()) {
        return cit.value();
    }

    qDebug() << "No such emoji:" << emoji;
    return {};
}

void EmojiProvider::setCustomEmojis(const QHash<QString, QString> &emojis)
{
    mCustomEmojis = emojis;
}
