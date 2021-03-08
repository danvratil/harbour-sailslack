/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "channelsortmodel.h"
#include "messagemodel.h"

#include <QLocale>

namespace {

static QString fieldUnreadCount = QStringLiteral("unread_count");
static QString fieldIsStarred = QStringLiteral("is_starred");
static QString fieldCategory = QStringLiteral("category");
static QString fieldIsPrivate = QStringLiteral("is_private");
static QString fieldName = QStringLiteral("name");

static QString chatCategory = QStringLiteral("chat");


} // namespace

ChannelSortModel::ChannelSortModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool ChannelSortModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    const auto channelLeft = data(source_left, static_cast<int>(MessageModel::Role::Channel)).toMap();
    const auto channelRight = data(source_right, static_cast<int>(MessageModel::Role::Channel)).toMap();

    const int leftUnread = channelLeft[fieldUnreadCount].toInt();
    const int rightUnread = channelRight[fieldUnreadCount].toInt();
    if (leftUnread != rightUnread) {
        return leftUnread > rightUnread;
    }

    const bool leftStarred = channelLeft[fieldIsStarred].toBool();
    const bool rightStarred = channelRight[fieldIsStarred].toBool();
    if (leftStarred != rightStarred) {
        if (rightStarred) {
            return false;
        } else if (leftStarred) {
            return true;
        }
    }

    const auto leftCategory = channelLeft[fieldCategory].toString();
    const auto rightCategory = channelRight[fieldCategory].toString();
    if (leftCategory != rightCategory) {
        if (rightCategory == chatCategory) {
            return true;
        }
        return false;
    }

    const bool leftPrivate = channelLeft[fieldIsPrivate].toBool();
    const bool rightPrivate = channelRight[fieldIsPrivate].toBool();
    if (leftPrivate != rightPrivate) {
        if (!leftPrivate) {
            return true;
        }
        return false;
    }

    return channelLeft[fieldName].toString().localeAwareCompare(channelRight[fieldName].toString()) < 0;
}
