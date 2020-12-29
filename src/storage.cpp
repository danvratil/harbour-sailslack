#include "storage.h"

#include <QDebug>
#include <QSequentialIterable>

void Storage::saveUser(QVariantMap user) {
    userMap.insert(user.value("id").toString(), user);
}

QVariantMap Storage::user(const QString &id) {
    return userMap.value(id).toMap();
}

QVariantList Storage::users() {
    return userMap.values();
}

void Storage::saveChannel(QVariantMap channel) {
    channelMap.insert(channel.value("id").toString(), channel);
}

QVariantMap Storage::channel(const QString &id) {
    return channelMap.value(id).toMap();
}

QVariantMap Storage::thread(const QString &id) {
    return threadMap.value(id).toMap();
}

QVariantList Storage::channels() {
    return channelMap.values();
}

QVariantList Storage::channelMessages(const QString &channelId) {
    return channelMessageMap.value(channelId).toList();
}

bool Storage::channelMessagesExist(const QString &channelId) {
    return channelMessageMap.contains(channelId);
}

QVariantList Storage::threadMessages(const QString &threadId) {
    return threadMessageMap.value(threadId).toList();
}

bool Storage::threadMessagesExist(const QString &threadId) {
    return channelMessageMap.contains(threadId);
}

QString messageThread(const QVariantMap& message) {
    return message.contains("thread_ts")
            ? message.value("thread_ts").toString()
            : QString();
}

bool isThreadStarter(const QVariantMap& message) {
    return message.value("thread_ts") == message.value("timestamp") || message.value("transient").toBool();
}

void Storage::setChannelMessages(const QString &channelId, QVariantList messages) {
    channelMessageMap.insert(channelId, messages);
    for (auto &message: messages) {
        auto threadId = messageThread(message.toMap());
        if (!threadId.isEmpty()) {
            createOrUpdateThread(threadId, message.toMap());
        }
    }
}

void Storage::setThreadMessages(const QString& threadId, QVariantList messages) {
    threadMessageMap.insert(threadId, messages);
}

void Storage::createOrUpdateThread(const QString &threadId, QVariantMap message) {
    if (!isThreadStarter(message)) {
        return;
    }
    if (!threadMap.contains(threadId)) {
        threadMessageMap.insert(threadId, QVariantList({message}));
        threadMap.insert(threadId, message);
    } else {
        qDebug() << "Thread already exists:" << threadId;
        threadMap.insert(threadId, message);
        threadMessageMap.value(threadId).toList().replace(0, message);
    }

    // Search the starting message in its channel
    // (or better do this from JS)
}

void Storage::prependChannelMessages(const QString &channelId, QVariantList messages) {
    QVariantList existing = channelMessages(channelId);
    messages.append(existing);
    setChannelMessages(channelId, messages);
}

QVariantList::const_iterator Storage::findSortedMessages(const QVariantList& messages, const QString& timestamp) {
    return std::lower_bound(messages.begin(),
                     messages.end(),
                     timestamp,
                     [](const QVariant& m1, const QString& timestamp)
    {
        return m1.toMap().value("timestamp").toString() < timestamp;
    });
}

void Storage::appendChannelMessage(const QString &channelId, QVariantMap message) {
    QVariantList messages = channelMessages(channelId);
    auto threadId = messageThread(message);
    if (threadId.isEmpty() || isThreadStarter(message)) {
        // append or update
        auto found = findSortedMessages(messages, threadId);
        if (found != messages.constEnd()) {
            auto messageIndex = found - messages.constBegin();
            messages[messageIndex] = message;
        } else {
            messages.append(message);
        }
        setChannelMessages(channelId, messages);
    } else {
        appendThreadMessage(threadId, message);
    }
}

void Storage::clearChannelMessages() {
    channelMessageMap.clear();
}

void Storage::appendThreadMessage(const QString &threadId, QVariantMap message) {
    if(isThreadStarter(message)) {
        createOrUpdateThread(threadId, message);
    }

    auto messages = threadMessageMap.value(threadId).toList();
    if (messages.size()) {
        if (isThreadStarter(message)) {
            qDebug() << "Updated thread starter for:" << threadId;
        }
        auto found = findSortedMessages(messages, message.value("timestamp").toString());
        if (found != messages.constEnd()) {
            auto messageIndex = found - messages.constBegin();
            messages[messageIndex] = message;
        } else {
            messages.append(message);
        }
    } else {
        qDebug("Thread without thread starter?");
        Q_ASSERT(false);
        return;
    }

    setThreadMessages(threadId, messages);
}

void Storage::clear() {
    userMap.clear();
    channelMap.clear();
    channelMessageMap.clear();
    threadMap.clear();
    threadMessageMap.clear();
}
