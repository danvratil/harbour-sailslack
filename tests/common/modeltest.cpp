/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "modeltest.h"

#include "messagemodel.h"

#include <QObject>
#include <QTest>
#include <QSignalSpy>

ModelTest::ModelTest(MessageModel *model)
    : watcher(model)
    , mModel{model}
{
}

void ModelTest::populateChannels(int count)
{
    // Populate channels
    QVariantList channels;
    channels.reserve(count);
    for (int i = 0; i < count; ++i) {
        channels.push_back(createChannel(QStringLiteral("channel-%1").arg(i), QStringLiteral("Channel %1").arg(i)));
    }
    mModel->setChannels(channels);

    // Check we got model reset
    QCOMPARE(watcher.modelResetSpy.size(), 1);
    watcher.modelResetSpy.clear();
    QCOMPARE(mModel->rowCount(), count);

    // .. and nothing else
    QVERIFY(watcher.rowsAddedSpy.empty());
    QVERIFY(watcher.rowsRemovedSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.dataChangedSpy.empty());
}

void ModelTest::addChannel(const QString &id, const QString &name)
{
    const auto rowCount = mModel->rowCount();

    // Insert channel
    mModel->addChannel(createChannel(id, name));

    // Check we got rowsAdded for the new channel
    QCOMPARE(watcher.rowsAddedSpy.size(), 1);
    const auto addedRow = watcher.rowsAddedSpy.takeFirst();
    QCOMPARE(addedRow[0].toModelIndex(), QModelIndex()); // parent
    QCOMPARE(addedRow[1].toInt(), rowCount); // beginning of insertion
    QCOMPARE(addedRow[1].toInt(), rowCount); // end of insertion

    // .. and nothing else
    QVERIFY(watcher.modelResetSpy.empty());
    QVERIFY(watcher.rowsRemovedSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.dataChangedSpy.empty());
}

void ModelTest::updateChannel(int row, const QString &newName)
{
    const auto [channelIdx, channelId] = getChannelIndexAndId(row);
    QVERIFY(channelIdx.isValid());

    // Update the channel
    mModel->updateChannel(createChannel(channelId, newName));

    // Check we got dataChanged for the channel
    QCOMPARE(watcher.dataChangedSpy.size(), 1);
    const auto dataChanged = watcher.dataChangedSpy.takeFirst();
    QCOMPARE(dataChanged[0].toModelIndex(), channelIdx);
    QCOMPARE(dataChanged[1].toModelIndex(), channelIdx);
    // Check the data are actually updated
    const auto idCheck = mModel->data(channelIdx, static_cast<int>(MessageModel::Role::Channel)).toMap();
    QCOMPARE(idCheck[QStringLiteral("id")].toString(), channelId);
    QCOMPARE(idCheck[QStringLiteral("name")].toString(), newName);

    // ... and we got no other signal
    QVERIFY(watcher.modelResetSpy.empty());
    QVERIFY(watcher.rowsAddedSpy.empty());
    QVERIFY(watcher.rowsRemovedSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
}

void ModelTest::populateChannelMessages(int channelRow, int messagesCount)
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
    QCOMPARE(watcher.rowsAddedSpy.size(), 1);
    const auto rowsAdded = watcher.rowsAddedSpy.takeFirst();
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
    QVERIFY(watcher.modelResetSpy.empty());
    QVERIFY(watcher.rowsRemovedSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.dataChangedSpy.empty());
}

void ModelTest::appendChannelMessage(int channelRow, const QString &messageId, const QString &message)
{
    const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
    QVERIFY(channelIdx.isValid());

    const int msgCount = mModel->rowCount(channelIdx);

    mModel->appendChannelMessage(channelId, createMessage(channelId, messageId, message));

    QCOMPARE(mModel->rowCount(channelIdx), msgCount + 1);
    QCOMPARE(watcher.rowsAddedSpy.size(), 1);
    const auto rowsAdded = watcher.rowsAddedSpy.takeFirst();
    QCOMPARE(rowsAdded[0].toModelIndex(), channelIdx);
    QCOMPARE(rowsAdded[1].toInt(), msgCount);
    QCOMPARE(rowsAdded[2].toInt(), msgCount);
    const auto newMsg = mModel->data(mModel->index(msgCount, 0, channelIdx), static_cast<int>(MessageModel::Role::Message)).toMap();
    QCOMPARE(newMsg[QStringLiteral("ts")].toString(), messageId);
    QCOMPARE(newMsg[QStringLiteral("text")].toString(), message);

    QVERIFY(watcher.modelResetSpy.empty());
    QVERIFY(watcher.rowsRemovedSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.dataChangedSpy.empty());
}

