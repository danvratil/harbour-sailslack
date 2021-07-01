/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "messagemodel.h"

#include <QLoggingCategory>

#include <vector>
#include <iterator>

Q_LOGGING_CATEGORY(logMM, "harbour-sailslack.MessageModel")

namespace {
    static const QString fieldChannelId = QStringLiteral("id");
    static const QString fieldMessageId = QStringLiteral("id");
    static const QString fieldThreadId = QStringLiteral("thread_ts");
} // namespace

void MessageModel::CacheCleanerHelper::operator()(Node *node)
{
    model->cleanupCache(node);
    delete node;
}

MessageModel::MessageModel(QObject *parent)
    : QAbstractItemModel{parent}
    , mCacheCleaner{this}
    , mRootNode{NodePtr{new Node, mCacheCleaner}}
{}

MessageModel::~MessageModel()
{
    // Explicitly release the the node tree here, before all other structures
    // are auto-destroyed
    mRootNode.reset();
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    const auto *parentNode = parent.isValid() ? static_cast<Node *>(parent.internalPointer()) : mRootNode.get();
    Q_ASSERT(parentNode != nullptr);
    return parentNode->children.size();
}

int MessageModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex MessageModel::index(int row, int column, const QModelIndex &parent) const
{
    const auto *parentNode = parent.isValid() ? static_cast<Node *>(parent.internalPointer()) : mRootNode.get();
    Q_ASSERT(parentNode != nullptr);
    Q_ASSERT(row >= 0);
    Q_ASSERT(row < static_cast<int>(parentNode->children.size()));
    return createIndex(row, column, parentNode->children[row].get());
}

QModelIndex MessageModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return {};
    }

    const auto *childNode = static_cast<Node *>(child.internalPointer());
    Q_ASSERT(childNode);
    auto *parentNode = childNode->parent;
    Q_ASSERT(parentNode);
    if (parentNode == mRootNode.get()) {
        return {};
    }

   return nodeIndex(parentNode);
}

bool MessageModel::hasChildren(const QModelIndex &parent) const
{
    const auto *parentNode = parent.isValid() ? static_cast<Node *>(parent.internalPointer()) : mRootNode.get();
    Q_ASSERT(parentNode != nullptr);
    return !parentNode->children.empty();
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    return {{static_cast<int>(Role::EntityType), "entityType"},
            {static_cast<int>(Role::Channel), "channel"},
            {static_cast<int>(Role::Message), "message"}};
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const auto *node = static_cast<Node *>(index.internalPointer());
    Q_ASSERT(node != nullptr);
    switch (static_cast<Role>(role)) {
        case Role::EntityType:
            return static_cast<int>(node->type);
        case Role::Channel:
            return node->data;
        case Role::Message:
            return node->data;
        default:
            return {};
    }
}

QModelIndex MessageModel::nodeIndex(const Node *node) const
{
    Q_ASSERT(node != nullptr);
    const auto *parentNode = node->parent;
    Q_ASSERT(parentNode != nullptr);

    const auto childIt = std::find_if(parentNode->children.cbegin(), parentNode->children.cend(),
                                      [node](const auto &n) { return node == n.get(); });
    Q_ASSERT(childIt != parentNode->children.cend());

    return createIndex(static_cast<int>(std::distance(parentNode->children.cbegin(), childIt)), 0, const_cast<Node *>(node));
}


void MessageModel::doAppendChannel(const QVariantMap &channel)
{
    NodePtr channelNode{new Node, mCacheCleaner};
    channelNode->type = EntityType::Channel;
    channelNode->parent = mRootNode.get();
    channelNode->data = channel;

    auto &ref = mRootNode->children.emplace_back(std::move(channelNode));
    mChannelLookup.insert(channel.value(fieldChannelId).toString(), ref.get());
}

QVariantMap MessageModel::channel(const ChannelID &id) const
{
    const auto node = mChannelLookup.constFind(id);
    if (node == mChannelLookup.cend()) {
        return {};
    }

    return node.value()->data;
}

void MessageModel::setChannels(const QVariantList &channels)
{
    beginResetModel();
    mRootNode->children.clear();
    mRootNode->children.reserve(channels.size());

    std::for_each(channels.cbegin(), channels.cend(), [this](const auto &c) { doAppendChannel(c.toMap()); });

    endResetModel();
}

void MessageModel::addChannel(const QVariantMap &channel)
{
    beginInsertRows({}, mRootNode->children.size(), mRootNode->children.size());
    doAppendChannel(channel);
    endInsertRows();
}

