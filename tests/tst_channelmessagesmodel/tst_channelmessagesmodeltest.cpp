/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include <QtTest>
#include "modeltest.h"
#include "modelwatcher.h"
#include "channelmessagesmodel.h"
#include "messagemodel.h"

class Test
{
public:
    Test()
    {
        model.setSourceModel(&messageModel);
        watcher.clear();
    }

    void populateChannels(int channelCount)
    {
        modelTest.populateChannels(channelCount);
        QCOMPARE(watcher.modelResetSpy.size(), 1);
        QVERIFY(watcher.emptyExcept(ModelWatcher::ModelReset));
        watcher.clear();
    }

    void populateChannelMessages(int channelRow, int messageCount)
    {
        modelTest.populateChannelMessages(channelRow, messageCount);
    }

    void populateChannelThreadMessages(int channelRow, int messageRow, int threadMsgCount)
    {
        modelTest.populateThreadMessages(channelRow, messageRow, threadMsgCount);
    }

    void setChannelId(const QString &channelId)
    {
        model.setChannelId(channelId);
        QTest::qWait(20);
        QCOMPARE(watcher.modelResetSpy.size(), 1);
        QVERIFY(watcher.emptyExcept(ModelWatcher::ModelReset));
        watcher.clear();
    }

    void appendChannelMessage(int channelRow, const QString &text)
    {
        const auto rowCount = model.rowCount();
        modelTest.appendChannelMessage(channelRow, QString::number(rowCount), text);
        QCOMPARE(model.rowCount(), rowCount + 1);
        QCOMPARE(watcher.rowsAddedSpy.size(), 1);
        const auto rowsAdded = watcher.rowsAddedSpy.takeFirst();
        QCOMPARE(rowsAdded[0].toModelIndex(), QModelIndex{});
        QCOMPARE(rowsAdded[1].toInt(), 10);
        QCOMPARE(rowsAdded[1].toInt(), 10);

        const auto msgIdx = model.index(10, 0);
        const auto msg = model.data(msgIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
        QVERIFY(!msg.empty());
        QCOMPARE(msg[QStringLiteral("ts")].toString(), QStringLiteral("10"));

        QVERIFY(watcher.emptyExcept(ModelWatcher::RowsAdded));
    }

    void updateChannelMessage(int messageRow, const QString &newText)
    {
        const auto msgIdx = model.index(messageRow, 0);
        QVERIFY(msgIdx.isValid());
        auto msg = model.data(msgIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
        msg[QStringLiteral("text")] = newText;

        messageModel.updateChannelMessage(msg[QStringLiteral("channel")].toString(), msg);

        QCOMPARE(watcher.dataChangedSpy.size(), 1);
        const auto dataChanged = watcher.dataChangedSpy.takeFirst();
        QCOMPARE(dataChanged[0].toModelIndex(), msgIdx);
        QCOMPARE(dataChanged[1].toModelIndex(), msgIdx);

        QVERIFY(watcher.emptyExcept(ModelWatcher::DataChanged));
    }

    void removeChannelMessage(int messageRow)
    {
        const auto msgIdx = model.index(messageRow, 0);
        QVERIFY(msgIdx.isValid());
        auto msg = model.data(msgIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
        messageModel.removeChannelMessage(model.channelId(), msg[QStringLiteral("ts")].toString());

        QCOMPARE(watcher.rowsRemovedSpy.size(), 1);
        const auto rowsRemoved = watcher.rowsRemovedSpy.takeFirst();
        QCOMPARE(rowsRemoved[0].toModelIndex(), QModelIndex{});
        QCOMPARE(rowsRemoved[1].toInt(), messageRow);
        QCOMPARE(rowsRemoved[2].toInt(), messageRow);

        QVERIFY(watcher.emptyExcept(ModelWatcher::RowsRemoved));
    }

    void appendThreadMessage(int channelRow, int messageRow, const QString &msgText)
    {
        const auto rowCount = model.rowCount();

        modelTest.appendThreadMessage(channelRow, messageRow, msgText);

        QCOMPARE(model.rowCount(), rowCount + 1);
        QCOMPARE(watcher.rowsAddedSpy.size(), 1);
        const auto rowsAdded = watcher.rowsAddedSpy.takeFirst();
        QCOMPARE(rowsAdded[0].toModelIndex(), QModelIndex{});
        QCOMPARE(rowsAdded[1].toInt(), rowCount);
        QCOMPARE(rowsAdded[2].toInt(), rowCount);

        QVERIFY(watcher.emptyExcept(ModelWatcher::RowsAdded));
    }

    void updateThreadMessage(int channelRow, int messageRow, int threadRow, const QString &msgText)
    {
        modelTest.updateThreadMessage(channelRow, messageRow, threadRow, msgText);

        QCOMPARE(watcher.dataChangedSpy.size(), 1);
        const auto dataChanged = watcher.dataChangedSpy.takeFirst();
        QCOMPARE(dataChanged[0].toModelIndex(), model.index(threadRow, 0));
        QCOMPARE(dataChanged[1].toModelIndex(), model.index(threadRow, 0));

        QVERIFY(watcher.emptyExcept(ModelWatcher::DataChanged));
    }

    void removeThreadMessage(int channelRow, int messageRow, int threadRow)
    {
        const auto rowCount = model.rowCount();
        modelTest.removeThreadMessage(channelRow, messageRow, threadRow);
        QCOMPARE(model.rowCount(), rowCount - 1);

        QCOMPARE(watcher.rowsRemovedSpy.size(), 1);
        const auto rowsRemoved = watcher.rowsRemovedSpy.takeFirst();
        QCOMPARE(rowsRemoved[0].toModelIndex(), QModelIndex{});
        QCOMPARE(rowsRemoved[1].toInt(), threadRow);
        QCOMPARE(rowsRemoved[2].toInt(), threadRow);

        QVERIFY(watcher.emptyExcept(ModelWatcher::RowsRemoved));
    }

    MessageModel messageModel;
    ChannelMessagesModel model;
    ModelTest modelTest{&messageModel};
    ModelWatcher watcher{&model};
};

class ChannelMessagesModelTest : public QObject
{
    Q_OBJECT
private slots:
    void testPopulation()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.setChannelId(QStringLiteral("channel-5"));

        QCOMPARE(test.model.rowCount(), 10);
        for (int i = 0; i < 10; ++i) {
            const auto msgIdx = test.model.index(i, 0);
            const auto msg = test.model.data(msgIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
            QVERIFY(!msg.empty());
        }
    }

    void testDelayedModelPopulatioin()
    {
        Test test;
        test.populateChannels(10);
        test.setChannelId(QStringLiteral("channel-5"));
        QCOMPARE(test.model.rowCount(), 0);

        test.populateChannelMessages(5, 10);
        QCOMPARE(test.model.rowCount(), 10);
        QCOMPARE(test.watcher.rowsAddedSpy.size(), 1);
        const auto rowsAdded = test.watcher.rowsAddedSpy.takeFirst();
        QCOMPARE(rowsAdded[0].toModelIndex(), QModelIndex{});
        QCOMPARE(rowsAdded[1].toInt(), 0);
        QCOMPARE(rowsAdded[2].toInt(), 9);
        QVERIFY(test.watcher.emptyExcept(ModelWatcher::RowsAdded));
    }

    void testMessageAdded()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.setChannelId(QStringLiteral("channel-5"));
        test.appendChannelMessage(5, QStringLiteral("New Message"));
    }

    void testMessageChanged()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.setChannelId(QStringLiteral("channel-5"));
        test.updateChannelMessage(5, QStringLiteral("Changed text"));
    }

    void testMessageRemoved()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.setChannelId(QStringLiteral("channel-5"));
        test.removeChannelMessage(5);
    }

