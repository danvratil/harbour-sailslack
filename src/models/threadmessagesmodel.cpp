/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "threadmessagesmodel.h"
#include "messagemodel.h"
#include "signalfilter.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logTM, "harbour-sailslack.ThreadMessageModel")

namespace {

static constexpr int threadLeaderMessage = 1;

}

ThreadMessagesModel::ThreadMessagesModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
    mDelayedUpdate.setInterval(10);
    mDelayedUpdate.setSingleShot(true);
    connect(&mDelayedUpdate, &QTimer::timeout, this, &ThreadMessagesModel::updateTopLevelIndex);
}

void ThreadMessagesModel::setChannelId(const QString &channelId)
{
    if (mChannelId == channelId) {
        return;
    }

    mChannelId = channelId;
    if (!mDelayedUpdate.isActive()) {
        mDelayedUpdate.start();
    }

    Q_EMIT channelIdChanged(mChannelId);
}

void ThreadMessagesModel::setThreadId(const QString &threadId)
{
    if (mThreadId == threadId) {
        return;
    }

    mThreadId = threadId;
    if (!mDelayedUpdate.isActive()) {
        mDelayedUpdate.start();
    }

    Q_EMIT threadIdChanged(mThreadId);
}

void ThreadMessagesModel::setSourceModel(QAbstractItemModel *sourceModel)
{
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
        const auto threadFilter = [this](const QModelIndex &index) {
            return mSourceRootIndex.isValid() && index == mSourceRootIndex;
        };
        const auto threadLeaderFilter = [this](const QModelIndex &index) {
            return mSourceRootIndex.isValid() && (index == mSourceRootIndex || index.parent() == mSourceRootIndex);
        };
        const auto mapper = [this](auto &&val) {
            using T = std::remove_cv_t<std::remove_reference_t<decltype(val)>>;
            if constexpr(std::is_same_v<T, QModelIndex>) {
                return (val == mSourceRootIndex) ? QModelIndex{} : val;
            } else if constexpr(std::is_same_v<T, int>) {
                return val + 1;
            } else {
                return val;
            }
        };

        connect(sourceModel, &QAbstractItemModel::rowsAboutToBeInserted, this,
                SignalFilter{this, &ThreadMessagesModel::rowsAboutToBeInserted, threadFilter, FilterBy::Index, mapper});
        connect(sourceModel, &QAbstractItemModel::rowsInserted, this,
                SignalFilter{this, &ThreadMessagesModel::rowsInserted, threadFilter, FilterBy::Index, mapper});
        connect(sourceModel, &QAbstractItemModel::rowsAboutToBeMoved, this,
                SignalFilter{this, &ThreadMessagesModel::rowsAboutToBeMoved, threadFilter, FilterBy::Index, mapper});
        connect(sourceModel, &QAbstractItemModel::rowsMoved, this,
                SignalFilter{this, &ThreadMessagesModel::rowsMoved, threadFilter, FilterBy::Index, mapper});
        connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this,
                SignalFilter{this, &ThreadMessagesModel::rowsAboutToBeRemoved, threadFilter, FilterBy::Index, mapper});
        connect(sourceModel, &QAbstractItemModel::rowsRemoved, this,
                SignalFilter{this, &ThreadMessagesModel::rowsRemoved, threadFilter, FilterBy::Index, mapper});
        connect(sourceModel, &QAbstractItemModel::dataChanged, this,
                SignalFilter{this, &ThreadMessagesModel::dataChanged, threadLeaderFilter});
        connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset, this, &ThreadMessagesModel::modelAboutToBeReset);
        connect(sourceModel, &QAbstractItemModel::modelReset, this, &ThreadMessagesModel::modelReset);
    }

    QAbstractProxyModel::setSourceModel(sourceModel);
}

int ThreadMessagesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return threadLeaderMessage + sourceModel()->rowCount(mSourceRootIndex);
}

int ThreadMessagesModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return sourceModel()->columnCount(mSourceRootIndex);
}

bool ThreadMessagesModel::hasChildren(const QModelIndex &) const
{
    return false;
}

QModelIndex ThreadMessagesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return {};
    }

    return createIndex(row, column);
}

QModelIndex ThreadMessagesModel::parent(const QModelIndex &) const
{
    return QModelIndex{};
}

QModelIndex ThreadMessagesModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (proxyIndex.parent().isValid()) {
        return {};
    }

    if (proxyIndex.row() == 0) {
        return mSourceRootIndex;
    }

    return sourceModel()->index(proxyIndex.row() - 1, proxyIndex.column(), mSourceRootIndex);
}

QModelIndex ThreadMessagesModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (sourceIndex == mSourceRootIndex) {
        return createIndex(0, 0);
    }

    if (sourceIndex.parent() == mSourceRootIndex) {
        return createIndex(sourceIndex.row() + 1, sourceIndex.column());
    }

    return {};
}

void ThreadMessagesModel::updateTopLevelIndex()
{
    const auto findIdx = [](QAbstractItemModel *model, const QModelIndex &parent, auto match) -> QModelIndex {
        for (int i = 0, count = model->rowCount(parent); i < count; ++i) {
            const auto index = model->index(i, 0, parent);
            if (match(index)) {
                return index;
            }
        }

        return {};
    };

    beginResetModel();
    if (!mChannelId.isEmpty()) {
        // FIXME: We should be able to do a much faster lookup than two linear scan using the MessageModel's internal lookup tables
        mSourceRootIndex = findIdx(sourceModel(), QModelIndex{}, [this](const QModelIndex &index) {
            const auto channel = sourceModel()->data(index, static_cast<int>(MessageModel::Role::Channel)).toMap();
            return channel[QStringLiteral("id")].toString() == mChannelId;
        });

        if (mSourceRootIndex.isValid() && !mThreadId.isEmpty()) {
            mSourceRootIndex = findIdx(sourceModel(), mSourceRootIndex, [this](const QModelIndex &index) {
                const auto msg = sourceModel()->data(index, static_cast<int>(MessageModel::Role::Message)).toMap();
                return msg[QStringLiteral("ts")].toString() == mThreadId;
            });
        }
    }
    endResetModel();

    if (!mSourceRootIndex.isValid()) {
        if (!mChannelId.isEmpty()) {
            qCWarning(logTM) << "Failed to find channel" << mChannelId << "in the source model";
        }
        if (!mThreadId.isEmpty()) {
            qCWarning(logTM) << "Failed to find message" << mThreadId << "in the source model";
        }
    }
}