/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "channelmessagesmodel.h"
#include "messagemodel.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logModel, "harbour-sailslack.ChannelMessageModel")

ChannelMessagesModel::ChannelMessagesModel(QObject *parent)
    : QAbstractProxyModel{parent}
{
}

void ChannelMessagesModel::setChannelId(const QString &channelId)
{
    if (mChannelId == channelId) {
        return;
    }

    beginResetModel();
    mChannelId = channelId;
    mSourceRootIndex = QPersistentModelIndex{};
    updateTopLevelIndex();
    endResetModel();
    Q_EMIT channelIdChanged(mChannelId);
}

QString ChannelMessagesModel::channelId() const
{
    return mChannelId;
}

void ChannelMessagesModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    beginResetModel();
    if (auto *oldSourceModel = this->sourceModel()) {
        disconnect(oldSourceModel, &QAbstractItemModel::rowsAboutToBeInserted, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsInserted, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsAboutToBeMoved, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsMoved, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::rowsRemoved, this, nullptr);
        disconnect(oldSourceModel, &QAbstractItemModel::dataChanged, this, nullptr);
    }

    if (sourceModel && !mChannelId.isEmpty()) {
        updateTopLevelIndex();
    }

    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeInserted, this, &ChannelMessagesModel::onRowsAboutToBeInserted);
    connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &ChannelMessagesModel::onRowsInserted);
    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeMoved, this, &ChannelMessagesModel::onRowsAboutToBeMoved);
    connect(sourceModel, &QAbstractItemModel::rowsMoved, this, &ChannelMessagesModel::onRowsMoved);
    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &ChannelMessagesModel::onRowsAboutToBeRemoved);
    connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, &ChannelMessagesModel::onRowsRemoved);
    connect(sourceModel, &QAbstractItemModel::dataChanged, this, &ChannelMessagesModel::onDataChanged);

    QAbstractProxyModel::setSourceModel(sourceModel);
    endResetModel();
}

void ChannelMessagesModel::updateTopLevelIndex()
{
    auto srcModel = sourceModel();
    for (int i = 0; i < srcModel->rowCount(); ++i) {
        const auto index = srcModel->index(i, 0);
        const auto channel = srcModel->data(index, static_cast<int>(MessageModel::Role::Channel)).toMap();
        if (channel[QStringLiteral("id")].toString() == mChannelId) {
            mSourceRootIndex = index;
            return;
        }
    }

    qCWarning(logModel) << "Failed to find channel" << mChannelId << "in the source model";
}

int ChannelMessagesModel::rowCount(const QModelIndex &parent) const
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
    return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), mSourceRootIndex);
}

QModelIndex ChannelMessagesModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (sourceIndex.parent() != mSourceRootIndex) {
        return {};
    }

    return createIndex(sourceIndex.row(), sourceIndex.column());
}

void ChannelMessagesModel::onRowsAboutToBeInserted(const QModelIndex &sourceParent, int first, int last)
{
    if (sourceParent == mSourceRootIndex) {
        Q_EMIT rowsAboutToBeInserted({}, first, last, {});
    }
}

void ChannelMessagesModel::onRowsInserted(const QModelIndex &sourceParent, int first, int last)
{
    if (sourceParent == mSourceRootIndex) {
        Q_EMIT rowsInserted({}, first, last, {});
    }
}

void ChannelMessagesModel::onRowsAboutToBeMoved(const QModelIndex &sourceParent, int first, int last, const QModelIndex &sourceDestination)
{
    if (sourceParent == mSourceRootIndex && sourceDestination != mSourceRootIndex) {
        Q_EMIT rowsAboutToBeRemoved({}, first, last, {});
    } else if (sourceParent != mSourceRootIndex && sourceDestination == mSourceRootIndex) {
        Q_EMIT rowsAboutToBeInserted({}, first, last, {});
    }
}

void ChannelMessagesModel::onRowsMoved(const QModelIndex &sourceParent, int first, int last, const QModelIndex &sourceDestination)
{
    if (sourceParent == mSourceRootIndex && sourceDestination != mSourceRootIndex) {
        Q_EMIT rowsRemoved({}, first, last, {});
    } else if (sourceParent != mSourceRootIndex && sourceDestination == mSourceRootIndex) {
        Q_EMIT rowsInserted({}, first, last, {});
    }
}

void ChannelMessagesModel::onRowsAboutToBeRemoved(const QModelIndex &sourceParent, int first, int last)
{
    if (sourceParent == mSourceRootIndex) {
        Q_EMIT rowsAboutToBeRemoved({}, first, last, {});
    }
}

void ChannelMessagesModel::onRowsRemoved(const QModelIndex &sourceParent, int first, int last)
{
    if (sourceParent == mSourceRootIndex) {
        Q_EMIT rowsRemoved({}, first, last, {});
    }
}

void ChannelMessagesModel::onDataChanged(const QModelIndex &sourceTopLeft, const QModelIndex &sourceBottomRight, const QVector<int> &roles)
{
    if (sourceTopLeft.parent() == mSourceRootIndex) {
        Q_ASSERT(sourceBottomRight.parent() == mSourceRootIndex);

        Q_EMIT dataChanged(index(sourceTopLeft.row(), sourceTopLeft.column(), {}),
                           index(sourceBottomRight.row(), sourceBottomRight.column(), {}),
                           roles);
    }
}
