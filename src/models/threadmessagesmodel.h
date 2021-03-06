/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#ifndef THREADMESSAGESMODEL_H
#define THREADMESSAGESMODEL_H

#include <QAbstractProxyModel>
#include <QPersistentModelIndex>
#include <QTimer>

//! Proxy model for \c MessageModel that displays messages from specified thread
/*! The model always includes the thread-leader message, followed by the rest of the thread replies. */
class ThreadMessagesModel : public QAbstractProxyModel
{
    Q_OBJECT

    //! Channel ID to thread belongs to
    Q_PROPERTY(QString channelId READ channelId WRITE setChannelId NOTIFY channelIdChanged)
    //! ID of the thread to display
    Q_PROPERTY(QString threadId READ threadId WRITE setThreadId NOTIFY threadIdChanged)
public:
    //! Constructor.
    explicit ThreadMessagesModel(QObject *parent = nullptr);
    //! Destructor.
    ~ThreadMessagesModel() override = default;

    //! Sets the channel that the thread belongs to.
    void setChannelId(const QString &channelId);
    //! Returns the current channel.
    QString channelId() const { return mChannelId; }

    //! Sets ID of the thread that should be displayed in the model.
    void setThreadId(const QString &threadId);
    //! Returns ID of the currently displayed thread.
    QString threadId() const { return mThreadId; }

    //! \copydoc QAbstractProxyModel::setSourceModel()
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    //! \copydoc QAbstractProxyModel::rowCount()
    int rowCount(const QModelIndex &parent = {}) const override;
    //! \copydoc QAbstractProxyModel::columnCount()
    int columnCount(const QModelIndex &parent = {}) const override;
    //! \copydoc QAbstractProxyModel::hasChildren()
    bool hasChildren(const QModelIndex &parent = QModelIndex{}) const override;
    //! \copydoc QAbstractProxyModel::index()
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    //! \copydoc QAbstractProxyModel::parent()
    QModelIndex parent(const QModelIndex &child) const override;

    //! \copydoc QAbstractProxyModel::mapToSource()
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    //! \copydoc QAbstractProxyModel::mapFromSource()
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

Q_SIGNALS:
    //! Emitted when current channel is changed
    void channelIdChanged(const QString &channelId);
    //! Emitted when the current thread in the model changes
    void threadIdChanged(const QString &threadId);

private:
#if 0
    void onSourceRowsAboutToBeInserted(const QModelIndex &parent, int first, int last, QPrivateSignal);
    void onSourceRowsInserted(const QModelIndex &parent, int first, int last, QPrivateSignal);
    void onSourceRowsAboutToBeMoved(const QModelIndex &parent, int first, int last, const QModelIndex &dest, int newFirst, QPrivateSignal);
    void onSourceRowsMoved(const QModelIndex &parent, int first, int last, const QModelIndex &dest, int newFirst, QPrivateSignal);
    void onSourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last, QPrivateSignal);
    void onSourceRowsRemoved(const QModelIndex &parent, int first, int last, QPrivateSignal);
    void onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, QPrivateSignal);
#endif


    //! Resets the model to display only messages from the currently specified thread.
    void updateTopLevelIndex();

    QString mChannelId;
    QString mThreadId;
    QPersistentModelIndex mSourceRootIndex;
    QTimer mDelayedUpdate;
};

#endif // THREADMESSAGESMODEL_H