void MessageModel::updateChannel(const QVariantMap &channel)
{
    const auto channelId = channel[fieldChannelId].toString();
    auto *node = mChannelLookup.value(channelId);
    if (node == nullptr) {
        qCWarning(logMM) << "Received channel update for channel" << channelId << ", which is not in the model!";
        return;
    }

    node->data = channel;
    const auto index = nodeIndex(node);
    Q_EMIT dataChanged(index, index);
}

void MessageModel::openChannel(const ChannelID &channelId)
{
    if (const auto evictedChannel = mChannelLRU.reference(channelId); evictedChannel) {
        auto *channelNode = mChannelLookup.value(*evictedChannel);
        Q_ASSERT(channelNode);
        clearChannelMessages(channelNode, nodeIndex(channelNode));
    }
}

void MessageModel::clearChannelMessages(Node *channelNode, const QModelIndex &channelIndex)
{
    if (!channelNode->children.empty()) {
        beginRemoveRows(channelIndex, 0, channelNode->children.size() - 1);
        channelNode->children.clear();
        endRemoveRows();
    }
}

void MessageModel::doInsertMessage(Node *parentNode, const QVariantMap &message, InsertMode insertMode)
{
    NodePtr node{new Node, mCacheCleaner};
    node->type = EntityType::Message;
    node->parent = parentNode;
    node->data = message;

    Node *nodePtr = nullptr;
    if (insertMode == InsertMode::Append) {
        nodePtr = parentNode->children.emplace_back(std::move(node)).get();
    } else {
        nodePtr = parentNode->children.emplace(parentNode->children.begin(), std::move(node))->get();
    }

    // We might be appending a thread reply.
    if (parentNode->type == EntityType::Message) {
        parentNode = parentNode->parent;
    }
    Q_ASSERT(parentNode->type == EntityType::Channel);
    const auto channelId = parentNode->data.value(fieldChannelId).toString();
    const auto messageId = message[fieldMessageId].toString();
    mMessageLookup.insert(channelId + messageId, nodePtr);
    qCDebug(logMM).nospace() << "channelId=" << channelId << ", messageId=" << messageId
                             << ((insertMode == InsertMode::Append) ? " Appended" : " Prepended")
                             << " new message to model.";
}

void MessageModel::setChannelMessages(const ChannelID &channelId, const QVariantList &messages)
{
    auto *channelNode = mChannelLookup.value(channelId);
    if (channelNode == nullptr) {
        qCWarning(logMM).nospace() << "channelId=" << channelId << ", messageCount=" << messages.size()
                                   << " Received messages for channel which is not in the model!";
        return;
    }

    const auto channelIndex = nodeIndex(channelNode);
    clearChannelMessages(channelNode, channelIndex);

    beginInsertRows(channelIndex, 0, messages.size() - 1);
    std::for_each(messages.cbegin(), messages.cend(),
                  [this, channelNode](const auto &m) { doInsertMessage(channelNode, m.toMap()); });
    endInsertRows();
}

void MessageModel::appendChannelMessage(const ChannelID &channelId, const QVariantMap &message)
{
    auto *channelNode = mChannelLookup.value(channelId);
    if (channelNode == nullptr) {
        qCWarning(logMM).nospace() << "channelId=" << channelId << ", messageId=" << message[fieldMessageId].toString()
                                   << " Received channel message for channel which is not in the model!";
        return;
    }

    auto threadId = message.find(fieldThreadId);
    if (threadId == message.cend() || threadId->isNull()) {
        const auto channelIndex = nodeIndex(channelNode);
        beginInsertRows(channelIndex, channelNode->children.size(), channelNode->children.size());
        doInsertMessage(channelNode, message);
        endInsertRows();
    } else {
        auto *tlNode = mMessageLookup.value(channelId + threadId->toString());
        if (tlNode == nullptr) {
            // That's OK, we may receive a reply for a message that we don't currently track
            qCDebug(logMM).nospace() << "channelId=" << channelId << ", threadId=" << threadId->toString()
                                     << ", messageId=" << message[fieldMessageId].toString()
                                     << " Received message for a currently untracked thread.";
            return;
        }

        const auto tlIndex = nodeIndex(tlNode);
        beginInsertRows(tlIndex, tlNode->children.size(), tlNode->children.size());
        doInsertMessage(tlNode, message);
        endInsertRows();

        // The thread-leader message has changed
        Q_EMIT dataChanged(tlIndex, tlIndex);
    }
}

void MessageModel::updateChannelMessage(const ChannelID &channelId, const QVariantMap &message)
{
    auto *messageNode = mMessageLookup.value(channelId + message[fieldMessageId].toString());
    if (!messageNode) {
        qCDebug(logMM).nospace() << "channelId=" << channelId << ", messageId=" << message[fieldMessageId].toString()
                                 << " Ignoring update for an untracked message.";
        return; // That's OK, we may receive update for a message we don't currently track.
    }

    const auto messageIndex = nodeIndex(messageNode);
    messageNode->data = message;
    Q_EMIT dataChanged(messageIndex, messageIndex);
}

