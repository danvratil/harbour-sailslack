/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "messagemodel.h"

#include <QTest>

class ModelTest : public QObject
{
    Q_OBJECT
public:
    explicit ModelTest(QAbstractItemModel *model)
        : mModel(model)
    {
        //connect(model, &QAbstractItemModel::)
    }


private:
    QAbstractItemModel *mModel = nullptr;
};

class MessageModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testPopulation() {
        MessageModel model;
    }
};

QTEST_GUILESS_MAIN(MessageModelTest)

#include "tst_messagemodel.moc"
