/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "channelmessagesmodel.h"
#include "messagemodel.h"
#include "signalfilter.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logModel, "harbour-sailslack.ChannelMessageModel")

ChannelMessagesModel::ChannelMessagesModel(QObject *parent)
    : QAbstractProxyModel{parent}
{}

void ChannelMessagesModel::setChannelId(const QString &channelId)
{
    if (mChannelId == channelId) {
        return;
    }

    mChannelId = channelId;
    updateTopLevelIndex();
    Q_EMIT channelIdChanged(mChannelId);
}

QString ChannelMessagesModel::channelId() const
{
    return mChannelId;
}

void ChannelMessagesModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (auto *oldSourceModel = this->sourceModel()) {
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

    const auto filter = [this](const QModelIndex &index) {
        return index == mSourceRootIndex;
    };
    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeInserted, this, SignalFilter{this, &ChannelMessagesModel::rowsAboutToBeInserted, filter});
    connect(sourceModel, &QAbstractItemModel::rowsInserted, this, SignalFilter{this, &ChannelMessagesModel::rowsInserted, filter});
    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeMoved, this, SignalFilter{this, &ChannelMessagesModel::rowsAboutToBeMoved, filter});
    connect(sourceModel, &QAbstractItemModel::rowsMoved, this, SignalFilter{this, &ChannelMessagesModel::rowsMoved, filter});
    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, SignalFilter{this, &ChannelMessagesModel::rowsAboutToBeRemoved, filter});
    connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, SignalFilter{this, &ChannelMessagesModel::rowsRemoved, filter});
    connect(sourceModel, &QAbstractItemModel::dataChanged, this, SignalFilter{this, &ChannelMessagesModel::dataChanged, filter, FilterBy::Parent});
    connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset, this, &ChannelMessagesModel::modelAboutToBeReset);
    connect(sourceModel, &QAbstractItemModel::modelReset, this, &ChannelMessagesModel::modelReset);

    if (!mChannelId.isEmpty()) {
        updateTopLevelIndex();
    }

    QAbstractProxyModel::setSourceModel(sourceModel);
}

void ChannelMessagesModel::updateTopLevelIndex()
{
    beginResetModel();

    mSourceRootIndex = QPersistentModelIndex{};
    if (!mChannelId.isEmpty()) {
        // FIXME: We should be able to do a much faster lookup than two scan using the MessageModel's internal lookup tables
        auto *model = sourceModel();
        for (int i = 0, count = model->rowCount(); i < count; ++i) {
            const auto index = model->index(i, 0);
            const auto channel = sourceModel()->data(index, static_cast<int>(MessageModel::Role::Channel)).toMap();
            if (channel[QStringLiteral("id")].toString() == mChannelId) {
                mSourceRootIndex = index;
                break;
            }
        }
    }
    endResetModel();

    if (!mSourceRootIndex.isValid() && !mChannelId.isEmpty()) {
        qCWarning(logModel) << "Failed to find channel" << mChannelId << "in the source model";
    }
}

int ChannelMessagesModel::rowCount(const QModelIndex &parent) const
{
    if (!mSourceRootIndex.isValid() || parent.isValid()) {
        return 0;
    }

    return sourceModel()->rowCount(mSourceRootIndex);
}

int ChannelMessagesModel::columnCount(const QModelIndex &parent) const
{
    if (!mSourceRootIndex.isValid() || parent.isValid()) {
        return 0;
    }

    return sourceModel()->rowCount(mSourceRootIndex);
}

bool ChannelMessagesModel::hasChildren(const QModelIndex &parent) const
{
    if (!mSourceRootIndex.isValid() || parent.isValid()) {
        return false;
    }

    return sourceModel()->hasChildren(mSourceRootIndex);
}

QModelIndex ChannelMessagesModel::parent(const QModelIndex &/*child*/) const
{
    return {};
}

QModelIndex ChannelMessagesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return {};
    }

    return createIndex(row, column);
}

QModelIndex ChannelMessagesModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid()) {
        return {};
    }

    return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), mSourceRootIndex);
}

QModelIndex ChannelMessagesModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (sourceIndex.parent() != mSourceRootIndex || !sourceIndex.isValid()) {
        return {};
    }

    return createIndex(sourceIndex.row(), sourceIndex.column());
}

