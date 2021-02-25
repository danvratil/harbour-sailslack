/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "messagemodel.h"

#include <QTest>
#include <QSignalSpy>

class ModelTest : public QObject
{
    Q_OBJECT
public:
    explicit ModelTest(MessageModel *model)
        : modelResetSpy{model, &QAbstractItemModel::modelReset}
        , rowsAddedSpy{model, &QAbstractItemModel::rowsInserted}
        , rowsRemovedSpy{model, &QAbstractItemModel::rowsRemoved}
        , rowsMovedSpy{model, &QAbstractItemModel::rowsMoved}
        , dataChangedSpy{model, &QAbstractItemModel::dataChanged}
        , mModel{model}
    {
    }

    void populateChannels(int count)
    {
        // Populate channels
        QVariantList channels;
        channels.reserve(count);
        for (int i = 0; i < count; ++i) {
            channels.push_back(createChannel(QStringLiteral("channel-%1").arg(i), QStringLiteral("Channel %1").arg(i)));
        }
        mModel->setChannels(channels);

        // Check we got model reset
        QCOMPARE(modelResetSpy.size(), 1);
        modelResetSpy.clear();
        QCOMPARE(mModel->rowCount(), count);

        // .. and nothing else
        QVERIFY(rowsAddedSpy.empty());
        QVERIFY(rowsRemovedSpy.empty());
        QVERIFY(rowsMovedSpy.empty());
        QVERIFY(dataChangedSpy.empty());
    }

    void addChannel(const QString &id, const QString &name)
    {
        const auto rowCount = mModel->rowCount();

        // Insert channel
        mModel->addChannel(createChannel(id, name));

        // Check we got rowsAdded for the new channel
        QCOMPARE(rowsAddedSpy.size(), 1);
        const auto addedRow = rowsAddedSpy.takeFirst();
        QCOMPARE(addedRow[0].toModelIndex(), QModelIndex()); // parent
        QCOMPARE(addedRow[1].toInt(), rowCount); // beginning of insertion
        QCOMPARE(addedRow[1].toInt(), rowCount); // end of insertion

        // .. and nothing else
        QVERIFY(modelResetSpy.empty());
        QVERIFY(rowsRemovedSpy.empty());
        QVERIFY(rowsMovedSpy.empty());
        QVERIFY(dataChangedSpy.empty());
    }

    void updateChannel(int row, const QString &newName)
    {
        const auto [channelIdx, channelId] = getChannelIndexAndId(row);
        QVERIFY(channelIdx.isValid());

        // Update the channel
        mModel->updateChannel(createChannel(channelId, newName));

        // Check we got dataChanged for the channel
        QCOMPARE(dataChangedSpy.size(), 1);
        const auto dataChanged = dataChangedSpy.takeFirst();
        QCOMPARE(dataChanged[0].toModelIndex(), channelIdx);
        QCOMPARE(dataChanged[1].toModelIndex(), channelIdx);
        // Check the data are actually updated
        const auto idCheck = mModel->data(channelIdx, static_cast<int>(MessageModel::Role::Channel)).toMap();
        QCOMPARE(idCheck[QStringLiteral("id")].toString(), channelId);
        QCOMPARE(idCheck[QStringLiteral("name")].toString(), newName);

        // ... and we got no other signal
        QVERIFY(modelResetSpy.empty());
        QVERIFY(rowsAddedSpy.empty());
        QVERIFY(rowsRemovedSpy.empty());
        QVERIFY(rowsMovedSpy.empty());
    }

