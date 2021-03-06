/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include <QTest>
#include "messagemodel.h"
#include "modeltest.h"
#include "threadmessagesmodel.h"

struct Test
{
    explicit Test()
    {
        model.setSourceModel(&messageModel);
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

    void setThread(const QString &channelId, const QString &threadId)
    {
        model.setChannelId(channelId);
        model.setThreadId(threadId);
        QTest::qWait(20);
        // Setting both properties should still compress into a single model reset
        QCOMPARE(watcher.modelResetSpy.size(), 1);
        QVERIFY(watcher.emptyExcept(ModelWatcher::ModelReset));
        watcher.clear();
    }

    void appendThreadMessage(int channelRow, int messageRow, const QString &msgText)
    {
        const auto rowCount = model.rowCount();

        modelTest.appendThreadMessage(channelRow, messageRow, msgText);

        QCOMPARE(model.rowCount(), rowCount + 1);

        // The thread-leader gets updated whenever a new reply is added
        QCOMPARE(watcher.dataChangedSpy.size(), 1);
        const auto dataChanged = watcher.dataChangedSpy.takeFirst();
        QCOMPARE(dataChanged[0].toModelIndex(), model.index(0, 0));
        QCOMPARE(dataChanged[0].toModelIndex(), model.index(0, 0));

        QCOMPARE(watcher.rowsAddedSpy.size(), 1);
        const auto rowsAdded = watcher.rowsAddedSpy.takeFirst();
        QCOMPARE(rowsAdded[0].toModelIndex(), QModelIndex{});
        QCOMPARE(rowsAdded[1].toInt(), rowCount);
        QCOMPARE(rowsAdded[2].toInt(), rowCount);

        QVERIFY(watcher.empty());
    }

    void updateThreadMessage(int channelRow, int messageRow, int threadRow, const QString &msgText)
    {
        modelTest.updateThreadMessage(channelRow, messageRow, threadRow, msgText);

        QCOMPARE(watcher.dataChangedSpy.size(), 1);
        const auto dataChanged = watcher.dataChangedSpy.takeFirst();
        // +1 to account for the thread-leader message on top
        QCOMPARE(dataChanged[0].toModelIndex(), model.index(threadRow + 1, 0));
        QCOMPARE(dataChanged[1].toModelIndex(), model.index(threadRow + 1, 0));

        QVERIFY(watcher.empty());
    }

    void removeThreadMessage(int channelRow, int messageRow, int threadRow)
    {
        const auto rowCount = model.rowCount();
        modelTest.removeThreadMessage(channelRow, messageRow, threadRow);
        QCOMPARE(model.rowCount(), rowCount - 1);

        // The thread-leader gets notified when a reply is removed, but we cannot filter that
        // out when it's part of the threads model
        QCOMPARE(watcher.dataChangedSpy.size(), 1);
        const auto dataChanged = watcher.dataChangedSpy.takeFirst();
        QCOMPARE(dataChanged[0].toModelIndex(), model.index(0, 0));
        QCOMPARE(dataChanged[1].toModelIndex(), model.index(0, 0));

        QCOMPARE(watcher.rowsRemovedSpy.size(), 1);
        const auto rowsRemoved = watcher.rowsRemovedSpy.takeFirst();
        QCOMPARE(rowsRemoved[0].toModelIndex(), QModelIndex{});
        // +1 to account for the thread-leader message ontop
        QCOMPARE(rowsRemoved[1].toInt(), threadRow + 1);
        QCOMPARE(rowsRemoved[2].toInt(), threadRow + 1);

        QVERIFY(watcher.empty());
    }

    ThreadMessagesModel model;
    MessageModel messageModel;
    ModelTest modelTest{&messageModel};
    ModelWatcher watcher{&model};
};

class ThreadMessagesModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testThreadPopulation()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 1);
        test.populateChannelThreadMessages(5, 0, 10);
        test.watcher.clear();

        test.setThread(QStringLiteral("channel-5"), QStringLiteral("0"));

        QCOMPARE(test.model.rowCount(), 11);
    }

    void testDelayedThreadPopulation()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 1);
        test.watcher.clear();

        test.setThread(QStringLiteral("channel-5"), QStringLiteral("0"));

        QCOMPARE(test.model.rowCount(), 1); // there's only the thread-leader message there
        test.populateChannelThreadMessages(5, 0, 10);
        QCOMPARE(test.model.rowCount(), 11);

        QCOMPARE(test.watcher.dataChangedSpy.size(), 1);
        const auto dataChanged = test.watcher.dataChangedSpy.takeFirst();
        // The thread leader gets updated whenever a new reply is added. We have no way of filtering
        // this out for the thread leader when its part of the threads model (could be another change)
        QCOMPARE(dataChanged[0].toModelIndex(), test.model.index(0, 0));
        QCOMPARE(dataChanged[0].toModelIndex(), test.model.index(0, 0));

        QCOMPARE(test.watcher.rowsAddedSpy.size(), 1);
        const auto rowsAdded = test.watcher.rowsAddedSpy.takeFirst();
        QCOMPARE(rowsAdded[0].toModelIndex(), QModelIndex{});
        QCOMPARE(rowsAdded[1].toInt(), 1);
        QCOMPARE(rowsAdded[2].toInt(), 10);

        QVERIFY(test.watcher.empty());
    }

    void testThreadMessageAdded()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.populateChannelThreadMessages(5, 5, 10);
        test.setThread(QStringLiteral("channel-5"), QStringLiteral("5"));

        test.appendThreadMessage(5, 5, QStringLiteral("New Message"));
    }

    void testThreadMessageChanged()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.populateChannelThreadMessages(5, 5, 10);
        test.setThread(QStringLiteral("channel-5"), QStringLiteral("5"));

        test.updateThreadMessage(5, 5, 1, QStringLiteral("New Text"));
    }

    void testThreadMessageRemoved()
    {
        Test test;
        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.populateChannelThreadMessages(5, 5, 10);
        test.setThread(QStringLiteral("channel-5"), QStringLiteral("5"));

        test.removeThreadMessage(5, 5, 5);
    }
};

QTEST_GUILESS_MAIN(ThreadMessagesModelTest)

#include "tst_threadmessagesmodeltest.moc"
