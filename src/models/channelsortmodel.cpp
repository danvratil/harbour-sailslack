/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "channelsortmodel.h"
#include "channellistmodel.h"
#include "messagemodel.h"

#include <QLocale>

#include <numeric>
#include <algorithm>

#include <QDebug>
#include <QTimer>

namespace {

static QString fieldUnreadCount = QStringLiteral("unreadCount");
static QString fieldIsStarred = QStringLiteral("is_starred");
static QString fieldCategory = QStringLiteral("category");
static QString fieldName = QStringLiteral("name");

static std::array<QString, 3> sectionPreference{QStringLiteral("starred"), QStringLiteral("channel"), QStringLiteral("chat")};

int sectionIndex(const QString &section)
{
    return std::distance(sectionPreference.cbegin(), std::find(sectionPreference.cbegin(), sectionPreference.cend(), section));
}

} // namespace

ChannelSortModel::ChannelSortModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortRole(static_cast<int>(MessageModel::Role::Channel));
    QTimer::singleShot(0, this, [this]() { sort(0); });
}

bool ChannelSortModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    const auto channelLeft = sourceModel()->data(source_left, static_cast<int>(MessageModel::Role::Channel)).toMap();
    const auto channelRight = sourceModel()->data(source_right, static_cast<int>(MessageModel::Role::Channel)).toMap();

    // First of all: group channels by their section/category type
    const auto leftSection = sourceModel()->data(source_left, static_cast<int>(ChannelListModel::Role::Section)).toString();
    const auto rightSection = sourceModel()->data(source_right, static_cast<int>(ChannelListModel::Role::Section)).toString();
    const auto leftSectionPos = sectionIndex(leftSection);
    const auto rightSectionPos = sectionIndex(rightSection);
    if (leftSectionPos != rightSectionPos) {
        return leftSectionPos < rightSectionPos;
    }

    // Within a section, channel with most unread messages goes first
    // FIXME: Does this actually make sense? Should we sort by most-recently-unread instead?
    const int leftUnread = channelLeft[fieldUnreadCount].toInt();
    const int rightUnread = channelRight[fieldUnreadCount].toInt();
    if (leftUnread != rightUnread) {
        return leftUnread > rightUnread;
    }

    // If everything else is equal, order by name.
    return channelLeft[fieldName].toString().localeAwareCompare(channelRight[fieldName].toString()) < 0;
}
