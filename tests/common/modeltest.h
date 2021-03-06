/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#pragma once

#include <QTest>
#include <QSignalSpy>

#include "modelwatcher.h"

class MessageModel;

class ModelTest : public QObject
{
    Q_OBJECT
public:
    explicit ModelTest(MessageModel *model);

    void populateChannels(int count);

    void addChannel(const QString &id, const QString &name);
    void updateChannel(int row, const QString &newName);
    void populateChannelMessages(int channelRow, int messagesCount);
    void appendChannelMessage(int channelRow, const QString &messageId, const QString &message);
    void removeChannelMessage(int channelRow, int messageRow);
    void updateChannelMessage(int channelRow, int messageRow, const QString &newMessage);
    void populateThreadMessages(int channelRow, int messageRow, int threadMsgCount);
    void appendThreadMessage(int channelRow, int messageRow, const QString &msgText);
    void updateThreadMessage(int channelRow, int messageRow, int threadRow, const QString &msgText);
    void removeThreadMessage(int channelRow, int messageRow, int threadMsgRow);
    static QVariantMap createChannel(const QString &id, const QString &name);
    static QVariantMap createMessage(const QString &channelId, const QString &messageId, const QString &msgText, const QString &threadId = {});

private:
    std::tuple<QModelIndex, QString> getChannelIndexAndId(int channelRow) const;
    std::tuple<QModelIndex, QString> getMessageIndexAndId(const QModelIndex &channelIdx, int messageRow) const;
    std::tuple<QModelIndex, QString> getThreadMessageIndexAndId(const QModelIndex &messageIdx, int threadRow) const;

    ModelWatcher watcher;
    MessageModel *mModel = nullptr;
};

