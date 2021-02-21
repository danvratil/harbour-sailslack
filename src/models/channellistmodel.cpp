/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "channellistmodel.h"

ChannelListModel::ChannelListModel(QObject *parent):
    QIdentityProxyModel{parent}
{}

int ChannelListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : QIdentityProxyModel::rowCount(parent);
}

bool ChannelListModel::hasChildren(const QModelIndex &parent) const
{
    return parent.isValid() ? false : QIdentityProxyModel::hasChildren(parent);
}

QModelIndex ChannelListModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return {};
}

QModelIndex ChannelListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return {};
    }

    return QIdentityProxyModel::index(row, column, parent);
}
