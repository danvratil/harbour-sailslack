/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "channellistmodel.h"
#include "signalfilter.h"

ChannelListModel::ChannelListModel(QObject *parent):
    QAbstractProxyModel{parent}
{}

void ChannelListModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    beginResetModel();
    if (auto *oldSourceModel = this->sourceModel(); oldSourceModel != nullptr) {
        disconnect(oldSourceModel, &QAbstractItemModel::rowsAboutToBeInserted, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsInserted, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsAboutToBeMoved, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsMoved, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsRemoved, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::dataChanged, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::modelAboutToBeReset, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::modelReset, this, nullptr);
    }

    if (sourceModel) {
        static constexpr auto filter = [](const QModelIndex &index) { return index.parent().isValid(); };
        connect(sourceModel, &QAbstractItemModel::rowsAboutToBeInserted, this, SignalFilter{this, &ChannelListModel::rowsAboutToBeInserted, filter});
        connect(sourceModel, &QAbstractItemModel::rowsInserted, this, SignalFilter{this, &ChannelListModel::rowsInserted, filter});
        connect(sourceModel, &QAbstractItemModel::rowsAboutToBeMoved, this, SignalFilter{this, &ChannelListModel::rowsAboutToBeMoved, filter});
        connect(sourceModel, &QAbstractItemModel::rowsMoved, this, SignalFilter{this, &ChannelListModel::rowsMoved, filter});
        connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, SignalFilter{this, &ChannelListModel::rowsAboutToBeRemoved, filter});
        connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, SignalFilter{this, &ChannelListModel::rowsRemoved, filter});
        connect(sourceModel, &QAbstractItemModel::dataChanged, this, SignalFilter{this, &ChannelListModel::dataChanged, filter, FilterBy::Parent});
        connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset, this, &ChannelListModel::modelAboutToBeReset);
        connect(sourceModel, &QAbstractItemModel::modelReset, this, &ChannelListModel::modelReset);
    }

    QAbstractProxyModel::setSourceModel(sourceModel);
    resetInternalData();
    endResetModel();
}

QHash<int, QByteArray> ChannelListModel::roleNames() const
{
    if (sourceModel() == nullptr) {
        return {};
    }

    auto roles = sourceModel()->roleNames();
    roles[static_cast<int>(Role::Section)] = "section";
    return roles;
}

int ChannelListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : sourceModel()->rowCount();
}

int ChannelListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : sourceModel()->columnCount();
}

bool ChannelListModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid();
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

    return createIndex(row, column);
}

namespace {

static const QString fieldIsStarred = QStringLiteral("is_starred");
static const QString fieldCategory = QStringLiteral("category");

static const QString categoryStarred = QStringLiteral("starred");

QString channelSection(const QVariantMap &channel)
{
    if (channel[fieldIsStarred].toBool()) {
        return categoryStarred;
    }

    return channel[fieldCategory].toString();
}

} // namespace

QVariant ChannelListModel::data(const QModelIndex &proxyIndex, int role) const
{
    if (role == static_cast<int>(ChannelListModel::Role::Section)) {
        return channelSection(data(proxyIndex, static_cast<int>(MessageModel::Role::Channel)).toMap());
    }

    return QAbstractProxyModel::data(proxyIndex, role);
}

QModelIndex ChannelListModel::mapToSource(const QModelIndex &index) const
{
    return index.isValid() ? sourceModel()->index(index.row(), index.column(), {}) : QModelIndex{};
}

QModelIndex ChannelListModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (sourceIndex.parent().isValid() || !sourceIndex.isValid()) {
        return {};
    }

    return index(sourceIndex.row(), sourceIndex.column());
}