void ModelTest::removeChannelMessage(int channelRow, int messageRow)
{
    const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
    QVERIFY(channelIdx.isValid());

    const auto [messageIdx, messageId] = getMessageIndexAndId(channelIdx, messageRow);
    QVERIFY(messageIdx.isValid());

    const int msgCount = mModel->rowCount(channelIdx);

    mModel->removeChannelMessage(channelId, messageId);

    QCOMPARE(mModel->rowCount(channelIdx), msgCount - 1);

    QCOMPARE(watcher.rowsRemovedSpy.size(), 1);
    const auto rowsRemoved = watcher.rowsRemovedSpy.takeFirst();
    QCOMPARE(rowsRemoved[0].toModelIndex(), channelIdx);
    QCOMPARE(rowsRemoved[1].toInt(), messageRow);
    QCOMPARE(rowsRemoved[2].toInt(), messageRow);

    QVERIFY(watcher.modelResetSpy.empty());
    QVERIFY(watcher.rowsAddedSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.dataChangedSpy.empty());
}

void ModelTest::updateChannelMessage(int channelRow, int messageRow, const QString &newMessage)
{
    const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
    QVERIFY(channelIdx.isValid());

    const auto [messageIdx, messageId] = getMessageIndexAndId(channelIdx, messageRow);
    QVERIFY(messageIdx.isValid());

    mModel->updateChannelMessage(channelId, createMessage(channelId, messageId, newMessage));

    QCOMPARE(watcher.dataChangedSpy.size(), 1);
    const auto dataChanged = watcher.dataChangedSpy.takeFirst();
    QCOMPARE(dataChanged[0].toModelIndex(), messageIdx);
    QCOMPARE(dataChanged[1].toModelIndex(), messageIdx);
    const auto msg = mModel->data(messageIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
    QCOMPARE(msg[QStringLiteral("ts")].toString(), messageId);
    QCOMPARE(msg[QStringLiteral("text")].toString(), newMessage);

    QVERIFY(watcher.modelResetSpy.empty());
    QVERIFY(watcher.rowsAddedSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.rowsRemovedSpy.empty());
}

void ModelTest::populateThreadMessages(int channelRow, int messageRow, int threadMsgCount)
{
    const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
    QVERIFY(channelIdx.isValid());

    const auto [messageIdx, messageId] = getMessageIndexAndId(channelIdx, messageRow);
    QVERIFY(messageIdx.isValid());

    QVariantList messages;
    messages.reserve(threadMsgCount);
    for (int i = 0 ; i < threadMsgCount; ++i) {
        messages.push_back(createMessage(channelId, QStringLiteral("%1-%2").arg(messageId).arg(i),
                                         QStringLiteral("ThreadMessage %1").arg(i), messageId));
    }
    mModel->setThreadMessages(channelId, messageId, messages);

    QCOMPARE(mModel->rowCount(messageIdx), threadMsgCount);

    // The thread leader has changed
    QCOMPARE(watcher.dataChangedSpy.size(), 1);
    const auto dataChanged = watcher.dataChangedSpy.takeFirst();
    QCOMPARE(dataChanged[0].toModelIndex(), messageIdx);
    QCOMPARE(dataChanged[0].toModelIndex(), messageIdx);

    QCOMPARE(watcher.rowsAddedSpy.size(), 1);
    const auto rowsAdded = watcher.rowsAddedSpy.takeFirst();
    QCOMPARE(rowsAdded[0].toModelIndex(), messageIdx);
    QCOMPARE(rowsAdded[1].toInt(), 0);
    QCOMPARE(rowsAdded[2].toInt(), threadMsgCount - 1);
    for (int i = 0; i < threadMsgCount; ++i) {
        const auto threadMsgIdx = mModel->index(i, 0, messageIdx);
        QVERIFY(threadMsgIdx.isValid());
        const auto msg = mModel->data(threadMsgIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
        QCOMPARE(msg["ts"].toString(), QStringLiteral("%1-%2").arg(messageId).arg(i));
        QCOMPARE(msg["text"].toString(), QStringLiteral("ThreadMessage %1").arg(i));
    }

    QVERIFY(watcher.rowsAddedSpy.empty());
    QVERIFY(watcher.modelResetSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.rowsRemovedSpy.empty());
}

void ModelTest::appendThreadMessage(int channelRow, int messageRow, const QString &msgText)
{
    const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
    QVERIFY(channelIdx.isValid());

    const auto [messageIdx, messageId] = getMessageIndexAndId(channelIdx, messageRow);
    QVERIFY(messageIdx.isValid());

    const int threadRepliesCount = mModel->rowCount(messageIdx);

    mModel->appendChannelMessage(channelId, createMessage(channelId, QStringLiteral("%1-%2").arg(messageId).arg(threadRepliesCount), msgText, messageId));

    QCOMPARE(mModel->rowCount(messageIdx), threadRepliesCount + 1);

    QCOMPARE(watcher.dataChangedSpy.size(), 1);
    const auto dataChanged = watcher.dataChangedSpy.takeFirst();
    QCOMPARE(dataChanged[0].toModelIndex(), messageIdx);
    QCOMPARE(dataChanged[1].toModelIndex(), messageIdx);

    QCOMPARE(watcher.rowsAddedSpy.size(), 1);
    const auto rowsAdded = watcher.rowsAddedSpy.takeFirst();
    QCOMPARE(rowsAdded[0].toModelIndex(), messageIdx);
    QCOMPARE(rowsAdded[1].toInt(), threadRepliesCount);
    QCOMPARE(rowsAdded[2].toInt(), threadRepliesCount);
    const auto threadMsgIdx = mModel->index(threadRepliesCount, 0, messageIdx);
    QVERIFY(threadMsgIdx.isValid());
    const auto msg = mModel->data(threadMsgIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
    QCOMPARE(msg["ts"].toString(), QStringLiteral("%1-%2").arg(messageId).arg(threadRepliesCount));
    QCOMPARE(msg["text"].toString(), msgText);

    QVERIFY(watcher.modelResetSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.rowsRemovedSpy.empty());
}

void ModelTest::updateThreadMessage(int channelRow, int messageRow, int threadRow, const QString &msgText)
{
    const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
    QVERIFY(channelIdx.isValid());

    const auto [messageIdx, messageId] = getMessageIndexAndId(channelIdx, messageRow);
    QVERIFY(messageIdx.isValid());

    const auto [threadMsgIdx, threadMsgId] = getThreadMessageIndexAndId(messageIdx, threadRow);
    QVERIFY(threadMsgIdx.isValid());

    auto msg = mModel->data(threadMsgIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
    QVERIFY(!msg.empty());
    QVERIFY(!msg["thread_ts"].isNull());
    msg["text"] = msgText;

    mModel->updateChannelMessage(channelId, msg);

    QCOMPARE(watcher.dataChangedSpy.size(), 1);
    const auto dataChanged = watcher.dataChangedSpy.takeFirst();
    QCOMPARE(dataChanged[0].toModelIndex(), threadMsgIdx);
    QCOMPARE(dataChanged[1].toModelIndex(), threadMsgIdx);

    const auto updatedMsg = mModel->data(threadMsgIdx, static_cast<int>(MessageModel::Role::Message)).toMap();
    QCOMPARE(updatedMsg["ts"], msg["ts"]);
    QCOMPARE(updatedMsg["text"].toString(), msgText);

    QVERIFY(watcher.rowsAddedSpy.empty());
    QVERIFY(watcher.modelResetSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.rowsRemovedSpy.empty());
}

void ModelTest::removeThreadMessage(int channelRow, int messageRow, int threadMsgRow)
{
    const auto [channelIdx, channelId] = getChannelIndexAndId(channelRow);
    QVERIFY(channelIdx.isValid());

    const auto [messageIdx, messageId] = getMessageIndexAndId(channelIdx, messageRow);
    QVERIFY(messageIdx.isValid());

    const auto [threadMsgIdx, threadMsgId] = getThreadMessageIndexAndId(messageIdx, threadMsgRow);
    QVERIFY(threadMsgIdx.isValid());

    const auto threadRepliesCount = mModel->rowCount(messageIdx);

    mModel->removeChannelMessage(channelId, threadMsgId);

    QCOMPARE(mModel->rowCount(messageIdx), threadRepliesCount - 1);

    QCOMPARE(watcher.dataChangedSpy.size(), 1);
    const auto dataChaned = watcher.dataChangedSpy.takeFirst();
    QCOMPARE(dataChaned[0].toModelIndex(), messageIdx);
    QCOMPARE(dataChaned[1].toModelIndex(), messageIdx);

    QCOMPARE(watcher.rowsRemovedSpy.size(), 1);
    const auto rowsRemoved = watcher.rowsRemovedSpy.takeFirst();
    QCOMPARE(rowsRemoved[0].toModelIndex(), messageIdx);
    QCOMPARE(rowsRemoved[1].toInt(), threadMsgRow);
    QCOMPARE(rowsRemoved[2].toInt(), threadMsgRow);

    QVERIFY(watcher.rowsAddedSpy.empty());
    QVERIFY(watcher.rowsMovedSpy.empty());
    QVERIFY(watcher.modelResetSpy.empty());
}

QVariantMap ModelTest::createChannel(const QString &id, const QString &name)
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("is_channel"), true}
    };
}

QVariantMap ModelTest::createMessage(const QString &channelId, const QString &messageId, const QString &msgText, const QString &threadId)
{
    QVariantMap msg{
        {QStringLiteral("type"), QStringLiteral("message")},
        {QStringLiteral("user"), QStringLiteral("@sender0")},
        {QStringLiteral("text"), msgText},
        {QStringLiteral("ts"), messageId},
        {QStringLiteral("channel"), channelId}
    };
    if (!threadId.isEmpty()) {
        msg[QStringLiteral("thread_ts")] = threadId;
    }
    return msg;
}

std::tuple<QModelIndex, QString> ModelTest::getChannelIndexAndId(int channelRow) const
{
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

std::tuple<QModelIndex, QString> ModelTest::getMessageIndexAndId(const QModelIndex &channelIdx, int messageRow) const
{
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

std::tuple<QModelIndex, QString> ModelTest::getThreadMessageIndexAndId(const QModelIndex &messageIdx, int threadRow) const
{
    const auto threadModelIndex = mModel->index(threadRow, 0, messageIdx);
    if (!threadModelIndex.isValid()) {
        QTest::qFail("Failed to obtain a valid model index for given thread message row", __FILE__, __LINE__);
        return {};
    }

    const auto messageId = mModel->data(messageIdx, static_cast<int>(MessageModel::Role::Message)).toMap()[QStringLiteral("ts")].toString();
    const auto threadMsgId = QStringLiteral("%1-%2").arg(messageId).arg(threadRow);
    const auto threadIdxCheck = mModel->data(threadModelIndex, static_cast<int>(MessageModel::Role::Message)).toMap();
    if (threadIdxCheck[QStringLiteral("ts")].toString() != threadMsgId) {
        qFatal("Message on row %d doesn't match the expected ID %s (was: %s)", threadRow, qUtf8Printable(threadMsgId),
               qUtf8Printable(threadIdxCheck[QStringLiteral("ts")].toString()));
        QTest::qFail("Message on given row doesn't match the expected ID", __FILE__, __LINE__);
        return {};
    }

    return std::make_tuple(threadModelIndex, threadMsgId);
}
