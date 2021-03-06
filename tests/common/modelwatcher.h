/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#pragma once

#include <QSignalSpy>
#include <QTest>

class QAbstractItemModel;

class ModelWatcher
{
public:
    enum Signal {
        None,
        ModelReset,
        RowsAdded,
        RowsMoved,
        RowsRemoved,
        DataChanged
    };

    explicit ModelWatcher(QAbstractItemModel *model);

    bool empty() const
    {
        return emptyExcept(None);
    }

    bool emptyExcept(Signal exclude) const
    {
        if (!modelResetSpy.empty() && exclude != ModelReset) {
            QTest::qFail("ModelReset signal spy expected to be empty, but is not!", __FILE__, __LINE__);
            return false;
        }
        if (!rowsAddedSpy.empty() && exclude != RowsAdded) {
            QTest::qFail("RowsAdded signal spy expected to be empty, but is not!", __FILE__, __LINE__);
            return false;
        }
        if (!rowsMovedSpy.empty() && exclude != RowsMoved) {
            QTest::qFail("RowsMoved signal spy expected to be empty, but is not!", __FILE__, __LINE__);
            return false;
        }
        if (!rowsRemovedSpy.empty() && exclude != RowsRemoved) {
            QTest::qFail("RowsRemoved signal spy expected to be empty, but is not!", __FILE__, __LINE__);
            return false;
        }
        if (!dataChangedSpy.empty() && exclude != DataChanged) {
            QTest::qFail("DataChanged signal spy expected to be empty, but is not!", __FILE__, __LINE__);
            return false;
        }

        return true;
   }

    void clear()
    {
        modelResetSpy.clear();
        rowsAddedSpy.clear();
        rowsMovedSpy.clear();
        rowsRemovedSpy.clear();
        dataChangedSpy.clear();
   }

    QSignalSpy modelResetSpy;
    QSignalSpy rowsAddedSpy;
    QSignalSpy rowsMovedSpy;
    QSignalSpy rowsRemovedSpy;
    QSignalSpy dataChangedSpy;
};
