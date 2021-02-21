/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "messagemodel.h"

#include <QLoggingCategory>

#include <vector>

Q_LOGGING_CATEGORY(logMM, "harbour-sailslack.MessageModel")

namespace {
    static const QString fieldChannelId = QStringLiteral("id");
    static const QString fieldMessageId = QStringLiteral("ts");
} // namespace

void MessageModel::CacheCleanerHelper::operator()(Node *node)
{
    model->cleanupCache(node);
    delete node;
}

MessageModel::MessageModel(QObject *parent)
    : QAbstractItemModel{parent}
    , mRootNode{NodePtr{new Node, CacheCleanerHelper{this}}}
{}

MessageModel::~MessageModel() = default;

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
    NodePtr channelNode{new Node, CacheCleanerHelper{this}};
    channelNode->type = EntityType::Channel;
    channelNode->parent = mRootNode.get();
    channelNode->data = channel;

    auto &ref = mRootNode->children.emplace_back(std::move(channelNode));
    mChannelLookup.insert(channel.value(fieldChannelId).toString(), ref.get());
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

void MessageModel::doAppendMessage(Node *channelNode, const QVariantMap &message)
{
    NodePtr node{new Node, CacheCleanerHelper{this}};
    node->type = EntityType::Message;
    node->parent = channelNode;
    node->data = message;

    auto &ref = channelNode->children.emplace_back(std::move(node));
    const auto channelId = channelNode->data.value(fieldChannelId).toString();
    mMessageLookup.insert(channelId + message[fieldMessageId].toString(), ref.get());
}

void MessageModel::setChannelMessages(const ChannelID &channelId, const QVariantList &messages)
{
    auto *channelNode = mChannelLookup.value(channelId);
    if (channelNode == nullptr) {
        qCWarning(logMM) << "Received channel messages for channel" << channelId << ", which is not in the model!";
        return;
    }

    const auto channelIndex = nodeIndex(channelNode);
    clearChannelMessages(channelNode, channelIndex);

    beginInsertRows(channelIndex, 0, messages.size() - 1);
    std::for_each(messages.cbegin(), messages.cend(),
                  [this, channelNode](const auto &m) { doAppendMessage(channelNode, m.toMap()); });
    endInsertRows();
}

void MessageModel::appendChannelMessage(const ChannelID &channelId, const QVariantMap &message)
{
    auto *channelNode = mChannelLookup.value(channelId);
    if (channelNode == nullptr) {
        qCWarning(logMM) << "Received channel message for channel" << channelId << ", which is not in the model!";
        return;
    }

    const auto channelIndex = nodeIndex(channelNode);
    beginInsertRows(channelIndex, channelNode->children.size(), channelNode->children.size());
    doAppendMessage(channelNode, message);
    endInsertRows();
}

void MessageModel::updateChannelMessage(const ChannelID &channelId, const QVariantMap &message)
{
    auto *messageNode = mMessageLookup.value(channelId + message[fieldMessageId].toString());
    if (!messageNode) {
        return; // That's OK, we may receive update for a message we don't currently track.
    }

    const auto messageIndex = nodeIndex(messageNode);
    messageNode->data = message;
    Q_EMIT dataChanged(messageIndex, messageIndex);
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

    beginRemoveRows(nodeIndex(parentNode), messageIdx, messageIdx);
    parentNode->children.erase(messageIt);
    endRemoveRows();
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
    std::for_each(messages.cbegin(), messages.cend(), [this, tlNode](const auto &m) { doAppendMessage(tlNode, m.toMap()); });
    endInsertRows();
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