void MessageModel::prependChannelMessages(const ChannelID &channelId, const QVariantList &messages)
{
    auto *channelNode = mChannelLookup.value(channelId);
    if (channelNode == nullptr) {
        qCWarning(logMM).nospace() << "channelId=" << channelId << " Received messages to prepend to an unknown channel.";
        return;
    }

    const auto channelIndex = nodeIndex(channelNode);
    beginInsertRows(channelIndex, 0, messages.count() - 1);
    for (auto it = std::make_reverse_iterator(messages.cend()), end = std::make_reverse_iterator(messages.cbegin()); it != end; ++it) {
        doInsertMessage(channelNode, it->toMap(), InsertMode::Prepend);
    }
    endInsertRows();
}

void MessageModel::removeChannelMessage(const ChannelID &channelId, const MessageID &messageId)
{
    const auto *messageNode = mMessageLookup.value(channelId + messageId);
    if (messageNode == nullptr) {
        return;
    }

    auto *parentNode = messageNode->parent;
    const auto messageIt = std::find_if(parentNode->children.begin(), parentNode->children.end(),
                                        [messageNode](const auto &n) { return n.get() == messageNode; });
    Q_ASSERT(messageIt != parentNode->children.end());
    const auto messageIdx = std::distance(parentNode->children.begin(), messageIt);

    const auto parentIdx = nodeIndex(parentNode);
    beginRemoveRows(parentIdx, messageIdx, messageIdx);
    parentNode->children.erase(messageIt);
    endRemoveRows();

    if (parentNode->type == EntityType::Message) {
       Q_EMIT dataChanged(parentIdx, parentIdx);
    }
}

void MessageModel::setThreadMessages(const ChannelID &channelId, const ThreadID &threadId, const QVariantList &messages)
{
    auto *tlNode = mMessageLookup.value(channelId + threadId);
    if (tlNode == nullptr) {
        qCDebug(logMM) << "Received thread messages for channel" << channelId << ", thread leader" << threadId << ", which is not part of the model!";
        return;
    }

    const auto tlIndex = nodeIndex(tlNode);
    clearThreadMessages(tlNode, tlIndex);

    beginInsertRows(tlIndex, 0, messages.size() - 1);
    tlNode->children.reserve(messages.size());
    std::for_each(messages.cbegin(), messages.cend(), [this, tlNode, &channelId, &threadId](const auto &m) {
        const auto msg = m.toMap();
        if (msg["thread_ts"].toString() != threadId) {
            qCWarning(logMM) << "Thread message" << msg["ts"].toString() << "doesn't belong to current thread (channelId=" << channelId << ", threadId=" << threadId;
            return;
        }
        doInsertMessage(tlNode, m.toMap()); });
    endInsertRows();

    // Update the thread-leader message, since it has become a thread leader now
    dataChanged(tlIndex, tlIndex);
}

void MessageModel::clearThreadMessages(Node *tlNode, const QModelIndex &tlIndex)
{
    if (!tlNode->children.empty()) {
        beginRemoveRows(tlIndex, 0, tlNode->children.size() - 1);
        tlNode->children.clear();
        endRemoveRows();
    }
}

void MessageModel::openThread(const ChannelID &channelId, const ThreadID &threadId)
{
    if (const auto evicted = mThreadLRU.reference(channelId + threadId); evicted) {
        auto *tlNode = mMessageLookup.value(*evicted);
        Q_ASSERT(tlNode);
        clearThreadMessages(tlNode, nodeIndex(tlNode));
    }
}

QModelIndex MessageModel::channelIndex(const ChannelID &channelId) const
{
    const auto node = mChannelLookup.constFind(channelId);
    if (node == mChannelLookup.cend()) {
        return {};
    }

    return nodeIndex(*node);
}

QModelIndex MessageModel::messageIndex(const ChannelID &channelId, const MessageID &messageId) const
{
    const auto node = mMessageLookup.constFind(channelId + messageId);
    if (node == mMessageLookup.cend()) {
        return {};
    }

    return nodeIndex(*node);
}

void MessageModel::cleanupCache(Node *node)
{
    switch (node->type) {
    case EntityType::Invalid:
        break;
    case EntityType::Channel:
        mChannelLookup.remove(node->data.value(QStringLiteral("id")).toString());
        break;
    case EntityType::Message:
    case EntityType::Reply:
        mMessageLookup.remove(node->data.value(QStringLiteral("ts")).toString());
        break;
    }
}
