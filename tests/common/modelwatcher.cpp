/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "modelwatcher.h"

#include <QAbstractItemModel>

ModelWatcher::ModelWatcher(QAbstractItemModel *model)
    : modelResetSpy(model, &QAbstractItemModel::modelReset)
    , rowsAddedSpy(model, &QAbstractItemModel::rowsInserted)
    , rowsMovedSpy(model, &QAbstractItemModel::rowsMoved)
    , rowsRemovedSpy(model, &QAbstractItemModel::rowsRemoved)
    , dataChangedSpy(model, &QAbstractItemModel::dataChanged)
{
}
