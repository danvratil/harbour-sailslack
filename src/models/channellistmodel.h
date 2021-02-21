/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */
#ifndef CHANNELLISTMODEL_H
#define CHANNELLISTMODEL_H

#include <QIdentityProxyModel>

//! Proxy model for MessageModel that only displays channels
/*! This model is designed to be placed on top of the \c MessageModel
 * and only display the list of channels.
 *
 * \todo Add support for section (read/unread/etc.)
 */
class ChannelListModel : public QIdentityProxyModel
{
    Q_OBJECT
public:
    //! Constructs a new \c ChannelListModel
    explicit ChannelListModel(QObject *parent = nullptr);
    //! Destructor.
    ~ChannelListModel() override = default;

    //! \copydoc QIdentityProxyModel::rowCount()
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    //! \copydoc QIdentityProxyModel::hasChildren()
    bool hasChildren(const QModelIndex &parent) const override;
    //! \copydoc QIdentityProxyModel::parent()
    QModelIndex parent(const QModelIndex &child) const override;
    //! \copydoc QIdentityProxyModel::index()
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
};

#endif // CHANNELLISTMODEL_H
