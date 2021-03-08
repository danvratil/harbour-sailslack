/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include "lru.h"

#include <QAbstractItemModel>
#include <QString>

#include <memory>
#include <optional>

using ChannelID = QString;
using MessageID = QString;
using ThreadID = QString;

class ChannelMessagesModel;

//! Storage model for the entire conversation tree (channels, messages, threads)
/*! The model holds the entire tree of channels, messages and threads. To obtain
 * only part of the subtree (e.g. only channel list, or messages from a particular
 * channel or thread, use one of the accompanying proxy models.
 *
 * The model is being populated externally, but it acts more as a cache in the sense
 * that it will only keep messages for limited number of channels and threads. The
 * channels and threads are kept in LRU, so the lest recently used channel or thread
 * will get evicted from the cache when the cache is full.
 *
 * The UI should notify the model via \c openChannel() and \c openThread() methods when
 * a channel or thread is opened, to ensure that the LRU is up-to-date and the opened
 * channel is not accidentally evicted from the cache too soon.
 */
class MessageModel : public QAbstractItemModel
{
    Q_OBJECT

    struct Node;
public:
    //! The type of the entity stored in the current item
    enum class EntityType : uint8_t {
        Invalid = 0, //!< Invalid type
        Channel, //!< Curernt entity is a channel
        Message, //!< Current entity is a message
        Reply //!< Current entity is a reply.
    };
    Q_ENUM(EntityType);

    enum class Role {
        EntityType = Qt::UserRole + 1, //!< Returns \c EntityType enum value decribing the type of the entry

        Channel, //!< QVariantMap describing the current channel
        Message, //!< QVariantMap describing the current message

        // TODO: Expose more entity properties
        _User
    };
    Q_ENUM(Role);

    //! Constructs a new MessageModel.
    explicit MessageModel(QObject *parent = nullptr);
    //! Destructor.
    ~MessageModel() override;

    //!@{
    //! QAbstractItemModel API
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    // TODO: canFetchMore(), fetchMore()
    // TODO: itemData()
    //!@}

    //! Clears the entire model and sets given \c channels.
    /*! \param[in] channels Channels to populate the model with. */
    void setChannels(const QVariantList &channels);
    //! Appends channel to the model.
    /*! \param[in] channel to append to the model. */
    void addChannel(const QVariantMap &channel);
    //! Updates an existing model in the channel.
    /*! The model assumes that it already is populated with channels, so that the
     * channel exists in the model.
     * \param[in] channel Channel data to update. */
    void updateChannel(const QVariantMap &channel);
    //! Notifies the model that a channel has been opened in the GUI.
    /*! Ensures that the messages of the opened channel are not evicted from the
     * cache, and evicts messages of old channels.
     * \param[in] channelId ID of the opened channel. */
    void openChannel(const ChannelID &channelId);

    //! Populates the channel with messages.
    /*! Clears the existing messages from the channel and populates it with the given
     * messages.
     * \param[in] channelId ID of the channel the messages belong to
     * \param[in] messages The messages to populate the channel with */
    void setChannelMessages(const ChannelID &channelId, const QVariantList &messages);
    //! Appends a new message to channel.
    /*! \param[in] channelId ID of the channel to append the \c message to
     * \param[in] message Message to append */
    void appendChannelMessage(const ChannelID &channelId, const QVariantMap &message);
    //! Updates an existing message.
    /*! If the message doesn't exist in the model, it's silently ignored.
     * \param[in] channelId ID of the channel the messages belongs to
     * \param[in] message Message to update. */
    void updateChannelMessage(const ChannelID &channelId, const QVariantMap &message);
    //! Removes specified message from the model.
    /*! If the message doesn't exist in the model, it's silently ignored.
     * \param[in] channelId ID of the channel to remove the message from
     * \param[in] messageId ID of te message to remove */
    void removeChannelMessage(const ChannelID &channelId, const MessageID &messageId);

    //! Replaces existing thread replies with the given messages.
    /*! \param[in] channelId ID of the channel the thread belongs to
     * \param[in] threadID ID of the thread leader message
     * \param[in] messages The thread replies, excluding the thread-leader message. */
    void setThreadMessages(const ChannelID &channelId, const ThreadID &threadId, const QVariantList &messages);

    //! Notifies the model that a thread has been opened in the GUI.
    /*! Ensures that the messages in the opened thread are not evicted from the cache
     * and evicts messages of old threads.
     * \param[in] channelId ID of the channel the opened thread belongs to
     * \param[in] threadId ID of the message that is the thread-leader of the opened thread */
    void openThread(const ChannelID &channelId, const ThreadID &threadId);

private:
    //! Returns QModelIndex representing the given \c node
    QModelIndex nodeIndex(const Node *node) const;
    //! Removes references to \c node from \c mChannelLookup and \c mMessageLookup caches.
    /*! This method is called automatically by the \c NodePtr when it's destroyed. */
    void cleanupCache(Node *node);
    //! Removes all replies from specified thread from the model
    /*! \param[in] tlNode thread leader node
     * \param[in] tlIndex model index of the thread-leader message */
    void clearThreadMessages(Node *tlNode, const QModelIndex &tlIndex);
    //! Removes all messages from specified channel from the model
    void clearChannelMessages(Node *channelNode, const QModelIndex &channelIndex);

    //! Appends given channel to the model.
    void doAppendChannel(const QVariantMap &channel);
    //! Appends given messages to the model.
    void doAppendMessage(Node *parentNode, const QVariantMap &message);

    //! Helper structure to call \c MessageModel::cleanupCache() when \c NodePtr is destroyed.
    struct CacheCleanerHelper {
        inline explicit CacheCleanerHelper(MessageModel *model): model{model} {}
        void operator()(Node *node);
    private:
        MessageModel *model;
    };

    using NodePtr = std::unique_ptr<Node, CacheCleanerHelper>;
    struct Node {
        QVariantMap data;
        std::vector<NodePtr> children;
        Node *parent = {};
        EntityType type = EntityType::Invalid;
    };

    CacheCleanerHelper mCacheCleaner;
    NodePtr mRootNode;
    QHash<ChannelID, Node *> mChannelLookup;
    QHash<MessageID, Node *> mMessageLookup;

    LRU<ChannelID, 10> mChannelLRU;
    LRU<ThreadID, 10> mThreadLRU;

    friend class ChannelMessagesModel;
};

#endif // MESSAGEMODEL_H
