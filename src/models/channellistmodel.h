/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#pragma once

#include <QAbstractProxyModel>
#include "messagemodel.h"

//! Proxy model for MessageModel that only displays channels
/*! This model is designed to be placed on top of the \c MessageModel
 * and only display the list of channels.
 *
 * \todo Add support for section (read/unread/etc.)
 */
class ChannelListModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    enum class Role {
        Section = static_cast<int>(MessageModel::Role::_User) + 1
    };


    //! Constructs a new \c ChannelListModel
    explicit ChannelListModel(QObject *parent = nullptr);
    //! Destructor.
    ~ChannelListModel() override = default;

    QHash<int, QByteArray> roleNames() const override;

    //! \copydoc QAbstractProxyModel::setSourceModel()
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    //! \copydoc QAbstractProxyModel::rowCount()
    int rowCount(const QModelIndex &parent = {}) const override;
    //! \copydoc QAbstractProxyModel::columnCount()
    int columnCount(const QModelIndex &parent = {}) const override;
    //! \copydoc QAbstractProxyModel::hasChildren()
    bool hasChildren(const QModelIndex &parent) const override;
    //! \copydoc QAbstractProxyMode::parent()
    QModelIndex parent(const QModelIndex &child) const override;
    //! \copydoc QAbstractProxyModel::index()
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;

    // \copydoc QAbstracyProxyModel::data
    QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const override;


    //! \copydoc QAbstractProxyModel::mapToSource()
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    //! \copydoc QAbstractProxyModel::mapFromSource()
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
};