    void testMessagesInUnmonitoredChannelIgnored()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.setChannelId(QStringLiteral("channel-5"));

        test.modelTest.populateChannelMessages(6, 10);
        QVERIFY(test.watcher.empty());

        test.modelTest.appendChannelMessage(2, QStringLiteral("0"), QStringLiteral("Message"));
        QVERIFY(test.watcher.empty());

        test.modelTest.updateChannelMessage(2, 0, QStringLiteral("NewText"));
        QVERIFY(test.watcher.empty());

        test.modelTest.removeChannelMessage(6, 5);
        QVERIFY(test.watcher.empty());
    }

    void testMessagesInUnmonitoredThreadAreIgnored()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.setChannelId(QStringLiteral("channel-5"));

        {
            test.modelTest.populateThreadMessages(5, 0, 5);
            QCOMPARE(test.watcher.dataChangedSpy.size(), 1);
            const auto dataChanged = test.watcher.dataChangedSpy.takeFirst();
            QCOMPARE(dataChanged[0].toModelIndex(), test.model.index(0, 0));
            QCOMPARE(dataChanged[1].toModelIndex(), test.model.index(0, 0));
            QVERIFY(test.watcher.emptyExcept(ModelWatcher::DataChanged));
        }

        {
            test.modelTest.updateThreadMessage(5, 0, 2, QStringLiteral("Update"));
            QVERIFY(test.watcher.empty());
        }

        {
            test.modelTest.removeThreadMessage(5, 0, 1);
            QCOMPARE(test.watcher.dataChangedSpy.size(), 1);
            const auto dataChanged = test.watcher.dataChangedSpy.takeFirst();
            QCOMPARE(dataChanged[0].toModelIndex(), test.model.index(0, 0));
            QCOMPARE(dataChanged[1].toModelIndex(), test.model.index(0, 0));
            QVERIFY(test.watcher.emptyExcept(ModelWatcher::DataChanged));
        }

        {
            test.modelTest.appendThreadMessage(5, 0, QStringLiteral("NewMsg"));
            const auto dataChanged = test.watcher.dataChangedSpy.takeFirst();
            QCOMPARE(dataChanged[0].toModelIndex(), test.model.index(0, 0));
            QCOMPARE(dataChanged[1].toModelIndex(), test.model.index(0, 0));
            QVERIFY(test.watcher.emptyExcept(ModelWatcher::DataChanged));
        }

        for (int i = 0; i < test.model.rowCount(); ++i) {
            const auto idx = test.model.index(i, 0);
            QVERIFY(idx.isValid());
            QCOMPARE(test.model.rowCount(idx), 0);
            QVERIFY(!test.model.hasChildren(idx));
        }
    }

};

QTEST_GUILESS_MAIN(ChannelMessagesModelTest)

#include "tst_channelmessagesmodeltest.moc"
