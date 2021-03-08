/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#pragma once

#include <QSortFilterProxyModel>

class ChannelSortModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
   explicit ChannelSortModel(QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

};
