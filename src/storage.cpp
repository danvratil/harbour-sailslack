#include "storage.h"

#include <QDebug>
#include <QSequentialIterable>

void Storage::saveUser(const QVariantMap &user) {
    userMap.insert(user.value("id").toString(), user);
}

QVariantMap Storage::user(const QString &id) const {
    return userMap.value(id);
}

QVariantList Storage::users() const {
    QVariantList users;
    users.reserve(userMap.size());
    std::copy(userMap.cbegin(), userMap.cend(), std::back_inserter(users));
    return users;
}

void Storage::saveChannel(const QVariantMap &channel) {
    channelMap[channel[QStringLiteral("id")].toString()].info = channel;
}

QVariantMap Storage::channel(const QString &id) const {
    return channelMap.value(id).info;
}

QVariantMap Storage::thread(const QString &channelId, const QString &threadId) const {
    const auto list = channelMap[channelId].threadMessages[threadId];
    if (list.empty()) {
        return {};
    }
    return list.first().toMap();
}

QVariantList Storage::channels() const {
    QVariantList channels;
    channels.reserve(channelMap.size());
    std::transform(channelMap.cbegin(), channelMap.cend(), std::back_inserter(channels), [](const auto &channel) { return channel.info; });
    return channels;
}

QVariantList Storage::channelMessages(const QString &channelId) const {
    return channelMap[channelId].messages;
}

auto Storage::findChannelMessage(const QString &channelId, const QString &messageId) const
    -> std::pair<bool, decltype(Storage::Channel::messages)::const_iterator>
{
    auto channelIt = channelMap.constFind(channelId);
    if (channelIt == channelMap.cend()) {
        return {false, {}};
    }

    auto msgIt = std::lower_bound(channelIt->messages.cbegin(), channelIt->messages.cend(), messageId,
                                  [](const auto &msg, const auto &msgId) {
                                    return msg.toMap().value(QStringLiteral("timestamp")) == msgId;
                                  });
    if (msgIt == channelIt->messages.cend() || msgIt->toMap().value(QStringLiteral("timestamp")) != messageId) {
        return {false, {}};
    }

    return std::make_pair(true, msgIt);
}

auto Storage::findChannelMessage(const QString &channelId, const QString &messageId)
    -> std::pair<bool, decltype(Storage::Channel::messages)::iterator>
{
    auto channelIt = channelMap.find(channelId);
    if (channelIt == channelMap.end()) {
        return {false, {}};
    }

    auto msgIt = std::lower_bound(channelIt->messages.begin(), channelIt->messages.end(), messageId,
                                  [](const auto &msg, const auto &msgId) {
                                    return msg.toMap().value(QStringLiteral("timestamp")) == msgId;
                                  });
    if (msgIt == channelIt->messages.end() || msgIt->toMap().value(QStringLiteral("timestamp")) != messageId) {
        return {false, {}};
    }

    return std::make_pair(true, msgIt);
}

QVariantMap Storage::channelMessage(const QString &channelId, const QString &messageId) const {
    const auto [found, msgIt] = findChannelMessage(channelId, messageId);
    if (!found) {
        return {};
    }

    return msgIt->toMap();
}

void Storage::updateChannelMessage(const QString &channelId, const QVariantMap &message) {
    auto [found, msgIt] = findChannelMessage(channelId, message[QStringLiteral("timestamp")].toString());
    if (!found) {
        return;
    }

    *msgIt = message;
}

bool Storage::channelHasMessages(const QString &channelId) const {
    auto channelIt = channelMap.constFind(channelId);
    if (channelIt == channelMap.cend()) {
        return false;
    }

    return !channelIt->messages.empty();
}

QVariantList Storage::threadMessages(const QString &channelId, const QString &threadId) const {
    auto channelIt = channelMap.constFind(channelId);
    if (channelIt == channelMap.cend()) {
        return {};
    }

    auto threadIt = channelIt->threadMessages.constFind(threadId);
    if (threadIt == channelIt->threadMessages.cend()) {
        return {};
    }

    return *threadIt;
}

