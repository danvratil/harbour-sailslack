/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */
#ifndef CHANNELMESSAGESMODEL_H
#define CHANNELMESSAGESMODEL_H

#include <QAbstractProxyModel>

//! Proxy model for the MessageModel that displays only messages from specified channel
class ChannelMessagesModel : public QAbstractProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QString channelId READ channelId WRITE setChannelId NOTIFY channelIdChanged)
public:
    //! Constructor.
    explicit ChannelMessagesModel(QObject *parent = nullptr);
    //! Destructor.
    ~ChannelMessagesModel() override = default;

    //! Sets the channel to display messages from
    void setChannelId(const QString &channelId);
    //! Returns the currently set channel
    QString channelId() const;

    //! \copydoc QAbstracyProxyModel::setSourceModel()
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    //! \copydoc QAbstractProxyModel::rowCount()
    int rowCount(const QModelIndex &parent = {}) const override;
    //! \copydoc QAbstractProxyModel::columnCount()
    int columnCount(const QModelIndex &parent = {}) const override;
    //! \copydoc QAbstractProxyModel::hasChildren()
    bool hasChildren(const QModelIndex &parent = {}) const override;
    //! \copydoc QAbstractProxyModel::parent()
    QModelIndex parent(const QModelIndex &child) const override;
    //! \copydoc QAbstractProxyModel::index()
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;

    //! \copydoc QAbstractProxyModel::mapToSource()
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    //! \copydoc QAbstractProxyModel::mapFromSource()
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

Q_SIGNALS:
    //! Emitted when the \c channelId property has changed
    void channelIdChanged(const QString &channelId);

private:
    void updateTopLevelIndex();

private:
    QString mChannelId;
    QPersistentModelIndex mSourceRootIndex;
};

#endif // CHANNELMESSAGESMODEL_H
