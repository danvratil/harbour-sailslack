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
    return message.value("thread_ts") == message.value("timestamp");
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
    Q_ASSERT(isThreadStarter(message));
    if (!threadMap.contains(threadId)) {
        threadMessageMap.insert(threadId, QVariantList({message}));
        threadMap.insert(threadId, message);
    } else {
        qDebug() << "Thread already exists:" << threadId;
    }

    // Search the starting message in its channel
    // (or better do this from JS)
}

void Storage::prependChannelMessages(const QString &channelId, QVariantList messages) {
    QVariantList existing = channelMessages(channelId);
    messages.append(existing);
    setChannelMessages(channelId, messages);
}

bool Storage::appendChannelMessage(const QString &channelId, QVariantMap message) {
    bool result = false;
    QVariantList messages = channelMessages(channelId);
    auto threadId = messageThread(message);
    if (threadId.isEmpty() || isThreadStarter(message)) {
        messages.append(message);
        setChannelMessages(channelId, messages);
        result = true;
    }
    if (!threadId.isEmpty()) {
        appendThreadMessage(threadId, message);
    }

    return result;
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
        } else {
            messages.push_back(message);
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