bool Storage::threadHasMessages(const QString &channelId, const QString &threadId) const {
    auto channelIt = channelMap.constFind(channelId);
    if (channelIt == channelMap.cend()) {
        return false;
    }

    auto threadIt = channelIt->threadMessages.constFind(threadId);
    if (threadIt == channelIt->threadMessages.cend()) {
        return false;
    }

    return threadIt->size() > 1;
}

QString messageThreadId(const QVariantMap &message) {
    return message.contains("thread_ts")
            ? message.value("thread_ts").toString()
            : QString();
}

bool isThreadStarter(const QVariantMap& message) {
    return message.value("thread_ts") == message.value("timestamp") || message.value("transient").toBool();
}

void Storage::setChannelMessages(const QString &channelId, const QVariantList &messages) {
    channelMap[channelId].messages = messages;
    for (const auto &message: messages) {
        auto threadId = messageThreadId(message.toMap());
        if (!threadId.isEmpty()) {
            createOrUpdateThread(channelId, threadId, message.toMap());
        }
    }
}

void Storage::setThreadMessages(const QString &channelId, const QString &threadId, const QVariantList &messages) {
    channelMap[channelId].threadMessages[threadId] = messages;
}

void Storage::createOrUpdateThread(const QString &channelId, const QString &threadId, const QVariantMap &message) {
    if (!isThreadStarter(message)) {
        return;
    }
    auto &channel = channelMap[channelId];
    auto &messages = channel.threadMessages[threadId];
    if (messages.empty()) {
        messages.push_back(message);
    } else {
        messages.replace(0, message);
    }

    // Search the starting message in its channel
    // (or better do this from JS)
}

void Storage::prependChannelMessages(const QString &channelId, const QVariantList &messages) {
    auto &channel = channelMap[channelId];
    channel.messages = messages + channel.messages;
}

QVariantList::const_iterator Storage::findSortedMessages(const QVariantList& messages, const QString& timestamp) const {
    return std::lower_bound(messages.begin(),
                     messages.end(),
                     timestamp,
                     [](const QVariant& m1, const QString& timestamp)
    {
        return m1.toMap().value("timestamp").toString() < timestamp;
    });
}

QVariantList::iterator Storage::findSortedMessages(QVariantList& messages, const QString& timestamp) {
    return std::lower_bound(messages.begin(),
                     messages.end(),
                     timestamp,
                     [](const QVariant& m1, const QString& timestamp)
    {
        return m1.toMap().value("timestamp").toString() < timestamp;
    });
}

void Storage::appendChannelMessage(const QString &channelId, const QVariantMap &message) {
    auto &channel = channelMap[channelId];
    auto threadId = messageThreadId(message);
    if (threadId.isEmpty() || isThreadStarter(message)) {
        // append or update
        auto msgIt = findSortedMessages(channel.messages, threadId);
        if (msgIt != channel.messages.end()) {
            *msgIt = message;
        } else {
            channel.messages.push_back(message);
        }
    } else {
        appendThreadMessage(channelId, threadId, message);
    }
}

void Storage::clearChannelMessages(const QString &channelId) {
    if (channelId.isNull()) {
        for (auto &channel : channelMap) {
            channel.messages.clear();
            channel.threadMessages.clear();
        }
    } else {
        channelMap[channelId].messages.clear();
   }
}

void Storage::appendThreadMessage(const QString &channelId, const QString &threadId, const QVariantMap &message) {
    if (isThreadStarter(message)) {
        createOrUpdateThread(channelId, threadId, message);
    }

    auto &channel = channelMap[channelId];
    auto &thread = channel.threadMessages[threadId];
    if (thread.empty()) {
        qWarning("Thread without a thread starter?");
        Q_ASSERT(false);
        return;
    }

    if (isThreadStarter(message)) {
        qDebug() << "Updated thread starter for:" << threadId;
    }
    auto msgIt = findSortedMessages(thread, message.value("timestamp").toString());
    if (msgIt != thread.end()) {
        *msgIt = message;
    } else {
        thread.push_back(message);
    }
}

void Storage::clear() {
    userMap.clear();
    channelMap.clear();
}
