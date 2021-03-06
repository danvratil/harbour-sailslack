/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include <QObject>
#include <QTest>

#include "messagemodel.h"
#include "channellistmodel.h"
#include "modeltest.h"

class ChannelListModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testChannelPopulation()
    {
        MessageModel messageModel;
        ChannelListModel channelModel;
        channelModel.setSourceModel(&messageModel);
        ModelTest test{&messageModel};
        ModelWatcher watcher{&channelModel};

        QCOMPARE(channelModel.rowCount(), 0);
        test.populateChannels(10);

        QCOMPARE(channelModel.rowCount(), 10);
        QCOMPARE(watcher.modelResetSpy.size(), 1);
        QVERIFY(watcher.rowsAddedSpy.empty());
        QVERIFY(watcher.rowsMovedSpy.empty());
        QVERIFY(watcher.rowsRemovedSpy.empty());
        QVERIFY(watcher.dataChangedSpy.empty());
    }

    void testChannelAdded()
    {
        MessageModel messageModel;
        ChannelListModel channelModel;
        channelModel.setSourceModel(&messageModel);
        ModelTest test{&messageModel};
        ModelWatcher watcher{&channelModel};

        test.populateChannels(5);
        QCOMPARE(watcher.modelResetSpy.size(), 1);
        QCOMPARE(channelModel.rowCount(), 5);
        test.addChannel(QStringLiteral("channel-1"), QStringLiteral("Channel 1"));
        QCOMPARE(channelModel.rowCount(), 6);

        QCOMPARE(watcher.rowsAddedSpy.size(), 1);
        const auto rowsAdded = watcher.rowsAddedSpy.takeFirst();
        QCOMPARE(rowsAdded[0].toModelIndex(), QModelIndex{});
        QCOMPARE(rowsAdded[1].toInt(), 5);
        QCOMPARE(rowsAdded[2].toInt(), 5);

        QVERIFY(watcher.rowsMovedSpy.empty());
        QVERIFY(watcher.rowsRemovedSpy.empty());
        QVERIFY(watcher.dataChangedSpy.empty());
    }

    void testChannelUpdated()
    {
        MessageModel messageModel;
        ChannelListModel channelModel;
        channelModel.setSourceModel(&messageModel);
        ModelTest test{&messageModel};
        ModelWatcher watcher{&channelModel};

        test.populateChannels(10);
        QCOMPARE(watcher.modelResetSpy.size(), 1);
        test.updateChannel(5, QStringLiteral("New Name"));

        const auto channelIdx = channelModel.index(5, 0);
        QVERIFY(channelIdx.isValid());
        QCOMPARE(watcher.dataChangedSpy.size(), 1);
        const auto dataChanged = watcher.dataChangedSpy.takeFirst();
        QCOMPARE(dataChanged[0].toModelIndex(), channelIdx);
        QCOMPARE(dataChanged[1].toModelIndex(), channelIdx);
        const auto channel = channelModel.data(channelIdx, static_cast<int>(MessageModel::Role::Channel)).toMap();
        QCOMPARE(channel[QStringLiteral("name")].toString(), QStringLiteral("New Name"));

        QVERIFY(watcher.rowsAddedSpy.empty());
        QVERIFY(watcher.rowsMovedSpy.empty());
        QVERIFY(watcher.rowsRemovedSpy.empty());
    }

    void testMessageUpdateIsIgnored()
    {
        MessageModel messageModel;
        ChannelListModel channelModel;
        channelModel.setSourceModel(&messageModel);
        ModelTest test{&messageModel};
        ModelWatcher watcher{&channelModel};

        test.populateChannels(10);
        QCOMPARE(watcher.modelResetSpy.size(), 1);

        test.populateChannelMessages(5, 10);
        const auto channelIdx = channelModel.index(5, 0);
        // The model should not show any descendants of the channel
        QCOMPARE(channelModel.rowCount(channelIdx), 0);
        QVERIFY(!channelModel.hasChildren(channelIdx));
        QVERIFY(watcher.rowsAddedSpy.empty());
        QVERIFY(watcher.rowsMovedSpy.empty());
        QVERIFY(watcher.rowsRemovedSpy.empty());
        QVERIFY(watcher.dataChangedSpy.empty());

        test.populateThreadMessages(5, 5, 10);
        // The model should not show any descendants of the channel
        QCOMPARE(channelModel.rowCount(channelIdx), 0);
        QVERIFY(!channelModel.hasChildren(channelIdx));
        QVERIFY(watcher.rowsAddedSpy.empty());
        QVERIFY(watcher.rowsMovedSpy.empty());
        QVERIFY(watcher.rowsRemovedSpy.empty());
        QVERIFY(watcher.dataChangedSpy.empty());
    }
};

QTEST_GUILESS_MAIN(ChannelListModelTest)

#include "tst_channellistmodeltest.moc"