    void populateChannelMessages(int channelRow, int messagesCount)
    {
        const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
        QVERIFY(channelIdx.isValid());

        // Populate messages
        QVariantList msgs;
        msgs.reserve(messagesCount);
        for (int i = 0; i < messagesCount; ++i) {
            msgs.push_back(createMessage(channelId, QString::number(i), QStringLiteral("message %1").arg(i)));
        }
        mModel->setChannelMessages(channelId, msgs);

        // Check we got rowsAdded
        QCOMPARE(rowsAddedSpy.size(), 1);
        const auto rowsAdded = rowsAddedSpy.takeFirst();
        QCOMPARE(rowsAdded[0].toModelIndex(), channelIdx); // parent
        QCOMPARE(rowsAdded[1].toInt(), 0); // insertion start
        QCOMPARE(rowsAdded[2].toInt(), messagesCount - 1); // insertion end
        for (int i = 0; i < messagesCount; ++i) {
            const auto msgIdx = mModel->index(i, 0, channelIdx);
            QVERIFY(msgIdx.isValid());
            const auto msg = mModel->data(msgIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
            QCOMPARE(msg[QStringLiteral("ts")].toString(), QString::number(i));
            QCOMPARE(msg[QStringLiteral("text")].toString(), QStringLiteral("message %1").arg(i));
        }
        QCOMPARE(mModel->rowCount(channelIdx), messagesCount);

        // ... and no other signal
        QVERIFY(modelResetSpy.empty());
        QVERIFY(rowsRemovedSpy.empty());
        QVERIFY(rowsMovedSpy.empty());
        QVERIFY(dataChangedSpy.empty());
    }

    void appendChannelMessage(int channelRow, const QString &messageId, const QString &message)
    {
        const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
        QVERIFY(channelIdx.isValid());

        const int msgCount = mModel->rowCount(channelIdx);

        mModel->appendChannelMessage(channelId, createMessage(channelId, messageId, message));

        QCOMPARE(mModel->rowCount(channelIdx), msgCount + 1);
        QCOMPARE(rowsAddedSpy.size(), 1);
        const auto rowsAdded = rowsAddedSpy.takeFirst();
        QCOMPARE(rowsAdded[0].toModelIndex(), channelIdx);
        QCOMPARE(rowsAdded[1].toInt(), msgCount);
        QCOMPARE(rowsAdded[2].toInt(), msgCount);
        const auto newMsg = mModel->data(mModel->index(msgCount, 0, channelIdx), static_cast<int>(MessageModel::Role::Message)).toMap();
        QCOMPARE(newMsg[QStringLiteral("ts")].toString(), messageId);
        QCOMPARE(newMsg[QStringLiteral("text")].toString(), message);

        QVERIFY(modelResetSpy.empty());
        QVERIFY(rowsRemovedSpy.empty());
        QVERIFY(rowsMovedSpy.empty());
        QVERIFY(dataChangedSpy.empty());
    }

    void removeChannelMessage(int channelRow, int messageRow)
    {
        const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
        QVERIFY(channelIdx.isValid());

        const auto [messageIdx, messageId] = getMessageIndexAndId(channelIdx, messageRow);
        QVERIFY(messageIdx.isValid());

        const int msgCount = mModel->rowCount(channelIdx);

        mModel->removeChannelMessage(channelId, messageId);

        QCOMPARE(mModel->rowCount(channelIdx), msgCount - 1);

        QCOMPARE(rowsRemovedSpy.size(), 1);
        const auto rowsRemoved = rowsRemovedSpy.takeFirst();
        QCOMPARE(rowsRemoved[0].toModelIndex(), channelIdx);
        QCOMPARE(rowsRemoved[1].toInt(), messageRow);
        QCOMPARE(rowsRemoved[2].toInt(), messageRow);

        QVERIFY(modelResetSpy.empty());
        QVERIFY(rowsAddedSpy.empty());
        QVERIFY(rowsMovedSpy.empty());
        QVERIFY(dataChangedSpy.empty());
    }

    void updateChannelMessage(int channelRow, int messageRow, const QString &newMessage)
    {
        const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
        QVERIFY(channelIdx.isValid());

        const auto [messageIdx, messageId] = getMessageIndexAndId(channelIdx, messageRow);
        QVERIFY(messageIdx.isValid());

        mModel->updateChannelMessage(channelId, createMessage(channelId, messageId, newMessage));

        QCOMPARE(dataChangedSpy.size(), 1);
        const auto dataChanged = dataChangedSpy.takeFirst();
        QCOMPARE(dataChanged[0].toModelIndex(), messageIdx);
        QCOMPARE(dataChanged[1].toModelIndex(), messageIdx);
        const auto msg = mModel->data(messageIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
        QCOMPARE(msg[QStringLiteral("ts")].toString(), messageId);
        QCOMPARE(msg[QStringLiteral("text")].toString(), newMessage);

        QVERIFY(modelResetSpy.empty());
        QVERIFY(rowsAddedSpy.empty());
        QVERIFY(rowsMovedSpy.empty());
        QVERIFY(rowsRemovedSpy.empty());
    }

    static QVariantMap createChannel(const QString &id, const QString &name)
    {
        return {
            {QStringLiteral("id"), id},
            {QStringLiteral("name"), name},
            {QStringLiteral("is_channel"), true}
        };
    }

    static QVariantMap createMessage(const QString &channelId, const QString &messageId, const QString &msg)
    {
        return {
            {QStringLiteral("type"), QStringLiteral("message")},
            {QStringLiteral("user"), QStringLiteral("@sender0")},
            {QStringLiteral("text"), msg},
            {QStringLiteral("ts"), messageId},
            {QStringLiteral("channel"), channelId}
        };
    }

    QSignalSpy modelResetSpy;
    QSignalSpy rowsAddedSpy;
    QSignalSpy rowsRemovedSpy;
    QSignalSpy rowsMovedSpy;
    QSignalSpy dataChangedSpy;

private:
    std::tuple<QModelIndex, QString> getChannelIndexAndId(int channelRow) const {
        const auto channelModelIndex = mModel->index(channelRow, 0, {});
        if (!channelModelIndex.isValid()) {
            QTest::qFail("Failed to obtain a valid model index for given channel row", __FILE__, __LINE__);
            return {};
        }

        const auto channelId = QStringLiteral("channel-%1").arg(channelRow);
        const auto channelIdxCheck = mModel->data(channelModelIndex, static_cast<int>(MessageModel::Role::Channel)).toMap();
        if (channelIdxCheck[QStringLiteral("id")].toString() != channelId) {
            QTest::qFail("Channel on given row doesn't match the expected ID", __FILE__, __LINE__);
            return {};
        }

        return std::make_tuple(channelModelIndex, channelId);
    }

    std::tuple<QModelIndex, QString> getMessageIndexAndId(const QModelIndex &channelIdx, int messageRow) const {
        const auto messageModelIndex = mModel->index(messageRow, 0, channelIdx);
        if (!messageModelIndex.isValid()) {
            QTest::qFail("Failed to obtain a valid model index for given message row", __FILE__, __LINE__);
            return {};
        }

        const auto messageId = QString::number(messageRow);
        const auto messageIdxCheck = mModel->data(messageModelIndex, static_cast<int>(MessageModel::Role::Message)).toMap();
        if (messageIdxCheck[QStringLiteral("ts")].toString() != messageId) {
            QTest::qFail("Message on given row doesn't match the expected ID", __FILE__, __LINE__);
            return {};
        }

        return std::make_tuple(messageModelIndex, messageId);
    }

    MessageModel *mModel = nullptr;
};


class MessageModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testChannelPopulation()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
    }

    void testAddChannel()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.addChannel(QStringLiteral("channel-11"), QStringLiteral("Channel 11"));
    }

    void testUpdateChannel()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.updateChannel(4, QStringLiteral("Happy Chanel"));
    }

    void testSetChannelMessages()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(5, 20);
    }

    void testAppendChannelMessage()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(3, 10);
        test.appendChannelMessage(3, QStringLiteral("100"), QStringLiteral("Hello there!"));
    }

    void testRemoveChannelMessage()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(6, 5);
        test.removeChannelMessage(6, 1);
    }

    void testUpdateChannelMessage()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(7, 10);
        test.updateChannelMessage(7, 4, QStringLiteral("FooBar"));
    }
};

QTEST_GUILESS_MAIN(MessageModelTest)

#include "tst_messagemodel.moc"
